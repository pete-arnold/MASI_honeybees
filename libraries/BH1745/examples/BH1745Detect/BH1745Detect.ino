#include <Arduino.h>
#include <BH1745.h>

BH1745 bh = BH1745();

// for ESP32 Change the SDA and SCl
#ifdef ESP32
#define SDA 26
#define SCL 25
#endif

void setup() {
  Serial.begin(115200);
  #ifdef ESP32
  bool result = bh.begin(SDA, SCL);
  #else
  bool result = bh.begin();
  #endif
  if (!result){
    Serial.println("Device Error");
    while (1){;;}
  }
  bh.setGain(bh.GAIN_16X);
  bh.setRgbcMode(bh.RGBC_8_BIT);
}

void loop() {
  Serial.print("is detect Red: ");Serial.println(bh.isDetectColor(bh.RED));
  delay(500);
}