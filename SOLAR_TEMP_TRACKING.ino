#include <math.h>
#include <Adafruit_AHT10.h>

#define NTC_PIN 34              // Your ADC pin
#define FIXED_RESISTOR 10000.0  // 10k fixed resistor
#define R0 1000.0               // NTC resistance at 25°C (approx 1k from your measurement)
#define BETA 3950.0             // Try 3950 first (common value)
#define T0 298.15               // 25°C in Kelvin
#define OFFSET 17.29            // Adjust later (+ or -)

// ADC settings
#define ADC_RESOLUTION 4095.0
#define VREF 3.3

Adafruit_AHT10 aht;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);  // Full 0–3.3V range
                                   // I2C setup
  Wire.begin(21, 22);

  if (!aht.begin()) {
    Serial.println("AHT10 not detected!");
    while (1)
      ;
  }

  Serial.println("NTC + AHT10 comparison started");
}

void loop() {

  // ----- Average 50 samples -----
  float adcSum = 0;
  for (int i = 0; i < 50; i++) {
    adcSum += analogRead(NTC_PIN);
    delay(5);
  }
  float adcValue = adcSum / 50.0;

  // ----- Convert ADC to voltage -----
  float voltage = adcValue * VREF / ADC_RESOLUTION;

  // ----- Calculate NTC resistance -----
  // For configuration: 3.3V -> NTC -> ADC -> 10k -> GND
  float rNTC = FIXED_RESISTOR * (VREF - voltage) / voltage;

  // ----- Beta formula -----
  float tempK = 1.0 / ((1.0 / T0) + (1.0 / BETA) * log(rNTC / R0));
  float tempC = tempK - 273.15;

  // ----- Add offset calibration -----
  tempC = tempC + OFFSET;

  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);

  float ahtTemp = temp.temperature;
  float ahtHumi = humidity.relative_humidity;


  Serial.print("Voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" V  |  Resistance: ");
  Serial.print(rNTC, 1);
  Serial.print(" ohm  |  Temperature: ");
  Serial.print(tempC, 2);
  Serial.print(" °C   |   AHT10 Temp: ");
  Serial.print(ahtTemp, 2);
  Serial.print(" °C   |   AHT10 Humi: ");
  Serial.print(ahtHumi, 2);
  Serial.println("%");

  delay(1000);
}