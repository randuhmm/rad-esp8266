
#pragma once

#include <Arduino.h>


namespace RAD {

  // Feature Types
  enum FeatureType {
    NullFeature      = 0,
    SwitchBinary     = 1,
    SensorBinary     = 2,
    SwitchMultiLevel = 3,
    SensorMultiLevel = 4,
    TriggerFeature   = 5
  };

  // Command Types
  enum CommandType {
    NullCommand  = 0,
    Get          = 1,
    Set          = 2,
    Trigger      = 3
  };

  // Event Types
  enum EventType {
    NullEvent  = 0,
    All        = 1,
    Start      = 2,
    State      = 3
  };

  // Payload Types
  enum PayloadType {
    NullPayload      = 0,
    BoolPayload      = 1,
    BytePayload      = 2,
    ByteArrayPayload = 3
  };

  // 8-bit Integer Definition
  typedef unsigned char uint8_t;

  // Payload Struct Definition
  struct Payload {
    PayloadType type;
    uint8_t len;
    uint8_t* data;
  };

  // Callback Definitions
  typedef bool (* TriggerFp)();
  typedef bool (* SetBoolFp)(bool);
  typedef bool (* SetByteFp)(uint8_t);
  typedef bool (* SetByteArrayFp)(uint8_t*, uint8_t);
  typedef Payload* (* GetFp)(void);


  FeatureType getFeatureType(const char* s);
  const char* sendFeatureType(FeatureType ft);
  CommandType getCommandType(const char* s);
  const char* sendCommandType(CommandType ct);
  EventType getEventType(const char* s);
  const char* sendEventType(EventType et);

}
