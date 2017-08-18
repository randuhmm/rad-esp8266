
#pragma once

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