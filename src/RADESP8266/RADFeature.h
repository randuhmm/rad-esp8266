
#pragma once


#pragma once

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <LinkedList.h>
#include "Defines.h"
#include "Types.h"
#include "RADSubscription.h"


class RADFeature {

  private:

    FeatureType _type;
    const char* _id;
    const char* _name;

    GetFp             _getCb;
    SetBoolFp         _setBoolCb;
    SetByteFp         _setByteCb;
    SetByteArrayFp    _setByteArrayCb;
    TriggerFp         _triggerCb;

    LinkedList<RADSubscription*> _subscriptions;

  public:

    RADFeature(FeatureType type, const char* id, const char* name=NULL);

    FeatureType getType() { return _type; };
    const char* getId() { return _id; };
    const char* getName() { return _name; };

    void callback(CommandType command_type, GetFp func) { _getCb = func; };
    void callback(CommandType command_type, SetBoolFp func) { _setBoolCb = func; };
    void callback(CommandType command_type, SetByteFp func) { _setByteCb = func; };
    void callback(CommandType command_type, SetByteArrayFp func) { _setByteArrayCb = func; };
    void callback(CommandType command_type, TriggerFp func) { _triggerCb = func; };

    bool execute(CommandType command_type, RADPayload* payload, RADPayload* response);

    void send(EventType event_type);
    void send(EventType event_type, bool data);
    void send(EventType event_type, uint8_t data);
    void send(EventType event_type, uint8_t* data, uint8_t len);
    void sendEvent(EventType event_type, JsonObject& json_body);

    void add(RADSubscription* subscription);
    void remove(RADSubscription* subscription);
};
