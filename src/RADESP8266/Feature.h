
#pragma once

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LinkedList.h>
#include <Wire.h>
#include "Defines.h"
#include "Types.h"
#include "Subscription.h"

namespace RAD {

  // Forward Declaration of ESP8266Server
  class ESP8266Server;

  class Feature {

    private:

      FeatureType _type;
      const char* _id;
      const char* _name;

      bool _isSlave = false;
      uint8_t _slaveId;
      uint8_t _slaveAddress;
      uint8_t _slaveDataOut[MAX_SLAVE_DATA_SIZE];
      uint8_t _slaveDataIn[MAX_SLAVE_DATA_SIZE];

      ESP8266Server* _server;

      GetFp             _getCb;
      SetBoolFp         _setBoolCb;
      SetByteFp         _setByteCb;
      SetByteArrayFp    _setByteArrayCb;
      TriggerFp         _triggerCb;

      LinkedList<Subscription*> _subscriptions;

    public:

      Feature(FeatureType type, const char* id, const char* name=NULL);

      FeatureType getType() { return _type; };
      const char* getId() { return _id; };
      const char* getName() { return _name; };

      void callback(CommandType command_type, GetFp func) { _getCb = func; };
      void callback(CommandType command_type, SetBoolFp func) { _setBoolCb = func; };
      void callback(CommandType command_type, SetByteFp func) { _setByteCb = func; };
      void callback(CommandType command_type, SetByteArrayFp func) { _setByteArrayCb = func; };
      void callback(CommandType command_type, TriggerFp func) { _triggerCb = func; };

      bool execute(CommandType command_type, Payload* payload, Payload* response);

      bool executeTriggerCb(Payload* payload, Payload* response);
      bool executeSetCb(Payload* payload, Payload* response);
      bool executeGetCb(Payload* payload, Payload* response);

      void send(EventType event_type);
      void send(EventType event_type, bool data);
      void send(EventType event_type, uint8_t data);
      void send(EventType event_type, uint8_t* data, uint8_t len);
      void sendEvent(EventType event_type, JsonObject& json_body);

      void add(Subscription* subscription);
      void remove(Subscription* subscription);

      void enableSlave(uint8_t id, uint8_t address = NULL);
      bool sendToSlave(CommandType command_type, Payload* payload, Payload* response);
  };

}
