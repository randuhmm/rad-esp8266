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


#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>

#define MAX_THINGS 8

#define RAD_HTTP_PORT 8080
#define RAD_DEVICE_TYPE "urn:rad:esp8266:1"
#define RAD_MODEL_NAME "RAD-ESP8266"
#define RAD_MODEL_NUM "9001"

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
    State = 1
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
  "    \"device\": {\r\n"
  "        \"deviceName\": \"%s\",\r\n"
  "        \"deviceType\": \"" RAD_DEVICE_TYPE "\",\r\n"
  "        \"modelDescription\": \"Rad ESP8266 WiFi Module for IoT Integration\",\r\n"
  "        \"modelName\": \"" RAD_MODEL_NAME "\",\r\n"
  "        \"serialNum\": \"\",\r\n"
  "        \"UDN\": \"uuid:%s\",\r\n"
  "        \"children\": [\r\n"
  "        ]\r\n"
  "    }\r\n"
  "}\r\n"
  "\r\n";


class RadThing {

  private:

    DeviceType _type;
    char *_name;

    SET_FP _set_callback;
    GET_FP _get_callback;

  public:

    RadThing(DeviceType type, char *name) { _type = type; _name = name; };

    DeviceType getType() { return _type; };
    char *getName() { return _name; };

    void callback(CommandType command_type, SET_FP func) { _set_callback = func; };
    void callback(CommandType command_type, GET_FP func) { _get_callback = func; };
    void command(CommandType command_type, uint8_t payload_length, uint8_t *payload);
    void event(EventType event_type, uint8_t payload_length, uint8_t *payload);
};


class RadConnect
{

  private:

    char *_name;
    bool _started;
    uint8_t _count;
    RadThing *_things[MAX_THINGS];
    char _uuid[SSDP_UUID_SIZE];
    ESP8266WebServer _http;

    void handleDevice(void);

  public:

    RadConnect(char *name);

    void add(RadThing *thing);

    bool begin(void);
    void update(void);

};


#endif // __RAD_CONNECT_H__
