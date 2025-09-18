#include <WiFi.h>
#include <HTTPClient.h>


const char* ssid = "crowley";
const char* password = "noodles123";


String GAS_URL = "https://script.google.com/macros/s/AKfycbyqEgtDAf8OjUL-LtwEues2tNptQDEhlUDK3V2cm2RDTgtz_mrZljbkfAzcwEhPFztlVQ/exec";


#define RXD2 16   // ESP32 RX2 from Arduino TX
#define TXD2 17   // ESP32 TX2 to Arduino RX (not used)


const int PIN_GUVA = 34;    
const int PIN_TDS  = 35;    
const int PIN_UVLED = 25;  

const float VREF = 3.3;
const int ADC_MAX = 4095;
float TDS_K = 0.5;           


String PRODUCT_LABEL = "Tulsi";  

float readVoltage(int pin, int samples = 16) {
  unsigned long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return (float(sum)/samples) / ADC_MAX * VREF;
}


void connectWiFi() {
  if(WiFi.status() == WL_CONNECTED) return;

  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if(attempts > 30) { // 15s timeout
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
  Serial.println("ESP32 Ready");
}

void loop() {
  connectWiFi();

  // 1️⃣ Read pH from Arduino
  String ph = "";
  if (Serial2.available()) {
    String rawPh = Serial2.readStringUntil('\n');
    rawPh.trim();

    // Extract only numeric value after "pH:"
    int index = rawPh.indexOf("pH:");
    if (index != -1) {
      ph = rawPh.substring(index + 3);
      ph.trim();
    }
  }

  digitalWrite(PIN_UVLED, HIGH);
  delay(100);
  float guvaV = readVoltage(PIN_GUVA);
  digitalWrite(PIN_UVLED, LOW);

  
  float tdsV = readVoltage(PIN_TDS);
  float tdsPPM = tdsV * 1000 * TDS_K;


  Serial.println("---- Measurement ----");
  if (ph.length() > 0) Serial.println("pH: " + ph);
  Serial.print("GUVA Voltage: "); Serial.println(guvaV, 3);
  Serial.print("TDS: "); Serial.println(tdsPPM, 1);
  Serial.println("Label: " + PRODUCT_LABEL);

  
  if (ph.length() > 0 && WiFi.status() == WL_CONNECTED) {
    // Encode label (spaces -> %20)
    String labelEncoded = PRODUCT_LABEL;
    labelEncoded.replace(" ", "%20");

    String url = GAS_URL + "?ph=" + ph + "&guva=" + String(guvaV,3) + "&tds=" + String(tdsPPM,1) + "&label=" + labelEncoded;
    Serial.println("Sending to URL: " + url);

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Server response: " + payload);
    } else {
      Serial.println("Error sending data, HTTP code: " + String(httpCode));
    }
    http.end();
  }

  delay(2000); // Sampling interval
}
