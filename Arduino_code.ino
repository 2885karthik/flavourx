#include <SoftwareSerial.h>
#include <math.h>

// ===== GUVA UV Sensor + UV LED =====
const int uvSensorPin = A1;   // GUVA analog output
const int uvLedPin    = 9;    // UV LED control pin
float I0 = 0;                 // baseline reference intensity (clear water)

// ===== pH Sensor =====
const int PIN_PH = A0;        // pH analog pin
const float VREF = 5;         // Arduino UNO ADC reference (5V)
const int ADC_MAX = 1023;

const float PH_LOW  = 4.01;   // calibration point 1
const float PH_HIGH = 9.18;   // calibration point 2
const float V4 = 0.936;       // Voltage at pH 4.01 buffer
const float V9 = 1.587;       // Voltage at pH 9.18 buffer

float PH_A = 0.0;  // slope
float PH_B = 0.0;  // intercept

// ===== Serial to ESP32 =====
SoftwareSerial espSerial(2, 3);

// ===== Utility functions =====
float measureUVIntensity(int samples) {
  digitalWrite(uvLedPin, HIGH);   // turn on UV LED
  delay(500);

  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(uvSensorPin);
    delay(10);
  }
  digitalWrite(uvLedPin, LOW);

  return (float)sum / samples;
}

float readVoltage(int pin, int samples = 16) {
  unsigned long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  float adc = (float)sum / samples;
  return (adc / ADC_MAX) * VREF;
}

void setup() {
  Serial.begin(9600);
  espSerial.begin(9600);

  pinMode(uvLedPin, OUTPUT);
  digitalWrite(uvLedPin, LOW);
  delay(1000);

  // UV baseline measurement (clear water)
  I0 = measureUVIntensity(20); // increase samples for stability
  Serial.print("Baseline I0 (clear water) = ");
  Serial.println(I0);

  // pH calibration
  PH_A = (PH_HIGH - PH_LOW) / (V9 - V4);
  PH_B = PH_LOW - PH_A * V4;
  Serial.println("pH Sensor Calibration Complete:");
  Serial.print("Slope (PH_A): "); Serial.println(PH_A, 4);
  Serial.print("Intercept (PH_B): "); Serial.println(PH_B, 4);
}

void loop() {
  // ===== UV Absorbance for suspended solution =====
  float I = measureUVIntensity(20);  // measure multiple times
  float absorbance = 0;

  if (I > 0 && I0 > 0) {
    absorbance = -log10(I / I0);  // Beer-Lambert Law
  }

  // Correct negative or very low readings
 

  // ===== pH Measurement =====
  float V = readVoltage(PIN_PH);
  float pH = PH_A * V + PH_B;

  // ===== Print to Serial Monitor =====
  Serial.print("UV Intensity: ");
  Serial.print(I);
  Serial.print(" | Absorbance: ");
  Serial.print(absorbance, 3);
  Serial.print(" || pH Voltage: ");
  Serial.print(V, 3);
  Serial.print(" V | pH: ");
  Serial.println(pH, 2);

  // ===== Send to ESP32 =====
  espSerial.print("UV:");
  espSerial.print(absorbance, 3);
  espSerial.print(",pH:");
  espSerial.println(pH, 2);

  delay(2000);
}
