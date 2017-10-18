#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LinkedList.h>
#include <RADESP8266.h>
#include <Ticker.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>


// For LED status indicator
Ticker ticker;
const int LED_PIN = 13;

// RAD variables
RADConnector rad("MySonoff");
RADFeature switch_1(SwitchBinary, "switch_1");
const int SWITCH_PIN = 12;
bool switch_1_state = false;

// State variables for momentary push button
const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;
const int BUTTON_PIN = 0;
int cmd = CMD_WAIT;
int button_state = HIGH;
static long start_press = 0;


void switch_1_set(bool value) {
  Serial.print("switch_1_set(): ");
  if(value) {
    Serial.println("ON");
    digitalWrite(SWITCH_PIN, HIGH);
  } else {
    Serial.println("OFF");
    digitalWrite(SWITCH_PIN, LOW);
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


bool switch_1_on_set(bool on) {
  Serial.print("switch_1_on_set(): ");
  switch_1_set(on);
  return true;
}


RADPayload* switch_1_on_get(void) {
  return RADConnector::BuildPayload(switch_1_state);
}


void tick() {
  // Toggle LED state
  int state = digitalRead(LED_PIN);  // get the current state of LED pin
  digitalWrite(LED_PIN, !state);     // set pin to the opposite state
}


// gets called when WiFiManager enters configuration mode
void config_mode_callback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  // if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  // entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}


void restart() {
  // TODO: turn off relays before restarting?
  ESP.reset();
  delay(1000);
}


void reset() {
  //reset settings to defaults
  /*
    WMSettings defaults;
    settings = defaults;
    EEPROM.begin(512);
    EEPROM.put(0, settings);
    EEPROM.end();
  */
  //reset wifi credentials
  WiFi.disconnect();
  delay(1000);
  restart();
}


void button_state_change() {
  cmd = CMD_BUTTON_CHANGE;
}


void setup_ota() {
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA: Ready");
}


void setup() {
  // Setup the relay pin as output and turn it off
  pinMode(SWITCH_PIN, OUTPUT);
  digitalWrite(SWITCH_PIN, LOW);

  // Wait 1 second before starting Serial
  delay(1000);
  Serial.begin(115200);
  Serial.println("Starting...");

  // Setup the led pin as output
  pinMode(LED_PIN, OUTPUT);
  // Start ticker with 0.6 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  WiFiManager wifiManager;
  //wifiManager.reset();

  // set callback that gets called when connecting to previous WiFi fails,
  // and enters Access Point mode
  wifiManager.setAPCallback(config_mode_callback);

  // fetches ssid and pass and tries to connect
  // if it does not connect it starts an access point with the specified name
  // here  "AutoConnectAP"
  // and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    // reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  // if you get here you have connected to the WiFi
  Serial.println("connected...");
  ticker.detach();
  // keep LED on
  digitalWrite(BUILTIN_LED, HIGH);

  // Setup OTA
  setup_ota();

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Create the devices here
  rad.add(&switch_1);
  switch_1.callback(Set, switch_1_on_set);
  switch_1.callback(Get, switch_1_on_get);
  rad.begin();

  // Setup the momentary push button pin
  pinMode(BUTTON_PIN, INPUT);
  attachInterrupt(BUTTON_PIN, button_state_change, CHANGE);
}


void loop() {
  rad.update();

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
          } else if (duration < 30000) {
            Serial.println("medium press - restart");
            restart();
          } else {
            Serial.println("long press - reset settings");
            reset();
          }
        } else if (button_state == HIGH && current_state == LOW) {
          start_press = millis();
        }
        button_state = current_state;
      }
      break;
  }
}
