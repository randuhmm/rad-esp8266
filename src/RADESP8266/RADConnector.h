
#pragma once

#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <LinkedList.h>
#include "Types.h"
#include "RADFeature.h"
#include "RADSubscription.h"
#include "Defines.h"

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


class RADConnector
{

  private:

    const char* _name;
    bool _started;
    LinkedList<RADFeature*> _features;
    LinkedList<RADSubscription*> _subscriptions;
    char _uuid[SSDP_UUID_SIZE];
    String _info;
    ESP8266WebServer* _http;


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
