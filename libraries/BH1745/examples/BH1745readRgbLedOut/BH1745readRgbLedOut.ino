#include <Arduino.h>
#include <BH1745.h>

BH1745 bh = BH1745();

int rPin = 9;
int gPin = 10;
int bPin = 11;

// If you are using common cathode RGB LED, uncommend this
#define COMMON_ANODE

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

  pinMode(rPin, OUTPUT);
  pinMode(gPin, OUTPUT);
  pinMode(bPin, OUTPUT);
}

void loop() {
  bh.read();
  #ifndef COMMON_ANODE
  analogWrite(rPin, bh.red);
  analogWrite(gPin, bh.green);
  analogWrite(bPin, bh.blue);
  #else
  analogWrite(rPin, 255-bh.red);
  analogWrite(gPin, 255-bh.green);
  analogWrite(bPin, 255-bh.blue);
  #endif
  delay(200);
}