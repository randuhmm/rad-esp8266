#include <ArduinoOTA.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266SSDP.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <RadConnect.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>

//for LED status
#include <Ticker.h>
Ticker ticker;

RadConnect radConnect("MyESP");
RadThing switch_1(SwitchBinary, "switch_1");


void switch_1_set(uint8_t value) {
  Serial.print("switch_1: ");
  Serial.println(value);
}


void tick() {
  //toggle state
  int state = digitalRead(BUILTIN_LED);  // get the current state of GPIO1 pin
  digitalWrite(BUILTIN_LED, !state);     // set pin to the opposite state
}


//gets called when WiFiManager enters configuration mode
void config_mode_callback(WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
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
  Serial.begin(115200);

  //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  // start ticker with 0.5 because we start in AP mode and try to connect
  ticker.attach(0.6, tick);

  WiFiManager wifiManager;
  //wifiManager.reset();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(config_mode_callback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect()) {
    Serial.println("failed to connect and hit timeout");
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(1000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...");
  ticker.detach();
  //keep LED on
  digitalWrite(BUILTIN_LED, LOW);

  // Setup OTA
  setup_ota();

  // Create the devices here
  radConnect.add(&switch_1);
  switch_1.callback(Set, switch_1_set);
  radConnect.begin();

}


void loop() {
  ArduinoOTA.handle();
  radConnect.update();
}
