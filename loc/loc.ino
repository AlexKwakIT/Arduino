#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <WebServer.h>
#include <ESPmDNS.h>

// #define BOARD_XIAO_C3
// #define BOARD_DEV_C3
#define BOARD_TENSTAR_C3

#ifdef BOARD_XIAO_C3
  #define FORWARD_PIN 5
  #define BACKWARD_PIN 5
#endif

#ifdef BOARD_DEV_C3
  #define FORWARD_PIN 5
  #define BACKWARD_PIN 5
#endif

#ifdef BOARD_TENSTAR_C3
  #define FORWARD_PIN 5
  #define BACKWARD_PIN 5
#endif

WebServer server(8080);

const char* ssid = "eTrackServer";
const char* password = "eTrack12321";
const char* possibleServerPorts[] = {
  "http://192.168.10.65:8000",
  "http://192.168.10.35:8000"
};
const int serverCount = sizeof(possibleServerPorts) / sizeof(possibleServerPorts[0]);
String serverUrl = "";

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
        String url = String(serverUrl) + "/loc-checkin/" + ESP32_ID + "/";
        http.begin(url);
        http.GET();
        http.end();
        return;
      }
    }
    delay(250);
  }
}

void setup() {
  uint64_t chipid = ESP.getEfuseMac();
  sprintf(ESP32_ID, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);

  Serial.begin(9600);

  Serial.println("");
  Serial.print("ESP32 C3: ");
  Serial.println(ESP32_ID);
  Serial.println("Starting up...");

  WiFi.mode(WIFI_MODE_STA);
  WiFi.setSleep(false);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  WiFi.begin(ssid, password, 0, nullptr, true);
  delay(1000);

  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(500);
  }

  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());

  findWorkingServer();

  String locServerName = String("loc_") + ESP32_ID;
  if (MDNS.begin(locServerName)) {
    Serial.print("mDNS started: ");
    Serial.println(locServerName);
  }

  server.onNotFound([]() {
    String uri = server.uri();  // e.g. "/speed/123"
    if (uri.startsWith("/speed/")) {
      String numberPart = uri.substring(7);  // after "/speed/"
      int speed = numberPart.toInt();
      Serial.println(speed);
      server.send(200, "text/plain", String("OK ") + String(speed));
      setSpeed(speed);
      return;
    }
    server.send(404, "text/plain", "Not found");
  });

  server.begin();

  int freq = 5000;   // 5 kHz
  int resolution = 8; // 0–255
  pinMode(FORWARD_PIN, OUTPUT);
  ledcAttach(FORWARD_PIN, freq, resolution);
  ledcWrite(FORWARD_PIN, 0); // Full stop
  pinMode(BACKWARD_PIN, OUTPUT);
  ledcAttach(BACKWARD_PIN, freq, resolution);
  ledcWrite(BACKWARD_PIN, 0); // Full stop

  Serial.println("Ready!");
}

void loop() {
  server.handleClient();
  recvData();
}

void recvData() {
  if (Serial.available() > 0) {
    char controlChar1 = Serial.read();
    switch (controlChar1) {
      case '?':
        Serial.print("{\"id\":\"eTrack Loc ");
        Serial.print(ESP32_ID);
        Serial.print("\"}");
        delay(250);
        break;
      case 'x':
        Serial.println("Restarting...");
        ESP.restart();
        break;
    }
  }
}

void setSpeed(int speed) {
  if (speed >= 0) {
    speed = min(speed, 255);
    ledcWrite(BACKWARD_PIN, 0);
    ledcWrite(FORWARD_PIN, speed);
  } else {
    speed = max(speed, -255);
    ledcWrite(BACKWARD_PIN, -speed);
    ledcWrite(FORWARD_PIN, 0);
  }
}


