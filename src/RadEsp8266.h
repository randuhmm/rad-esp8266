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
#include <FS.h>
#include <LinkedList.h>

#define MAX_CALLBACK_SIZE 255
#define RAD_MIN_TIMEOUT 90
#define RAD_MIN_WRITE_INTERVAL 60

#define RAD_HTTP_PORT 8080
#define RAD_DEVICE_TYPE "urn:rad:device:esp8266:1"
#define RAD_MODEL_NAME "RAD-ESP8266"
#define RAD_MODEL_NUM "9001"
#define RAD_INFO_URL "http://example.com"

#define RAD_INFO_PATH "/"
#define RAD_DEVICES_PATH "/devices"
#define RAD_SUBSCRIPTIONS_PATH "/subscriptions"
#define RAD_COMMANDS_PATH "/commands"
#define RAD_EVENTS_PATH "/events"

#define RAD_SUBSCRIPTIONS_FILE "/rad/subscriptions.json"

#define HEADER_HOST      "HOST"
#define HEADER_CALLBACK  "CALLBACK"
#define HEADER_NT        "NT"
#define HEADER_TIMEOUT   "TIMEOUT"


// Device Types
enum DeviceType {
    NullDevice       = 0,
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

typedef std::function<void(LinkedList<String>&)> PATH_FP;
class RadDevice;

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


class Segment {

  public:

    Segment(const char* seg)
      : _seg(seg)
    {
      if(seg[0] == '{' && seg[strlen(seg) - 1] == '}') {
        _isParam = true;
      } else {
        _isParam = false;
      }
    }

    bool isParam(void) {
      return _isParam;
    }

    bool check(String seg) {
      return seg == _seg;
    }

    String get() {
      return _seg;
    }

  protected:

    String _seg;
    bool _isParam;

};


class PathSegmentRequestHandler : public RequestHandler {

  public:

    PathSegmentRequestHandler(PATH_FP fn, const char* uri, HTTPMethod method)
      : _fn(fn)
      , _uri(uri)
      , _method(method)
    {
      Segment *segment = NULL;
      int start = 1;
      int current = _uri.indexOf('/', start);
      while(current != -1) {
        if(current - start == 0) {
          segment = new Segment("");
        } else {
          segment = new Segment(_uri.substring(start, current).c_str());
        }
        _segments.add(segment);
        start = current + 1;
        current = _uri.indexOf('/', start);
      }
      if(start <= _uri.length()) {
        segment = new Segment(_uri.substring(start, current).c_str());
        _segments.add(segment);
      }
    }

    bool canHandle(HTTPMethod requestMethod, String requestUri) override  {
      bool result = true;
      bool clean = !_uriProcessed;
      splitUri(requestUri);
      if(_method == HTTP_ANY || _method == requestMethod) {
        if(_currentSegments.size() != _segments.size()) {
          // Serial.println("SIZE mismatch.");
          result = false;
        } else {
          Segment* s;
          for(int i = 0; i < _segments.size(); i++) {
            s = _segments.get(i);
            if(!s->isParam() && !s->check(_currentSegments.get(i))) {
              // Serial.println("SEGMENT mismatch.");
              // Serial.println(s->get());
              // Serial.println(_currentSegments.get(i));
              result = false;
              break;
            }
          }
        }
      }
      if(clean) {
        cleanup();
      }
      return result;
    }

    bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) override {
      bool result = false;
      splitUri(requestUri);
      if(canHandle(requestMethod, requestUri)) {
        LinkedList<String> paths;
        Segment* s;
        for(int i = 0; i < _segments.size(); i++) {
          s = _segments.get(i);
          if(s->isParam()) {
            paths.add(_currentSegments.get(i));
          }
        }
        _fn(paths);
      }
      cleanup();
      return result;
    }

  protected:

    PATH_FP _fn;
    String _uri;
    HTTPMethod _method;
    LinkedList<Segment*> _segments;

    bool _uriProcessed = false;
    LinkedList<String> _currentSegments;

  private:

    void splitUri(String uri) {
      if(_uriProcessed) return;
      _uriProcessed = true;
      int start = 1;
      int current = uri.indexOf('/', start);
      while(current != -1) {
        if(current - start == 0) {
          _currentSegments.add(String(""));
        } else {
          _currentSegments.add(uri.substring(start, current));
        }
        start = current + 1;
        current = uri.indexOf('/', start);
      }
      if(start <= uri.length()) {
        _currentSegments.add(uri.substring(start));
      }
    }

    void cleanup() {
      _currentSegments.clear();
      _uriProcessed = false;
    }

};


class Subscription {

  private:

    char _sid[SSDP_UUID_SIZE];
    char _callback[MAX_CALLBACK_SIZE];
    EventType _type;
    int _timeout;
    long _started;
    long _end;
    RadDevice* _device;
    int _calls;
    int _errors;

  public:

    Subscription(RadDevice* device, const char* sid, EventType type, const char* callback, int timeout, int calls=0, int errors=0) {
      _device = device;
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
    RadDevice* getDevice() { return _device; };
    bool isActive(long current) {
      return _end > current;
    }
};


class RadDevice {

  private:

    DeviceType _type;
    const char* _name;

    SET_FP _setCallback;
    GET_FP _getCallback;

    LinkedList<Subscription*> _subscriptions;

  public:

    RadDevice(DeviceType type, const char* name);

    DeviceType getType() { return _type; };
    const char* getName() { return _name; };

    void callback(CommandType command_type, SET_FP func) { _setCallback = func; };
    void callback(CommandType command_type, GET_FP func) { _getCallback = func; };

    uint8_t execute(CommandType command_type);
    bool execute(CommandType command_type, uint8_t value);

    void send(EventType event_type);
    void send(EventType event_type, uint8_t value);

    void add(Subscription* subscription);
    void remove(Subscription* subscription);
};


class RadConnect
{

  private:

    const char* _name;
    bool _started;
    LinkedList<RadDevice*> _devices;
    LinkedList<Subscription*> _subscriptions;
    char _uuid[SSDP_UUID_SIZE];
    String _info;
    ESP8266WebServer _http;


    uint8_t _subscriptionCount;
    long _lastWrite;
    bool _subscriptionsChanged;

    // HTTP Path Handler Functions
    void handleInfo(void);
    void handleDevices(void);
    void handleSubscriptions(void);
    void handleDeviceCommands(LinkedList<String>& segments);
    void handleDeviceEvents(LinkedList<String>& segments);
    void handleSubscription(LinkedList<String>& segments);

    // Execution Methods
    uint8_t execute(const char* name, CommandType command_type);
    uint8_t execute(RadDevice* device, CommandType command_type);
    bool execute(const char* name, CommandType command_type, uint8_t value);
    bool execute(RadDevice* device, CommandType command_type, uint8_t value);

    // Segment Path Request Registrations
    void on(const char* uri, PATH_FP fn);
    void on(const char* uri, HTTPMethod method, PATH_FP fn);

  public:

    RadConnect(const char* name);

    void add(RadDevice* device);

    Subscription* subscribe(RadDevice* device, EventType type,
                            const char* callback, int timeout=RAD_MIN_TIMEOUT);
    void unsubscribe(int index);

    bool begin(void);
    void update(void);

    RadDevice* getDevice(const char* name);

};


#endif // __RAD_CONNECT_H__
