#include <WiFi.h>
#include <FirebaseESP32.h>

// ===== WiFi details =====
const char* ssid = "crowley";
const char* password = "noodles123";

// ===== Firebase details =====
#define FIREBASE_HOST "https://e-tongue-94eeb-default-rtdb.asia-southeast1.firebasedatabase.app/";
#define FIREBASE_AUTH "PXTNrFoCIN2oF7hZ1JPu66DjpUxw8b5dLGyB5Y0e"      
FirebaseData firebaseData;

// ===== UART pins for Arduino connection =====
#define RXD2 16   // ESP32 RX2 from Arduino TX
#define TXD2 17   // ESP32 TX2 to Arduino RX (not used)

// ===== Sensor pins =====
const int PIN_GUVA = 34;    
const int PIN_TDS  = 35;    
const int PIN_UVLED = 25;   

const float VREF = 3.3;
const int ADC_MAX = 4095;
float TDS_K = 0.5;          



// ===== Function to read analog voltage =====
float readVoltage(int pin, int samples = 16) {
  unsigned long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return (float(sum)/samples) / ADC_MAX * VREF;
}

// ===== WiFi connection =====
void connectWiFi() {
  if(WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if(attempts > 30) { 
      Serial.println("\nFailed to connect. Restarting ESP32...");
      ESP.restart();
    }
  }
  Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
}

void setup() {
  Serial.begin(115200);                        
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); 

  pinMode(PIN_UVLED, OUTPUT);
  digitalWrite(PIN_UVLED, LOW);

  delay(1000);
  connectWiFi();
  
  // Initialize Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);

  Serial.println("ESP32 Ready");
  Serial.println("time_ms,pH,TDS_ppm,UV_V,label");
}

void loop() {
  connectWiFi();


  String ph = "";
  if (Serial2.available()) {
    String rawPh = Serial2.readStringUntil('\n');
    rawPh.trim();
    int index = rawPh.indexOf("pH:");
    if (index != -1) {
      ph = rawPh.substring(index + 3);
      ph.trim();
    }
  }

  digitalWrite(PIN_UVLED, HIGH);
  delay(4000);
  float guvaV = readVoltage(PIN_GUVA);
  digitalWrite(PIN_UVLED, LOW);

 
  float tdsV = readVoltage(PIN_TDS);
  float tdsPPM = tdsV * 1000 * TDS_K;

 
  unsigned long now = millis();
  Serial.print(now); Serial.print(",");
  if (ph.length() > 0) Serial.print(ph); else Serial.print("NA");
  Serial.print(",");
  Serial.print(tdsPPM, 1);
  Serial.print(",");
  Serial.print(guvaV, 3);
  Serial.print(",");
  Serial.println(PRODUCT_LABEL);

 
  if (ph.length() > 0 && WiFi.status() == WL_CONNECTED) {
    String path = "/sensor_data/" + String(now);
    if (Firebase.RTDB.setFloat(&firebaseData, path + "/pH", ph.toFloat()) &&
        Firebase.RTDB.setFloat(&firebaseData, path + "/TDS", tdsPPM) &&
        Firebase.RTDB.setFloat(&firebaseData, path + "/GUVA", guvaV)) {
      Serial.println("Data sent to Firebase successfully!");
    } else {
      Serial.println("Firebase error: " + firebaseData.errorReason());
    }

    // Set label
    Firebase.RTDB.setString(&firebaseData, path + "/label", PRODUCT_LABEL);
  }

  delay(2000); // Sampling interval
}
