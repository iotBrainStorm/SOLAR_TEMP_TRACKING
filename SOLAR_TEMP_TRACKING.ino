#include <Arduino.h>
#include <math.h>               // To calculate NTC
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>  // For web server
#include <HTTPClient.h>         // Node-Red
#include <Wire.h>               // I2C communication
#include <SPI.h>
#include <EEPROM.h>             // Settings storage
#include <SPIFFS.h>             // File system
#include <HardwareSerial.h>     // Serial communication
#include "time.h"               // Time management
#include <U8g2lib.h>            // OLED display
#include <ArduinoJson.h>        // make json format
#include <Preferences.h>        // To store users' settings
#include <Adafruit_AHT10.h>     // For temperature, humidity
#include <BH1750.h>             // For LUX measurement of sunlight

const char* ssid = "MRINAL";
const char* password = "***mrinal***";

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);

  // 🔴 MUST mount SPIFFS FIRST
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
  Serial.println("SPIFFS Mounted");
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    Serial.println(file.name());
    file = root.openNextFile();
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nConnected!");
  Serial.println(WiFi.localIP());

  // Serve all static files
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    // Serial.println("Root Requested");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/config.html", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/config.html", "text/html");
  });

  server.on("/dashboard.svg", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/dashboard.svg", "image/svg+xml");
  });

  server.on("/settings.svg", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/settings.svg", "image/svg+xml");
  });

  server.begin();
  Serial.println("Server Started");
}

void loop() {}