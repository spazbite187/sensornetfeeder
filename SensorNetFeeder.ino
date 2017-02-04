/*
    SensorNet Feeder

    Program to control ESP8266 as a sensor data collector. Device connects
    to configured WiFi network, then caputures WiFi signal strength data
    and temp data and sends as json to an end point.
*/

#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS D4              // Data wire for temp sensor data line
OneWire oneWire(ONE_WIRE_BUS);       // Setup a oneWire instance to communicate
DallasTemperature sensors(&oneWire); // Pass our oneWire reference to temp sensor

const char* SSID     = "";
const char* PASSWD = "";
const String SEND_TO_URL = "http://192.168.81.1/api/v1/sensor/";
const int PAYLOAD_SIZE = 256;
const int LOOP_DELAY = 60000;  // 60 seconds, or 1 minute

HTTPClient http;
String ssidStr = String(SSID);
String host = "";
String ipAddress = "";

void setup() {  
  Serial.begin(115200);
  delay(10);
  sensors.begin(); // Setup Temp Sensor
  
  // Set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(50);

  Serial.println("\nConnecting to "+ ssidStr);
  WiFi.begin(SSID, PASSWD);   // start by connecting to a WiFi network

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // setup Wifi info
  host = String(ESP.getChipId(), HEX);
  WiFi.hostname(host);
  ipAddress = IPAddress2String(WiFi.localIP());

  Serial.println("\nWiFi connected to " + ssidStr);
  Serial.println("IP address: " + ipAddress);
}

void loop() {
  char jsonData[PAYLOAD_SIZE];

  genJSON(jsonData);
  sendData(jsonData);

  delay(LOOP_DELAY);
}

void sendData(char* data) {
        http.begin(SEND_TO_URL + host); 
        int httpCode = http.sendRequest("PUT", data);
        if(httpCode > 0) {
                if(httpCode != HTTP_CODE_OK) {
                        Serial.printf("http failed - error: %s\n", http.errorToString(httpCode).c_str());
                }
        } else {
                Serial.printf("http failed - error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
}

void genJSON(char* data) {
        // get uptime
        int sec = millis() / 1000;
        String secStr = String(sec) + "s";
        // get wifi signal
        int wifiSignal = WiFi.RSSI();
        // Get temp data
        float tempC = 0;
        sensors.requestTemperatures(); // Send the command to get temperatures
        tempC = sensors.getTempCByIndex(0);

        // Generate json object
        StaticJsonBuffer<200> jsonBuffer;
        JsonObject& root = jsonBuffer.createObject();
        // Add values in the object
        root["uptime"] = secStr;
        root["ip"] = ipAddress;
        root["ssid"] = ssidStr;
        root["signal"] = wifiSignal;
        root["temp"] = tempC;
        
        char buffer[PAYLOAD_SIZE];        
        root.printTo(buffer, sizeof(buffer));
        strcpy(data, buffer); // copy created json data to provided char array
}

String IPAddress2String(IPAddress address)
{
 return String(address[0]) + "." + 
        String(address[1]) + "." + 
        String(address[2]) + "." + 
        String(address[3]);
}
