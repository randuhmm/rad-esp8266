/*
 Copyright (C) 2016 Jonny Morrill jonny@morrill.me>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * @file RADESP8266.h
 *
 * Class declaration for RADConnector, RADFeature and helpers
 */

#ifndef __RADESP8266_H__
#define __RADESP8266_H__

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <LinkedList.h>

#define MAX_CALLBACK_SIZE 255
#define RAD_MIN_TIMEOUT 90
#define RAD_MIN_WRITE_INTERVAL 60

#define RAD_HTTP_PORT 80
#define RAD_DEVICE_TYPE "urn:rad:device:esp8266:1"
#define RAD_MODEL_NAME "RAD-ESP8266"
#define RAD_MODEL_NUM "9001"
#define RAD_INFO_URL "http://example.com"

#define RAD_INFO_PATH "/"
#define RAD_FEATURES_PATH "/features"
#define RAD_SUBSCRIPTIONS_PATH "/subscriptions"
#define RAD_COMMANDS_PATH "/commands"
#define RAD_EVENTS_PATH "/events"

#define RAD_SUBSCRIPTIONS_FILE "/rad-subscriptions.json"

#define HEADER_HOST      "HOST"
#define HEADER_CALLBACK  "CALLBACK"
#define HEADER_NT        "NT"
#define HEADER_TIMEOUT   "TIMEOUT"


// Feature Types
enum FeatureType {
    NullFeature      = 0,
    SwitchBinary     = 1,
    SensorBinary     = 2,
    SwitchMultiLevel = 3,
    SensorMultiLevel = 4,
    TriggerFeature   = 5
};

// Command Types
enum CommandType {
    NullCommand  = 0,
    Get          = 1,
    Set          = 2,
    Trigger      = 3
};

// Event Types
enum EventType {
    NullEvent  = 0,
    All        = 1,
    Start      = 2,
    State      = 3
};

// Payload Types
enum PayloadType {
  NullPayload      = 0,
  BoolPayload      = 1,
  BytePayload      = 2,
  ByteArrayPayload = 3
};

// 8-bit Integer Definition
typedef unsigned char uint8_t;

// Payload Struct Definition
struct RADPayload {
  PayloadType type;
  uint8_t len;
  uint8_t* data;
};

// Callback Definitions
typedef bool (* TriggerFp)();
typedef bool (* SetBoolFp)(bool);
typedef bool (* SetByteFp)(uint8_t);
typedef bool (* SetByteArrayFp)(uint8_t*, uint8_t);
typedef RADPayload* (* GetFp)(void);

// Forward Declaration of RADFeature
class RADFeature;

static const char* _info_template =
  "{\r\n"
  "    \"name\": \"%s\",\r\n"
  "    \"type\": \"" RAD_DEVICE_TYPE "\",\r\n"
  "    \"model\": \"" RAD_MODEL_NAME "\",\r\n"
  "    \"description\": \"Rad ESP8266 WiFi Module for IoT Integration\",\r\n"
  "    \"serial\": \"\",\r\n"
  "    \"UDN\": \"uuid:%s\",\r\n"
  "    \"links\": {\r\n"
  "        \"features\": \"/features\",\r\n"
  "        \"commands\": \"/commands\",\r\n"
  "        \"events\": \"/events\",\r\n"
  "        \"subscriptions\": \"/subscriptions\"\r\n"
  "    }\r\n"
  "}\r\n"
  "\r\n";


class RADSubscription {

  private:

    char _sid[SSDP_UUID_SIZE];
    char _callback[MAX_CALLBACK_SIZE];
    EventType _type;
    int _timeout;
    long _started;
    long _end;
    RADFeature* _feature;
    int _calls;
    int _errors;

  public:

    RADSubscription(RADFeature* feature, const char* sid, EventType type,
                    const char* callback, int timeout, int calls=0,
                    int errors=0) {
      _feature = feature;
      _type = type;
      _timeout = timeout;
      _started = millis();
      _end = _started + timeout * 1000;
      _calls = calls;
      _errors = errors;
      strncpy(_sid, sid, sizeof(_sid));
      strncpy(_callback, callback, sizeof(_callback));
    };
    EventType getType() { return _type; };
    char* getSid() { return _sid; };
    char* getCallback() { return _callback; };
    int getTimeout() { return _timeout; };
    int getDuration(long current) { return (current - _started) / 1000; }
    RADFeature* getFeature() { return _feature; };
    bool isActive(long current) {
      return _end > current;
    }
};


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
    void sendEvent(EventType event_type, String body);

    void add(RADSubscription* subscription);
    void remove(RADSubscription* subscription);
};


class RADConnector
{

  private:

    const char* _name;
    bool _started;
    LinkedList<RADFeature*> _features;
    LinkedList<RADSubscription*> _subscriptions;
    char _uuid[SSDP_UUID_SIZE];
    String _info;
    ESP8266WebServer _http;


    uint8_t _subscriptionCount;
    long _lastWrite;
    bool _subscriptionsChanged;

    // HTTP Path Handler Functions
    void handleInfo(void);
    void handleFeatures(void);
    //void handleFeature(RADFeature* feature);
    void handleSubscriptions(RADFeature* feature);
    void handleCommands(RADFeature* feature);
    void handleEvents(RADFeature* feature);
    // void handleSubscription(LinkedList<String>& segments);

    // Execution Methods
    bool execute(const char* feature_id, CommandType command_type, RADPayload* response);
    bool execute(const char* feature_id, CommandType command_type, bool data, RADPayload* response);
    bool execute(const char* feature_id, CommandType command_type, uint8_t data, RADPayload* response);
    //bool execute(const char* feature_id, CommandType command_type, uint8_t* data, uint8_t len, RADPayload* response);
    bool execute(const char* feature_id, CommandType command_type, RADPayload* payload, RADPayload* response);

  public:

    RADConnector(const char* name);

    void add(RADFeature* feature);

    RADSubscription* subscribe(RADFeature* feature, EventType event_type,
                               const char* callback, int timeout=RAD_MIN_TIMEOUT);
    void unsubscribe(int index);

    bool begin(void);
    void update(void);

    RADFeature* getFeature(const char* feature_id);

    static RADPayload* BuildPayload(bool data);
    static RADPayload* BuildPayload(uint8_t data);
    static RADPayload* BuildPayload(uint8_t* data, uint8_t len);

};


#endif // __RADESP8266_H__
