/*
Simple library to use a BH1745NUC Digital Light Sensor.
SunFounder 2021
*/

#include "BH1745.h"

/**
 * Constructor
 */
BH1745::BH1745() {}

/**
 * Configure sensor
 */
#ifdef ESP32
bool BH1745::begin(int sda, int scl) {
  Wire.begin(sda, scl);
  return init();
}
#else
bool BH1745::begin() {
  Wire.begin();
  return init();
}
#endif

/**
 * Configure sensor
 */
bool BH1745::init() {
  Wire.beginTransmission(BH1745_ADDRESS);
  Wire.write(CMD_SYSTEM_CONTROL);
  Wire.endTransmission();

  /* check if sensor, i.e. the chip ID is correct
  _sensorID = read8(BME280_REGISTER_CHIPID);
  if (_sensorID != 0x60)
    return false;
   */
  Wire.requestFrom(BH1745_ADDRESS, 1);
  _sensorID = Wire.read();
  if (_sensorID != BH1745_SENSOR_ID){
    return false;
  }

  Wire.beginTransmission(BH1745_ADDRESS);
  Wire.write(CMD_MODE_CONTROL1);
  Wire.write(RGBC_MEASUREMENT_TIME_160_MSEC);
  Wire.write(RGBC_ENABLE + RGBC_GAIN_1X);
  Wire.write(0);
  Wire.write(MODE_CONTROL3_DATA);
  Wire.endTransmission();
  // write8(CMD_SYSTEM_CONTROL, BH1745_SENSOR_ID);
  // write8(CMD_MODE_CONTROL1, MEASUREMENT_TIME_160);
  // write8(CMD_MODE_CONTROL2, RGBC_ENABLE + RGBC_GAIN_16X);
  // write8(CMD_MODE_CONTROL3, MODE_CONTROL3_DATA);
  return true;
}

int8_t BH1745::sensorID(){
  return(_sensorID);
}

/**
 *  Set gain of the internal ADC
 */
void BH1745::setGain(int gain){
  if(gain == RGBC_GAIN_1X){
    write8(CMD_MODE_CONTROL2, RGBC_ENABLE + RGBC_GAIN_1X);
  }else if(gain == RGBC_GAIN_2X){
    write8(CMD_MODE_CONTROL2, RGBC_ENABLE + RGBC_GAIN_2X);
  }else if(gain == RGBC_GAIN_16X){
    write8(CMD_MODE_CONTROL2, RGBC_ENABLE + RGBC_GAIN_16X);
  }else {
    #if DEBUG == 1
    Serial.println("Gain invalid, Try 'RGBC_GAIN_1X/2/16X', like RGBC_GAIN_2X");
    #endif
    return;
  }
}

/**
 *  Set RGBC Mode 8bit or 16bit
 */
void BH1745::setRGBCMode(int mode){
  if (mode != RGBC_8_BIT && mode != RGBC_16_BIT){
    #if DEBUG == 1
    Serial.println("RGBC Mode invalid, Try 'RGBC_8/16_BIT' like RGBC_8_BIT");
    #endif
  } else {
    rgbcMode = mode;
  }
}

/**
 * Read rgbc from sensor.
 * Returns -1 if read is timed out
 */
bool BH1745::read() {
  Wire.beginTransmission(BH1745_ADDRESS);
  Wire.write(RED_DATA_LSB);
  Wire.endTransmission();

  byte data[8];
  Wire.requestFrom(BH1745_ADDRESS, 8);
  int count = 0;
  while (Wire.available()) {
    byte c = Wire.read();
    data[count] = c;
    count++;
  }
  if (count != 8) return false;
  
  red = ((data[1] << 8) + data[0]) & 0xffff;
  green = ((data[3] << 8) + data[2]) & 0xffff;
  blue = ((data[5] << 8) + data[4]) & 0xffff;
  clear = ((data[7] << 8) + data[6]) & 0xffff;
  cr = (clear << 16) + red;
  gb = (green << 16) + blue;
  Serial.print("BH1745 RGB = ");
  Serial.print(cr, 16);
  Serial.print(" ");
  Serial.println(gb, 16);

  // if (rgbcMode == RGBC_8_BIT){
  byte shift = 6;
    red = (red >> shift) & 0xff;
    green = (green >> shift) & 0xff;
    blue = (blue >> shift) & 0xff;
    clear = (clear >> shift) & 0xff;
    crgb = (clear << 24) + (red << 16) + (green << 8) + blue;
    Serial.print("BH1745 CRGB (8 bit) = ");
    Serial.println(crgb, 16);
  // }
  
  rgb2hsv();
  
  return true;
}

void BH1745::rgb2hsv(){
  float r_, g_, b_;
  if (rgbcMode == RGBC_8_BIT){
    r_ = red / 255.0;
    g_ = green / 255.0;
    b_ = blue / 255.0;
  } else {
    r_ = red / 65535.0;
    g_ = green / 65535.0;
    b_ = blue / 65535.0;
  }
  float maxRG_ = max(r_, g_);
  float max_ = max(maxRG_, b_);
  float minRG_ = min(r_, g_);
  float min_ = min(minRG_, b_);
  float delta_ = max_ - min_;

  // Serial.print("r_: ");Serial.print(r_);
  // Serial.print(", g_: ");Serial.print(g_);
  // Serial.print(", b_: ");Serial.println(b_);

  if (delta_ == 0){
    hue = 0;
  } else if (max_ == r_) {
    if (g_ >= b_){
      hue = 60 * ((g_ - b_) / delta_ + 0);
    }else {
      hue = 60 * ((g_ - b_) / delta_ + 6);
    }
  } else if (max_ == g_) {
    hue = 60 * ((b_ - r_) / delta_ + 2);
  } else if (max_ == b_) {
    hue = 60 * ((r_ - g_) / delta_ + 4);
  }
  lightness = (max_ + min_) / 2;
  if (lightness == 0) {
    saturation = 0;
  } else if (lightness <= 0.5) {
    saturation = delta_ / (2 * lightness);
  } else {
    saturation = delta_ / (2 - (2 * lightness));
  }
}

byte BH1745::readColour() {
  read();
  // Serial.println(hue);

  if (hue < 20) {  //change from 15, according to actual test
    return RED;
  } else if (hue < 45) {
    return ORANGE;
  } else if (hue < 90) {
    return YELLOW;
  } else if (hue < 150) {
    return GREEN;
  } else if (hue < 210) {
    return CYAN;
  } else if (hue < 270) {
    return BLUE;
  } else if (hue < 330) {
    return PURPLE;
  } else {
    return RED;
  }
}

unsigned long BH1745::getCRGB() {
  return(crgb);
}

void BH1745::printColour() {
  Serial.print("CRGB: 0x");
  Serial.println(crgb, 16);
  Serial.print("Red: ");
  Serial.println(red);
  Serial.print("Green: ");
  Serial.println(green);
  Serial.print("Blue: ");
  Serial.println(blue);
  Serial.print("Clear: ");
  Serial.println(clear);
  Serial.println();
}

void BH1745::setLED(bool state) {
  Wire.beginTransmission(BH1745_ADDRESS); 
  Wire.write(INTERRUPT);
  if (state) {
    Wire.write(0x81);
  } else {
    Wire.write(0x80);
  }
  Wire.write(0x00);
  Wire.endTransmission();
}

bool BH1745::isDetectColor(byte colour) {
  byte c = readColour();
  if (colour == c){
    return true;
  } else {
    return false;
  }
}

void BH1745::write8(uint8_t cmd, uint8_t data) {
  Wire.beginTransmission(BH1745_ADDRESS); 
  #if (ARDUINO >= 100)
    Wire.write(cmd); // Send register to write with CMD field
    Wire.write(data);  // Write data
  #else
    Wire.send(cmd); // Send register to write with CMD field
    Wire.send(data);  // Write data
  #endif
  Wire.endTransmission(); 
}
