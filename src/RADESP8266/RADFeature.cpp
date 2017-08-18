
#include "RADFeature.h"


RADFeature::RADFeature(FeatureType type, const char* id, const char* name) {
  _type = type;
  _id = id;
  if(name == NULL) {
    _name = _id;
  } else {
    _name = name;
  }
  _getCb = NULL;
  _setBoolCb = NULL;
  _setByteCb = NULL;
  _setByteArrayCb = NULL;
  _triggerCb = NULL;
}


bool RADFeature::execute(CommandType command_type, RADPayload* payload, RADPayload* response) {
  Serial.println("RADFeature::execute");
  bool result = false;
  RADPayload* getResponse;
  TriggerFp trigger = NULL;
  SetBoolFp setBool = NULL;
  SetByteFp setByte = NULL;
  SetByteArrayFp setByteArray = NULL;
  GetFp get = NULL;
  switch(command_type) {
    case Trigger:
      switch(_type) {
        case TriggerFeature:
          trigger = _triggerCb;
          break;
      }
      if(trigger != NULL) {
        result = trigger();
      }
      break;
    case Set:
      Serial.println("RADFeature::execute - case Set:");
      switch(_type) {
        case SwitchBinary:
          setBool = _setBoolCb;
          break;
      }
      if(setBool != NULL) {
        Serial.println("RADFeature::execute - setBool");
        if(payload != NULL && payload->type == BoolPayload && payload->len == 1) {
          result = setBool((bool)payload->data[0]);
        }
      } else if(setByte != NULL) {
        result = false;
      } else if(setByteArray != NULL) {
        result = false;
      }
      break;
    case Get:
      switch(_type) {
        case SwitchBinary:
          get = _getCb;
          break;
      }
      if(get != NULL) {
        getResponse = get();
        result = true;
        if(response != NULL) {
          response->type = getResponse->type;
          response->len = getResponse->len;
          response->data = getResponse->data;
        }
        delete getResponse;
      }
      break;
  }
  return result;
}


void RADFeature::send(EventType event_type) {
  StaticJsonBuffer<1024> json_buffer;
  JsonObject& json_body = json_buffer.createObject();
  json_body["event_type"] = sendEventType(event_type);
  sendEvent(event_type, json_body);
}


void RADFeature::send(EventType event_type, bool data) {
  StaticJsonBuffer<1024> json_buffer;
  JsonObject& json_body = json_buffer.createObject();
  json_body["event_type"] = sendEventType(event_type);
  json_body["data"] = data;
  sendEvent(event_type, json_body);
}


void RADFeature::send(EventType event_type, uint8_t data) {
  StaticJsonBuffer<1024> json_buffer;
  JsonObject& json_body = json_buffer.createObject();
  json_body["event_type"] = sendEventType(event_type);
  json_body["data"] = data;
  sendEvent(event_type, json_body);
}


void RADFeature::send(EventType event_type, uint8_t* data, uint8_t len) {

}


void RADFeature::sendEvent(EventType event_type, JsonObject& json_body) {
  RADSubscription* s;
  char body[1024];
  json_body.printTo(body, sizeof(body));
  for(int i = 0; i < _subscriptions.size(); i++) {
    s = _subscriptions.get(i);
    if(s->getType() == event_type) {
      HTTPClient http;
      http.begin(s->getCallback());
      http.addHeader("SID", s->getSid());
      http.addHeader("RAD-ID", _id);
      http.addHeader("RAD-EVENT", sendEventType(event_type));
      http.addHeader("Content-Type", "application/json");
      int httpCode = http.sendRequest("NOTIFY", body);
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