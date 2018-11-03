#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LinkedList.h>
#include <RADESP8266.h>
#include <WiFiUdp.h>


// WiFi Config
const char* WIFI_SSID = "YOUR_SSID";
const char* WIFI_PASS = "YOUR_PASSWORD";

// RAD Config
const char* RAD_NAME = "MyESP";
const char* SWITCH_1_ID = "switch_1";
const uint8_t SWITCH_1_SLAVE_ID = 1;
const uint8_t SWITCH_1_SLAVE_ADDRESS = 8;

// State variables for momentary push button
const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;
const int BUTTON_PIN = 0;
int cmd = CMD_WAIT;
int button_state = HIGH;
static long start_press = 0;

// RAD Server and Features
RAD::ESP8266Server server = RAD::ESP8266Server(RAD_NAME);
RAD::Feature switch_1(RAD::SwitchMultiLevel, SWITCH_1_ID);


void restart() {
  // TODO: turn off relays before restarting?
  ESP.reset();
  delay(1000);
}


void button_state_change() {
  cmd = CMD_BUTTON_CHANGE;
}


void setup() {

  // Wait 1 second before starting Serial
  delay(1000);
  Serial.begin(115200);
  Serial.println("Starting...");

  // We start by connecting to a WiFi network
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Setup the features here
  server.enableWire();
  switch_1.enableSlave(SWITCH_1_SLAVE_ID, SWITCH_1_SLAVE_ADDRESS);
  server.add(&switch_1);
  server.begin();

  // Setup the momentary push button pin
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(BUTTON_PIN, button_state_change, CHANGE);
}


void loop() {
  server.update();

  // Handle button presses
  switch(cmd) {
    case CMD_WAIT:
      break;
    case CMD_BUTTON_CHANGE:
      int current_state = digitalRead(BUTTON_PIN);
      if (current_state != button_state) {
        if (button_state == LOW && current_state == HIGH) {
          long duration = millis() - start_press;
          if (duration < 5000) {
            Serial.println("short press - noop");
          } else {
            Serial.println("long press - reset");
            restart();
          }
        } else if (button_state == HIGH && current_state == LOW) {
          start_press = millis();
        }
        button_state = current_state;
      }
      break;
  }
}
