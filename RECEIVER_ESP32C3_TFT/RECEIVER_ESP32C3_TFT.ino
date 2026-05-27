// =====================================================
// HERMES SATELLITE RECEIVER — HEADLESS GATEWAY VERSION
// ESP32-C3 + LoRa + WiFi + Firebase (No Screen)
// =====================================================

#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <HTTPClient.h>

// =====================================================
// WIFI + FIREBASE CONFIGURATION
// =====================================================
const char* WIFI_SSID     = "Shreyansh";
const char* WIFI_PASS     = "infinity";

// Firebase RTDB URL Node Configuration
const char* FIREBASE_URL  = "https://hermes-5aea5-default-rtdb.asia-southeast1.firebasedatabase.app/gps.json";

// =====================================================
// ESP32-C3 PINOUT SPECIFICATION (SPI for LoRa Only)
// =====================================================
#define SPI_SCK    4
#define SPI_MISO   5
#define SPI_MOSI   6

#define LORA_CS    7
#define LORA_RST   3
#define LORA_DIO0  2

#define BTN_PIN    0
#define BUZZER_PIN 1

// =====================================================
// SENSOR SCALE FACTORS
// =====================================================
#define GYRO_SCALE   131.0f
#define ACCEL_SCALE  16384.0f

// =====================================================
// TELEMETRY STORAGE STRUCTURE
// =====================================================
struct Telemetry {
  float temp     = 0.0;
  float pressure = 0.0;
  float altitude = 0.0;

  float ax = 0.0;
  float ay = 0.0;
  float az = 0.0;

  float gx = 0.0;
  float gy = 0.0;
  float gz = 0.0;

  float lat = 0.0;
  float lon = 0.0;
  float gpsAlt = 0.0;

  String event = "NORMAL";
  int rssi = 0;
};

Telemetry tel;

// =====================================================
// CORE SYSTEM TIMERS & STATE MANAGEMENT
// =====================================================
bool wifiOK  = false;
String satStatus = "NORMAL";

unsigned long lastFirebaseUpload = 0;
#define UPLOAD_INTERVAL 1000 // Stream up to once per second

#define CLICK_WINDOW 700
unsigned long lastPress = 0;
int pressCount = 0;
bool prevBtn = HIGH;

// =====================================================
// OPTIMIZED STRING CSV EXTRACTION PARSER
// =====================================================
String getVal(String& s, int idx) {
  int found = 0;
  int prev  = 0;
  for (int i = 0; i <= s.length(); i++) {
    if (i == s.length() || s[i] == ',') {
      if (found == idx) {
        return s.substring(prev, i);
      }
      found++;
      prev = i + 1;
    }
  }
  return "0";
}

void parseCSV(String& csv, int rssi) {
  tel.temp     = getVal(csv, 0).toFloat();
  tel.pressure = getVal(csv, 1).toFloat();
  tel.altitude = getVal(csv, 2).toFloat();

  tel.ax = getVal(csv, 3).toFloat() / ACCEL_SCALE;
  tel.ay = getVal(csv, 4).toFloat() / ACCEL_SCALE;
  tel.az = getVal(csv, 5).toFloat() / ACCEL_SCALE;

  tel.gx = getVal(csv, 6).toFloat() / GYRO_SCALE;
  tel.gy = getVal(csv, 7).toFloat() / GYRO_SCALE;
  tel.gz = getVal(csv, 8).toFloat() / GYRO_SCALE;

  tel.lat    = getVal(csv, 9).toFloat();
  tel.lon    = getVal(csv, 10).toFloat();
  tel.gpsAlt = getVal(csv, 11).toFloat();

  tel.event  = getVal(csv, 12);
  tel.event.trim();
  
  tel.rssi   = rssi;
}

// =====================================================
// NON-BLOCKING CLOUD SYNCHRONIZATION (HTTP PATCH)
// =====================================================
void uploadLiveTelemetry() {
  if (!wifiOK || WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(FIREBASE_URL);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Connection", "keep-alive");

  String currentStatus = (satStatus != "NORMAL") ? satStatus : tel.event;

  String json = "{";
  json += "\"lat\":" + String(tel.lat, 6) + ",";
  json += "\"lon\":" + String(tel.lon, 6) + ",";
  json += "\"alt\":" + String(tel.altitude, 2) + ",";
  json += "\"temp\":\"" + String(tel.temp, 2) + "\",";
  json += "\"press\":\"" + String(tel.pressure, 2) + "\",";
  json += "\"accel\":\"" + String(tel.ax, 3) + "/" + String(tel.ay, 3) + "/" + String(tel.az, 3) + "\",";
  json += "\"gyro\":\"" + String(tel.gx, 2) + "/" + String(tel.gy, 2) + "/" + String(tel.gz, 2) + "\",";
  json += "\"status\":\"" + currentStatus + "\",";
  json += "\"rssi\":" + String(tel.rssi) + ",";
  json += "\"timestamp\":" + String(millis()) + ",";
  json += "\"readableTime\":\"Gateway Update\"";
  json += "}";

  int httpResponseCode = http.PATCH(json);
  
  if (httpResponseCode > 0) {
    Serial.printf("[CLOUD] Sync OK [%d]\n", httpResponseCode);
  } else {
    Serial.printf("[CLOUD] Error: %s\n", http.errorToString(httpResponseCode).c_str());
  }
  http.end();
}

// =====================================================
// AUDIO ALERT HARNESS
// =====================================================
void beep(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(150);
    digitalWrite(BUZZER_PIN, LOW);
    delay(100);
  }
}

// =====================================================
// SETUP LOGIC ROUTINE
// =====================================================
void setup() {
  // Give USB connection time to settle for CDC serial outputs
  delay(2000); 
  Serial.begin(115200);
  
  Serial.println("\n=================================");
  Serial.println("  HERMES HEADLESS GATEWAY ONLINE ");
  Serial.println("=================================");

  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(LORA_CS, OUTPUT);   
  digitalWrite(LORA_CS, HIGH);

  // Initialize Hardware SPI Bus for LoRa Module exclusively
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Initialize LoRa Transceiver
  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("[CRITICAL] LoRa Radio Initialization Failed!");
    while (1) {
      digitalWrite(BUZZER_PIN, HIGH); delay(50);
      digitalWrite(BUZZER_PIN, LOW);  delay(50);
    }
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("[RADIO] LoRa Engine Active.");

  // Initialize Wi-Fi Connection
  Serial.println("[NETWORK] Configuring Radio Modules...");
  WiFi.disconnect(true); // Clear older profile states
  delay(200);
  WiFi.mode(WIFI_STA);   // Explicit Station configuration
  
  Serial.printf("[NETWORK] Attempting Connection to SSID: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(500);
    Serial.print(".");
    tries++;
  }
}

// =====================================================
// MAIN EXECUTION LOOP
// =====================================================
void loop() {
  bool hasPacket = false;

  // 1. ASYNCHRONOUS PACKET HARVESTER FROM LORA RADIO
  int pktSize = LoRa.parsePacket();
  if (pktSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    Serial.printf("[RADIO] Received Packet -> CSV Payload: %s\n", incoming.c_str());
    parseCSV(incoming, LoRa.packetRssi());
    hasPacket = true;
  }

  // 2. NON-BLOCKING CLOUD UPLOAD CONTROLLER
  if (hasPacket && (millis() - lastFirebaseUpload > UPLOAD_INTERVAL)) {
    uploadLiveTelemetry();
    lastFirebaseUpload = millis();
  }

  // 3. HARDWARE BUTTON STATE MACHINE CONTROLLER
  bool curBtn = (digitalRead(BTN_PIN) == LOW);
  if (curBtn && !prevBtn) {
    unsigned long now = millis();
    if (now - lastPress < CLICK_WINDOW) pressCount++;
    else                                pressCount = 1;
    lastPress = now;
  }
  prevBtn = curBtn;

  if (pressCount > 0 && (millis() - lastPress > CLICK_WINDOW)) {
    
    // Single-Click: Trigger Manual PRECRASH State Overwrite
    if (pressCount == 1) {
      satStatus = "PRECRASH";
      Serial.println("[STATE] Flag Forced to: PRECRASH");
      beep(2);
      uploadLiveTelemetry(); 
    }
    // Double-Click: Trigger Manual CRASH State Overwrite
    else if (pressCount == 2) {
      satStatus = "CRASH";
      Serial.println("[STATE] Flag Forced to: CRASH");
      beep(4);
      uploadLiveTelemetry(); 
    }
    // Triple-Click: Reset Back to Normal Operation Mode
    else if (pressCount >= 3) {
      satStatus = "NORMAL";
      Serial.println("[STATE] Reset complete. Status: NORMAL");
      beep(1);
      uploadLiveTelemetry();
    }
    pressCount = 0;
  }
}