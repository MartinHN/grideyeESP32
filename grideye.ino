#include <Arduino.h>

#include "./lib/OSCAPI.h"
#include "./lib/connectivity.hpp"
#include "./lib/time.hpp"
#include "GAPI.hpp"
#include "GrideyeImpl.hpp"
#include <string>

GrideyeImpl ge;
GAPI<GrideyeImpl> gapi;
std::string myOSCID = "0";

Timeout matTimeout(50);
Timeout bgSnapShot(1000);
OSCAPI api;
APIInstance<GrideyeImpl> gAPIInstance(ge, gapi);
void setup() {
  // Pour a bowl of serial
  Serial.begin(115200);

  ge.bgRemovalMode = GrideyeImpl::BGRemovalMode::NO;
  connectivity::setup(myOSCID);

  delay(100);
  api.registerAPI("/g", &gAPIInstance);
  ge.setup();
}
OSCBundle bundle;
void loop() {
  auto t = millis();
  ge.loop();
  if (connectivity::handleConnection()) {

    if (connectivity::receiveOSC(bundle)) {

      for (int i = 0; i < bundle.size(); i++) {
        Serial.println("new msg : ");
        // bundle.getOSCMessage(i)->getAddress(OSCAPI::OSCEndpoint::getBuf());
        // Serial.println(OSCAPI::OSCEndpoint::getBuf());
        auto res = api.processOSC(*bundle.getOSCMessage(i));
        Serial.print("res : ");
        Serial.println(res.toString().c_str());
      }
      bundle.empty();
    }

    if (matTimeout.update(t)) {

      // gapi.doAction(ge, "thermistor");
      // connectivity::sendOSC("/mat", ge.tempBGTRemoved());
    }
    if (bgSnapShot.updateOneShot(t)) {
      // gapi.set<float>(ge, "test", 5.0f);
      // Serial.print(" test set to : ");
      // Serial.println(ge.test);
      // Serial.print("gett : ");
      // Serial.print(gapi.get<float>(ge, "getTherm"));
      // Serial.print(" : ");
      // Serial.print(gapi.get<float>(ge, "getTherm2"));
      Serial.println("echoing");
      auto r = gAPIInstance.call("echo", {1.f});
      Serial.println(r.toString().c_str());
      // ge.bgRemovalMode = GrideyeImpl::BGRemovalMode::REMOVE;
    }
  } else {
    ge.dbg();
    delay(1000);
  }
}
