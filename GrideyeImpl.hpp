#define AMG 1 // amg reads in 15ms instead of 40ms for Sparkfun

#if AMG
#include <Adafruit_AMG88xx.h>
#else
#include <SparkFun_GridEYE_Arduino_Library.h>
#endif

#include <Wire.h>
#include <vector>
using std::vector;
struct GrideyeImpl {

  float test = 0;
  float echo(float f) {
    Serial.print("inside echo :");
    Serial.println(f);
    return f;
  }

  GrideyeImpl() : tempBuf(64, 0.0f), tempMeans(64, 0.0f) {}

  bool setup() {

    Wire.begin();
#if AMG
    amg.begin();
    amg.disableInterrupt();

    amg.setMovingAverageMode(true);
#else
    grideye.begin();
    grideye.setFramerate10FPS();
#endif

    delay(100); // sensor heat up time?
    return true;
  }

  bool loop() { return true; }

  float getThermistor() {
#if AMG
    return amg.readThermistor();
#else
#error not implemented
#endif
  }

  const vector<float> &temperatureMat() {
    // auto t = millis();
#if AMG
    amg.readPixels(tempBuf.data());
#else
    for (unsigned char i = 0; i < 64; i++) {
      tempBuf[i] = grideye.getPixelTemperature(i);
    }
#endif
    // Serial.print("took : ");
    // Serial.println(millis() - t);
    if (!inited) {
      inited = true;
      for (unsigned char i = 0; i < 64; i++) {
        tempMeans[i] = tempBuf[i];
      }
    }
    return tempBuf;
  }

  void updateTempMean(bool updateTemp = true) {
    if (updateTemp) {
      temperatureMat();
    }
    // constexpr float degDecay = 2;
    float slope = 0.1; // deltaT * 2;
    for (unsigned char i = 0; i < 64; i++) {
      tempMeans[i] += slope * (tempBuf[i] - tempMeans[i]);
    }
  }

  vector<float> tempDiff(bool updateTemp = true, float deltaT = 0) {
    float activityThreshold = 3;
    auto oldMeans = tempMeans;
    updateTempMean(updateTemp);
    vector<float> res(tempBuf);
    int i = 0;
    float slope = 0.1;
    for (auto &f : res) {
      f -= tempMeans[i];
      if (abs(f) > activityThreshold) {
        tempMeans[i] = slope * tempMeans[i] + (1.f - slope) * oldMeans[i];
        f = 100.0;
      }
      i++;
    }
    return res;
  }

  vector<float> tempBGTRemoved() {

    switch (bgRemovalMode) {
    case NO:
      return temperatureMat();
    case CAPTURING:
      updateTempMean(true);
      return tempBuf;

    case REMOVE:
      temperatureMat();
      auto res = tempBuf;
      int i = 0;
      for (auto &e : res) {
        e -= tempMeans[i];
        i++;
      }
      return res;
    }
  }

  void dbg() {
#if AMG
#else
    Serial.print("Thermistor Temperature in Celsius: ");
    Serial.println(grideye.getDeviceTemperature());
    Serial.print("Thermistor Temperature in Fahrenheit: ");
    Serial.println(grideye.getDeviceTemperatureFahrenheit());
    Serial.print("Thermistor Temperature register contents: ");
    Serial.println(grideye.getDeviceTemperatureRaw(), BIN);

    Serial.println();
    Serial.println();
#endif
  }

#if AMG
  Adafruit_AMG88xx amg;
#else
  GridEYE grideye;
#endif

  typedef enum {
    NO,
    CAPTURING,
    REMOVE,

  } BGRemovalMode;

  BGRemovalMode bgRemovalMode = NO;
  vector<float> tempBuf;
  vector<float> tempMeans;
  bool inited = false;
};
