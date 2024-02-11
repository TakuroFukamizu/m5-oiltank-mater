#include <Arduino.h>
#include <M5Unified.h>
#include "HX711.h"

HX711 scale;

void setup() {
  auto cfg = M5.config();
  M5.begin(cfg);

  // setup scale
  uint8_t datPin = M5.getPin(m5::port_a_scl); // PIN1
  uint8_t clkPin = M5.getPin(m5::port_a_sda); // PIN2
  scale.begin(datPin, clkPin);
  // The scale value is the adc value corresponding to 1g
  scale.set_scale(27.61f);  // set scale
  scale.tare();             // auto set offset
}

void loop() {
  M5.delay(10);
  M5.update();
  if (M5.BtnA.wasSingleClicked() || M5.BtnPWR.wasSingleClicked()) {
    // 計測する
    float weight = scale.get_units(10) / 1000.0;
    if (weight >= 0) {
      Serial.printf("%.2f\n", weight);
    } else {
      //
    }
  }
}
