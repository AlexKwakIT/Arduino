#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

#define SDA_PIN 5
#define SCL_PIN 6

const char* ssid = "eTrackServer";
const char* password = "eTrack12321";
const char* possibleServerPorts[] = {
  "http://192.168.10.65:8000",
  "http://192.168.10.35:8000"
};
const int serverCount = sizeof(possibleServerPorts) / sizeof(possibleServerPorts[0]);
String serverUrl = "";

// PN532 setup
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);

char ESP32_ID[20];

bool isServerAlive(String url) {
  HTTPClient http;
  String testUrl = url + "/ping/";
  Serial.println("testUrl: " + testUrl);
  http.begin(testUrl);
  int code = http.GET();
  http.end();
  return (code > 0 && code < 400);
}

void findWorkingServer(int i) {
  Serial.print("Finding server, try ");
  Serial.print(i);
  Serial.println(" of 5");
  for (int i = 0; i < serverCount; i++) {
    String url = String(possibleServerPorts[i]);

    if (isServerAlive(url)) {
      serverUrl = url;
      Serial.println("Found server: " + serverUrl);
      return;
    }
  }
}

void setup() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(ESP32_ID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  Serial.begin(115200);
  Serial.println("");
  Serial.print("ESP32 C3: ");
  Serial.println(ESP32_ID);
  Serial.println("Starting up...");

  Wire.begin(SDA_PIN, SCL_PIN);

  WiFi.mode(WIFI_MODE_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(ssid, password, 0, nullptr, true);
  delay(1000);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
  }

  for (int i=1; i<=5; i++) {
    findWorkingServer(i);
    if (serverUrl != "") break;
    delay(2000);
  }
  if (serverUrl == "") {
    Serial.println("Stopping");
    while (true) {}
  }

  nfc.begin();
  nfc.SAMConfig();
  Serial.println("Ready!");
}

void loop() {

  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    String uidString = "";

    for (int i = 0; i < uidLength; i++) {
      if(uid[i] < 0x10) uidString += "0";
      uidString += String(uid[i], HEX);
    }
    uidString.toUpperCase();

    char UID[20];
    uidString.toCharArray(UID, sizeof(UID));

    sendUID(uidString);

    delay(100);
  }
  recvData();
}

void recvData() {
  if (Serial.available() > 0) {
    char controlChar1 = Serial.read();
    switch (controlChar1) {
      case '?':
        Serial.print("{\"id\":\"eTrack Detector ");
        Serial.print(ESP32_ID);
        Serial.print("\"}");
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
  Serial.print(" to ");
  Serial.println(serverUrl);

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(serverUrl) + "/detect/" + ESP32_ID + "/" + uid + "/";
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
