#include <Arduino.h>
#include <M5Unified.h>
#include "HX711.h"

#define SCALE 27.61f

HX711 scale;
int y = 0;
uint8_t batteryLevel;

void calibration();

void setup() {
  // init M5
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Power.begin();

  M5.Display.setTextSize(1.25);
  M5.Display.setCursor(0, y);
  batteryLevel = M5.Power.getBatteryLevel();
  M5.Display.printf("Battery: %3d%%", batteryLevel);
  M5_LOGI("battery:%d", batteryLevel);
  y += M5.Display.fontHeight();

  // setup scale
  uint8_t datPin = M5.getPin(m5::port_a_scl); // PIN1
  uint8_t clkPin = M5.getPin(m5::port_a_sda); // PIN2
  scale.begin(datPin, clkPin);
  // The scale value is the adc value corresponding to 1g
  // scale.set_scale(27.61f);  // set scale
  // scale.tare();             // auto set offset
  calibration();
}

void loop() {
  M5.delay(10);
  M5.update();
  batteryLevel = M5.Power.getBatteryLevel();

  if (M5.BtnA.wasReleasedAfterHold()) { // 長押しでバッテリー残量
    M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.fontHeight(), M5.Display.isEPD() ? TFT_WHITE : TFT_BLACK);
    batteryLevel = M5.Power.getBatteryLevel();
    M5.Display.setCursor(0, 0);
    M5.Display.printf("Battery: %3d%%", batteryLevel);
    M5_LOGI("battery:%d", batteryLevel);
    M5.Display.setCursor(0, y);
  } else if (M5.BtnA.wasSingleClicked()) {
    // 計測する
    float weight = scale.get_units(10) / 1000.0;
    M5.Display.startWrite();
    // M5.Display.fillScreen(M5.Display.isEPD() ? TFT_WHITE : TFT_BLACK);
    M5.Display.setCursor(0, y);
    if (weight >= 0) {
      M5.Display.printf("%.2f", weight);
    } else {
      M5.Display.printf("0.00", weight);
    }
    Serial.printf("%.2f\n", weight);
    M5_LOGI("weight:%.2f", weight);
    y += M5.Display.fontHeight();
    M5.Display.endWrite();
  }
}

// ------------------------------------------------------------------------------------------------

void calibration() {
  Serial.println("Before setting up the scale:");
  Serial.printf("read: \t\t%.2f\n", scale.read()); // print a raw reading from the ADC
  Serial.printf("read average: \t\t%.2f\n", scale.read_average(20)); // print the average of 20 readings from the ADC
  Serial.printf("get value: \t\t%.2f\n", scale.get_value(5)); // print the average of 5 readings from the ADC minus the tare weight (not set yet)
  Serial.printf("get units: \t\t%.2f\n", scale.get_units(5));
  // print the average of 5 readings from the ADC minus tare weight (not set)
  // divided by the SCALE parameter (not set yet)
  scale.set_scale(SCALE); // this value is obtained by calibrating the scale 
  // with known weights; see the README for details ----------------(2)
  scale.tare(); // reset the scale to 0
  Serial.println("After setting up the scale:");
  Serial.printf("read: \t\t%.2f\n", scale.read()); // print a raw reading from the ADC
  Serial.printf("read average: \t\t%.2f\n", scale.read_average(20));
  // print the average of 20 readings from the ADC
  Serial.printf("get value: \t\t%.2f\n", scale.get_value(5));
  // print the average of 5 readings from the ADC minus the tare weight, 
  // set with tare()
  Serial.printf("get units: \t\t%.2f\n", scale.get_units(5));
  // print the average of 5 readings from the ADC minus tare weight, divided
  // by the SCALE parameter set with set_scale
}