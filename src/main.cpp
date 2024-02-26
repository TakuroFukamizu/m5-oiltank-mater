#include <Arduino.h>
#include "M5GFX.h"
#include <M5Unified.h>
#include "HX711.h"

#define FLG_DEBUG_NOHX711 false

#define SCALE 27.61f

HX711 scale;
uint8_t batteryLevel;
unsigned long previousMillis = 0;

uint8_t chartWidth = 30;
uint8_t maxLitter = 18.f;

float prevValue = 0.f;

void calibration();
void measure();
void updateBatteryLevel();

void setup() {
  // init M5
  auto cfg = M5.config();
  M5.begin(cfg);
  M5.Power.begin();
  M5.Display.startWrite(); 
  M5.Display.setEpdMode(m5gfx::epd_fast);
  M5.Display.fillScreen(TFT_WHITE);

  pinMode(GPIO_NUM_38, INPUT_PULLUP);

  // setup scale
  uint8_t datPin = M5.getPin(m5::port_a_scl); // PIN1
  uint8_t clkPin = M5.getPin(m5::port_a_sda); // PIN2
  #if FLG_DEBUG_NOHX711 == false
  scale.begin(datPin, clkPin);
  #endif

  // calibration();
  updateBatteryLevel();
}

void loop() {
  M5.delay(100);
  M5.update();

  if (M5.BtnEXT.wasSingleClicked()) { // calibrate scale manually
    previousMillis = millis();
    calibration();
  }

  measure(); // measure and display results

  // enter to deepsleep when after 5 seconds of bootup.
  bool isNeedToSleep = false;
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < 5000) {
    // count down
    unsigned long remainingSec = 5 - (currentMillis - previousMillis) / 1000;
    M5.Display.setTextSize(1);
    M5.Display.setTextColor(TFT_BLACK);
    M5.Display.fillRect(0, M5.Display.height() - M5.Display.fontHeight(), M5.Display.width(), M5.Display.fontHeight(), TFT_WHITE);
    M5.Display.setCursor(0, M5.Display.height() - M5.Display.fontHeight());
    M5.Display.printf("Sleep after: %d sec...", remainingSec);
  } else {
    // enter to deep sleep
    M5.Display.fillRect(0, M5.Display.height() - M5.Display.fontHeight(), M5.Display.width(), M5.Display.fontHeight(), TFT_WHITE);
    isNeedToSleep = true;
  }
  if (M5.Display.isEPD()) M5.Display.display(); // write EPD
  if (isNeedToSleep) {
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_38, LOW);
    esp_light_sleep_start(); // goto LIGHT-SLEEP

    // esp_sleep_wakeup_cause_t wakeup_reason;
    // wakeup_reason = esp_sleep_get_wakeup_cause();
    // switch(wakeup_reason){
    //   case ESP_SLEEP_WAKEUP_EXT0:
    //   case ESP_SLEEP_WAKEUP_TIMER:
    //     break;
    //   default:
    //     // calibrate scale when turn-on device manually
    //     calibration();
    // }

    /// ピポッ
    M5.Speaker.tone(2000, 100, 0, true);
    M5.Speaker.tone(1000, 100, 0, false);

    previousMillis = millis();
    updateBatteryLevel();
  }
}

// ------------------------------------------------------------------------------------------------

void updateBatteryLevel() {
  batteryLevel = M5.Power.getBatteryLevel();
  M5_LOGI("battery:%d", batteryLevel);

  // updaet header area
  M5.Display.setTextSize(1.5);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.fillRect(0, 0, M5.Display.width(), M5.Display.fontHeight(), TFT_BLACK);
  M5.Display.setCursor(0, 0);
  M5.Display.printf("Oil Monitor");
  M5.Display.setCursor(M5.Display.width() - M5.Display.fontWidth() * 9, 0);
  M5.Display.printf("BAT: %3d%%", batteryLevel);
}

void measure() {
  #if FLG_DEBUG_NOHX711 == false
  float weight = scale.get_units(10) / 1000.0;
  #else
  float weight = 18.f / 0.78;
  #endif
  float litter = weight * 0.78;
  if (maxLitter < litter) {
    litter = maxLitter;
  }
  M5_LOGI("measure: weight=%.2f, value=%.2f", weight, litter);
  if (prevValue == litter) {
    return; // pass procedure when no change
  }
  prevValue = litter;
  float percent = litter / (float)maxLitter;

  uint8_t chartSize = M5.Display.height() - 30;
  uint8_t y = (M5.Display.height() / 2);
  uint8_t x = (M5.Display.width() / 2);
  uint8_t r0 = (chartSize / 2);
  float angle0 = ((float)(360.f * (1.f - percent)) - 90.f);
  float angle1 = -90.f; // 0 point(270)
  if (angle0 == angle1) {
    angle0 = -89.f;
  }

  M5.Display.fillRect(0, (y - chartSize / 2), M5.Display.width(), chartSize, TFT_WHITE);
  // pi chart
  M5.Display.drawCircle(x, y, r0, TFT_BLACK);
  M5.Display.drawCircle(x, y, (r0 - chartWidth), TFT_BLACK);
  M5.Display.fillArc(x, y, r0, (r0 - chartWidth), angle0, angle1, TFT_BLACK);
  // value labels
  M5.Display.setTextColor(TFT_BLACK);
  M5.Display.setTextSize(3);
  y = y - M5.Display.fontHeight() / 2;
  M5.Display.setCursor((x - (M5.Display.fontWidth() * 2.f)), y);
  M5.Display.printf("%02.1f%%", percent * 100);
  y += M5.Display.fontHeight();
  M5.Display.setTextSize(1.5);
  M5.Display.setCursor((x - (M5.Display.fontWidth() * 2.5f)), y);
  M5.Display.printf("%.2fL", litter);
}

void calibration() {
  M5_LOGI("calibration");
  #if FLG_DEBUG_NOHX711 == false
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
  #endif
}