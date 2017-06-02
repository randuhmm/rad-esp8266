#include <ArduinoJson.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LinkedList.h>
#include <RadEsp8266.h>
#include <WiFiUdp.h>


// WiFi Config
const char* ssid = "YOUR_SSID";
const char* pass = "YOUR_PASSWORD";


// RAD variables
RadConnect radConnect("MyESP");
RadDevice switch_1(SwitchBinary, "switch_1");
const int SWITCH_PIN = 2;
uint8_t switch_1_state = false;


// State variables for momentary push button
const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;
const int BUTTON_PIN = 0;
int cmd = CMD_WAIT;
int button_state = HIGH;
static long start_press = 0;


void switch_1_set(uint8_t value) {
  Serial.print("switch_1_set(): ");
  if(value) {
    Serial.println("ON");
    digitalWrite(SWITCH_PIN, LOW);
  } else {
    Serial.println("OFF");
    digitalWrite(SWITCH_PIN, HIGH);
  }
  switch_1_state = value;
}


void switch_1_toggle() {
  if(switch_1_state) {
    switch_1_set(false);
    switch_1.send(State, false);
  } else {
    switch_1_set(true);
    switch_1.send(State, true);
  }
}


uint8_t switch_1_get(void) {
  return switch_1_state;
}


void restart() {
  // TODO: turn off relays before restarting?
  ESP.reset();
  delay(1000);
}


void button_state_change() {
  cmd = CMD_BUTTON_CHANGE;
}


void setup() {
  // Setup the LED pin
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, HIGH);

  // Wait 1 second before starting Serial
  delay(1000);
  Serial.begin(115200);
  Serial.println("Starting...");

  // We start by connecting to a WiFi network
  WiFiMulti.addAP(ssid, pass);

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while(WiFiMulti.run() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Create the devices here
  radConnect.add(&switch_1);
  switch_1.callback(Set, switch_1_set);
  switch_1.callback(Get, switch_1_get);
  radConnect.begin();

  // Setup the momentary push button pin
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(BUTTON_PIN, button_state_change, CHANGE);
}


void loop() {
  radConnect.update();

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
            Serial.println("short press - toggle relay");
            switch_1_toggle();
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
