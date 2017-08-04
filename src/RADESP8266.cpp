#include "RADESP8266.h"


FeatureType getFeatureType(const char* s) {
  FeatureType ft = NullFeature;
  if(strcmp(s, "SwitchBinary") == 0) {
    ft = SwitchBinary;
  } else if(strcmp(s, "SensorBinary") == 0) {
    ft = SensorBinary;
  } else if(strcmp(s, "SwitchMultiLevel") == 0) {
    ft = SwitchMultiLevel;
  } else if(strcmp(s, "SensorMultiLevel") == 0) {
    ft = SensorMultiLevel;
  }
  return ft;
}


const char* sendFeatureType(FeatureType ft) {
  const char* s = "NullFeature";
  if(ft == SwitchBinary) {
    s = "SwitchBinary";
  } else if(ft == SensorBinary) {
    s = "SensorBinary";
  } else if(ft == SwitchMultiLevel) {
    s = "SwitchMultiLevel";
  } else if(ft == SensorMultiLevel) {
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


RADConnector::RADConnector(const char* name) {
  _name = name;
  _http = ESP8266WebServer(RAD_HTTP_PORT);
  _subscriptionCount = 0;
  _lastWrite = 0;
}


void RADConnector::add(RADFeature* feature) {
  _features.add(feature);
}


RADSubscription* RADConnector::subscribe(RADFeature* feature, EventType type,
                                    const char* callback, int timeout) {
  RADSubscription* s;
  RADFeature* f;
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
  s = new RADSubscription(feature, sid, type, callback, timeout);
  _subscriptions.add(s);
  feature->add(s);
  _subscriptionsChanged = true;
  return s;
}


void RADConnector::unsubscribe(int index) {
  RADSubscription* s;
  s = _subscriptions.get(index);
  _subscriptions.remove(index);
  RADFeature* feature = s->getFeature();
  feature->remove(s);
  delete s;
  _subscriptionsChanged = true;
}


bool RADConnector::begin(void) {

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
        RADFeature* feature;
        RADSubscription* s;
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
            s = new RADSubscription(feature, id, type, callback, timeout);
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
  _http.on(RAD_INFO_PATH, std::bind(&RADConnector::handleInfo, this));
  _http.on(RAD_FEATURES_PATH, std::bind(&RADConnector::handleFeatures, this));
  _http.on(RAD_SUBSCRIPTIONS_PATH, std::bind(&RADConnector::handleSubscriptions, this, (RADFeature*)NULL));
  _http.on(RAD_COMMANDS_PATH, std::bind(&RADConnector::handleCommands, this, (RADFeature*)NULL));
  _http.on(RAD_EVENTS_PATH, std::bind(&RADConnector::handleEvents, this, (RADFeature*)NULL));

  // Loop through features and add HTTP handlers
  RADFeature* _http_feature = NULL;
  char _path_buffer[255];
  for(int i = 0; i < _features.size(); i++) {
    _http_feature = _features.get(i);
    snprintf(_path_buffer, sizeof(_path_buffer), RAD_FEATURES_PATH "/%s" RAD_SUBSCRIPTIONS_PATH, _http_feature->getId());
    _http.on(_path_buffer, std::bind(&RADConnector::handleSubscriptions, this, _http_feature));
    snprintf(_path_buffer, sizeof(_path_buffer), RAD_FEATURES_PATH "/%s" RAD_COMMANDS_PATH, _http_feature->getId());
    _http.on(_path_buffer, std::bind(&RADConnector::handleCommands, this, _http_feature));
    snprintf(_path_buffer, sizeof(_path_buffer), RAD_FEATURES_PATH "/%s" RAD_EVENTS_PATH, _http_feature->getId());
    _http.on(_path_buffer, std::bind(&RADConnector::handleEvents, this, _http_feature));
  }

  // Prepare the SSDP configuration
  _http.collectHeaders(HEADERS, 4);
  _http.begin();
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
}


void RADConnector::update(void) {
  // loop
  _http.handleClient();
  yield(); // Allow WiFi stack a chance to run

  long current = millis();
  // Check for expired subscriptions
  RADSubscription* s;
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


RADFeature* RADConnector::getFeature(const char* feature_id) {
  RADFeature* feature = NULL;
  for(int i = 0; i < _features.size(); i++) {
    if(strcmp(_features.get(i)->getId(), feature_id) == 0) {
      feature = _features.get(i);
      break;
    }
  }
  return feature;
}


void RADConnector::handleInfo(void) {
  Serial.println("/");
  if(_http.method() == HTTP_GET) {
    _http.send(200, "application/json", _info);
  } else {
    _http.send(405);
  }
}


void RADConnector::handleFeatures() {
  Serial.println("RADConnector::handleFeatures");
  int code = 200;
  if(_http.method() == HTTP_GET) {
    // Prepare the JSON response
    StaticJsonBuffer<1024> featuresBuffer;
    char featuresString[1024];
    char linkBuff[255];
    JsonArray& features = featuresBuffer.createArray();
    RADFeature* feature;
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
    _http.send(code, "application/json", featuresString);
  } else {
    _http.send(405);
  }
}


void RADConnector::handleSubscriptions(RADFeature* feature) {
  Serial.println("RADConnector::handleSubscriptions");
  int code = 200;
  long current = millis();
  if(_http.method() == HTTP_GET) {
    // Prepare the JSON response
    StaticJsonBuffer<1024> subscriptionsBuffer;
    char subscriptionsString[1024];
    JsonArray& subscriptions = subscriptionsBuffer.createArray();
    RADFeature* featureIt;
    RADSubscription* subscription;
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
    _http.send(code, "application/json", subscriptionsString);
  // POST Method
  } else if(_http.method() == HTTP_POST) {
    String message = "";
    StaticJsonBuffer<255> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_http.arg("plain"));
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
      RADFeature* featureTarget;
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
        RADSubscription* subscription = subscribe(featureTarget, type, callback, timeout);
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


void RADConnector::handleCommands(RADFeature* feature) {
  int code = 200;
  uint8_t value;
  bool result = false;
  RADPayload* response = NULL;
  String message = "";
  if(_http.method() == HTTP_POST) {
    StaticJsonBuffer<255> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(_http.arg("plain"));
    if(feature = NULL && !root.containsKey("feature_id")) {
      code = 400;
      message = "{\"error\": \"Missing required property, 'feature_id'.\"}";
    }else  if(!root.containsKey("command_type")) {
      code = 400;
      message = "{\"error\": \"Missing required property, 'command_type'.\"}";
    } else {
      RADFeature* featureTarget;
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
                result = execute(featureTarget->getId(), Set, value, (RADPayload*)NULL);
                if(result) {
                  message = "{\"result\": true}";
                } else {
                  code = 500;
                }
              }
            }
            break;
          case Get:
            response = new RADPayload();
            result = execute(featureTarget->getId(), Get, response);
            char buff[100];
            snprintf(buff, sizeof(buff), "{\"value\": %d}", response->data[0]);
            message = buff;
            break;
          default:
            code = 400;
            message = "{\"error\": \"Unsuported command type.\"}";
            break;
        }
      }
    }
    _http.send(code, "application/json", message);
  } else {
    _http.send(405);
  }
}


void RADConnector::handleEvents(RADFeature* feature) {
  Serial.println("RADConnector::handleEvents");
  _http.send(200, "application/json", "RADConnector::handleEvents");
  return;
}


bool RADConnector::execute(const char* feature_id, CommandType command_type, RADPayload* response) {
  return execute(feature_id, command_type, (RADPayload*)NULL, response);
}


bool RADConnector::execute(const char* feature_id, CommandType command_type, bool data, RADPayload* response) {
  RADPayload* payload = new RADPayload();
  payload->len = 1;
  payload->data = (uint8_t*)malloc(payload->len * sizeof(uint8_t));
  if(data) {
    payload->data[0] = 0;
  } else {
    payload->data[0] = 255;
  }
  bool result = execute(feature_id, command_type, payload, response);
  delete payload;
  return result;
}


bool RADConnector::execute(const char* feature_id, CommandType command_type, uint8_t data, RADPayload* response) {
  RADPayload* payload = new RADPayload();
  payload->len = 1;
  payload->data = (uint8_t*)malloc(payload->len * sizeof(uint8_t));
  payload->data[0] = data;
  bool result = execute(feature_id, command_type, payload, response);
  delete payload;
  return result;
}


bool RADConnector::execute(const char* feature_id, CommandType command_type, RADPayload* payload, RADPayload* response) {
  RADFeature* feature;
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


RADFeature::RADFeature(FeatureType type, const char* id, const char* name) {
  _type = type;
  _id = id;
  if(name == NULL) {
    _name = _id;
  } else {
    _name = name;
  }
  _setCallback = NULL;
  _getCallback = NULL;
}


bool RADFeature::execute(CommandType command_type, RADPayload* payload, RADPayload* response) {
  bool result = false;
  RADPayload* tmpResponse;
  SetFp set = NULL;
  GetFp get = NULL;
  switch(command_type) {
    case Set:
      switch(_type) {
        case SwitchBinary:
          set = _setCallback;
          break;
      }
      if(set != NULL) {
        result = set(payload);
      }
      break;
    case Get:
      switch(_type) {
        case SwitchBinary:
          get = _getCallback;
          break;
      }
      if(get != NULL) {
        tmpResponse = get();
        result = true;
        if(response != NULL) {
          response->len = tmpResponse->len;
          response->data = tmpResponse->data;
        }
        delete tmpResponse;
      }
      break;
  }
  return result;
}


void RADFeature::send(EventType event_type) {

}


void RADFeature::send(EventType event_type, bool data) {

}


void RADFeature::send(EventType event_type, uint8_t data) {

}


void RADFeature::send(EventType event_type, uint8_t* data, uint8_t len) {

}


void RADFeature::send(EventType event_type, RADPayload* payload) {
  RADSubscription* s;
  String message = "";
  // switch(event_type) {
  //   case State:
  //     switch(_type) {
  //       case SwitchBinary:
  //       char buff[100];
  //       snprintf(buff, sizeof(buff), "{\"type\": \"state\", \"value\": %d}", value);
  //       message = buff;
  //     }
  //     break;
  // }

  for(int i = 0; i < _subscriptions.size(); i++) {
    s = _subscriptions.get(i);
    if(s->getType() == event_type) {
      HTTPClient http;
      http.begin(s->getCallback());
      http.addHeader("SID", s->getSid());
      http.addHeader("RAD-ID", _id);
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.sendRequest("NOTIFY", message);
      if(httpCode > 0) {
        if(httpCode == HTTP_CODE_OK) {
            String response = http.getString();
        }
      } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
      }
      http.end();
    }
  }
}


void RADFeature::add(RADSubscription* subscription) {
  _subscriptions.add(subscription);
}


void RADFeature::remove(RADSubscription* subscription) {
  for(int i = 0; i < _subscriptions.size(); i++) {
    if(_subscriptions.get(i) == subscription) {
      _subscriptions.remove(i);
      break;
    }
  }
}
