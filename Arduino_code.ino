#include <SoftwareSerial.h>
#include <math.h>


const int uvSensorPin = A1;   // GUVA analog output
const int uvLedPin    = 9;    // UV LED control pin
float I0 = 0;                 // baseline reference uv intensity 


const int PIN_PH = A0;        // pH analog pin
const float VREF = 5;         // Arduino UNO ADC reference 
const int ADC_MAX = 1023;

const float PH_LOW  = 4.01;   // calibration 
const float PH_HIGH = 9.18;   // calibration 
const float V4 = 0.936;       // Voltage at pH 4.01 
const float V9 = 1.587;       // Voltage at pH 9.18 

float PH_A = 0.0;  
float PH_B = 0.0;  


SoftwareSerial espSerial(2, 3);


float measureUVIntensity(int samples) {
  digitalWrite(uvLedPin, HIGH);   
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

  
  I0 = measureUVIntensity(20); 
  Serial.print("Baseline I0 (clear water) = ");
  Serial.println(I0);

  
  PH_A = (PH_HIGH - PH_LOW) / (V9 - V4);
  PH_B = PH_LOW - PH_A * V4;
  Serial.println("pH Sensor Calibration Complete:");
  Serial.print("Slope (PH_A): "); Serial.println(PH_A, 4);
  Serial.print("Intercept (PH_B): "); Serial.println(PH_B, 4);
}

void loop() {
  
  float I = measureUVIntensity(20);  
  float absorbance = 0;

  if (I > 0 && I0 > 0) {
    absorbance = -log10(I / I0);  
  }

  

  float V = readVoltage(PIN_PH);
  float pH = PH_A * V + PH_B;

  
  Serial.print("UV Intensity: ");
  Serial.print(I);
  Serial.print(" | Absorbance: ");
  Serial.print(absorbance, 3);
  Serial.print(" || pH Voltage: ");
  Serial.print(V, 3);
  Serial.print(" V | pH: ");
  Serial.println(pH, 2);

 
  espSerial.print("UV:");
  espSerial.print(absorbance, 3);
  espSerial.print(",pH:");
  espSerial.println(pH, 2);

  delay(2000);
}
