#include <Wire.h>
#include <RADESP8266.h>

// RAD Config
const char* SWITCH_1_ID = "switch_1";
const uint8_t SWITCH_1_SLAVE_ID = 1;
const uint8_t SLAVE_ADDRESS = 8;
const int SWITCH_PIN = 2;

// Switch State
uint8_t switch_1_state = 0;

// State variables for momentary push button
const int CMD_WAIT = 0;
const int CMD_BUTTON_CHANGE = 1;
const int BUTTON_PIN = 0;
int cmd = CMD_WAIT;
int button_state = HIGH;
static long start_press = 0;

// RAD Server and Features
RAD::FeatureSlave slave = RAD::FeatureSlave(SLAVE_ADDRESS);
RAD::Feature switch_1(RAD::SwitchMultiLevel, SWITCH_1_ID);


bool switch_1_on_set(uint8_t value) {
  Serial.print("switch_1_on_set(): ");
  switch_1_set(value);
  return true;
}


RAD::Payload* switch_1_on_get(void) {
  return RAD::ESP8266Server::BuildPayload(switch_1_state);
}


void switch_1_set(uint8_t value) {
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


void setup() {
  // Wire.begin(8);                // join i2c bus with address #8
  // Wire.onReceive(receiveEvent); // register event
  // Wire.onRequest(requestEvent); // register event
  // Serial.begin(9600);           // start serial for output

  // Setup the features here
  slave.add(&switch_1);
  switch_1.callback(RAD::Set, switch_1_on_set);
  switch_1.callback(RAD::Get, switch_1_on_get);
  slave.begin();




}

void loop() {
  slave.update();
}
