#pragma once
#include "./lib/API/API.h"

template <typename GrideyeImplT> struct GAPI : public API<GrideyeImplT> {

  GAPI() {
    this->template rMember<float>("test", &GrideyeImplT::test);
    this->template rGetter<float>(
        "getThermf", [](GrideyeImplT &owner) { return owner.getThermistor(); });
    this->template rGetter<float>("getTherm", &GrideyeImplT::getThermistor);

    this->template rTrig("thermistor", [](GrideyeImplT &owner) {
      PRINT("temp");
      PRINTLN(owner.getThermistor());
    });
    this->template rFunction<float, float>("echo", &GrideyeImplT::echo);
  }
};
