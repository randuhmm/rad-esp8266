
#include "ESP8266Server.h"

using namespace RAD;

ESP8266Server::ESP8266Server(const char* name, Config* config) {
  _name = name;
  _http = new ESP8266WebServer(RAD_HTTP_PORT);
  _subscriptionCount = 0;
  _lastWrite = 0;
  _wireEnabled = false;
}


static const char* HEADERS[] = {
  HEADER_HOST,
  HEADER_CALLBACK,
  HEADER_NT,
  HEADER_TIMEOUT
};


void ESP8266Server::add(Feature* feature) {
  _features.add(feature);
}


void ESP8266Server::enableWire(int sda, int scl) {
  if(!_started) {
    _wireEnabled = true;
    _wireSDA = sda;
    _wireSCL = scl;
  }
}


Subscription* ESP8266Server::subscribe(Feature* feature, EventType type,
                                    const char* callback, int timeout) {
  Subscription* s;
  Feature* f;
  for(int i = 0; i < _subscriptions.size(); i++) {
    s = _subscriptions.get(i);
    f = s->getFeature();
    if(feature == f && s->getType() == type && strcmp(s->getCallback(), callback) == 0) {
      unsubscribe(i);
      i -= 1;
    }
  }

  _subscriptionCount += 1;
  char sid[SSDP_UUID_SIZE];
  uint32_t chipId = ESP.getChipId();
  sprintf(sid, "38323636-4558-4dda-9188-cd%02x%02x%02x%04x",
  (uint16_t) ((chipId >> 16) & 0xff),
  (uint16_t) ((chipId >>  8) & 0xff),
  (uint16_t)   chipId        & 0xff ,
              _subscriptionCount);
  s = new Subscription(feature, sid, type, callback, timeout);
  _subscriptions.add(s);
  feature->add(s);
  _subscriptionsChanged = true;
  return s;
}


void ESP8266Server::unsubscribe(int index) {
  Subscription* s;
  s = _subscriptions.get(index);
  _subscriptions.remove(index);
  Feature* feature = s->getFeature();
  feature->remove(s);
  delete s;
  _subscriptionsChanged = true;
}


bool ESP8266Server::begin(void) {

  // Start the SPIFFS object
  SPIFFS.begin();

  // Load the existing subscriptions from SPIFFS
  if(SPIFFS.exists(RAD_SUBSCRIPTIONS_FILE)) {
    File f = SPIFFS.open(RAD_SUBSCRIPTIONS_FILE, "r");
    // Check if file opened sucessfully
    if(f) {
      String subcription_data = f.readString();
      f.close();
      StaticJsonBuffer<2048> jsonBuffer;
      JsonObject& subscription_json = jsonBuffer.parseObject(subcription_data);
      if(subscription_json.containsKey("count")) {
        _subscriptionCount = subscription_json["count"];
      }
      if(subscription_json.containsKey("subscriptions")) {
        JsonArray& subscription_array = subscription_json["subscriptions"];
        Feature* feature;
        Subscription* s;
        for (auto value : subscription_array) {
          JsonObject& subscription = value.as<JsonObject&>();
          // TODO: print out the data for testing
          if(!subscription.containsKey("id") ||
             !subscription.containsKey("feature_id") ||
             !subscription.containsKey("type") ||
             !subscription.containsKey("callback") ||
             !subscription.containsKey("timeout") ||
             !subscription.containsKey("calls") ||
             !subscription.containsKey("errors")) {
            continue;
          } else {
            const char* id = subscription["id"];
            const char* feature_id = subscription["feature_id"];
            EventType type = getEventType(subscription["type"]);
            const char* callback = subscription["callback"];
            int timeout = subscription["timeout"];
            int calls = subscription["calls"];
            int errors = subscription["errors"];
            feature = getFeature(feature_id);
            if(feature == NULL) {
              continue;
            }
            s = new Subscription(feature, id, type, callback, timeout);
            _subscriptions.add(s);
            feature->add(s);
          }
        }
      }
    }
  }

  // Prepare the uuid
  uint32_t chipId = ESP.getChipId();
  sprintf(_uuid, "38323636-4558-4dda-9188-cda0e6%02x%02x%02x",
  (uint16_t) ((chipId >> 16) & 0xff),
  (uint16_t) ((chipId >>  8) & 0xff),
  (uint16_t)   chipId        & 0xff);

  // Prepare the info response
  char buffer[512];
  sprintf(buffer, _info_template,
    _name,
    _uuid
  );
  _info = String(buffer);

  // Debugging...
  // Serial.println("ChipId: ");
  // Serial.println(chipId);
  // Serial.println((uint16_t) ((chipId >> 16) & 0xff));
  // Serial.println((uint16_t) ((chipId >>  8) & 0xff));
  // Serial.println((uint16_t)   chipId        & 0xff);
  // Serial.print("IP address: ");
  // Serial.println(WiFi.localIP());

  // Add the HTTP handlers
  _http->on(RAD_INFO_PATH, std::bind(&ESP8266Server::handleInfo, this));
  _http->on(RAD_FEATURES_PATH, std::bind(&ESP8266Server::handleFeatures, this));
  _http->on(RAD_SUBSCRIPTIONS_PATH, std::bind(&ESP8266Server::handleSubscriptions, this, (Feature*)NULL));
  _http->on(RAD_COMMANDS_PATH, std::bind(&ESP8266Server::handleCommands, this, (Feature*)NULL));
  _http->on(RAD_EVENTS_PATH, std::bind(&ESP8266Server::handleEvents, this, (Feature*)NULL));

  // Loop through features and add HTTP handlers
  Feature* _http_feature = NULL;
  char _path_buffer[255];
  for(int i = 0; i < _features.size(); i++) {
    _http_feature = _features.get(i);
    snprintf(_path_buffer, sizeof(_path_buffer), RAD_FEATURES_PATH "/%s" RAD_SUBSCRIPTIONS_PATH, _http_feature->getId());
    _http->on(_path_buffer, std::bind(&ESP8266Server::handleSubscriptions, this, _http_feature));
    snprintf(_path_buffer, sizeof(_path_buffer), RAD_FEATURES_PATH "/%s" RAD_COMMANDS_PATH, _http_feature->getId());
    _http->on(_path_buffer, std::bind(&ESP8266Server::handleCommands, this, _http_feature));
    snprintf(_path_buffer, sizeof(_path_buffer), RAD_FEATURES_PATH "/%s" RAD_EVENTS_PATH, _http_feature->getId());
    _http->on(_path_buffer, std::bind(&ESP8266Server::handleEvents, this, _http_feature));
  }

  // Prepare the SSDP configuration
  _http->collectHeaders(HEADERS, 4);
  _http->begin();
  SSDP.setDeviceType(RAD_DEVICE_TYPE);
  SSDP.setName(_name);
  SSDP.setURL("");
  SSDP.setSchemaURL("");
  SSDP.setSerialNumber(WiFi.macAddress());
  SSDP.setModelName(RAD_MODEL_NAME);
  SSDP.setModelNumber(RAD_MODEL_NUM);
  SSDP.setModelURL(RAD_INFO_URL);
  SSDP.setManufacturer("NA");
  SSDP.setManufacturerURL(RAD_INFO_URL);
  SSDP.setHTTPPort(RAD_HTTP_PORT);
  SSDP.begin();

  // Setup the Wire interface if enabled
  if(_wireEnabled) {
    Wire.begin(_wireSDA, _wireSCL);
  }

}


void ESP8266Server::update(void) {
  // loop
  _http->handleClient();
  yield(); // Allow WiFi stack a chance to run

  long current = millis();
  // Check for expired subscriptions
  Subscription* s;
  for(int i = 0; i < _subscriptions.size(); i++) {
    s = _subscriptions.get(i);
    if(!s->isActive(current)) {
      //Serial.println("Found expired subscription!");
      unsubscribe(i);
      i -= 1;
    }
  }
  yield(); // Allow WiFi stack a chance to run

  // Check to see if we need to write to SPIFFS
  if(_subscriptionsChanged && current - _lastWrite >= RAD_MIN_WRITE_INTERVAL) {
    // TODO: Write subscriptions to file
    // Serial.println("Writing subscriptions to file!");
    _lastWrite = current;
    _subscriptionsChanged = false;
  }
  yield(); // Allow WiFi stack a chance to run

}


Feature* ESP8266Server::getFeature(const char* feature_id) {
  Feature* feature = NULL;
  for(int i = 0; i < _features.size(); i++) {
    if(strcmp(_features.get(i)->getId(), feature_id) == 0) {
      feature = _features.get(i);
      break;
    }
  }
  return feature;
}


void ESP8266Server::handleInfo(void) {
  Serial.println("/");
  if(_http->method() == HTTP_GET) {
    _http->send(200, "application/json", _info);
  } else {
    _http->send(405);
  }
}


void ESP8266Server::handleFeatures() {
  Serial.println("ESP8266Server::handleFeatures");
  int code = 200;
  if(_http->method() == HTTP_GET) {
    // Prepare the JSON response
    StaticJsonBuffer<1024> featuresBuffer;
    char featuresString[1024];
    char linkBuff[255];
    JsonArray& features = featuresBuffer.createArray();
    Feature* feature;
    for(int i = 0; i < _features.size(); i++) {
      feature = _features.get(i);
      JsonObject& feature_json = features.createNestedObject();
      feature_json["id"] = feature->getId();
      feature_json["name"] = feature->getName();
      feature_json["type"] = sendFeatureType(feature->getType());
      feature_json["description"] = "";
      JsonObject& links_json = feature_json.createNestedObject("links");
      snprintf(linkBuff, sizeof(linkBuff), RAD_FEATURES_PATH "/%s", feature->getId());
      links_json["details"] = String(linkBuff);
      snprintf(linkBuff, sizeof(linkBuff), RAD_FEATURES_PATH "/%s" RAD_COMMANDS_PATH, feature->getId());
      links_json["commands"] = String(linkBuff);
      snprintf(linkBuff, sizeof(linkBuff), RAD_FEATURES_PATH "/%s" RAD_SUBSCRIPTIONS_PATH, feature->getId());
      links_json["subscriptions"] = String(linkBuff);
      snprintf(linkBuff, sizeof(linkBuff), RAD_FEATURES_PATH "/%s" RAD_EVENTS_PATH, feature->getId());
      links_json["events"] = String(linkBuff);
    }
    features.printTo(featuresString, sizeof(featuresString));
    _http->send(code, "application/json", featuresString);
  } else {
    _http->send(405);
  }
}


void ESP8266Server::handleSubscriptions(Feature* feature) {
  Serial.println("ESP8266Server::handleSubscriptions");
  int code = 200;
  long current = millis();
  if(_http->method() == HTTP_GET) {
    // Prepare the JSON response
    StaticJsonBuffer<1024> subscriptionsBuffer;
    char subscriptionsString[1024];
    JsonArray& subscriptions = subscriptionsBuffer.createArray();
    Feature* featureIt;
    Subscription* subscription;
    for(int i = 0; i < _subscriptions.size(); i++) {
      subscription = _subscriptions.get(i);
      if(subscription->isActive(current)) {
        featureIt = subscription->getFeature();
        if(feature != NULL && feature != featureIt) continue;
        JsonObject& subscription_json = subscriptions.createNestedObject();
        subscription_json["id"] = subscription->getSid();
        subscription_json["feature_id"] = featureIt->getId();
        subscription_json["event_type"] = sendEventType(subscription->getType());
        subscription_json["callback"] = subscription->getCallback();
        subscription_json["timeout"] = subscription->getTimeout();
        subscription_json["duration"] = subscription->getDuration(current);
      }
    }
    subscriptions.printTo(subscriptionsString, sizeof(subscriptionsString));
    _http->send(code, "application/json", subscriptionsString);
  // POST Method
  } else if(_http->method() == HTTP_POST) {
    String message = "";
    StaticJsonBuffer<255> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_http->arg("plain"));
    if(feature == NULL && !root.containsKey("feature_id")) {
      code = 400;
      message = "{\"error\": \"Missing required property 'feature_id'.\"}";
    } else if(!root.containsKey("event_type")) {
      code = 400;
      message = "{\"error\": \"Missing required property 'event_type'.\"}";
    } else if(!root.containsKey("callback")) {
      code = 400;
      message = "{\"error\": \"Missing required property 'callback'.\"}";
    } else {
      EventType type = getEventType(root["event_type"]);
      const char* callback = root["callback"];
      int timeout = RAD_MIN_TIMEOUT;
      if(root.containsKey("timeout")) {
        timeout = root["timeout"];
      }
      Feature* featureTarget;
      if(feature == NULL) {
        const char* feature_id = root["feature_id"];
        featureTarget = getFeature(feature_id);
      } else {
        featureTarget = feature;
      }
      if (featureTarget == NULL) {
        code = 400;
        message = "{\"error\": \"Feature could not be located.\"}";
      } else if(type == NullEvent) {
        code = 400;
        message = "{\"error\": \"Unsuported event type.\"}";
      } else if(timeout < RAD_MIN_TIMEOUT) {
        code = 400;
        message = "{\"error\": \"The timeout property can not be less than 600 seconds.\"}";
      } else {
        Subscription* subscription = subscribe(featureTarget, type, callback, timeout);
        char sid[100];
        snprintf(sid, sizeof(sid), "uuid:%s", subscription->getSid());
        _http->sendHeader("SID", sid);
        _http->sendHeader(HEADER_TIMEOUT, String(timeout));
      }
    }
    _http->send(code, "application/json", message);
  } else {
    _http->send(405);
  }
}


void ESP8266Server::handleCommands(Feature* feature) {
  Serial.println("ESP8266Server::handleCommands");
  int code = 200;
  uint8_t value;
  bool result = false;
  Payload* response = NULL;
  String message = "";
  if(_http->method() == HTTP_POST) {
    Serial.println("ESP8266Server::handleCommands - POST");
    StaticJsonBuffer<255> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_http->arg("plain"));
    Serial.println(_http->arg("plain"));
    if(feature == NULL && !root.containsKey("feature_id")) {
      code = 400;
      message = "{\"error\": \"Missing required property, 'feature_id'.\"}";
    } else  if(!root.containsKey("command_type")) {
      code = 400;
      message = "{\"error\": \"Missing required property, 'command_type'.\"}";
    } else {
      Feature* featureTarget;
      if(feature == NULL) {
        const char* feature_id = root["feature_id"];
        featureTarget = getFeature(feature_id);
      } else {
        featureTarget = feature;
      }
      if(featureTarget == NULL) {
        code = 400;
        message = "{\"error\": \"Invalid 'feature_id' value.\"}";
      } else {
        CommandType type = getCommandType(root["command_type"]);
        switch(type) {
          case Set:
            Serial.println("ESP8266Server::handleCommands - case Set:");
            if(!root.containsKey("data")) {
              code = 400;
              message = "{\"error\": \"Missing required property, 'data'.\"}";
            } else {
              if(root["data"].is<bool>()) {
                result = execute(featureTarget->getId(), Set, (bool)root["data"].as<bool>(), (Payload*)NULL);
              } else if(root["data"].is<int>()) {
                result = execute(featureTarget->getId(), Set, (uint8_t)root["data"].as<int>(), (Payload*)NULL);
              } else if(root["data"].is<char*>()) {
                // TODO: handle base64 encoded data
              }
            }
            break;
          case Get:
            response = new Payload();
            result = execute(featureTarget->getId(), Get, response);
            if(result && response != NULL) {
              switch(response->type) {
                case BoolPayload:
                  if(response->data[0]) {
                    message = "{\"data\": true}";
                  } else {
                    message = "{\"data\": false}";
                  }
                  break;
                case BytePayload:
                  char buff[100];
                  snprintf(buff, sizeof(buff), "{\"data\": %d}", response->data[0]);
                  message = buff;
                  break;
                case ByteArrayPayload:
                  // TODO: handle get payload
                  break;
              }
            } else {
              code = 500;
              message = "{\"error\": \"Failure.\"}";
            }
            break;
          default:
            code = 400;
            message = "{\"error\": \"Unsuported command type.\"}";
            break;
        }
      }
    }
    _http->send(code, "application/json", message);
  } else {
    _http->send(405);
  }
}


void ESP8266Server::handleEvents(Feature* feature) {
  Serial.println("ESP8266Server::handleEvents");
  _http->send(200, "application/json", "ESP8266Server::handleEvents");
  return;
}


bool ESP8266Server::execute(const char* feature_id, CommandType command_type, Payload* response) {
  Serial.println("ESP8266Server::execute - empty");
  return execute(feature_id, command_type, (Payload*)NULL, response);
}


bool ESP8266Server::execute(const char* feature_id, CommandType command_type, bool data, Payload* response) {
  Serial.print("ESP8266Server::execute - bool = ");
  Serial.println(data);
  Payload* payload = ESP8266Server::BuildPayload(data);
  bool result = execute(feature_id, command_type, payload, response);
  delete payload;
  return result;
}


bool ESP8266Server::execute(const char* feature_id, CommandType command_type, uint8_t data, Payload* response) {
  Serial.println("ESP8266Server::execute - byte");
  Payload* payload = ESP8266Server::BuildPayload(data);
  bool result = execute(feature_id, command_type, payload, response);
  delete payload;
  return result;
}


bool ESP8266Server::execute(const char* feature_id, CommandType command_type, Payload* payload, Payload* response) {
  Serial.println("ESP8266Server::execute - payload");
  Feature* feature;
  bool result = false;
  for(int i = 0; i < _features.size(); i++) {
    feature = _features.get(i);
    if(strcmp(feature->getId(), feature_id) == 0) {
      result = feature->execute(command_type, payload, response);
      break;
    }
  }
  return result;
}


Payload* ESP8266Server::BuildPayload(bool data) {
  Payload* payload = new Payload();
  payload->type = BoolPayload;
  payload->len = 1;
  payload->data = (uint8_t*)malloc(payload->len * sizeof(uint8_t));
  if(data) {
    payload->data[0] = 255;
  } else {
    payload->data[0] = 0;
  }
  return payload;
}


Payload* ESP8266Server::BuildPayload(uint8_t data) {
  Payload* payload = new Payload();
  payload->type = BytePayload;
  payload->len = 1;
  payload->data = (uint8_t*)malloc(payload->len * sizeof(uint8_t));
  payload->data[0] = data;
  return payload;
}


Payload* ESP8266Server::BuildPayload(uint8_t* data, uint8_t len) {
  Payload* payload = new Payload();
  payload->type = ByteArrayPayload;
  payload->len = len;
  payload->data = data;
  return payload;
}
