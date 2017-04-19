#include "RAD_config.h"
#include "RAD.h"

#include <vector>


void(* resetFunc) (void) = 0;
bool __on = false;


Device::Device(uint8_t device_id, uint8_t device_type)
{
  // Initialize member variables
  _time = 0;
  _last_time = 0;

  // Return on multiple instances?
  if(_instance != NULL) return;
  _instance = this;

  // Set the device id & type
  _device_id = device_id;
  _device_type = device_type;

  // Initialize groups
  _group_count = 0;
  for(int x = 0; x < MAX_GROUPS; x++) {
    _groups[x] = NULL;
  }

  // Initalize messages
  _message_count = 0;
  _current_message = 0;

}


RootDevice Device::CreateRoot(bool enable_i2c, bool enable_rf24)
{
  uint8_t device_type = 0;
  if(enable_i2c) {
    device_type |= I2C_MASTER;
  }
  if(enable_rf24) {
    device_type |= RF24_MASTER;
  }
  return RootDevice(0, device_type);
}


I2CLeafDevice Device::CreateI2CLeaf(uint8_t device_id)
{
  return I2CLeafDevice(device_id);
}


DeviceGroup *Device::group(uint8_t count, ... )
{
  uint8_t id = _group_count;
  _group_count += 1;
  DeviceGroup *group = new DeviceGroup(id);
  va_list arguments;
  va_start(arguments, count);
  for(int x = 0; x < count; x++) {
    group->addFeature(va_arg(arguments, int));
  }
  va_end(arguments);
  _groups[id] = group;
  return group;
}


void Device::loop(void)
{
  _time = millis();
  if(500 < (_time - _last_time)) {
    this->update();
    _last_time = _time;
  }
}


void Device::update(void)
{
  return;
}


bool Device::begin(void)
{
  printf_P(PSTR("Device::begin\r\n"));

  printf_P(PSTR("Ready!\r\n"));
  for(int x = 0; x < MAX_GROUPS; x++) {
    if(_groups[x] != NULL) {
      printf_P(PSTR("%02d "), x);
    }
  }

  return true;
}


void Device::send(uint8_t to, uint8_t type, uint8_t length=0, uint8_t *data=NULL)
{
  DeviceMessage *message = (DeviceMessage*)malloc(sizeof(DeviceMessage));
  message->to = to;
  message->from = _device_id;
  message->type = type;
  message->length = length;
  if(length == 0) {
    message->data = NULL;
  } else {
    message->data = (uint8_t*)malloc(length * sizeof(uint8_t));
    for(int x = 0; x < length; x++) {
      message->data[x] = data[x];
    }
  }

  _messages[_message_count] = message;
  _message_count += 1;
}


DeviceMessage *Device::getNextMessage()
{
  if(_message_count == 0) return NULL;
  if(_current_message >= _message_count) {
    for(int x = 0; x < _message_count; x++) {
      if(_messages[x]->data != NULL) {
        free(_messages[x]->data);
      }
      free(_messages[x]);
    }
    _message_count = 0;
    _current_message = 0;
    return NULL;
  } else {
    int i = _current_message;
    _current_message += 1;
    return _messages[i];
  }
}


void Device::sendDeviceInfo()
{
  uint8_t bytes = 3;
  uint8_t *data = (uint8_t*)malloc(bytes * sizeof(uint8_t));
  data[0] = _device_id;
  data[1] = _device_type;
  data[2] = _group_count;
  send(0, DEVICE_INFO, bytes, data);
  free(data);
}


void Device::sendGroupInfo()
{
  for(uint8_t i = 0; i < _group_count; i++) {
    DeviceGroup *group = _groups[i];
    uint8_t features = group->getFeatureCount();
    uint8_t *feature_types = group->getFeatureTypes();
    uint8_t bytes = 2 + features;
    uint8_t *data = (uint8_t*)malloc(bytes * sizeof(uint8_t));
    data[0] = i;
    data[1] = features;
    for(uint8_t i = 0; i < features; i++) {
      data[i+2] = feature_types[i];
    }
    send(0, GROUP_INFO, bytes, data);
    free(data);
  }
}


void Device::command(uint8_t group_id, uint8_t feature_type, uint8_t command_type, uint8_t payload_length, uint8_t *payload)
{
  printf_P(PSTR("Device::command"));
  printf_P(PSTR("feature_type = %02d \r\n"), feature_type);
  if(group_id >= _group_count) return;
  if(!_groups[group_id]->hasFeature(feature_type)) return;
  BaseFeature *f = _groups[group_id]->getFeature(feature_type);
  f->command(command_type, payload_length, payload);
}


Device* Device::_instance = NULL;


RootDevice::RootDevice(uint8_t device_id, uint8_t device_type)
: Device(device_id, device_type)
{
  for(int x = 0; x < MAX_DEVICES; x++) {
    _devices[x] = NULL;
  }
  _last_discover = 0;
  _last_info = 0;
}


bool RootDevice::begin(void)
{
  printf_P(PSTR("RootDevice::begin\r\n"));
  Device::begin();

  # if !defined(ESP8266)
    Wire.begin();
  # else
    Wire.begin(0, 2);
    Wire.setClock(100000);
  # endif
  return true;
}


void RootDevice::printInfo(void)
{
  // printf_P(PSTR("update data: %02d \r\n"), message->type);
  uint8_t device_count = 0;
  uint8_t x, y, z;
  DeviceInfo *device;
  DeviceGroupInfo *group;
  for(x = 0; x < MAX_DEVICES; x++) {
    if(_devices[x] != NULL) {
      device_count++;
    }
  }
  printf_P(PSTR("Connected Devices: %02d \r\n"), device_count);
  for(x = 0; x < MAX_DEVICES; x++) {
    if(_devices[x] == NULL) {
      continue;
    }
    device = _devices[x];
    printf_P(PSTR("Device: \r\n"));
    printf_P(PSTR("  id     = %02d \r\n"), device->id);
    printf_P(PSTR("  type   = %02d \r\n"), device->type);
    printf_P(PSTR("  addr   = %02d \r\n"), device->address);
    printf_P(PSTR("  groups = %02d \r\n"), device->group_count);
    printf_P(PSTR("  Groups: \r\n"));
    for(y = 0; y < device->group_count; y++) {
      group = &device->groups[y];
      printf_P(PSTR("    id       = %02d \r\n"), group->id);
      printf_P(PSTR("    features = (%02d) "), group->feature_count);
      for(z = 0; z < group->feature_count; z++) {
        printf_P(PSTR(" %02d"), group->features[z]);
      }
      printf_P(PSTR("\r\n"));
    }
  }
}

uint8_t RootDevice::getDeviceAddress(uint8_t device_id)
{
  DeviceInfo *device;
  for(uint8_t x = 0; x < MAX_DEVICES; x++) {
    if(_devices[x] == NULL) {
      continue;
    }
    device = _devices[x];
    if(device->id == device_id) {
      return device->address;
    }
  }
  return 0;
}


void RootDevice::receiveMessages(void)
{
  DeviceInfo *device;
  uint8_t addr;
  uint8_t bytes;
  uint8_t type;
  uint8_t to;
  uint8_t from;
  uint8_t length;
  int data[16];
  uint8_t i;

  bool get_info = (1000 < _time - _last_info);
  if(get_info) {
    _last_info = _time;
  }

  // Message Requests
  for(int x = 0; x < MAX_DEVICES; x++) {
    if(_devices[x] == NULL) {
      continue;
    } else {
      device = _devices[x];
      addr = I2C_START_ADDR + x;

      // Get data from device
      Wire.requestFrom(addr, (uint8_t)16);
      bytes = Wire.available();
      if(bytes) {
        for(i = 0; i < 16; i++) {
          data[i] = Wire.read();
        }

        type = data[0];
        if(type != 0) {
          to = data[1];
          from = data[2];
          length = data[3];

          // Handle message types
          switch(type) {
            case DEVICE_INFO:
              printf_P(PSTR("handling DEVICE_INFO \r\n"));
              device->id = data[4];
              device->type = data[5];
              device->group_count = data[6];
              break;
            case GROUP_INFO:
              printf_P(PSTR("handling GROUP_INFO \r\n"));
              DeviceGroupInfo *group = &device->groups[data[4]];
              group->id = data[4];
              group->feature_count = data[5];
              group->features = (uint8_t*)malloc(group->feature_count * sizeof(uint8_t));
              for(i = 0; i < group->feature_count; i++) {
                group->features[i] = data[i+6];
              }
              break;
          }

        }
      }

      if(get_info) {
        // Decide what to request from device
        if(device->id == 0) {
          // Request device info
          printf_P(PSTR("requesting DEVICE_INFO \r\n"));
          send(addr, DEVICE_INFO);
        } else if(device->groups == NULL && device->group_count != 0) {
          printf_P(PSTR("requesting GROUP_INFO \r\n"));
          device->groups = (DeviceGroupInfo*)malloc(device->group_count * sizeof(DeviceGroupInfo));
          send(addr, GROUP_INFO);
        }
      }


    }
  }
}


void RootDevice::update(void)
{
  // Discover newly connected devices
  discover();

  // Receive messages from connected devices
  receiveMessages();

  DeviceMessage *message = _instance->getNextMessage();
  while(message != NULL) {
    printf_P(PSTR("update messages: %02d \r\n"), _message_count);
    uint8_t length = message->length + 1;
    uint8_t *data = (uint8_t*)malloc(length);
    printf_P(PSTR("update data: %02d \r\n"), message->type);
    data[0] = message->type;
    for(int x = 0; x < message->length; x++) {
      data[x+1] = message->data[x];
      printf_P(PSTR("data: %02d \r\n"), data[x+1]);
    }
    Wire.beginTransmission(message->to);
    Wire.write(data, length);
    Wire.endTransmission();
    free(data);
    message = _instance->getNextMessage();
  }

}


void RootDevice::discover(void)
{
  if(5000 > (_time - _last_discover)) {
    return;
  }
  _last_discover = _time;

  // printf_P(PSTR("RootDevice::discover\r\n"));
  Wire.requestFrom((uint8_t)I2C_DISCOVERY_ADDR, (uint8_t)1);
  if(Wire.available()) {
    uint8_t value = Wire.read();
    printf_P(PSTR("RootDevice::discover - 0x%02x\r\n"), value);
    if(value != 0) {
      printf_P(PSTR("RootDevice::discover - FOUND DEVICE\r\n"));
      // Get next open address
      for(int x = 0; x < MAX_DEVICES; x++) {
        if(_devices[x] == NULL) {

          uint8_t address = I2C_START_ADDR + x;
          DeviceInfo *info = (DeviceInfo*)malloc(sizeof(DeviceInfo));
          info->id = 0;
          info->type = 0;
          info->group_count = 0;
          info->address = address;
          info->groups = NULL;
          info->time_since_info = 255;

          _devices[x] = info;
          Wire.beginTransmission(I2C_DISCOVERY_ADDR);
          Wire.write(address);
          Wire.endTransmission();
          return;
        }
      }
    }
  }
}


I2CLeafDevice::I2CLeafDevice(uint8_t device_id)
: Device(device_id, I2C_SLAVE)
{

  // Return on multiple instances?
  if(_leaf_instance != NULL) return;
  _leaf_instance = this;

  // Init I2C if Enabled
  if(_device_type & I2C_SLAVE) {
    _i2c_address = EEPROM.read(I2C_EEPROM_ADDR);
    if(_i2c_address == 0x00) {
      _i2c_address = I2C_DISCOVERY_ADDR;
    }
  }

}


bool I2CLeafDevice::begin()
{
  printf_P(PSTR("I2CLeafDevice::begin\r\n"));

  printf_P(PSTR("device_type: 0x%02x \r\n"), _device_type);
  printf_P(PSTR("i2c_address: 0x%02x \r\n"), _i2c_address);

  Device::begin();


  if(_device_type & I2C_SLAVE) {
    printf_P(PSTR("I2CLeafDevice::begin I2C_SLAVE\r\n"));
    Wire.begin(_i2c_address);
    if(_i2c_address == I2C_DISCOVERY_ADDR) {
      Wire.onReceive(I2CLeafDevice::i2cSlaveResetReceive);
      Wire.onRequest(I2CLeafDevice::i2cSlaveResetRequest);
    } else {
      EEPROM.write(I2C_EEPROM_ADDR, I2C_DISCOVERY_ADDR);
      Wire.onReceive(I2CLeafDevice::i2cSlaveHandleReceive);
      Wire.onRequest(I2CLeafDevice::i2cSlaveHandleRequest);
    }
  }

  return true;
}


void I2CLeafDevice::update(void)
{
  printf_P(PSTR("I2CLeafDevice::update\r\n"));
}


// callback for received data
void I2CLeafDevice::i2cSlaveResetReceive(int n)
{
  printf_P(PSTR("I2CLeafDevice::i2cSlaveResetReceive\r\n"));
  while(0 < Wire.available()) {
    int new_i2c_address = Wire.read();
    EEPROM.write(I2C_EEPROM_ADDR, new_i2c_address);
    resetFunc();
  }
}


// callback for sending data
void I2CLeafDevice::i2cSlaveResetRequest(void)
{
  printf_P(PSTR("I2CLeafDevice::i2cSlaveResetRequest\r\n"));
  Wire.write(0xFF);
}


// callback for received data
void I2CLeafDevice::i2cSlaveHandleReceive(int bytes)
{
  uint8_t message_type = Wire.read();

  // retrieve the rest of the message
  uint8_t message[32];
  for(int i = 0; i < (bytes-1); i++) {
    message[i] = Wire.read();
    printf_P(PSTR("%02x"), message[i]);
  }

  uint8_t group_id;
  switch(message_type) {
    case DEVICE_INFO:
      printf_P(PSTR("I2CLeafDevice::i2cSlaveHandleReceive - DEVICE_INFO\r\n"));
      _leaf_instance->sendDeviceInfo();
      break;
    case GROUP_INFO:
      printf_P(PSTR("I2CLeafDevice::i2cSlaveHandleReceive - GROUP_INFO\r\n"));
      _leaf_instance->sendGroupInfo();
      break;
    case COMMAND:
      group_id = message[1];
      uint8_t feature_type = message[2];
      uint8_t command_type = message[3];
      uint8_t payload_length = bytes - 5;
      if(payload_length) {
        _leaf_instance->command(group_id, feature_type, command_type, payload_length, &message[4]);
      } else {
        // TODO: command without payload?
      }
      break;
  }
}


// callback for sending data
void I2CLeafDevice::i2cSlaveHandleRequest(void)
{
  DeviceMessage *message = _leaf_instance->getNextMessage();
  uint8_t *data = _leaf_instance->_i2c_data;
  if(message != NULL) {
    uint8_t bytes = message->length + 4;
    data[0] = message->type;
    data[1] = message->to;
    data[2] = message->from;
    data[3] = message->length;
    for(int i = 4; i < bytes; i++) {
      data[i] = message->data[i-4];
    }
    Wire.write(data, bytes);
  } else {
    Wire.write(0);
  }
}


I2CLeafDevice* I2CLeafDevice::_leaf_instance = NULL;


DeviceGroup::DeviceGroup(uint8_t id)
{
  printf_P(PSTR("DeviceGroup::DeviceGroup:  0x%02x \r\n"), id);
  _id = id;
  _feature_count = 0;
}


void DeviceGroup::addFeature(uint8_t feature_type)
{

  printf_P(PSTR("DeviceGroup::addFeature:  0x%02x \r\n"), feature_type);
  BaseFeature *f = NULL;
  switch(feature_type) {
    case SWITCH_BINARY:
      f = new SwitchBinaryFeature();
      break;
  }

  if(f != NULL) {
    printf_P(PSTR("f != NULL\r\n"));
    int i = _feature_count;
    _feature_count += 1;
    _features[i] = f;
    _feature_types[i] = f->getType();
    printf_P(PSTR("feature_count = %02d \r\n"), _feature_count);
  }
}


BaseFeature *DeviceGroup::getFeature(uint8_t feature_type)
{
  for(int x = 0; x < _feature_count; x++) {
    if(feature_type == _features[x]->getType()) {
      return _features[x];
    }
  }
  return NULL;
}


bool DeviceGroup::hasFeature(uint8_t feature_type)
{
  printf_P(PSTR("DeviceGroup::hasFeature: 0x%02x \r\n"), feature_type);
  printf_P(PSTR("feature_count = %02d \r\n"), _feature_count);
  for(int x = 0; x < _feature_count; x++) {
    if(feature_type == _features[x]->getType()) {
      printf_P(PSTR("true\r\n"));
      return true;
    }
  }
  printf_P(PSTR("false\r\n"));
  return false;
}


uint8_t DeviceGroup::getFeatureCount()
{
  return _feature_count;
}


uint8_t *DeviceGroup::getFeatureTypes()
{
  return _feature_types;
}


void SwitchBinaryFeature::addCallback(uint8_t command_type, SET_FP func)
{
  printf_P(PSTR("addCallback - set\r\n"));
  switch(command_type) {
    case SET:
      _set_callback = func;
      break;
  }
}


void SwitchBinaryFeature::addCallback(uint8_t command_type, GET_FP func)
{
  printf_P(PSTR("addCallback - get\r\n"));
}


void SwitchBinaryFeature::command(uint8_t command_type, uint8_t payload_length, uint8_t *payload)
{
  printf_P(PSTR("SwitchBinaryFeature::command\r\n"));
  switch(command_type) {
    case SET:
      _set_callback(*payload);
      break;
  }
}

