🌞 Solar Panel Smart Monitoring System (ESP32 Based)

Real-time Solar Monitoring + Web Dashboard + Configurable Node-RED Integration
Built with ESP32, Async Web Server, and Advanced Sensor Monitoring

📸 Project Preview
🖥 OLED Display Output

🌐 Web Dashboard (Real-Time)

⚙ Configuration Page

🚀 Features

✅ Real-Time Solar Panel Monitoring
✅ Temperature Monitoring (NTC + AHT)
✅ Ambient Light (LUX) Monitoring
✅ WiFi Signal Strength (% Based)
✅ Node-RED Data Sharing (Configurable Interval)
✅ Web-Based Configuration Page
✅ Fully Async Web Server (Non-Blocking)
✅ OLED Live Display with Smart UI
✅ WiFi Signal Bars + Status Icons
✅ Sunlight Progress Bar
✅ LUX Increasing/Decreasing Indicator
✅ 12H / 24H Time Format Support
✅ EEPROM Settings Storage
✅ Industrial-Ready Architecture

📊 Parameters Monitored
Parameter Sensor Used Purpose
Panel Temperature NTC Thermistor Monitor panel heating
Ambient Temperature AHT Sensor Weather condition
Humidity AHT Sensor Environmental condition
Sunlight Intensity BH1750 LUX measurement
WiFi Strength Internal RSSI Network stability
Node-RED Status HTTP Check Data sharing health
🛠 Hardware Used

ESP32 Development Board

128x64 OLED Display (I2C)

NTC Thermistor

AHT Temperature & Humidity Sensor

BH1750 LUX Sensor

Voltage Divider Circuit (for NTC)

Stable 5V/3.3V Power Supply

---

## 🔌 Pin Configuration (Example)

| Device     | ESP32 Pin |
| ---------- | --------- |
| OLED SDA   | GPIO 21   |
| OLED SCL   | GPIO 22   |
| BH1750 SDA | GPIO 21   |
| BH1750 SCL | GPIO 22   |
| AHT SDA    | GPIO 21   |
| AHT SCL    | GPIO 22   |
| NTC Analog | GPIO 34   |

> _Modify according to your wiring_

🌐 Web Interface
🖥 Dashboard

Real-time sensor values

WiFi strength indicator

Sunlight progress visualization

Node-RED heartbeat status

LUX trend indicator (+/-)

⚙ Configuration Page

WiFi Settings

Reading Intervals (Temperature / NTC / LUX)

Node-RED Enable / Disable

Node-RED Data Share Interval

Time Format Selection (12H / 24H)

EEPROM Save

🔄 Node-RED Integration

Configurable Data Share Interval

HTTP POST based data sending

Timeout protection

Network failure safe handling

🖼 Circuit Diagram

📌 (Add your circuit diagram image here)

Example:

![Circuit Diagram](circuit.png)

If you want, I can generate a clean professional circuit diagram layout for you.

🎥 YouTube Demo

📺 Watch Full Working Demo Here:

👉 [Your YouTube Video Link Here]

🧠 System Architecture

Non-blocking Async Web Server

millis() based timing system

EEPROM persistent settings

Modular sensor handling

WiFi auto reconnect logic

Scalable design

📂 Project Structure
/data
index.html
config.html
dashboard assets
/src
main.ino
README.md
display.png
dashboard.png
config.png
🔐 Stability & Safety

HTTP timeout protection

WiFi reconnect logic

Memory-efficient design

Long runtime tested

📈 Future Improvements

Watchdog protection

OTA Firmware Update

Dust Monitoring Sensor

Cloud Backup Integration

Deep Sleep Power Mode

Data Logging to SD Card

👨‍💻 Developed By

Mrinal Maity
ESP32 Solar Monitoring System
Made with dedication and engineering passion ❤️

⭐ Support

If you like this project:

⭐ Star the repository

🍴 Fork it

📢 Share it

---

## 💎 Extra Professional Touch (Optional Additions)

- GitHub badges (ESP32, Arduino, License)
- Animated GIF demo
- Block diagram
- Feature comparison table
- Version changelog
- License section
