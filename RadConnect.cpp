#include "RadConnect.h"


RadConnect::RadConnect(const char *name) {
  _name = name;
  _index = 0;
  _http = ESP8266WebServer(RAD_HTTP_PORT);
}


void RadConnect::add(RadThing *thing) {
  if(_index < MAX_THINGS) {
    _things[_index] = thing;
    _index += 1;
  }
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

  // Prepare the JSON response
  JsonArray& things = _thingsBuffer.createArray();
  RadThing* thing;
  for(uint8_t i = 0; i < _index; i++) {
    thing = _things[i];
    JsonObject& thing_json = things.createNestedObject();
    char *type = "";
    switch(thing->getType()) {
      case SwitchBinary:
        type = "SwitchBinary";
        break;
    }
    thing_json["type"] = type;
    thing_json["name"] = thing->getName();
  }
  things.printTo(_thingsString, sizeof(_thingsString));


  // SSDP
  // Serial.println("Starting http...");
  //_http.on("/", std::bind(&RadConnect::handleRoot, this));
  _http.on("/" RAD_INFO_PATH, std::bind(&RadConnect::handleInfo, this));
  _http.on("/" RAD_COMMAND_PATH, std::bind(&RadConnect::handleCommand, this));
  _http.on("/" RAD_EVENT_PATH, std::bind(&RadConnect::handleEvent, this));
  _http.begin();
  SSDP.setDeviceType(RAD_DEVICE_TYPE);
  SSDP.setName(_name);
  SSDP.setURL("");
  SSDP.setSchemaURL(RAD_INFO_PATH);
  SSDP.setSerialNumber(WiFi.macAddress());
  SSDP.setModelName(RAD_MODEL_NAME);
  SSDP.setModelNumber(RAD_MODEL_NUM);
  SSDP.setModelURL(RAD_INFO_URL);
  SSDP.setManufacturer("NA");
  SSDP.setManufacturerURL(RAD_INFO_URL);
  SSDP.setHTTPPort(RAD_HTTP_PORT);
  SSDP.begin();
}


void RadConnect::update(void) {
  // loop
  _http.handleClient();
}


void RadConnect::handleInfo(void) {
  Serial.println("/info");
  _http.client().printf(_ssdp_schema_template,
    _name,
    _uuid,
    _thingsString
  );
}


void RadConnect::handleCommand(void) {
  Serial.println("/command");

  int code = 200;
  String message = "";
  if(_http.arg("name") == "" || _http.arg("type") == "") {
    code = 400;
    message = "{\"error\": \"Missing required parameter(s): 'name' and/or 'type'\"}";
  } else {
    if(_http.arg("type") == "set") {
      if(_http.arg("value") == "") {
        code = 400;
        message = "{\"error\": \"Missing required parameter(s): 'value'\"}";
      } else if(execute(_http.arg("name").c_str(), Set, atoi(_http.arg("value").c_str()))) {
        code = 200;
        message = "{\"result\": true}";
      }
    } else if(_http.arg("type") == "get") {
      uint8_t value = execute(_http.arg("name").c_str(), Get);
      code = 200;
      char buff[100];
      snprintf(buff, sizeof(buff), "{\"value\": %d}", value);
      message = buff;
    } else {
      code = 400;
      message = "{\"error\": \"Unsuported command type.\"}";
    }
  }
  _http.send(code, "application/json", message);
}


void RadConnect::handleEvent(void) {
  Serial.println("/event");
  _http.send(200, "application/json", "true");
}





uint8_t RadConnect::execute(const char* name, CommandType command_type) {
  RadThing* thing;
  for(uint8_t i = 0; i < _index; i++) {
    thing = _things[i];
    if(strcmp(thing->getName(), name) == 0) {
      return thing->execute(command_type);
    }
  }
}


bool RadConnect::execute(const char* name, CommandType command_type, uint8_t value) {
  RadThing* thing;
  for(uint8_t i = 0; i < _index; i++) {
    thing = _things[i];
    if(strcmp(thing->getName(), name) == 0) {
      return thing->execute(command_type, value);
    }
  }
}



RadThing::RadThing(DeviceType type, const char *name) {
  _type = type;
  _name = name;
  _set_callback = NULL;
  _get_callback = NULL;
}


uint8_t RadThing::execute(CommandType command_type) {
  uint8_t result = 0;
  switch(command_type) {
    case Get:
      GET_FP get = NULL;
      switch(_type) {
        case SwitchBinary:
          get = _get_callback;
        break;
      }
      if(get != NULL) {
        result = get();
      }
    break;
  }
  return result;
}


bool RadThing::execute(CommandType command_type, uint8_t value) {
  bool result = false;
  switch(command_type) {
    case Set:
      SET_FP set = NULL;
      switch(_type) {
        case SwitchBinary:
          set = _set_callback;
        break;
      }
      if(set != NULL) {
        set(value);
        result = true;
      }
    break;
  }
  return result;
}

