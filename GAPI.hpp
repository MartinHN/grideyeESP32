#pragma once
#include "./lib/API/API.hpp"

struct GAPI : public APIAndInstance<GrideyeImpl>, public LeafNode {

  GAPI(GrideyeImpl &inst) : APIAndInstance<GrideyeImpl>(inst) {
    rMember<float>("test", &GrideyeImpl::test);
    rGetter<float>("getThermf",
                   [](GrideyeImpl &owner) { return owner.getThermistor(); });
    rGetter<float>("getTherm", &GrideyeImpl::getThermistor);

    rTrig("thermistor", [](GrideyeImpl &owner) {
      PRINT("temp");
      PRINTLN(owner.getThermistor());
    });
    rFunction<float, float>("echo", &GrideyeImpl::echo);
  }
};
