#include <WiFi.h>
#include <HTTPClient.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

#define SDA_PIN 5
#define SCL_PIN 6

#define SCREEN_WIDTH 72
#define SCREEN_HEIGHT 40

const char* ssid = "eTrackServer";
const char* password = "eTrack12321";

const char* serverURL = "http://192.168.1.100:8000/detect/";

U8G2_SSD1306_72X40_ER_F_HW_I2C display(U8G2_R0, U8X8_PIN_NONE);

// PN532 setup
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

char ESP32_ID[20];

void setup() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(ESP32_ID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  Serial.begin(115200);
  Serial.print("ESP32 C3: ");
  Serial.println(ESP32_ID);

  Wire.begin(SDA_PIN, SCL_PIN);

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

  nfc.begin();
  nfc.SAMConfig();
}

void loop() {

  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength)) {
    String uidString = "";

    for (int i = 0; i < uidLength; i++) {
      if(uid[i] < 0x10) uidString += "0";
      uidString += String(uid[i], HEX);
    }
    uidString.toUpperCase();

    char UID[20];
    uidString.toCharArray(UID, sizeof(UID));

    display.clearBuffer();
    display.drawStr(0, 10, "ESP32 C3");
    display.drawStr(0, 20, ESP32_ID);
    display.drawStr(0, 30, "Tag id");
    display.drawStr(0, 40, UID);
    display.sendBuffer();

    sendUID(uidString);

    delay(1000);
  }
}

void sendUID(String uid) {
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
