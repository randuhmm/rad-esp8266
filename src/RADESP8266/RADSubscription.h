
#pragma once

#include "Defines.h"
#include "Types.h"

#define SID_UUID_SIZE 37

// Forward Declaration of RADFeature
class RADFeature;

class RADSubscription {

  private:

    char _sid[SID_UUID_SIZE];
    char _callback[MAX_CALLBACK_SIZE];
    EventType _type;
    int _timeout;
    long _started;
    long _end;
    RADFeature* _feature;
    int _calls;
    int _errors;

  public:

    RADSubscription(RADFeature* feature, const char* sid, EventType type,
                    const char* callback, int timeout, int calls=0,
                    int errors=0) {
      _feature = feature;
      _type = type;
      _timeout = timeout;
      _started = millis();
      _end = _started + timeout * 1000;
      _calls = calls;
      _errors = errors;
      strncpy(_sid, sid, sizeof(_sid));
      strncpy(_callback, callback, sizeof(_callback));
    };
    EventType getType() { return _type; };
    char* getSid() { return _sid; };
    char* getCallback() { return _callback; };
    int getTimeout() { return _timeout; };
    int getDuration(long current) { return (current - _started) / 1000; }
    RADFeature* getFeature() { return _feature; };
    bool isActive(long current) {
      return _end > current;
    }
};
