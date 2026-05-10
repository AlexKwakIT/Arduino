#include <WiFi.h>
#include <HTTPClient.h>
#include <U8g2lib.h>
#include <Wire.h>

#include <PN5180.h>
#include <PN5180FeliCa.h>

#define SDA_PIN 5
#define SCL_PIN 6

#define SCREEN_WIDTH 72
#define SCREEN_HEIGHT 40

#define PIN_SCK   4
#define PIN_MISO  1 // 5
#define PIN_MOSI  2 // 6
#define PIN_SS    7
#define PIN_BUSY  8
#define PIN_RST   9
#define PIN_IRQ   10

const char* ssid = "eTrackServer";
const char* password = "eTrack12321";
const char* serverURL = "http://192.168.10.35:8000/detect/";

U8G2_SSD1306_72X40_ER_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

char ESP32_ID[20];

PN5180 pn5180(/* NSS=*/ PIN_SS, /* BUSY=*/ PIN_BUSY, /* RST=*/ PIN_RST); // ESP32

void setup() {
  Serial.begin(115200);
  Serial.println("A");
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(ESP32_ID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_SS);
  
  pinMode(PIN_BUSY, INPUT);
  pinMode(PIN_IRQ, INPUT);

  delay(10);
  Serial.println("B");
  while (digitalRead(PIN_BUSY) == HIGH);
  Serial.println("C");
  

  Serial.println("");
  Serial.print("ESP32 C3: ");
  Serial.println(ESP32_ID);

  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN);
  pn5180.begin();
  //pn5180.reset();

  display.begin();
  display.setFont(u8g2_font_ncenB08_tr);
  display.clearBuffer();
  display.drawStr(0, 10, "Init...");
  display.sendBuffer();

  WiFi.mode(WIFI_MODE_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(ssid, password, 0, nullptr, true);
  delay(1000);

  display.clearBuffer();
  display.drawStr(0, 10, "Connect to");
  display.drawStr(0, 20, ssid);
  static char rssiStr[10];
  sprintf(rssiStr, "%d", WiFi.RSSI());
  display.drawStr(0, 30, rssiStr);
  display.drawStr(20, 30, "dB");
  display.sendBuffer();

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
  }

  display.clearBuffer();
  display.drawStr(0, 10, "ESP32 C3");
  display.drawStr(0, 20, ESP32_ID);
  if (WiFi.status() != WL_CONNECTED) {
    display.drawStr(0, 30, "Connection");
    display.drawStr(0, 40, "FAILED");
    while (true);
  } else {
    display.drawStr(0, 30, "Connected:");
    display.drawStr(0, 40, ssid);
  }
  display.sendBuffer();
}

void loop() {

  // Create an 8-byte integer array to store UID of any detected tag
  uint8_t uid[8];

  Serial.print(".");

  if (pn5180.readCardSerial(uid, &uidLength)) {

      Serial.print("UID: ");
      for (int i = 0; i < uidLength; i++) {
        Serial.print(uid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      delay(1000);
    }

  // if (pn5180.getInventory(uid)) {
  //   // If tag was found, print its ID to the serial monitor
  //   Serial.print("Tag UID: ");
  // 	// Format each byte as HEX, padded with leading zeroes if required
  //   for (int i = 7; i >= 0; i--) {
  //     if (uid[i] < 0x10) Serial.print("0");
  //     Serial.print(uid[i], HEX);
  //   }
  //   Serial.println();

  //   // char UID[20];
  //   // uidString.toCharArray(UID, sizeof(UID));

  //   // display.clearBuffer();
  //   // display.drawStr(0, 10, "ESP32 C3");
  //   // display.drawStr(0, 20, ESP32_ID);
  //   // display.drawStr(0, 30, "Tag id");
  //   // display.drawStr(0, 40, UID);
  //   // display.sendBuffer();

  //   // sendUID(uidString);

  //   delay(100);
  // }
  recvData();
}

void recvData() {
  if (Serial.available() > 0) {
    char controlChar1 = Serial.read();
    switch (controlChar1) {
      case '?':
        Serial.print("{\"id\":\"eTrack Detector\"}");
        break;
    }
  }
}

void sendUID(String uid) {
  Serial.println("");
  Serial.print("Sending UID ");
  Serial.print(uid);
  Serial.print(" from ");
  Serial.println(ESP32_ID);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverURL) + ESP32_ID + "/" + uid + "/";
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.print("Response code: ");
      Serial.println(httpResponseCode);

      String payload = http.getString();
      Serial.print("Response: ");
      Serial.println(payload);
    } else {
      Serial.print("Error: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else{
      Serial.print("Wrong WiFi status: ");
      Serial.println(WiFi.status());
  }
}
