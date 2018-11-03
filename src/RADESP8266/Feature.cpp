
#include "Feature.h"

using namespace RAD;

Feature::Feature(FeatureType type, const char* id, const char* name) {
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


void Feature::enableSlave(uint8_t id, uint8_t address) {
  _isSlave = true;
  _slaveId = id;
  if(address == NULL) {
    _slaveAddress = RAD_DEFAULT_SLAVE_ADDRESS;
  } else {
    _slaveAddress = address;
  }
}


bool Feature::execute(CommandType command_type, Payload* payload, Payload* response) {
  Serial.println("Feature::execute");
  if(_isSlave) {
    return sendToSlave(command_type, payload, response);
  } else {
    switch(command_type) {
      case Trigger:
        return executeTriggerCb(payload, response);
        break;
      case Set:
        return  executeSetCb(payload, response);
        break;
      case Get:
        return executeGetCb(payload, response);
        break;
    }
  }
}


bool Feature::sendToSlave(CommandType command_type, Payload* payload, Payload* response) {
  // Data Bytes:
  // [0] = slave_id
  // [1] = command_type
  // [2] = payload_type
  // [3] = payload_len
  // [4] = payload
  // ...
  // [N] = payload

  // Basic payload is always at least 4 bytes
  uint8_t payloadBytes = 4;
  _slaveDataOut[0] = _slaveId;
  _slaveDataOut[1] = (uint8_t)command_type;
  if(payload != NULL) {
    _slaveDataOut[2] = (uint8_t)payload->type;
    _slaveDataOut[3] = payload->len;
    uint8_t i = 0;
    for(; i < payload->len; i++) {
      _slaveDataOut[4 + i] = payload->data[i];
    }
    payloadBytes += payload->len;
  } else {
    _slaveDataOut[2] = (uint8_t)NullPayload;
    _slaveDataOut[3] = 0;
  }

  // Send the data
  Wire.beginTransmission(_slaveAddress);
  Wire.write(_slaveDataOut, payloadBytes);
  Wire.endTransmission();

  // Retrieve the response if we are issuing a Get command
  if(command_type == Get) {
    delay(1);
    // Wire.requestFrom(_slaveAddress, 10);
    // Serial.print("Request Return:[");
    // while (Wire.available()) {
    //   delay(1);
    //   char c = Wire.read();
    //   Serial.print(c);
    // }
    // Serial.println("]");
  }

}


bool Feature::executeTriggerCb(Payload* payload, Payload* response) {
  Serial.println("Feature::executeTriggerCb");
  if(_triggerCb == NULL) return false;
  switch(_type) {
    case TriggerFeature:
      return _triggerCb();
      break;
    default:
      return false;
  }
}


bool Feature::executeSetCb(Payload* payload, Payload* response) {
  Serial.println("Feature::executeSetCb");
  if(payload == NULL) return false;
  switch(payload->type) {
    case BoolPayload:
      if(_setBoolCb == NULL) return false;
      if(payload->len != 1) return false;
      switch(_type) {
        case SwitchBinary:
          return _setBoolCb((bool)payload->data[0]);
          break;
        default:
          return false;
      }
      break;
    case BytePayload:
      if(_setByteCb == NULL) return false;
      if(payload->len != 1) return false;
      switch(_type) {
        case SwitchMultiLevel:
          return _setByteCb((uint8_t)payload->data[0]);
          break;
        default:
          return false;
      }
      break;
    case ByteArrayPayload:
      return false;
      break;
  }
  return false;
}


bool Feature::executeGetCb(Payload* payload, Payload* response) {
  Serial.println("Feature::executeGetCb");
  if(_getCb == NULL) return false;
  switch(_type) {
    case SwitchBinary:
    case SwitchMultiLevel:
      Payload* getResponse;
      getResponse = _getCb();
      if(response != NULL) {
        response->type = getResponse->type;
        response->len = getResponse->len;
        response->data = getResponse->data;
      }
      delete getResponse;
      return true;
    default:
      return false;
  }
}


void Feature::send(EventType event_type) {
  StaticJsonBuffer<1024> json_buffer;
  JsonObject& json_body = json_buffer.createObject();
  json_body["event_type"] = sendEventType(event_type);
  sendEvent(event_type, json_body);
}


void Feature::send(EventType event_type, bool data) {
  StaticJsonBuffer<1024> json_buffer;
  JsonObject& json_body = json_buffer.createObject();
  json_body["event_type"] = sendEventType(event_type);
  json_body["data"] = data;
  sendEvent(event_type, json_body);
}


void Feature::send(EventType event_type, uint8_t data) {
  StaticJsonBuffer<1024> json_buffer;
  JsonObject& json_body = json_buffer.createObject();
  json_body["event_type"] = sendEventType(event_type);
  json_body["data"] = data;
  sendEvent(event_type, json_body);
}


void Feature::send(EventType event_type, uint8_t* data, uint8_t len) {

}


void Feature::sendEvent(EventType event_type, JsonObject& json_body) {
  Subscription* s;
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


void Feature::add(Subscription* subscription) {
  _subscriptions.add(subscription);
}


void Feature::remove(Subscription* subscription) {
  for(int i = 0; i < _subscriptions.size(); i++) {
    if(_subscriptions.get(i) == subscription) {
      _subscriptions.remove(i);
      break;
    }
  }
}
