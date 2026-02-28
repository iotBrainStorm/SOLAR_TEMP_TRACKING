#include <Arduino.h>
#include <math.h>  // To calculate NTC
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>  // For web server
#include <HTTPClient.h>         // Node-Red
#include <Wire.h>               // I2C communication
#include <SPI.h>
#include <EEPROM.h>          // Settings storage
#include <SPIFFS.h>          // File system
#include <HardwareSerial.h>  // Serial communication
#include "time.h"            // Time management
#include <U8g2lib.h>         // OLED display
#include <ArduinoJson.h>     // make json format
#include <Preferences.h>     // To store users' settings
#include <Adafruit_AHT10.h>  // For temperature, humidity
#include <BH1750.h>          // For LUX measurement of sunlight

// -- NTC Setup
#define NTC_PIN 34              // ADC pin
#define FIXED_RESISTOR 10000.0  // 10k fixed resistor
#define R0 1000.0               // NTC resistance at 25°C
#define BETA 3950.0             // 3950 (common value)
#define T0 298.15               // 25°C in Kelvin
#define OFFSET 17.29            // Adjust later (+ or -)
#define ADC_RESOLUTION 4095.0   // 12 bits (1111 1111 1111)
#define VREF 3.3                // Maximum sensing voltage of ESP32

// -- LCD Setup
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

// -- Temperature and Humidity sensor setup
Adafruit_AHT10 aht;

// -- Sensor Values Storage
float ntcTemp = 0.0;
float ahtTemp = 0.0;
float humidity = 0.0;
uint32_t luxValue = 0;

// -- Time Management
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

// -- Web Server
AsyncWebServer server(80);
bool webServerStarted = false;

// -- Welcome Message
const char MSG_WELCOME[] PROGMEM = "ESP";
const char MSG_SUBTITLE[] PROGMEM = "SOLAR - TEM";
const char MSG_DEVELOPER[] PROGMEM = "developed by M.Maity";


//////////////////////   WIFI SETUP   //////////////////////

bool connectToSavedWiFi() {

  Serial.println("\n==============================");
  Serial.println("WiFi Connection Started");
  Serial.println("==============================");

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 12, "Connecting WiFi...");
  u8g2.sendBuffer();

  WiFiManager wm;
  bool success = false;

  WiFi.mode(WIFI_STA);
  WiFi.begin();

  int attempts = 0;
  const int MAX_ATTEMPTS = 5;

  while (attempts < MAX_ATTEMPTS) {

    char attemptStr[16];
    snprintf(attemptStr, sizeof(attemptStr), "Attempt: %d/5", attempts + 1);

    u8g2.drawStr(0, 24, attemptStr);
    u8g2.sendBuffer();

    Serial.printf("[INFO] %s\n", attemptStr);

    if (WiFi.status() == WL_CONNECTED) {

      Serial.println("\n[SUCCESS] Connected to Saved WiFi");
      Serial.printf("SSID       : %s\n", WiFi.SSID().c_str());
      Serial.printf("IP Address : %s\n", WiFi.localIP().toString().c_str());
      Serial.println("==============================\n");

      u8g2.clearBuffer();
      u8g2.drawStr(0, 12, "WiFi Connected!");
      u8g2.drawStr(0, 24, "SSID:");
      u8g2.drawStr(0, 36, WiFi.SSID().c_str());
      u8g2.drawStr(0, 48, "IP:");
      u8g2.drawStr(0, 60, WiFi.localIP().toString().c_str());
      u8g2.sendBuffer();

      delay(2000);
      return true;
    }

    delay(2000);
    attempts++;
  }

  // Failed to connect
  Serial.println("\n[WARNING] No saved WiFi found!");
  Serial.println("[INFO] Starting Config Portal...");
  Serial.println("AP SSID    : Solar Weather");
  Serial.println("AP IP      : 192.168.4.1");
  Serial.println("Timeout    : 60 seconds");
  Serial.println("------------------------------");

  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "No Saved WiFi!");
  u8g2.drawStr(0, 24, "Starting AP...");
  u8g2.drawStr(0, 36, "AP IP:");
  u8g2.drawStr(0, 48, "192.168.4.1");
  u8g2.drawStr(0, 60, "Connect & Setup");
  u8g2.sendBuffer();

  delay(1500);

  wm.setConfigPortalTimeout(60);
  success = wm.autoConnect("Solar Weather");

  if (success) {

    Serial.println("\n[SUCCESS] WiFi Connected via Config Portal");
    Serial.printf("SSID       : %s\n", WiFi.SSID().c_str());
    Serial.printf("IP Address : %s\n", WiFi.localIP().toString().c_str());
    Serial.println("==============================\n");

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "WiFi Connected!");
    u8g2.drawStr(0, 24, "SSID:");
    u8g2.drawStr(0, 36, WiFi.SSID().c_str());
    u8g2.drawStr(0, 48, "IP:");
    u8g2.drawStr(0, 60, WiFi.localIP().toString().c_str());
    u8g2.sendBuffer();

    delay(2000);
    return true;

  } else {

    Serial.println("\n[ERROR] Config Portal Timeout!");
    Serial.println("Device not connected to WiFi.");
    Serial.println("==============================\n");

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Time over!");
    u8g2.sendBuffer();

    delay(1000);
    return false;  // Better logic than returning true
  }
}

//////////////////////   WELCOME MESSAGE   //////////////////////

void welcomeMsg() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB18_tr);
  u8g2.drawStr(0, 22, MSG_WELCOME);
  u8g2.setFont(u8g2_font_ncenR12_tr);
  u8g2.drawStr(0, 40, MSG_SUBTITLE);
  u8g2.setFont(u8g2_font_t0_11_tr);
  u8g2.drawStr(2, 60, MSG_DEVELOPER);
  u8g2.sendBuffer();
}

void setup() {
  Serial.begin(115200);
  pinMode(NTC_PIN, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);  // Full 0–3.3V range

  u8g2.begin();

  // --- Welcome ---
  welcomeMsg();
  delay(2000);

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

  // --- WiFi Setup ---
  connectToSavedWiFi();
  delay(500);

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