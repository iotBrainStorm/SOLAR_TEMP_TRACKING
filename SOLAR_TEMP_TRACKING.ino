#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

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