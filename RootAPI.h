#pragma once
#include "./lib/API/Node.h"
#include "./lib/OSCAPI.h"
#include "GAPI.hpp"
OSCAPI api;

struct RootAPI : public APIAndInstance<RootAPI>, MapNode {
  RootAPI(GrideyeImpl &ge)
      : APIAndInstance<RootAPI>(*this), MapNode(), gapi(ge) {
    rGetSet<std::string>("state", &RootAPI::getState, &RootAPI::setState);
    rMember<bool>("sendMat", &RootAPI::sendMat);
    MapNode::addChild("g", &gapi);
    // MapNode::addChild("r", this); // false));
  }

  void setState(const std::string &st) { gapi.fromString(st); }
  std::string getState() const {
    return ((APIInstanceBase *)&gapi)->toString();
  }
  GAPI gapi;

  bool sendMat = false;
};
