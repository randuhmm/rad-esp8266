#include "RadConnect.h"


DeviceType getDeviceType(const char* s) {
  DeviceType dt = NullDevice;
  if(strcmp(s, "SwitchBinary") == 0) {
    dt = SwitchBinary;
  } else if(strcmp(s, "SensorBinary") == 0) {
    dt = SensorBinary;
  } else if(strcmp(s, "SwitchMultiLevel") == 0) {
    dt = SwitchMultiLevel;
  } else if(strcmp(s, "SensorMultiLevel") == 0) {
    dt = SensorMultiLevel;
  }
  return dt;
}


const char* sendDeviceType(DeviceType dt) {
  const char* s = "NullDevice";
  if(dt == SwitchBinary) {
    s = "SwitchBinary";
  } else if(dt == SensorBinary) {
    s = "SensorBinary";
  } else if(dt == SwitchMultiLevel) {
    s = "SwitchMultiLevel";
  } else if(dt == SensorMultiLevel) {
    s = "SensorMultiLevel";
  }
  return s;
}


CommandType getCommandType(const char* s) {
  CommandType ct = NullCommand;
  if(strcmp(s, "Get") == 0) {
    ct = Get;
  } else if(strcmp(s, "Set") == 0) {
    ct = Set;
  }
  return ct;
}


const char* sendCommandType(CommandType ct) {
  const char* s = "NullCommand";
  if(ct == Get) {
    s = "Get";
  } else if(ct == Set) {
    s = "Set";
  }
  return s;
}


EventType getEventType(const char* s) {
  EventType et = NullEvent;
  if(strcmp(s, "All") == 0) {
    et = All;
  } else if(strcmp(s, "Start") == 0) {
    et = Start;
  } else if(strcmp(s, "State") == 0) {
    et = State;
  }
  return et;
}


const char* sendEventType(EventType et) {
  const char* s = "NullEvent";
  if(et == All) {
    s = "All";
  } else if(et == Start) {
    s = "Start";
  } else if(et == State) {
    s = "State";
  }
  return s;
}


static const char* HEADERS[] = {
  HEADER_HOST,
  HEADER_CALLBACK,
  HEADER_NT,
  HEADER_TIMEOUT
};


RadConnect::RadConnect(const char* name) {
  _name = name;
  _http = ESP8266WebServer(RAD_HTTP_PORT);
  _subscription_count = 0;
}


void RadConnect::add(RadDevice* device) {
  _devices.add(device);
}


Subscription* RadConnect::subscribe(RadDevice* device, EventType type,
                                    const char* callback, int timeout) {
  Subscription* s;
  for(int i = 0; i < _subscriptions.size(); i++) {
    s = _subscriptions.get(i);
    if(s->getType() == type && strcmp(s->getCallback(), callback) == 0) {
      Serial.print("Deleting Subscription: ");
      Serial.println(s->getSid());
      _subscriptions.remove(i);
      device->remove(s);
      delete s;
      i -= 1;
    }
  }

  _subscription_count += 1;
  char sid[SSDP_UUID_SIZE];
  uint32_t chipId = ESP.getChipId();
  sprintf(sid, "38323636-4558-4dda-9188-cd%02x%02x%02x%04x",
  (uint16_t) ((chipId >> 16) & 0xff),
  (uint16_t) ((chipId >>  8) & 0xff),
  (uint16_t)   chipId        & 0xff ,
              _subscription_count);
  Subscription* subscription = new Subscription(device, sid, type, callback, timeout);
  _subscriptions.add(subscription);
  device->add(subscription);
  Serial.print("SID: ");
  Serial.println(sid);
  return subscription;
}


bool RadConnect::begin(void) {

  // Start the SPIFFS object
  SPIFFS.begin();

  // Load the existing subscriptions from SPIFFS
  if(SPIFFS.exists(RAD_SPIFFS_SUBSCRIPTIONS_PATH)) {
    Dir dir = SPIFFS.openDir(RAD_SPIFFS_SUBSCRIPTIONS_PATH);
    while (dir.next()) {
      Serial.print(dir.fileName());
      File f = dir.openFile("r");
      Serial.println(f.size());
    }
  }

  // Prepare the uuid
  uint32_t chipId = ESP.getChipId();
  sprintf(_uuid, "38323636-4558-4dda-9188-cda0e6%02x%02x%02x",
  (uint16_t) ((chipId >> 16) & 0xff),
  (uint16_t) ((chipId >>  8) & 0xff),
  (uint16_t)   chipId        & 0xff);

  char buffer[512];
  sprintf(buffer, _info_template,
    _name,
    _uuid
  );
  _info = String(buffer);

  Serial.println("ChipId: ");
  Serial.println(chipId);
  Serial.println((uint16_t) ((chipId >> 16) & 0xff));
  Serial.println((uint16_t) ((chipId >>  8) & 0xff));
  Serial.println((uint16_t)   chipId        & 0xff);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  _http.on(RAD_INFO_PATH, std::bind(&RadConnect::handleInfo, this));
  _http.on(RAD_DEVICES_PATH, std::bind(&RadConnect::handleDevices, this));
  _http.on(RAD_SUBSCRIPTIONS_PATH, std::bind(&RadConnect::handleSubscriptions, this));

  on(RAD_DEVICES_PATH "/{}" RAD_COMMANDS_PATH,
     std::bind(&RadConnect::handleDeviceCommands, this, std::placeholders::_1));
  on(RAD_DEVICES_PATH "/{}" RAD_EVENTS_PATH,
     std::bind(&RadConnect::handleDeviceEvents, this, std::placeholders::_1));
  on(RAD_SUBSCRIPTIONS_PATH "/{}",
     std::bind(&RadConnect::handleSubscription, this, std::placeholders::_1));

  // SSDP
  // Serial.println("Starting http...");
  //_http.on("/", std::bind(&RadConnect::handleRoot, this));
  // _http.on("/" RAD_COMMAND_PATH, std::bind(&RadConnect::handleCommand, this));
  // _http.on("/" RAD_SUBSCRIBE_PATH, std::bind(&RadConnect::handleSubscribe, this));
  _http.collectHeaders(HEADERS, 4);
  _http.begin();
  SSDP.setDeviceType(RAD_DEVICE_TYPE);
  SSDP.setName(_name);
  SSDP.setURL("");
  SSDP.setSchemaURL(RAD_INFO_PATH);
  SSDP.setSerialNumber(WiFi.macAddress());
  SSDP.setModelName(RAD_MODEL_NAME);
  SSDP.setModelNumber(RAD_MODEL_NUM);
  SSDP.setModelURL(RAD_INFO_URL);
  SSDP.setManufacturer("NA");
  SSDP.setManufacturerURL(RAD_INFO_URL);
  SSDP.setHTTPPort(RAD_HTTP_PORT);
  SSDP.begin();
}


void RadConnect::update(void) {
  // loop
  _http.handleClient();
}


RadDevice* RadConnect::getDevice(const char* name) {
  RadDevice* device = NULL;
  for(int i = 0; i < _devices.size(); i++) {
    if(strcmp(_devices.get(i)->getName(), name) == 0) {
      device = _devices.get(i);
      break;
    }
  }
  return device;
}


void RadConnect::on(const char* uri, PATH_FP fn) {
  on(uri, HTTP_ANY, fn);
}


void RadConnect::on(const char* uri, HTTPMethod method, PATH_FP fn) {
  _http.addHandler(new PathSegmentRequestHandler(fn, uri, method));
}


void RadConnect::handleInfo(void) {
  Serial.println("/");
  if(_http.method() == HTTP_GET) {
    _http.send(200, "application/json", _info);
  } else {
    _http.send(405);
  }
}


void RadConnect::handleDevices(void) {
  Serial.println("/devices");
  int code = 200;
  if(_http.method() == HTTP_GET) {
    // Prepare the JSON response
    StaticJsonBuffer<1024> devicesBuffer;
    char devicesString[1024];
    JsonArray& devices = devicesBuffer.createArray();
    RadDevice* device;
    for(int i = 0; i < _devices.size(); i++) {
      device = _devices.get(i);
      JsonObject& device_json = devices.createNestedObject();
      device_json["name"] = device->getName();
      device_json["type"] = sendDeviceType(device->getType());
      device_json["description"] = "";
    }
    devices.printTo(devicesString, sizeof(devicesString));
    _http.send(code, "application/json", devicesString);
  } else {
    _http.send(405);
  }
}


void RadConnect::handleSubscriptions(void) {
  Serial.println("/subscriptions");
  int code = 200;
  long current = millis();
  if(_http.method() == HTTP_GET) {
    // Prepare the JSON response
    StaticJsonBuffer<1024> subscriptionsBuffer;
    char subscriptionsString[1024];
    JsonArray& subscriptions = subscriptionsBuffer.createArray();
    RadDevice* device;
    Subscription* subscription;
    for(int i = 0; i < _subscriptions.size(); i++) {
      subscription = _subscriptions.get(i);
      if(subscription->isActive(current)) {
        device = subscription->getDevice();
        JsonObject& subscription_json = subscriptions.createNestedObject();
        subscription_json["id"] = subscription->getSid();
        subscription_json["device_name"] = device->getName();
        subscription_json["type"] = sendEventType(subscription->getType());
        subscription_json["callback"] = subscription->getCallback();
        subscription_json["timeout"] = subscription->getTimeout();
        subscription_json["duration"] = subscription->getDuration(current);
      }
    }
    subscriptions.printTo(subscriptionsString, sizeof(subscriptionsString));
    _http.send(code, "application/json", subscriptionsString);
  } else if(_http.method() == HTTP_POST) {
    String message = "";
    StaticJsonBuffer<255> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_http.arg("plain"));
    if(!root.containsKey("device_name")) {
      code = 400;
      message = "{\"error\": \"Missing required property 'device_name'.\"}";
    } else if(!root.containsKey("type")) {
      code = 400;
      message = "{\"error\": \"Missing required property 'type'.\"}";
    } else if(!root.containsKey("callback")) {
      code = 400;
      message = "{\"error\": \"Missing required property 'callback'.\"}";
    } else {
      const char* device_name = root["device_name"];
      EventType type = getEventType(root["type"]);
      const char* callback = root["callback"];
      int timeout = RAD_MIN_TIMEOUT;
      if(root.containsKey("timeout")) {
        timeout = root["timeout"];
      }
      RadDevice* device = getDevice(device_name);
      if (device == NULL) {
        code = 400;
        message = "{\"error\": \"Device could not be located.\"}";
      } else if(type == NullEvent) {
        code = 400;
        message = "{\"error\": \"Unsuported event type.\"}";
      } else if(timeout < RAD_MIN_TIMEOUT) {
        code = 400;
        message = "{\"error\": \"The timeout property can not be less than 600 seconds.\"}";
      } else {
        // Subscription* subscription = device->subscribe(type, callback);
        // add(subscription);
        Subscription* subscription = subscribe(device, type, callback, timeout);
        char sid[100];
        snprintf(sid, sizeof(sid), "uuid:%s", subscription->getSid());
        _http.sendHeader("SID", sid);
        _http.sendHeader(HEADER_TIMEOUT, String(timeout));
      }
    }
    _http.send(code, "application/json", message);
  } else {
    _http.send(405);
  }
}


void RadConnect::handleDeviceCommands(LinkedList<String>& segments) {
  String device_name = segments.get(0);
  RadDevice* device = getDevice(device_name.c_str());
  int code = 200;
  uint8_t value;
  String message = "";
  if(device == NULL) {
    _http.send(404);
  } else if(_http.method() == HTTP_POST) {
    StaticJsonBuffer<255> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_http.arg("plain"));
    if(!root.containsKey("type")) {
      code = 400;
      message = "{\"error\": \"Missing required property, 'type'.\"}";
    } else {
      CommandType type = getCommandType(root["type"]);
      switch(type) {
        case Set:
          if(!root.containsKey("data")) {
            code = 400;
            message = "{\"error\": \"Missing required property, 'data'.\"}";
          } else {
            JsonObject& data = root["data"].asObject();
            if(data == JsonObject::invalid()) {
              code = 400;
              message = "{\"error\": \"Property 'data' must be an object.\"}";
            } else if(!data.containsKey("value")) {
                code = 400;
                message = "{\"error\": \"Missing required property, 'value'.\"}";
            } else {
              value = data["value"];
              bool result = execute(device, Set, value);
              if(result) {
                message = "{\"result\": true}";
              } else {
                code = 500;
              }
            }
          }
          break;
        case Get:
          value = execute(device, Get);
          char buff[100];
          snprintf(buff, sizeof(buff), "{\"value\": %d}", value);
          message = buff;
          break;
        default:
          code = 400;
          message = "{\"error\": \"Unsuported command type.\"}";
          break;
      }
    }
    _http.send(code, "application/json", message);
  } else {
    _http.send(405);
  }
}


void RadConnect::handleDeviceEvents(LinkedList<String>& segments) {
  Serial.println("/devices/{}/events");
  for(int i = 0; i < segments.size(); i++) {
    Serial.print("Segment " + (String)i + ": ");
    Serial.println(segments.get(i));
  }
  _http.send(200, "application/json", "/devices/{}/events");
  return;
}


void RadConnect::handleSubscription(LinkedList<String>& segments) {
  Serial.println("/subscriptions/{}");
  for(int i = 0; i < segments.size(); i++) {
    Serial.print("Segment " + (String)i + ": ");
    Serial.println(segments.get(i));
  }
  _http.send(200, "application/json", "/subscriptions/{}");
  return;
}


uint8_t RadConnect::execute(const char* name, CommandType command_type) {
  RadDevice* device;
  for(int i = 0; i < _devices.size(); i++) {
    device = _devices.get(i);
    if(strcmp(device->getName(), name) == 0) {
      return execute(device, command_type);
    }
  }
}


uint8_t RadConnect::execute(RadDevice* device, CommandType command_type) {
  return device->execute(command_type);
}


bool RadConnect::execute(const char* name, CommandType command_type, uint8_t value) {
  RadDevice* device;
  for(int i = 0; i < _devices.size(); i++) {
    device = _devices.get(i);
    if(strcmp(device->getName(), name) == 0) {
      return execute(device, command_type, value);
    }
  }
}


bool RadConnect::execute(RadDevice* device, CommandType command_type, uint8_t value) {
  return device->execute(command_type, value);
}


RadDevice::RadDevice(DeviceType type, const char* name) {
  _type = type;
  _name = name;
  _set_callback = NULL;
  _get_callback = NULL;
}


uint8_t RadDevice::execute(CommandType command_type) {
  uint8_t result = 0;
  switch(command_type) {
    case Get:
      GET_FP get = NULL;
      switch(_type) {
        case SwitchBinary:
          get = _get_callback;
        break;
      }
      if(get != NULL) {
        result = get();
      }
    break;
  }
  return result;
}


bool RadDevice::execute(CommandType command_type, uint8_t value) {
  bool result = false;
  switch(command_type) {
    case Set:
      SET_FP set = NULL;
      switch(_type) {
        case SwitchBinary:
          set = _set_callback;
        break;
      }
      if(set != NULL) {
        set(value);
        result = true;
      }
    break;
  }
  return result;
}


void RadDevice::send(EventType event_type) {

}


void RadDevice::send(EventType event_type, uint8_t value) {
  Subscription* s;
  String message = "";
  switch(event_type) {
    case State:
      switch(_type) {
        case SwitchBinary:
        char buff[100];
        snprintf(buff, sizeof(buff), "{\"type\": \"state\", \"value\": %d}", value);
        message = buff;
      }
      break;
  }

  for(int i = 0; i < _subscriptions.size(); i++) {
    s = _subscriptions.get(i);
    if(s->getType() == event_type) {
      Serial.print("Subscription Found: ");
      Serial.println(s->getCallback());

      HTTPClient http;
      http.begin(s->getCallback());
      http.addHeader("SID", s->getSid());
      http.addHeader("RAD-NAME", _name);
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.sendRequest("NOTIFY", message);
      if(httpCode > 0) {
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  }
}


void RadDevice::add(Subscription* subscription) {
  _subscriptions.add(subscription);
}


void RadDevice::remove(Subscription* subscription) {
  for(int i = 0; i < _subscriptions.size(); i++) {
    if(_subscriptions.get(i) == subscription) {
      _subscriptions.remove(i);
      break;
    }
  }
}
