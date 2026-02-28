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

//////////////////////   TIME SETUP   //////////////////////

void configDateTime() {

  Serial.println("\n==============================");
  Serial.println("Date & Time Configuration");
  Serial.println("==============================");

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println("[WARNING] WiFi not connected!");
    Serial.println("[INFO] Running in offline mode.");
    Serial.println("[INFO] Setting default time: 01/01/2025 12:00:00");

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawStr(0, 24, "No WiFi!");
    u8g2.drawStr(0, 36, "Time not synced!");
    u8g2.drawStr(0, 48, "Starting offline...");
    u8g2.sendBuffer();

    unsigned long start = millis();
    while (millis() - start < 2000) yield();

    struct tm tm;
    tm.tm_year = 2025 - 1900;
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    tm.tm_hour = 12;
    tm.tm_min = 0;
    tm.tm_sec = 0;

    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, nullptr);

    Serial.println("[SUCCESS] Default time applied.");
    Serial.println("==============================\n");
    return;
  }

  Serial.println("[INFO] WiFi connected.");
  Serial.println("[INFO] Starting NTP sync...");

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 12, "Syncing Time...");
  u8g2.sendBuffer();

  unsigned long start = millis();
  while (millis() - start < 1000) yield();

  int attempts = 0;
  const int MAX_ATTEMPTS = 5;
  struct tm timeinfo;

  while (attempts < MAX_ATTEMPTS) {

    Serial.printf("[INFO] NTP Attempt %d/%d\n", attempts + 1, MAX_ATTEMPTS);

    char attemptStr[16];
    snprintf(attemptStr, sizeof(attemptStr), "Attempt: %d/5", attempts + 1);

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Syncing Time...");
    u8g2.drawStr(0, 24, attemptStr);
    u8g2.sendBuffer();

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    start = millis();
    while (millis() - start < 1000) yield();

    if (getLocalTime(&timeinfo)) {
      Serial.println("[SUCCESS] Time synced from NTP server.");
      break;
    }

    attempts++;
  }

  if (getLocalTime(&timeinfo)) {

    char timeStr[16];
    char dateStr[18];
    char gmtStr[30];

    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    strftime(dateStr, sizeof(dateStr), "%d.%m.%Y", &timeinfo);
    strftime(gmtStr, sizeof(gmtStr), "%z %Z", &timeinfo);

    Serial.println("-------- Current Time --------");
    Serial.printf("Time : %s\n", timeStr);
    Serial.printf("Date : %s\n", dateStr);
    Serial.printf("Zone : %s\n", gmtStr);
    Serial.println("------------------------------");
    Serial.println("==============================\n");

    strftime(timeStr, sizeof(timeStr), "Time: %H:%M:%S", &timeinfo);
    strftime(dateStr, sizeof(dateStr), "Date: %d.%m.%Y", &timeinfo);
    strftime(gmtStr, sizeof(gmtStr), "GMT: %z %Z", &timeinfo);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_t0_14_tr);
    u8g2.drawStr(0, 16, timeStr);
    u8g2.drawStr(0, 32, dateStr);
    u8g2.drawStr(0, 62, gmtStr);
    u8g2.sendBuffer();

    start = millis();
    while (millis() - start < 2000) yield();

  } else {

    Serial.println("[ERROR] NTP sync failed!");
    Serial.println("[INFO] Applying default offline time.");
    Serial.println("[INFO] Default: 01/01/2025 12:00:00");

    struct tm tm;
    tm.tm_year = 2025 - 1900;
    tm.tm_mon = 0;
    tm.tm_mday = 1;
    tm.tm_hour = 12;
    tm.tm_min = 0;
    tm.tm_sec = 0;

    time_t t = mktime(&tm);
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, nullptr);

    Serial.println("[SUCCESS] Default time applied.");
    Serial.println("==============================\n");

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Time not synced!");
    u8g2.drawStr(0, 24, "Check internet!");
    u8g2.drawStr(0, 48, "Starting offline...");
    u8g2.sendBuffer();

    start = millis();
    while (millis() - start < 2000) yield();
  }
}

//////////////////////   SERVER SETUP   //////////////////////

void setupWebServer() {
  if (webServerStarted) return;  // Already started

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

  // server.on("/sensor.json", HTTP_GET, [](AsyncWebServerRequest* request) {
  //   portENTER_CRITICAL(&measureMux);
  //   Measurements current = readings;
  //   portEXIT_CRITICAL(&measureMux);

  //   StaticJsonDocument<512> doc;
  //   doc["voltage"] = String(current.voltage, 1);
  //   doc["current"] = String(current.current, 2);
  //   doc["power"] = String(current.power, 1);
  //   doc["energy"] = String(current.energy, 2);
  //   doc["frequency"] = String(current.frequency, 2);
  //   doc["pf"] = String(current.pf, 2);
  //   doc["uptime"] = String(current.uptime, 1);
  //   doc["days"] = String(current.totalDays, 1);
  //   doc["mainOutput"] = settings.mainOutput ? 1 : 0;

  //   String response;
  //   serializeJson(doc, response);
  //   request->send(200, "application/json", response);
  // });

  // // Main Output Control Route - SET new state
  // server.on(
  //   "/output/set", HTTP_POST, [](AsyncWebServerRequest* request) {}, NULL,
  //   [](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
  //     StaticJsonDocument<200> doc;
  //     DeserializationError error = deserializeJson(doc, (const char*)data);

  //     if (!error) {
  //       bool state = doc["state"];
  //       bool previousState = settings.mainOutput;

  //       settings.mainOutput = state;
  //       saveSettings();

  //       // Control relay pin
  //       digitalWrite(MAIN_RELAY, state ? LOW : HIGH);

  //       Serial.printf("Web Control: Output set to %s\n", state ? "ON" : "OFF");

  //       // Push to Firebase only on change
  //       if (fbSettings.enabled && WiFi.status() == WL_CONNECTED && previousState != state) {
  //         writeOutputToFirebase(state);
  //       }

  //       // Send success response
  //       StaticJsonDocument<100> response;
  //       response["success"] = true;
  //       response["mainOutput"] = state ? 1 : 0;

  //       String jsonResponse;
  //       serializeJson(response, jsonResponse);
  //       request->send(200, "application/json", jsonResponse);
  //     } else {
  //       request->send(400, "application/json", "{\"success\":false,\"error\":\"Invalid JSON\"}");
  //     }
  //   });

  server.begin();
  webServerStarted = true;
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

  // --- Time Setup ---
  configDateTime();
  delay(500);


  // --- Server Setup ---
  if (WiFi.status() == WL_CONNECTED) {

    Serial.println("\n==============================");
    Serial.println("Web Server Initialization");
    Serial.println("==============================");
    Serial.println("[INFO] WiFi Connected.");
    Serial.println("[INFO] Starting Web Server...");

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tf);
    u8g2.drawStr(0, 12, "Starting Server...");
    u8g2.sendBuffer();
    delay(300);

    setupWebServer();

    Serial.println("[SUCCESS] Web Server Started!");
    Serial.printf("SSID       : %s\n", WiFi.SSID().c_str());
    Serial.printf("Server IP  : %s\n", WiFi.localIP().toString().c_str());
    Serial.println("==============================\n");

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Server Started!");
    u8g2.drawStr(0, 24, "IP Address:");
    u8g2.drawStr(0, 36, WiFi.localIP().toString().c_str());
    u8g2.sendBuffer();
    delay(2000);

  } else {

    Serial.println("\n==============================");
    Serial.println("Web Server Initialization");
    Serial.println("==============================");
    Serial.println("[ERROR] WiFi not connected!");
    Serial.println("[INFO] Server will auto-start once WiFi reconnects.");
    Serial.println("==============================\n");

    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "WiFi Failed!");
    u8g2.drawStr(0, 24, "Will auto-start");
    u8g2.drawStr(0, 36, "when connected");
    u8g2.sendBuffer();
    delay(2000);
  }

}

void loop() {}