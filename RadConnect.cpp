#include "RadConnect.h"


RadConnect::RadConnect(char *name) {
  _name = name;
  _count = 0;
  _http = ESP8266WebServer(RAD_HTTP_PORT);
}


void RadConnect::add(RadThing *thing) {

}


bool RadConnect::begin(void) {

  // Prepare the uuid
  uint32_t chipId = ESP.getChipId();
  sprintf(_uuid, "38323636-4558-4dda-9188-cda0e6%02x%02x%02x",
  (uint16_t) ((chipId >> 16) & 0xff),
  (uint16_t) ((chipId >>  8) & 0xff),
  (uint16_t)   chipId        & 0xff  );

  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // SSDP
  // Serial.println("Starting http...");
  //_http.on("/", std::bind(&RadConnect::handleRoot, this));
  _http.on("/device", std::bind(&RadConnect::handleDevice, this));
  //_http.on("/command", std::bind(&RadConnect::handleCommand, this));
  _http.begin();
  SSDP.setDeviceType(RAD_DEVICE_TYPE);
  SSDP.setName(_name);
  SSDP.setURL("");
  SSDP.setSchemaURL("device");
  SSDP.setSerialNumber(WiFi.macAddress());
  SSDP.setModelName(RAD_MODEL_NAME);
  SSDP.setModelNumber(RAD_MODEL_NUM);
  SSDP.setModelURL("http://www.example.com");
  SSDP.setManufacturer("NA");
  SSDP.setManufacturerURL("http://www.example.com");
  SSDP.setHTTPPort(RAD_HTTP_PORT);
  SSDP.begin();
}


void RadConnect::update(void) {
  // loop
  _http.handleClient();
}


void RadConnect::handleDevice(void) {
  Serial.println("/device");
  _http.client().printf(_ssdp_schema_template,
    _name,
    _uuid
  );
}
