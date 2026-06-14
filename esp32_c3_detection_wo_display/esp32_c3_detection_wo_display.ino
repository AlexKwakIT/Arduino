#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

#define BOARD_XIAO_C3
// #define BOARD_DEV_C3
// #define BOARD_TENSTAR_C3

#ifdef BOARD_XIAO_C3
  #define SDA_PIN 7
  #define SCL_PIN 21
  #define PIN_LED_RED 5
  #define PIN_LED_GREEN 6
  #define PIN_LED_BLUE 4
#endif

#ifdef BOARD_DEV_C3
  #define SDA_PIN 8
  #define SCL_PIN 9
  #define PIN_LED_RED 4
  #define PIN_LED_GREEN 5
  #define PIN_LED_BLUE 3
#endif

#ifdef BOARD_TENSTAR_C3
  #define SDA_PIN 4
  #define SCL_PIN 3
  #define PIN_LED_RED 5
  #define PIN_LED_GREEN 7
  #define PIN_LED_BLUE 6
#endif

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
  http.begin(testUrl);
  int code = http.GET();
  http.end();
  return (code > 0 && code < 400);
}

void findWorkingServer() {
  Serial.print("Finding server..");
  while (true) {
    for (int i = 0; i < serverCount; i++) {
      Serial.print(".");
      String url = String(possibleServerPorts[i]);
      if (isServerAlive(url)) {
        serverUrl = url;
        Serial.println();
        Serial.println("Found server: " + serverUrl);
        HTTPClient http;
        String url = String(serverUrl) + "/detection-checkin/" + ESP32_ID + "/";
        http.begin(url);
        http.GET();
        http.end();
        return;
      }
    }
    delay(250);
  }
}

void setLedRed() {
  digitalWrite(PIN_LED_RED, HIGH);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
}

void setLedGreen() {
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, HIGH);
  digitalWrite(PIN_LED_BLUE, LOW);
}

void setLedBlue() {
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_BLUE, HIGH);
}

void setLedOff() {
  digitalWrite(PIN_LED_RED, LOW);
  digitalWrite(PIN_LED_GREEN, LOW);
  digitalWrite(PIN_LED_BLUE, LOW);
}

void setup() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(ESP32_ID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  Serial.begin(115200);

  pinMode(PIN_LED_RED, OUTPUT);
  pinMode(PIN_LED_GREEN, OUTPUT);
  pinMode(PIN_LED_BLUE, OUTPUT);
  setLedRed();

  Serial.println("");
  Serial.print("ESP32 C3: ");
  Serial.println(ESP32_ID);
  Serial.print("SDA: ");
  Serial.println(SDA_PIN);
  Serial.print("SCL: ");
  Serial.println(SCL_PIN);
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

  findWorkingServer();

  nfc.begin();
  nfc.SAMConfig();
  Serial.println("Ready!");

  setLedGreen();
}

void loop() {

  uint8_t uid[7];
  uint8_t uidLength;

  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    setLedBlue();

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
    setLedGreen();
  }

  recvData();
}

void recvData() {
  if (Serial.available() > 0) {
    char controlChar1 = Serial.read();
    switch (controlChar1) {
      case '?':
        setLedBlue();
        Serial.print("{\"id\":\"eTrack Detector ");
        Serial.print(ESP32_ID);
        Serial.print("\"}");
        delay(250);
        setLedGreen();
        break;
      case 'x':
        Serial.println("Restarting...");
        ESP.restart();
        break;
    }
  }
}

void sendUID(String uid) {
  Serial.println("");
  Serial.print("Sending UID ");
  Serial.print(uid);
  Serial.print(" from ");
  Serial.print(ESP32_ID);
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
