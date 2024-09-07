/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   ESP32S3 LoRa Receiver
   Receive messages with sensor data from LoRa Transmitter.
   19.7.2024
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   Include web server so that the receiver can be examined over WiFi.
   Includes publishing the messages to MQTT.
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

/*********
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Modified from the examples of the Arduino LoRa library
  More resources: https://RandomNerdTutorials.com/esp32-lora-rfm95-transceiver-arduino-ide/
*********/
#include <secrets.h>  /* Authentication secrets etc. */
#include <SPI.h>      /* Use the default SPI library for the LoRa transceiver access. */
#include <LoRa.h>     /* Sandeep Mistry's library. */
#include <WiFi.h>     /* Use the default WiFi library which includes a webserver. */
#include <MQTT.h>     /* Use the MQTT library from Joel Gaehwiler (a light-weight MQTT client). */
#include <ArduinoJson.h>  /* To process the JSON message. */

/* Definitions for the LoRa transceiver SPI interface. */
#define SPI_NSS 10
#define SPI_RESET 14
#define SPI_DIO0 9

String message = "";

/* WiFi definitions. */
/* SSID and password in secrets. */
WiFiServer server(80);
WiFiClient net;

/* MQTT definitions. */
MQTTClient MQTT_client;
// char* MQTT_broker = "public.cloud.shiftr.io";
char* MQTT_broker = "192.168.0.110";

String topic = "/apiary";

/* JSON definitions. */
JsonDocument jsonDoc;
long int message_time = 0;
float temperature = 0.0;
float pressure = 0.0;
float humidity = 0.0;
String colour = "No colour";
float data[24][32] = {0.0};
String colour_table[] = {
"#00000a","#000014","#000025","#00002a","#000032",
"#00003e","#000042","#00004a","#00004f","#010055",
"#010057","#02005c","#03005e","#040063","#050065",
"#070069","#08006b","#0a0070","#0b0073","#0d0075",
"#100078","#120079","#15007c","#17007d","#1b0080",
"#1c0081","#200084","#220085","#260087","#280089",
"#2c008a","#2e008b","#32008d","#34008e","#38008f",
"#3c0092","#3e0093","#410094","#420095","#450096",    
"#470096","#4a0096","#4c0097","#4f0097","#510097",
"#540098","#560098","#5a0099","#5c0099","#5f009a",
"#64009b","#66009b","#6a009b","#6c009c","#6f009c",
"#70009c","#73009d","#75009d","#78009d","#7a009d",
"#7e009d","#7f009d","#83009d","#84009d","#87009d",
"#8b009d","#8d009d","#91009c","#93009c","#96009b",
"#98009b","#9b009b","#9c009b","#9f009b","#a0009b",
"#a3009b","#a4009b","#a7009a","#a8009a","#aa0099",
"#ae0198","#af0198","#b00198","#b10197","#b30196",
"#b40296","#b60295","#b70395","#b90495","#ba0495",
"#bb0593","#bc0593","#be0692","#bf0692","#c00791",
"#c10990","#c20a8f","#c30b8e","#c40c8d","#c60d8b",
"#c60e8a","#c81088","#c91187","#ca1385","#cb1385",
"#cc1582","#cd1681","#ce187e","#cf187c","#d01a79",
"#d21c75","#d21d74","#d32071","#d4216f","#d5236b",
"#d52469","#d72665","#d82764","#d92a60","#da2b5e",
"#db2e5a","#db2f57","#dd3051","#dd314e","#de3347",
"#df363d","#e0373a","#e03933","#e13a30","#e23c2a",
"#e33d26","#e43f20","#e4411d","#e5431b","#e54419",
"#e64616","#e74715","#e74913","#e84a12","#e84c0f",
"#ea4e0c","#ea4f0c","#eb510a","#eb520a","#ec5409",
"#ec5608","#ec5808","#ed5907","#ed5b06","#ee5c06",
"#ee5d05","#ee5e05","#ef6004","#ef6104","#f06303",
"#f16603","#f16603","#f16803","#f16902","#f16b02",
"#f16b02","#f26d01","#f26e01","#f37001","#f37101",
"#f47300","#f47400","#f47600","#f47700","#f47a00",
"#f57e00","#f57f00","#f68100","#f68200","#f78400",
"#f78500","#f88700","#f88800","#f88900","#f88a00",
"#f88c00","#f98d00","#f98e00","#f98f00","#f99100",
"#fa9400","#fa9500","#fb9800","#fb9900","#fb9c00",
"#fc9d00","#fca000","#fca100","#fda300","#fda400",
"#fda700","#fda800","#fdab00","#fdac00","#fdae00",
"#feb100","#feb200","#feb400","#feb500","#feb800",
"#feb900","#feba00","#febb00","#febd00","#febe00",
"#fec100","#fec200","#fec400","#fec500","#fec700",
"#feca01","#feca01","#fecc02","#fecd02","#fecf04",
"#fecf04","#fed106","#fed308","#fed50a","#fed60a",
"#fed80c","#fed90d","#ffda0e","#ffdb10","#ffdc14",
"#ffde1b","#ffdf1e","#ffe122","#ffe224","#ffe328",
"#ffe42b","#ffe531","#ffe635","#ffe73c","#ffe83f",
"#ffea46","#ffeb49","#ffec50","#ffed54","#ffee5b",
"#ffef67","#fff06a","#fff172","#fff177","#fff280",
"#fff285","#fff38e","#fff492","#fff49a","#fff59e",
"#fff5a6","#fff6aa","#fff7b3","#fff7b6","#fff8bd",
"#fff9c7","#fff9ca","#fffad1","#fffad4","#fffcdb",
"#fffcdf","#fffde5","#fffde8","#fffeee","#fffef1"
};

float arrayMin(float* flarray, int size) {
  float min = 1000.0;
  for (int i = 0; i < size; i++) {
    if (flarray[i] < min) min = flarray[i];
  }
  return(min);
}

float arrayMax(float* flarray, int size) {
  float max = -1000.0;
  for (int i = 0; i < size; i++) {
    if (flarray[i] > max) max = flarray[i];
  }
  return(max);
}

void setup() {
  /* Start the serial monitor. */
  Serial.begin(115200);
  while (!Serial);
  Serial.println("LoRa Receiver");

  /* Prepare the LoRa interface. */
  LoRa_init();

  /* WiFi set-up. */
  WiFi_init();

  /* MQTT set-up. */
  MQTT_init();
}

void loop() {
  /* Get and process the any packets. */
  int packetSize = LoRa.parsePacket();
  if (packetSize > 0) {
    Serial.print("Received packet '");
    while (LoRa.available()) {
      String LoRaData = LoRa.readString();
      Serial.print(LoRaData);
      message = LoRaData;
    }
    int rssi = LoRa.packetRssi();
    Serial.print("' with RSSI ");
    Serial.println(rssi);
    if (rssi > -50) {
      MQTT_publish(message);
      processMessage(message);
    }
  }
  WiFi_client();
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

/* WiFi functions. */
void WiFi_init()
{
  /* Connect to Wi-Fi network with SSID and password. */
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  /* Print local IP address and start web server. */
  Serial.println("");
  Serial.println("WiFi connected.");
  printWifiStatus();
  server.begin();
}

void printWifiStatus() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void WiFi_client()
{
  unsigned long currentTime_ms = 0;
  unsigned long startTime_ms = 0; 
  const long timeoutTime_ms = 500;
  String header;                            // Variable to store the HTTP request

  WiFiClient client = server.available();   // Listen for incoming requests.

  if (client) {
    Serial.println("New request received:");
    startTime_ms = millis();
    currentTime_ms = startTime_ms;
    String currentLine = "";
    while (client.connected() && (currentTime_ms - startTime_ms <= timeoutTime_ms)) {
      currentTime_ms = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\"></head>");
            // Webpage content.
            client.println("<body><h1>MASI Honey Bees MQTT Lora Gateway Webserver</h1>");
            // Display current state, and ON/OFF buttons for GPIO 26  
            client.print("<p>Current data:");

            /* The temp etc. */
            client.print("<br/>Time = ");
            client.print(message_time);
            //client.println((long)data["time"]);
            client.print("<br/>Temperature = ");
            client.print(temperature);
            //client.println((double)data["temperature"]);
            client.print("<br/>Pressure = ");
            client.print(pressure);
            //client.println((double)data["pressure"]);
            client.print("<br/>Humidity = ");
            client.print(humidity);
            //client.println((double)data["humidity"]);
            client.print("<br/>Colour = ");
            client.print(colour);
            //client.println((String)data["colour"]);
            client.print("<br/>Thermal =<br/>");
            client.print("<table style=\"border-spacing:0;border:0;\">");
            float min = arrayMin((float *)data, 768);
            float max = arrayMax((float *)data, 768);
            float inc = (max - min) / 255;
            Serial.print("Thermal data range: ");
            Serial.print(min);
            Serial.print(" - ");
            Serial.print(max);
            Serial.print(" with inc ");
            Serial.println(inc);
            for(int row = 0; row < 24; row++) {
              client.print("<tr>");
              for(int i = 0; i < 32; i++) {
                client.print("<td style=\"margin:0;padding:0;background-color:");
                byte scale = (data[row][i] - min) / inc;
                //char sscale[3];
                client.print(colour_table[scale]);
                //client.print(sscale);client.print(sscale);client.print(sscale);
                //Serial.print("Scale: ");
                //Serial.println(sscale);
                /*if(data[row][i] < 20.0) client.print("blue");
                else if (data[row][i] < 22.0) client.print("cyan");
                else if (data[row][i] < 24.0) client.print("green");
                else if (data[row][i] < 26.0) client.print("yellow");
                else if (data[row][i] < 28.0) client.print("orange");
                else client.print("red");*/
                client.print(";\">");
                client.print("&nbsp;&nbsp;");
                // client.print(data[row][i]);
                client.print("</td>");
              }
              //client.print("<td>");
              //client.print(data[row][31]);
              client.print("</tr>");
            }
            client.println("</table>");

            /* client.print(message); */
            client.println("</p>");               
            client.println("</body></html>");
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop.
            Serial.println(header);
            Serial.println("Webpage response sent.");
            break;
          } else { // If there's a newline, then clear currentLine.
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine.
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Request connection closed.");
  }
}

/* MQTT functions. */
void MQTT_init()
{
  MQTT_client.begin(MQTT_broker, net);
  MQTT_client.onMessage(MQTT_messageReceived);
  MQTT_client.setTimeout(2000);

  MQTT_connect();
}

void MQTT_connect()
{
  Serial.print("Checking WiFi");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("[OK]");
  Serial.print("MQTT connecting");
  while(!MQTT_client.connect("honeybees", "honeybees", "n31f1l3")) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("[OK]");
  MQTT_client.subscribe("/apiary");
}

void MQTT_messageReceived(String &topic, String &payload)
{
  Serial.println("MQTT incoming: " + topic + " --- " + payload);
}

void MQTT_publish(String message)
{
  MQTT_client.loop();
  if (!MQTT_client.connected()) {
    MQTT_connect();
  }
  Serial.println("Publishing: " + topic + " - " + message);
  MQTT_client.publish(topic, message);
  MQTT_client.disconnect();
}

void processMessage(String message) {
  DeserializationError jsonError = deserializeJson(jsonDoc, message);
  if(jsonError) {
    Serial.println("deserializeJson() failed");
    Serial.println(jsonError.c_str());
    //return false;
  } else {
    if (jsonDoc.size() > 2) {
      //Serial.print("Received a long message, ");
      //Serial.println(jsonDoc.size());
      message_time = (long)jsonDoc["time"];
      temperature = (double)jsonDoc["temperature"];
      pressure = (double)jsonDoc["pressure"];
      humidity = (double)jsonDoc["humidity"];
      colour = (String)jsonDoc["colour"];
    } else {
      //Serial.print("Received a short message, ");
      //Serial.println(jsonDoc.size());
      for (int i = 0; i < 32; i++) {
        data[(int)(jsonDoc["row"])][i] = (float)(jsonDoc["thermal"])[i];
      }
    }
  }
}