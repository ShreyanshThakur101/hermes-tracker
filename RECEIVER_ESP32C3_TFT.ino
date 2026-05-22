#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SPI_SCK   4
#define SPI_MISO  5
#define SPI_MOSI  6

#define LORA_CS    7
#define LORA_RST   3
#define LORA_DIO0  2

#define TFT_CS    10
#define TFT_DC     8
#define TFT_RST    9

#define BUTTON_PIN 0
#define BUZZER_PIN 1

// WiFi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// Firebase RTDB URL (from firebase-config.js)
const char* firebase_url = "https://hermes-5aea5-default-rtdb.asia-southeast1.firebasedatabase.app/gps.json";

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

unsigned long lastButtonTime = 0;
unsigned long lastFirebaseUpdateTime = 0;
const long firebaseInterval = 1000; // Update Firebase every 1 second

int pressCount = 0;

String manualStatus = "NORMAL";

// Global variables for last received data
float lastLat = 0;
float lastLon = 0;
float lastAlt = 0;

void sendToFirebase(float lat, float lon, float alt, String temp, String press, String accel, String gyro, String status) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(firebase_url);
    http.addHeader("Content-Type", "application/json");

    String json = "{\"lat\":" + String(lat, 6) + 
                  ", \"lon\":" + String(lon, 6) + 
                  ", \"alt\":" + String(alt, 2) + 
                  ", \"temp\":\"" + temp + "\"" +
                  ", \"press\":\"" + press + "\"" +
                  ", \"accel\":\"" + accel + "\"" +
                  ", \"gyro\":\"" + gyro + "\"" +
                  ", \"status\":\"" + status + "\"" +
                  ", \"timestamp\":" + String(millis()) + 
                  ", \"readableTime\":\"Hardware Update\"}";

    int httpResponseCode = http.PATCH(json);

    if (httpResponseCode > 0) {
      Serial.print("Firebase Update Response: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Error on sending PATCH: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

String getValue(String data, char separator, int index) {

  int found = 0;

  int strIndex[] = {0, -1};

  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {

    if (data.charAt(i) == separator || i == maxIndex) {

      found++;

      strIndex[0] = strIndex[1] + 1;

      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }

  return found > index ?
         data.substring(strIndex[0], strIndex[1]) : "";
}

void beep(int count) {

  for (int i = 0; i < count; i++) {

    digitalWrite(BUZZER_PIN, HIGH);

    delay(150);

    digitalWrite(BUZZER_PIN, LOW);

    delay(150);
  }
}

void drawSatellite(int gx, int gy) {

  int centerX = 120;

  int centerY = 90;

  int moveX = map(gx, -20000, 20000, -15, 15);

  int moveY = map(gy, -20000, 20000, -15, 15);

  tft.drawRect(centerX + moveX,
               centerY + moveY,
               18,
               18,
               ST7735_CYAN);

  tft.drawRect(centerX - 14 + moveX,
               centerY + 4 + moveY,
               10,
               8,
               ST7735_YELLOW);

  tft.drawRect(centerX + 22 + moveX,
               centerY + 4 + moveY,
               10,
               8,
               ST7735_YELLOW);

  tft.drawLine(centerX + 9 + moveX,
               centerY - 10 + moveY,
               centerX + 9 + moveX,
               centerY + moveY,
               ST7735_WHITE);
}

void setup() {

  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);

  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  tft.initR(INITR_BLACKTAB);

  tft.setRotation(1);

  tft.fillScreen(ST7735_BLACK);

  tft.setTextWrap(false);

  tft.setTextSize(1);

  tft.setCursor(10, 20);

  tft.setTextColor(ST7735_GREEN);

  tft.println("BOOTING HERMES");

  tft.setCursor(10, 40);

  tft.println("INITIALIZING...");

  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);

  if (!LoRa.begin(433E6)) {

    tft.fillScreen(ST7735_RED);

    tft.setCursor(10, 60);

    tft.setTextColor(ST7735_WHITE);

    tft.println("LORA FAILED");

    while (1);
  }

  LoRa.setSyncWord(0xF3);

  // WiFi Connection
  tft.setCursor(10, 60);
  tft.setTextColor(ST7735_YELLOW);
  tft.print("WIFI CONNECTING...");
  
  WiFi.begin(ssid, password);
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    tft.setCursor(10, 75);
    tft.setTextColor(ST7735_GREEN);
    tft.println("WIFI CONNECTED!");
  } else {
    tft.setCursor(10, 75);
    tft.setTextColor(ST7735_RED);
    tft.println("WIFI FAILED");
  }

  delay(1500);

  tft.fillScreen(ST7735_BLACK);

  tft.setCursor(15, 20);

  tft.setTextColor(ST7735_CYAN);

  tft.println("HERMES RECEIVER");

  tft.setCursor(25, 40);

  tft.setTextColor(ST7735_GREEN);

  tft.println("SYSTEM READY");

  delay(1200);

  tft.fillScreen(ST7735_BLACK);
}

void loop() {

  if (digitalRead(BUTTON_PIN) == LOW) {

    pressCount++;

    lastButtonTime = millis();

    delay(250);
  }

  if (pressCount > 0 && millis() - lastButtonTime > 700) {

    if (pressCount == 1) {

      manualStatus = "CRASH";

      beep(1);
    }

    else if (pressCount >= 2) {

      manualStatus = "PRECRASH";

      beep(2);
    }

    pressCount = 0;
  }

  int packetSize = LoRa.parsePacket();

  if (packetSize) {

    String incoming = "";

    while (LoRa.available()) {

      incoming += (char)LoRa.read();
    }

    Serial.println(incoming);

    String temp = getValue(incoming, ',', 0);

    String pressure = getValue(incoming, ',', 1);

    String altitude = getValue(incoming, ',', 2);

    String ax = getValue(incoming, ',', 3);

    String ay = getValue(incoming, ',', 4);

    String az = getValue(incoming, ',', 5);

    String gx = getValue(incoming, ',', 6);

    String gy = getValue(incoming, ',', 7);

    String gz = getValue(incoming, ',', 8);

    String lat = getValue(incoming, ',', 9);

    String lon = getValue(incoming, ',', 10);

    String eventStatus = getValue(incoming, ',', 11);

    // Update global variables for Firebase
    lastLat = lat.toFloat();
    lastLon = lon.toFloat();
    lastAlt = altitude.toFloat();

    String accelStr = ax + "/" + ay + "/" + az;
    String gyroStr = gx + "/" + gy + "/" + gz;
    String currentStatus = (manualStatus != "NORMAL") ? manualStatus : eventStatus;

    // Send all telemetry to Firebase immediately on packet reception
    sendToFirebase(lastLat, lastLon, lastAlt, temp, pressure, accelStr, gyroStr, currentStatus);

    tft.fillScreen(ST7735_BLACK);

    tft.setCursor(10, 0);

    tft.setTextColor(ST7735_CYAN);

    tft.println("HERMES LIVE DATA");

    tft.drawFastHLine(0, 10, 160, ST7735_BLUE);

    tft.setTextColor(ST7735_WHITE);

    tft.setCursor(0, 15);

    tft.print("TEMP : ");

    tft.print(temp);

    tft.println(" C");

    tft.print("PRES : ");

    tft.print(pressure);

    tft.println(" hPa");

    tft.print("ALT  : ");

    tft.print(altitude);

    tft.println(" m");

    tft.setTextColor(ST7735_YELLOW);

    tft.print("LAT:");

    tft.println(lat);

    tft.print("LON:");

    tft.println(lon);

    tft.setTextColor(ST7735_MAGENTA);

    tft.print("GX:");

    tft.print(gx);

    tft.print(" ");

    tft.print("GY:");

    tft.println(gy);

    tft.setTextColor(ST7735_RED);

    tft.print("STATUS:");

    if (manualStatus != "NORMAL") {

      tft.println(manualStatus);
    }

    else {

      tft.println(eventStatus);
    }

    tft.setTextColor(ST7735_GREEN);

    tft.print("RSSI:");

    tft.print(LoRa.packetRssi());

    tft.println(" dBm");

    drawSatellite(gx.toInt(), gy.toInt());
  }
}