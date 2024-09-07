/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Nano33Ble BH1745
   Query the BH1745 and communicate to the ESP32 via BLE.
   31.7.2024
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   This program uses the ArduinoBLE library to set-up an Arduino Nano 33 BLE 
   as a central device and looks for the specified service and
   characteristic in a peripheral device (the ESP32/LoRa). If the specified
   service and characteristic is found in the peripheral device, the last
   detected value of the BH1745 sensor is written in the specified
   characteristic. 
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <BH1745.h>           /* The BH1745 library. */
#include <Wire.h>             /* Default I2C interface (to explicitly define the pins). */
#include <ArduinoBLE.h>       /* The BLE library. */

#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

#define INTERVAL 15000

/* Definitions for the BH1745 breakout board. */
#define I2C_SDA A4
#define I2C_SCL A5
#define BH1745_ADDRESS 0x38   // If the sensor does not work, try the 0x39 address instead.
#define BH1745_USE_LEDS 1     // Whether the use the on-board LEDS.

BH1745 bh1745;
long colour = 0;
long old_colour = 0;

/* Definitions for the BLE colour service. */
const char* colourDeviceServiceUuid = "1af8745a-e788-441d-9186-8c2f27a47d21";
const char* colourDeviceServiceCharacteristicUuid = "78b26028-a586-421c-ba5f-81a9873a75f1";

/* Definitions for the BLE thermal service. */
const char* thermalDeviceServiceUuid = "716ff1c3-c47c-419f-9bd0-24a57ece6364";
const char* thermalDeviceServiceCharacteristicUuid = "dd50324b-e3c3-441e-aebf-9ea617c75dd7";

BLEDevice peripheral;

/* Definitions for the MLX90640. */
const byte MLX90640_address = 0x33;   //Default 7-bit unshifted address of the MLX90640
#define TA_SHIFT 8                    //Default shift for MLX90640 in open air
static float mlx90640To[768];
paramsMLX90640 mlx90640;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  /* Start the serial monitor. */
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Nano33Ble BH1745");

  BH1745_init();

  BLE_init();

  MLX90640_init();
}

// the loop function runs over and over again forever
void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  if (BH1745_USE_LEDS) {
    bh1745.setLED(true);
  }
  bh1745.readColour();
  colour = bh1745.getCRGB();
  // Serial.print("Colour = ");
  // Serial.println(colour, 16);
  readMLX();
  if (BH1745_USE_LEDS) {
    bh1745.setLED(false);
  }
  // bh1745.printColour();
  // if (colour != old_colour) {
      connectToColourPeripheral();
      old_colour = colour;
  // }
  connectToThermalPeripheral();
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  // delay(500);                      // wait for a second
  delay(INTERVAL);                      // wait for a second
}

/* BH1745 functions. */
void BH1745_init()
{
  unsigned status;

  /* Set-up the I2C bus at the required pins. */
  Serial.println("Initialising BH1745.");
  // sprintf(message, "(%d)%d: T=%.1f, P=%.0f, H=%.1f", counter, millis()/1000, temperature, pressure, humidity);
  String message = "I2C_SDA: " + String(A4) + ", I2C_SCL: " + String(A5) + ".";
  Serial.println(message);
  status = bh1745.begin();
  /* Start the BH1745 with the address and I2C wires defined. */
  Serial.print("SensorID was: 0x");
  Serial.println(bh1745.sensorID(), 16);
  if (!status) {
    Serial.println("Could not find a valid BH1745 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bh1745.sensorID(), 16);
    while (1) delay(10);
  }
  bh1745.setRGBCMode(RGBC_8_BIT);
  bh1745.setGain(RGBC_GAIN_16X);
  Serial.println("BH1745 initialised.");
}

/* BLE functions. */
void BLE_init(){
  if (!BLE.begin()) {
    Serial.println("Bluetooth® Low Energy module set-up failed!");
    while (1);
  }
  
  BLE.setLocalName("Nano 33 BLE (Central - Sensor)"); 
  BLE.advertise();

  Serial.println("Arduino Nano 33 BLE Sense (Central / Sensor Device)");
  Serial.println();
}

void connectToColourPeripheral() {
  //BLEDevice peripheral;
  
  Serial.println("BLE:Looking for colour peripheral device...");
  do
  {
    BLE.scanForUuid(colourDeviceServiceUuid);
    peripheral = BLE.available();
  } while (!peripheral);
  
  if (peripheral) {
    /* Serial.println("  Colour peripheral device found!");
    Serial.print("    Device MAC address: ");
    Serial.println(peripheral.address());
    Serial.print("    Device name: ");
    Serial.println(peripheral.localName());
    Serial.print("    Advertised service UUID: ");
    Serial.println(peripheral.advertisedServiceUuid());
    Serial.println();
     */
    BLE.stopScan();
    controlColourPeripheral(peripheral);
  } else {
    Serial.println("  No colour peripheral device found.");
  }
}

void controlColourPeripheral(BLEDevice peripheral) {
  // Serial.println("BLE:Connecting to colour peripheral device...");
  if (peripheral.connect()) {
    Serial.println("Connected to colour peripheral device!");
  } else {
    Serial.println("Connection to colour peripheral device failed!");
    return;
  }
  // Serial.println("- Discovering colour peripheral device attributes...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Colour peripheral device attributes discovered!");
  } else {
    Serial.println("Colour peripheral device attributes discovery failed!");
    peripheral.disconnect();
    return;
  }

  BLECharacteristic colourCharacteristic = peripheral.characteristic(colourDeviceServiceCharacteristicUuid);
    
  if (!colourCharacteristic) {
    Serial.println("Peripheral device does not have colour characteristic!");
    peripheral.disconnect();
    return;
  } else if (!colourCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable colour characteristic!");
    peripheral.disconnect();
    return;
  }
  
  while (peripheral.connected()) {
    // Serial.print("Writing value to colour characteristic: ");
    //Serial.println(colour, 16);
    colourCharacteristic.writeValue((colour >> 24) & 0xff);
    //Serial.println((colour >> 24) & 0xff, 16);
    colourCharacteristic.writeValue((colour >> 16) & 0xff);
    //Serial.println((colour >> 16) & 0xff, 16);
    colourCharacteristic.writeValue((colour >> 8) & 0xff);
    //Serial.println((colour >> 8) & 0xff, 16);
    colourCharacteristic.writeValue(colour & 0xff);
    //Serial.println(colour & 0xff, 16);
    Serial.println("Writing value to colour characteristic done!");
    peripheral.disconnect();
  }
  Serial.println("Colour peripheral device disconnected!");
}

/* MLX90640 functions. */
void MLX90640_init() {
  if (isMLXConnected() == false)
  {
    Serial.println("MLX90640 not detected at default I2C address. Please check wiring. Freezing.");
    while (1);
  }
  Serial.println("MLX90640 online!");

  //Get device parameters - We only have to do this once
  int status;
  uint16_t eeMLX90640[832];
  status = MLX90640_DumpEE(MLX90640_address, eeMLX90640);
  if (status != 0)
    Serial.println("Failed to load system parameters");

  status = MLX90640_ExtractParameters(eeMLX90640, &mlx90640);
  if (status != 0)
    Serial.println("Parameter extraction failed");

  //Once params are extracted, we can release eeMLX90640 array
}

// Is the MLX90640 is detected on the I2C bus?
boolean isMLXConnected()
{
  Wire.beginTransmission((uint8_t)MLX90640_address);
  if (Wire.endTransmission() != 0)
    return (false); // Sensor did not ACK.
  return (true);
}

void readMLX() {
  for (byte x = 0 ; x < 2 ; x++) //Read both subpages
  {
    uint16_t mlx90640Frame[834];
    int status = MLX90640_GetFrameData(MLX90640_address, mlx90640Frame);
    if (status < 0)
    {
      Serial.print("GetFrame Error: ");
      Serial.println(status);
    }

    float vdd = MLX90640_GetVdd(mlx90640Frame, &mlx90640);
    float Ta = MLX90640_GetTa(mlx90640Frame, &mlx90640);

    float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
    float emissivity = 0.95;

    MLX90640_CalculateTo(mlx90640Frame, &mlx90640, emissivity, tr, mlx90640To);
  }

  /* for (int x=0; x<24; x++) {
    Serial.print("Row ");
    if (x<10) Serial.print("0");
    Serial.print(x);
    Serial.print(": ");
    for (int y=0; y<32; y++) {
      Serial.print(mlx90640To[x*32+y], 1);
      Serial.print("  ");
    }
    Serial.println();
  }
   */
}

void connectToThermalPeripheral() {
  // BLEDevice peripheral;
  
  /*Serial.println("BLE:Looking for thermal peripheral device...");
  do
  {
    BLE.scanForUuid(colourDeviceServiceUuid);
    peripheral = BLE.available();
  } while (!peripheral);
   */
  if (peripheral) {
    /* Serial.println("  Thermal peripheral device found!");
    Serial.print("    Device MAC address: ");
    Serial.println(peripheral.address());
    Serial.print("    Device name: ");
    Serial.println(peripheral.localName());
    Serial.print("    Advertised service UUID: ");
    Serial.println(peripheral.advertisedServiceUuid());
    Serial.println();
     */
    // BLE.stopScan();
    controlThermalPeripheral(peripheral);
  } else {
    Serial.println("  No thermal peripheral device found.");
  }
}

void controlThermalPeripheral(BLEDevice peripheral) {
  // Serial.println("BLE:Connecting to thermal peripheral device...");
  if (peripheral.connect()) {
    Serial.println("Connected to thermal peripheral device!");
  } else {
    Serial.println("Connection to thermal peripheral device failed!");
    return;
  }

  // Serial.println("Discovering thermal peripheral device attributes...");
  if (peripheral.discoverAttributes()) {
    Serial.println("Peripheral thermal device attributes discovered!");
  } else {
    Serial.println("Peripheral thermal device attributes discovery failed!");
    peripheral.disconnect();
    return;
  }

  BLECharacteristic thermalCharacteristic = peripheral.characteristic(thermalDeviceServiceCharacteristicUuid);
    
  if (!thermalCharacteristic) {
    Serial.println("Peripheral device does not have thermal characteristic!");
    peripheral.disconnect();
    return;
  } else if (!thermalCharacteristic.canWrite()) {
    Serial.println("Peripheral does not have a writable thermal characteristic!");
    peripheral.disconnect();
    return;
  }
  
  while (peripheral.connected()) {
    Serial.println("Writing value to thermal characteristic ...");
    for (int x=0; x<24; x++) {
      Serial.print("Row ");
      if (x<10) Serial.print("0");
      Serial.print(x);
      // Serial.print(": ");
      for (int y=0; y<32; y++) {
        // Serial.print(mlx90640To[x*32+y], 1);
        Serial.print("  ");
        thermalCharacteristic.writeValue(((byte*)&mlx90640To[x*32+y])[0]);
        thermalCharacteristic.writeValue(((byte*)&mlx90640To[x*32+y])[1]);
        thermalCharacteristic.writeValue(((byte*)&mlx90640To[x*32+y])[2]);
        thermalCharacteristic.writeValue(((byte*)&mlx90640To[x*32+y])[3]);
      }
      Serial.println();
    }
    peripheral.disconnect();
  }
  Serial.println("Peripheral thermal device disconnected!");
}

