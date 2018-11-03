
#pragma once

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <LinkedList.h>
#include <Wire.h>
#include "Types.h"
#include "Feature.h"
#include "Subscription.h"
#include "Defines.h"


namespace RAD {

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


  class Config
  {

  };

  class ESP8266Server
  {

    private:

      const char* _name;
      bool _started;
      LinkedList<Feature*> _features;
      LinkedList<Subscription*> _subscriptions;
      char _uuid[SSDP_UUID_SIZE];
      String _info;
      ESP8266WebServer* _http;

      uint8_t _subscriptionCount;
      long _lastWrite;
      bool _subscriptionsChanged;

      // HTTP Path Handler Functions
      void handleInfo(void);
      void handleFeatures(void);
      //void handleFeature(Feature* feature);
      void handleSubscriptions(Feature* feature);
      void handleCommands(Feature* feature);
      void handleEvents(Feature* feature);
      // void handleSubscription(LinkedList<String>& segments);

      // Execution Methods
      bool execute(const char* feature_id, CommandType command_type, Payload* response);
      bool execute(const char* feature_id, CommandType command_type, bool data, Payload* response);
      bool execute(const char* feature_id, CommandType command_type, uint8_t data, Payload* response);
      //bool execute(const char* feature_id, CommandType command_type, uint8_t* data, uint8_t len, Payload* response);
      bool execute(const char* feature_id, CommandType command_type, Payload* payload, Payload* response);

      // Wire state
      bool _wireEnabled;
      int _wireSDA;
      int _wireSCL;

    public:

      ESP8266Server(const char* name, Config* config = NULL);

      void add(Feature* feature);

      void enableWire(int sda = 0, int scl = 2);

      Subscription* subscribe(Feature* feature, EventType event_type,
                                const char* callback, int timeout=RAD_MIN_TIMEOUT);
      void unsubscribe(int index);

      bool begin(void);
      void update(void);

      Feature* getFeature(const char* feature_id);

      static Payload* BuildPayload(bool data);
      static Payload* BuildPayload(uint8_t data);
      static Payload* BuildPayload(uint8_t* data, uint8_t len);

  };

}