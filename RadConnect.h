/*
 Copyright (C) 2016 Jonny Morrill jonny@morrill.me>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * @file RadConnect.h
 *
 * Class declaration for RadConnect and helper enums
 */

#ifndef __RAD_CONNECT_H__
#define __RAD_CONNECT_H__

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <LinkedList.h>

#define MAX_THINGS 8
#define MAX_CALLBACK_SIZE 255

#define RAD_HTTP_PORT 8080
#define RAD_DEVICE_TYPE "urn:rad:device:esp8266:1"
#define RAD_MODEL_NAME "RAD-ESP8266"
#define RAD_MODEL_NUM "9001"
#define RAD_INFO_URL "http://example.com"

#define RAD_INFO_PATH "info"
#define RAD_COMMAND_PATH "command"
#define RAD_SUBSCRIBE_PATH "subscribe"


#define HEADER_HOST      "HOST"
#define HEADER_CALLBACK  "CALLBACK"
#define HEADER_NT        "NT"
#define HEADER_TIMEOUT   "TIMEOUT"


// Device Types
enum DeviceType {
    SwitchBinary     = 1,
    SensorBinary     = 2,
    SwitchMultiLevel = 3,
    SensorMultiLevel = 4
};

// Message Types
enum MessageType {
    Command = 1,
    Event   = 2
};

// Command Types
enum CommandType {
    Get = 1,
    Set = 2
};

// Event Types
enum EventType {
    Start = 1,
    State = 2
};

typedef unsigned char       uint8_t;

typedef void (* SET_FP)(uint8_t);
typedef uint8_t (* GET_FP)(void);


static const char* _ssdp_schema_template =
  "HTTP/1.1 200 OK\r\n"
  "Content-Type: application/json\r\n"
  "Connection: close\r\n"
  "Access-Control-Allow-Origin: *\r\n"
  "\r\n"
  "{\r\n"
  "    \"deviceName\": \"%s\",\r\n"
  "    \"deviceType\": \"" RAD_DEVICE_TYPE "\",\r\n"
  "    \"modelDescription\": \"Rad ESP8266 WiFi Module for IoT Integration\",\r\n"
  "    \"modelName\": \"" RAD_MODEL_NAME "\",\r\n"
  "    \"serialNum\": \"\",\r\n"
  "    \"UDN\": \"uuid:%s\",\r\n"
  "    \"things\": %s\r\n"
  "}\r\n"
  "\r\n";


class Subscription {

  private:

    char _sid[SSDP_UUID_SIZE];
    char _callback[MAX_CALLBACK_SIZE];
    char _url[MAX_CALLBACK_SIZE];
    EventType _type;
    int _timeout;

  public:

    Subscription(const char *sid, EventType type, const char *callback, int timeout=0) {
      _type = type;
      _timeout = timeout;
      strncpy(_sid, sid, sizeof(_sid));
      strncpy(_callback, callback, sizeof(_callback));
      memset(_url, 0, sizeof(_url));
      strncpy(_url, &callback[1], strlen(callback) - 2);
    };
    EventType getType() { return _type; };
    char *getSid() { return _sid; };
    char *getCallback() { return _callback; };
    char *getUrl() { return _url; };

};


class RadThing {

  private:

    DeviceType _type;
    const char *_name;
    uint8_t _id;
    uint8_t _subscription_count;

    SET_FP _set_callback;
    GET_FP _get_callback;

    LinkedList<Subscription*> _subscriptions;

  public:

    RadThing(DeviceType type, const char *name);

    DeviceType getType() { return _type; };
    const char *getName() { return _name; };
    void setId(uint8_t id) { _id = id; };

    void callback(CommandType command_type, SET_FP func) { _set_callback = func; };
    void callback(CommandType command_type, GET_FP func) { _get_callback = func; };

    uint8_t execute(CommandType command_type);
    bool execute(CommandType command_type, uint8_t value);

    void send(EventType event_type);
    void send(EventType event_type, uint8_t value);

    Subscription *subscribe(EventType type, const char *callback, int timeout=0);
};


class RadConnect
{

  private:

    const char *_name;
    bool _started;
    uint8_t _index;
    RadThing *_things[MAX_THINGS];
    char _uuid[SSDP_UUID_SIZE];
    ESP8266WebServer _http;

    StaticJsonBuffer<1024> _thingsBuffer;
    char _thingsString[1024];

    void handleInfo(void);
    void handleCommand(void);
    void handleSubscribe(void);

    uint8_t execute(const char* name, CommandType command_type);
    bool execute(const char* name, CommandType command_type, uint8_t value);

  public:

    RadConnect(const char *name);

    void add(RadThing *thing);

    bool begin(void);
    void update(void);

    RadThing *getThing(const char *name);

};


#endif // __RAD_CONNECT_H__
