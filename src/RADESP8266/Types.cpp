
#include "Types.h"

FeatureType getFeatureType(const char* s) {
  FeatureType ft = NullFeature;
  if(strcmp(s, "SwitchBinary") == 0) {
    ft = SwitchBinary;
  } else if(strcmp(s, "SensorBinary") == 0) {
    ft = SensorBinary;
  } else if(strcmp(s, "SwitchMultiLevel") == 0) {
    ft = SwitchMultiLevel;
  } else if(strcmp(s, "SensorMultiLevel") == 0) {
    ft = SensorMultiLevel;
  }
  return ft;
}


const char* sendFeatureType(FeatureType ft) {
  const char* s = "NullFeature";
  if(ft == SwitchBinary) {
    s = "SwitchBinary";
  } else if(ft == SensorBinary) {
    s = "SensorBinary";
  } else if(ft == SwitchMultiLevel) {
    s = "SwitchMultiLevel";
  } else if(ft == SensorMultiLevel) {
    s = "SensorMultiLevel";
  }
  return s;
}


CommandType getCommandType(const char* s) {
  CommandType ct = NullCommand;
  if(strcmp(s, "Get") == 0) {
    ct = Get;
  } else if(strcmp(s, "Set") == 0) {
    ct = Set;
  }
  return ct;
}


const char* sendCommandType(CommandType ct) {
  const char* s = "NullCommand";
  if(ct == Get) {
    s = "Get";
  } else if(ct == Set) {
    s = "Set";
  }
  return s;
}


EventType getEventType(const char* s) {
  EventType et = NullEvent;
  if(strcmp(s, "All") == 0) {
    et = All;
  } else if(strcmp(s, "Start") == 0) {
    et = Start;
  } else if(strcmp(s, "State") == 0) {
    et = State;
  }
  return et;
}


const char* sendEventType(EventType et) {
  const char* s = "NullEvent";
  if(et == All) {
    s = "All";
  } else if(et == Start) {
    s = "Start";
  } else if(et == State) {
    s = "State";
  }
  return s;
}

