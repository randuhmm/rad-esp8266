#include "RadConnect.h"


static const char* HEADERS[] = {
  HEADER_HOST,
  HEADER_CALLBACK,
  HEADER_NT,
  HEADER_TIMEOUT
};


RadConnect::RadConnect(const char *name) {
  _name = name;
  _http = ESP8266WebServer(RAD_HTTP_PORT);
}


void RadConnect::add(RadThing *thing) {
  _things.add(thing);
}


bool RadConnect::begin(void) {

  // Prepare the uuid
  uint32_t chipId = ESP.getChipId();
  sprintf(_uuid, "38323636-4558-4dda-9188-cda0e6%02x%02x%02x",
  (uint16_t) ((chipId >> 16) & 0xff),
  (uint16_t) ((chipId >>  8) & 0xff),
  (uint16_t)   chipId        & 0xff);

  Serial.println("ChipId: ");
  Serial.println(chipId);
  Serial.println((uint16_t) ((chipId >> 16) & 0xff));
  Serial.println((uint16_t) ((chipId >>  8) & 0xff));
  Serial.println((uint16_t)   chipId        & 0xff);

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Prepare the JSON response
  JsonArray& things = _thingsBuffer.createArray();
  RadThing* thing;
  for(int i = 0; i < _things.size(); i++) {
    thing = _things.get(i);
    JsonObject& thing_json = things.createNestedObject();
    char *type = "";
    switch(thing->getType()) {
      case SwitchBinary:
        type = "SwitchBinary";
        break;
    }
    thing_json["type"] = type;
    thing_json["name"] = thing->getName();
  }
  things.printTo(_thingsString, sizeof(_thingsString));

  on("/devices/{}/events", std::bind(&RadConnect::handleTest, this, std::placeholders::_1));

  // SSDP
  // Serial.println("Starting http...");
  //_http.on("/", std::bind(&RadConnect::handleRoot, this));
  _http.on("/" RAD_INFO_PATH, std::bind(&RadConnect::handleInfo, this));
  _http.on("/" RAD_COMMAND_PATH, std::bind(&RadConnect::handleCommand, this));
  _http.on("/" RAD_SUBSCRIBE_PATH, std::bind(&RadConnect::handleSubscribe, this));
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


RadThing *RadConnect::getThing(const char *name) {
  RadThing* thing = NULL;
  for(int i = 0; i < _things.size(); i++) {
    if(strcmp(_things.get(i)->getName(), name) == 0) {
      thing = _things.get(i);
      break;
    }
  }
  return thing;
}


void RadConnect::on(const char* uri, PATH_FP fn) {
  on(uri, HTTP_ANY, fn);
}


void RadConnect::on(const char* uri, HTTPMethod method, PATH_FP fn) {
  _http.addHandler(new PathSegmentRequestHandler(fn, uri, method));
}


void RadConnect::handleTest(LinkedList<String>& segments) {
  Serial.println("/devices/{}/events");
  for(int i = 0; i < segments.size(); i++) {
    Serial.print("Segment " + (String)i + ": ");
    Serial.println(segments.get(i));
  }
  _http.send(200, "application/json", "/devices/{}/events");
}


void RadConnect::handleInfo(void) {
  Serial.println("/info");
  _http.client().printf(_ssdp_schema_template,
    _name,
    _uuid,
    _thingsString
  );
}


void RadConnect::handleCommand(void) {
  Serial.println("/command");

  int code = 200;
  String message = "";
  if(_http.arg("name") == "" || _http.arg("type") == "") {
    code = 400;
    message = "{\"error\": \"Missing required parameter(s): 'name' and/or 'type'\"}";
  } else {
    if(_http.arg("type") == "set") {
      if(_http.arg("value") == "") {
        code = 400;
        message = "{\"error\": \"Missing required parameter(s): 'value'\"}";
      } else if(execute(_http.arg("name").c_str(), Set, atoi(_http.arg("value").c_str()))) {
        code = 200;
        message = "{\"result\": true}";
      } else {
        code = 500;
      }
    } else if(_http.arg("type") == "get") {
      uint8_t value = execute(_http.arg("name").c_str(), Get);
      char buff[100];
      snprintf(buff, sizeof(buff), "{\"value\": %d}", value);
      message = buff;
    } else {
      code = 400;
      message = "{\"error\": \"Unsuported command type.\"}";
    }
  }
  _http.send(code, "application/json", message);
}


void RadConnect::handleSubscribe(void) {
  Serial.println("/subscribe");

  String test = "Number of args received: ";
  test += _http.args() + "\n";
  for (int i = 0; i < _http.args(); i++) {
    test += "Arg nº" + (String)i + " –> ";
    test += _http.argName(i) + ": ";
    test += _http.arg(i) + "\n";
  }
  test += "\nNumber of headers received: ";
  test += (String)_http.headers() + "\n";
  for (int i = 0; i < _http.headers(); i++) {
    test += "Header nº" + (String)i + " –> ";
    test += _http.headerName(i) + ": ";
    test += _http.header(i) + "\n";
  }
  Serial.println(test);


  int code = 200;
  String message = "";
  if(_http.arg("name") == "" || _http.arg("type") == "") {
    code = 400;
    message = "{\"error\": \"Missing required parameter(s): 'name' and/or 'type'\"}";
  } else {
    RadThing* thing = getThing(_http.arg("name").c_str());
    if(thing == NULL) {
      code = 404;
      message = "{\"error\": \"Unable to find device.\"}";
    } else if(_http.arg("type") == "state") {
      Subscription *subscription = thing->subscribe(State, _http.header(HEADER_CALLBACK).c_str());
      char sid[100];
      snprintf(sid, sizeof(sid), "uuid:%s", subscription->getSid());
      _http.sendHeader("SID", sid);
      _http.sendHeader(HEADER_TIMEOUT, _http.header(HEADER_TIMEOUT));
    } else {
      code = 400;
      message = "{\"error\": \"Unsuported event type.\"}";
    }
  }

  //_http.sendHeader(HEADER_TIMEOUT, _http.header(HEADER_TIMEOUT));
  _http.send(code, "application/json", message);
}


uint8_t RadConnect::execute(const char* name, CommandType command_type) {
  RadThing* thing;
  for(int i = 0; i < _things.size(); i++) {
    thing = _things.get(i);
    if(strcmp(thing->getName(), name) == 0) {
      return thing->execute(command_type);
    }
  }
}


bool RadConnect::execute(const char* name, CommandType command_type, uint8_t value) {
  RadThing* thing;
  for(int i = 0; i < _things.size(); i++) {
    thing = _things.get(i);
    if(strcmp(thing->getName(), name) == 0) {
      return thing->execute(command_type, value);
    }
  }
}


RadThing::RadThing(DeviceType type, const char *name) {
  _type = type;
  _name = name;
  _set_callback = NULL;
  _get_callback = NULL;
  _subscription_count = 0;
}


uint8_t RadThing::execute(CommandType command_type) {
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


bool RadThing::execute(CommandType command_type, uint8_t value) {
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


void RadThing::send(EventType event_type) {

}


void RadThing::send(EventType event_type, uint8_t value) {
  Subscription *s;
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
      http.begin(s->getUrl());
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


Subscription *RadThing::subscribe(EventType type, const char *callback, int timeout) {

  Subscription *s;
  for(int i = 0; i < _subscriptions.size(); i++) {
    s = _subscriptions.get(i);
    if(s->getType() == type && strcmp(s->getCallback(), callback) == 0) {
      Serial.print("Deleting Subscription: ");
      Serial.println(s->getSid());
      _subscriptions.remove(i);
      delete s;
      i -= 1;
    }
  }

  _subscription_count += 1;
  char sid[SSDP_UUID_SIZE];
  uint32_t chipId = ESP.getChipId();
  sprintf(sid, "38323636-4558-4dda-9188-cd%02x%02x%02x%02x%02x",
  (uint16_t) ((chipId >> 16) & 0xff),
  (uint16_t) ((chipId >>  8) & 0xff),
  (uint16_t)   chipId        & 0xff ,
              _id,
              _subscription_count);
  Subscription *subscription = new Subscription(sid, type, callback, timeout);
  _subscriptions.add(subscription);
  Serial.print("SID: ");
  Serial.println(sid);
  return subscription;
}
