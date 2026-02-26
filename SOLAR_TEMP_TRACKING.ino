#include <Arduino.h>
#include <math.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <ESPAsyncWebServer.h>
#include <HTTPClient.h> // Node-Red
#include <Wire.h>       // I2C communication
#include <SPI.h>
#include <EEPROM.h>         // Settings storage
#include <SPIFFS.h>         // File system
#include <HardwareSerial.h> // Serial communication
#include "time.h"           // Time management
#include <U8g2lib.h>        // OLED display
#include <ArduinoJson.h>    // make json format
#include <Preferences.h>
#include <Adafruit_AHT10.h>

#define NTC_PIN 34             // Your ADC pin
#define FIXED_RESISTOR 10000.0 // 10k fixed resistor
#define R0 1000.0              // NTC resistance at 25°C (approx 1k from your measurement)
#define BETA 3950.0            // Try 3950 first (common value)
#define T0 298.15              // 25°C in Kelvin
#define OFFSET 17.29           // Adjust later (+ or -)

// ADC settings
#define ADC_RESOLUTION 4095.0
#define VREF 3.3

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

Adafruit_AHT10 aht;

// -- Sensor Values Storage
float ntcTemp = 0.0;
float ahtTemp = 0.0;
float humidity = 0.0;

// -- Welcome Message
const char MSG_WELCOME[] PROGMEM = "ESP";
const char MSG_SUBTITLE[] PROGMEM = "SOLAR - TEM";
const char MSG_DEVELOPER[] PROGMEM = "developed by M.Maity";

// -- Time Management
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;

//////////////////////   WIFI SETUP   //////////////////////

bool connectToSavedWiFi()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 12, "Connecting WiFi...");
  u8g2.sendBuffer();

  WiFiManager wm;
  bool success = false;

  WiFi.mode(WIFI_STA);
  WiFi.begin(); // Try to connect to saved network

  int attempts = 0;
  const int MAX_ATTEMPTS = 5;

  while (attempts < MAX_ATTEMPTS)
  {

    char attemptStr[16];
    snprintf(attemptStr, 16, "Attempt: %d/5", attempts + 1);
    u8g2.drawStr(0, 24, attemptStr);
    u8g2.sendBuffer();

    if (WiFi.status() == WL_CONNECTED)
    {
      // Success - show connection details
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

  // If not connected, start WiFiManager config portal
  u8g2.clearBuffer();
  u8g2.drawStr(0, 12, "No Saved WiFi!");
  u8g2.drawStr(0, 24, "Starting AP...");
  u8g2.drawStr(0, 36, "AP IP:");
  u8g2.drawStr(0, 48, "192.168.4.1");
  u8g2.drawStr(0, 60, "Connect & Setup");
  u8g2.sendBuffer();
  delay(1500);

  wm.setConfigPortalTimeout(60);             // 60 seconds timeout
  success = wm.autoConnect("Solar Weather"); // Start AP

  if (success)
  {
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
  else
  {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Time over!");
    u8g2.sendBuffer();
    delay(1000);
    return true;
  }
}

//////////////////////   TIME SETUP   //////////////////////

void configDateTime()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    u8g2.drawStr(0, 24, "No WiFi!");
    u8g2.sendBuffer();
    delay(1000);
    u8g2.drawStr(0, 36, "Time not synced!");
    u8g2.sendBuffer();
    delay(1000);
    u8g2.drawStr(0, 48, "Starting offline...");
    u8g2.sendBuffer();
    delay(1000);

    // ✅ Set default time: 12:00, 01/01/2025
    struct tm tm;
    tm.tm_year = 2025 - 1900; // years since 1900
    tm.tm_mon = 0;            // January (0-based)
    tm.tm_mday = 1;           // Day
    tm.tm_hour = 12;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    time_t t = mktime(&tm);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, nullptr);

    return;
  }

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.drawStr(0, 12, "Syncing Time...");
  u8g2.sendBuffer();

  delay(1000);
  int attempts = 0;
  const int MAX_ATTEMPTS = 5;
  struct tm timeinfo;

  while (attempts < MAX_ATTEMPTS)
  {
    char attemptStr[16];
    snprintf(attemptStr, 16, "Attempt: %d/5", attempts + 1);
    u8g2.drawStr(0, 24, attemptStr);
    u8g2.sendBuffer();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(2000);
    if (getLocalTime(&timeinfo))
    {
      break;
    }
    attempts++;
  }

  if (getLocalTime(&timeinfo))
  {
    char timeStr[16];
    char dateStr[18];
    char gmtStr[30];
    strftime(timeStr, sizeof(timeStr), "Time:  %H:%M:%S", &timeinfo);
    strftime(dateStr, sizeof(dateStr), "Date:  %d.%m.%Y", &timeinfo);
    strftime(gmtStr, sizeof(gmtStr), "GMT :  %z %Z", &timeinfo);

    u8g2.clearBuffer();
    // u8g2.drawStr(0, 12, "Time Updated!");
    u8g2.setFont(u8g2_font_t0_14_tr);
    u8g2.drawStr(0, 16, timeStr);
    u8g2.drawStr(0, 32, dateStr);
    u8g2.drawStr(0, 62, gmtStr);
    u8g2.sendBuffer();
    delay(2000);
    // return;
  }
  else
  {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "Time not synced!");
    u8g2.drawStr(0, 24, "Check internet!");
    u8g2.drawStr(0, 48, "Starting offline...");
    u8g2.sendBuffer();
    delay(2000);

    // ✅ Apply default time if NTP fails
    struct tm tm;
    tm.tm_year = 2025 - 1900; // years since 1900
    tm.tm_mon = 0;            // January
    tm.tm_mday = 1;           // Day
    tm.tm_hour = 12;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    time_t t = mktime(&tm);
    struct timeval now = {.tv_sec = t};
    settimeofday(&now, nullptr);
  }
}

//////////////////////   CENTRE TEXT   //////////////////////

void drawCenteredStr(int y, const char *str, const uint8_t *font)
{
  u8g2.setFont(font);
  int16_t strWidth = u8g2.getStrWidth(str);
  int16_t x = (128 - strWidth) / 2;
  u8g2.drawStr(x, y, str);
}

//////////////////////   WELCOME MESSAGE   //////////////////////

void welcomeMsg()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB18_tr);
  u8g2.drawStr(0, 22, MSG_WELCOME);
  u8g2.setFont(u8g2_font_ncenR12_tr);
  u8g2.drawStr(0, 40, MSG_SUBTITLE);
  u8g2.setFont(u8g2_font_t0_11_tr);
  u8g2.drawStr(2, 60, MSG_DEVELOPER);
  u8g2.sendBuffer();
}

//////////////////////   CALCULATE SENSOR VALUES   //////////////////////

void calculateSensorValues()
{
  // ----- Average 50 NTC samples -----
  float adcSum = 0;
  for (int i = 0; i < 50; i++)
  {
    adcSum += analogRead(NTC_PIN);
    delay(5);
  }
  float adcValue = adcSum / 50.0;

  // ----- Convert ADC to voltage -----
  float voltage = adcValue * VREF / ADC_RESOLUTION;

  // ----- Calculate NTC resistance -----
  float rNTC = FIXED_RESISTOR * (VREF - voltage) / voltage;

  // ----- Beta formula -----
  float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(rNTC / R0));
  ntcTemp = tempK - 273.15;

  // ----- Add offset calibration -----
  ntcTemp = ntcTemp + OFFSET;

  // ----- Read AHT10 sensor -----
  sensors_event_t humidityEvent, tempEvent;
  aht.getEvent(&humidityEvent, &tempEvent);
  ahtTemp = tempEvent.temperature;
  humidity = humidityEvent.relative_humidity;
}

//////////////////////   DISPLAY SENSOR VALUES   //////////////////////

void displaySensorValues()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);

  // Display NTC Temperature
  char ntcStr[20];
  snprintf(ntcStr, sizeof(ntcStr), "NTC: %.2f C", ntcTemp);
  u8g2.drawStr(0, 12, ntcStr);

  // Display AHT Temperature
  char ahtTempStr[20];
  snprintf(ahtTempStr, sizeof(ahtTempStr), "AHT: %.2f C", ahtTemp);
  u8g2.drawStr(0, 24, ahtTempStr);

  // Display Humidity
  char humidityStr[20];
  snprintf(humidityStr, sizeof(humidityStr), "Humidity: %.2f%%", humidity);
  u8g2.drawStr(0, 36, humidityStr);

  u8g2.sendBuffer();
}

//////////////////////   SETUP   //////////////////////

void setup()
{
  Serial.begin(115200);
  pinMode(NTC_PIN, INPUT);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db); // Full 0–3.3V range

  u8g2.begin();

  // --- Welcome ---
  welcomeMsg();
  delay(2000);

  // --- Storage Init ---
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_t0_14_tr);
  u8g2.drawStr(0, 18, "Settings Init:");
  u8g2.sendBuffer();
  delay(500);
  EEPROM.begin(512);

  // loadSettings();
  // u8g2.drawStr(0, 18, "Settings Init: OK");
  // u8g2.sendBuffer();

  // --- SPIFFS ---
  u8g2.clearBuffer();
  if (!SPIFFS.begin(true))
  {
    u8g2.drawStr(0, 18, "SPIFFS: ERROR");
  }
  else
  {
    u8g2.drawStr(0, 18, "SPIFFS: OK");
  }
  u8g2.sendBuffer();
  delay(1000);

  // --- Sensor Init ---
  u8g2.clearBuffer();
  if (!aht.begin())
  {
    u8g2.drawStr(0, 18, "AHT10 Sensor: ERROR");
  }
  else
  {
    u8g2.drawStr(0, 18, "AHT10 Sensor: OK");
  }
  u8g2.sendBuffer();
  delay(1000);

  // --- WiFi Setup ---
  connectToSavedWiFi();
  delay(500);

  // --- Time Sync ---
  configDateTime();
  delay(1500);

  // --- Ready ---
  u8g2.clearBuffer();
  drawCenteredStr(35, "System Ready!", u8g2_font_t0_14_tr);
  u8g2.sendBuffer();
  delay(1500);

  u8g2.clearBuffer();
}

void loop()
{
  // Calculate and store sensor values
  calculateSensorValues();

  // Display sensor values on LCD
  displaySensorValues();

  // Print to serial for debugging
  Serial.print("NTC Temp: ");
  Serial.print(ntcTemp, 2);
  Serial.print(" °C   |   AHT10 Temp: ");
  Serial.print(ahtTemp, 2);
  Serial.print(" °C   |   Humidity: ");
  Serial.print(humidity, 2);
  Serial.println("%");

  delay(1000);
}