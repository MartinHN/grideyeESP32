#include <Arduino.h>

#include "./lib/connectivity.hpp"
#include "./lib/time.hpp"
#include "grideyeImpl.hpp"
#include <string>

GrideyeImpl ge;
std::string myOSCID = "0";

Timeout matTimeout(500);
void setup() {
  ge.setup();
  // Pour a bowl of serial
  Serial.begin(115200);
  connectivity::setup(myOSCID);
}

void loop() {
  auto t = millis();
  ge.loop();
  if (connectivity::handleConnection()) {
    if (matTimeout.update(t)) {
      connectivity::sendOSC("/mat", ge.temperatureMat());
    }
  } else {
    ge.dbg();

    delay(1000);
  }
}
