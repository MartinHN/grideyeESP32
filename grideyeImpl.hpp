#include <SparkFun_GridEYE_Arduino_Library.h>
#include <Wire.h>
#include <vector>
using std::vector;
struct GrideyeImpl {
  GrideyeImpl() : tempBuf(64, 0.0f) {}

  bool setup() {
    Wire.begin();
    grideye.begin();

    return true;
  }

  bool loop() { return true; }

  const vector<float> &temperatureMat() {
    for (unsigned char i = 0; i < 64; i++) {
      tempBuf[i] = grideye.getPixelTemperature(i);
    }
    return tempBuf;
  }

  void dbg() {
    Serial.print("Thermistor Temperature in Celsius: ");
    Serial.println(grideye.getDeviceTemperature());
    Serial.print("Thermistor Temperature in Fahrenheit: ");
    Serial.println(grideye.getDeviceTemperatureFahrenheit());
    Serial.print("Thermistor Temperature register contents: ");
    Serial.println(grideye.getDeviceTemperatureRaw(), BIN);

    Serial.println();
    Serial.println();
  }
  GridEYE grideye;

  vector<float> tempBuf;
};
