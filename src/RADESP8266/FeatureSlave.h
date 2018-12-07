
#pragma once

#include "Defines.h"
#include "Feature.h"

namespace RAD {

  class FeatureSlave
  {

    private:

      uint8_t _address;
      LinkedList<Feature*> _features;

    public:

      FeatureSlave(const uint8_t address) { _address = address; };

      void begin();
      void update();      
      void add(Feature* feature);

  };

}
