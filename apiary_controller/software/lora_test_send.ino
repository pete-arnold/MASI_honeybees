/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   ESP32S3 LoRa Transmitter
   Query the BME280 and send messages with sensor data to LoRa Receiver.
   26.7.2024
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Changed message format to JSON and rounding processes.
   TPH all rounded to 1 d.p.
   * Can we trigger this much less frequently?
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Based on example in Random Nerd Tutorials by Rui Santos and Sara Santos,
   itself from the examples in the Arduino LoRa library.
   See:
   https://RandomNerdTutorials.com/esp32-lora-rfm95-transceiver-arduino-ide/
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <SPI.h>              /* Use the default SPI library for the LoRa transceiver access. */
#include <LoRa.h>             /* Sandeep Mistry's library. */
#include <Wire.h>             /* Default I2C interface (to explicitly define the pins). */
#include <Adafruit_BME280.h>  /* Adafruit BME280. */
#include <ArduinoBLE.h>

/* Definitions for the app. */
#define INTERVAL 5000

/* Definitions for the LoRa transceiver SPI interface. */
#define SPI_NSS 10
#define SPI_RESET 14
#define SPI_DIO0 46

int counter = 0;
char* message = "";

/* Definitions for the BME280 breakout board. */
#define I2C_SDA 8
#define I2C_SCL 9
#define BME280_ADDRESS 0x76  // If the sensor does not work, try the 0x77 address instead.
#define ALTITUDE 50.0
#define SEALEVELPRESSURE_HPA 1013.25

Adafruit_BME280 bme;
float last_temperature = -1;
float last_pressure = -1;
float last_humidity = -1;
char buffer[16384];

/* Definitions for the Arduino Nano 33 Central Sensor (Luminance) BLE device. */
/* Definitions for the BLE colour service. */
const char* deviceServiceUuid = "1af8745a-e788-441d-9186-8c2f27a47d21";
const char* deviceServiceCharacteristicUuid = "78b26028-a586-421c-ba5f-81a9873a75f1";

BLEService colourService(deviceServiceUuid); 
BLEByteCharacteristic colourCharacteristic(deviceServiceCharacteristicUuid, BLERead | BLEWrite);

long colour = -1;
bool new_colour_data = false;
bool new_thermal_data = false;

/* Definitions for the BLE thermal service. */
const char* thermalDeviceServiceUuid = "716ff1c3-c47c-419f-9bd0-24a57ece6364";
const char* thermalDeviceServiceCharacteristicUuid = "dd50324b-e3c3-441e-aebf-9ea617c75dd7";

BLEService thermalService(thermalDeviceServiceUuid); 
BLECharacteristic thermalCharacteristic(thermalDeviceServiceCharacteristicUuid, BLERead | BLEWrite, 768 * 4, true);

float thermal[768] = { 0.0 };

/* Set-up the hardware. */
void setup() {
  /* Start the serial monitor. */
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Sender");

  /* Prepare the LoRa interface. */
  LoRa_init();

  /* Prepare the BME280. */
  BME280_init();

  /* Prepare the BLE. */
  BLE_init();
}

char *array_to_str(char * str, float *array, unsigned int n) {
  char s[256];
  str[0] = 0;
  for(int i = 0; i < n-1; i++ )
  {
      s[0]=0;
      sprintf(s, "%2.1f,", array[i]);
      strcat(str, s);    
  }
  s[0]=0;
  sprintf(s, "%2.1f", array[n-1]);
  strcat(str, s);    
  return str;
}

void loop() {
  /* Get the BME280 sensor data. */
  float temperature = getTemperature();
  float humidity = getHumidity();
  float pressure = getPressure();

  /* Get the luminance data from the BLE Nano 33. */
  Serial.println("BLE:Discovering central device...");
  BLEDevice central = BLE.central();
  delay(500);

  if (central) {
    Serial.println("BLE: Connected to central device!");
    //Serial.print("Device MAC address: ");
    //Serial.println(central.address());

    colour = 0;
    unsigned int thermal_index = 0;

    while (central.connected()) {
      if (colourCharacteristic.written()) {
        new_colour_data = true;
        colour = (colour * 256) + colourCharacteristic.value();
        //Serial.print("Current colour from BH1745 is 0x");
        //Serial.println(colour, 16);
      }
      if (thermalCharacteristic.written()) {
        new_thermal_data = true;
        //Serial.print("Thermal data (byte ");
        //Serial.print(thermal_index);
        //Serial.print(") = ");
        byte thermal_byte = *thermalCharacteristic.value();
        ((byte*)&thermal)[thermal_index++] = thermal_byte;
        //Serial.println(thermal_byte, 16);
      }
    }
    Serial.println("BLE:Disconnected from central device!");
  }

  /* Only send a message if the data has changed. */
  if (new_colour_data) {
    new_colour_data = false;

    Serial.print("Sending tph/colour packet: ");
    // sprintf(message, "(%d)%d: T=%.1f, P=%.0f, H=%.1f", counter, millis()/1000, temperature, pressure, humidity);
    sprintf(message, "{\"time\":%d,\"temperature\":%.1f,\"pressure\":%.1f,\"humidity\":%.1f,\"colour\":\"0x%08x\"}", millis()/1000, temperature, pressure, humidity, colour);
    Serial.println(message);
    /* Send LoRa packet to receiver. */
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
    delay(100);
  }
  if (new_thermal_data) {
    new_thermal_data = false;
    Serial.print("Sending thermal packets ... ");
    for (int row=0; row<24; row++) {
      sprintf(message, "{\"row\":%d, \"thermal\":[%s]}", row, array_to_str(buffer, &thermal[row*32], 32));
      //Serial.println(message);
      /* Send LoRa packet to receiver. */
      LoRa.beginPacket();
      LoRa.print(message);
      LoRa.endPacket();
      delay(100);
    }
    Serial.println("[OK]");
  }
  counter++;
  last_temperature = temperature;
  last_humidity = humidity;
  last_pressure = pressure;

  // delay(INTERVAL);
}

/* LoRa functions. */
void LoRa_init()
{
  /* Get the default parameters for the SPI. */
  Serial.begin(115200);
  Serial.print("MOSI: ");
  Serial.println(MOSI);
  Serial.print("MISO: ");
  Serial.println(MISO);
  Serial.print("SCK: ");
  Serial.println(SCK);
  Serial.print("SS: ");
  Serial.println(SS);  

  /* Define the optional LoRa interface pins. */
  LoRa.setPins(SPI_NSS, SPI_RESET, SPI_DIO0);
  
  /* Replace the LoRa.begin(---E-) argument with your location's frequency 
     433E6 for Asia
     868E6 for Europe
     915E6 for North America */
  while (!LoRa.begin(868E6)) {
    Serial.println(".");
    delay(500);
  }
  /* Change sync word (0xF3) to match the receiver.
     The sync word assures you don't get LoRa messages from other LoRa transceivers
     range 0x00-0xFF. */
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa initialised.");
}

/* BME280 functions. */
void BME280_init()
{
  unsigned status = 0;

  /* According to Luis Llamas and https://www.studiopieters.nl/esp32-s3-wroom-pinout
     the default pin-out is SDA=GPIO8 and SCL=GPIO9. However, these appear to have
     to be explicitly set. */
  /* Set-up the I2C bus at the required pins. */
  Wire.begin(I2C_SDA, I2C_SCL);
  /* Start the BME280 with the address and I2C wires defined. */
  status = bme.begin(BME280_ADDRESS, &Wire);
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bme.sensorID(),16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1) delay(10);
  }
  Serial.println("BME280 initialised.");
}

float getTemperature()
{
  float temperature = round(bme.readTemperature() * 10) / 10;
  return (temperature);
}

float getHumidity()
{
  float humidity = round(bme.readHumidity() * 10) / 10;
  return (humidity);
}

float getPressure()
{
  float pressure = bme.readPressure();
  pressure = bme.seaLevelForAltitude(ALTITUDE, pressure);
  pressure = pressure / 100.0F;
  return (round(pressure * 10) / 10);
}

/* BLE Functions. */
void BLE_init() {
  if (!BLE.begin()) {
    Serial.println("Bluetooth® Low Energy module set-up failed!");
    while (1);
  }

  BLE.setLocalName("ESP32 BLE (Peripheral)");
  BLE.setAdvertisedService(colourService);
  colourService.addCharacteristic(colourCharacteristic);
  BLE.addService(colourService);
  colourCharacteristic.writeValue(-1);
  // BLE.advertise();

  // BLE.setAdvertisedService(thermalService);
  thermalService.addCharacteristic(thermalCharacteristic);
  BLE.addService(thermalService);
  // thermalCharacteristic.writeValue(-1);
  BLE.advertise();

  Serial.println("ESP32 BLE (Peripheral Device) set-up complete.");
}