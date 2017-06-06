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
    SensorMultiLevel = 4
};

// Command Types
enum CommandType {
    NullCommand  = 0,
    Get          = 1,
    Set          = 2
};

// Event Types
enum EventType {
    NullEvent  = 0,
    All        = 1,
    Start      = 2,
    State      = 3
};

typedef unsigned char uint8_t;

typedef void (* SET_FP)(uint8_t);
typedef uint8_t (* GET_FP)(void);

class RADFeature;

static const char* _info_template =
  "{\r\n"
  "    \"name\": \"%s\",\r\n"
  "    \"type\": \"" RAD_DEVICE_TYPE "\",\r\n"
  "    \"model\": \"" RAD_MODEL_NAME "\",\r\n"
  "    \"description\": \"Rad ESP8266 WiFi Module for IoT Integration\",\r\n"
  "    \"serial\": \"\",\r\n"
  "    \"UDN\": \"uuid:%s\"\r\n"
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

    RADSubscription(RADFeature* feature, const char* sid, EventType type, const char* callback, int timeout, int calls=0, int errors=0) {
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
    const char* _name;

    SET_FP _setCallback;
    GET_FP _getCallback;

    LinkedList<RADSubscription*> _subscriptions;

  public:

    RADFeature(FeatureType type, const char* name);

    FeatureType getType() { return _type; };
    const char* getName() { return _name; };

    void callback(CommandType command_type, SET_FP func) { _setCallback = func; };
    void callback(CommandType command_type, GET_FP func) { _getCallback = func; };

    uint8_t execute(CommandType command_type);
    bool execute(CommandType command_type, uint8_t value);

    void send(EventType event_type);
    void send(EventType event_type, uint8_t value);

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
    void handleSubscriptions(void);
    void handleCommands(void);
    // void handleEvents(LinkedList<String>& segments);
    // void handleSubscription(LinkedList<String>& segments);

    // Execution Methods
    uint8_t execute(const char* feature_name, CommandType command_type);
    uint8_t execute(RADFeature* feature, CommandType command_type);
    bool execute(const char* feature_name, CommandType command_type, uint8_t value);
    bool execute(RADFeature* feature, CommandType command_type, uint8_t value);

  public:

    RADConnector(const char* name);

    void add(RADFeature* feature);

    RADSubscription* subscribe(RADFeature* feature, EventType event_type,
                            const char* callback, int timeout=RAD_MIN_TIMEOUT);
    void unsubscribe(int index);

    bool begin(void);
    void update(void);

    RADFeature* getFeature(const char* feature_name);

};


#endif // __RADESP8266_H__
