// Sketch for an Arduino Nano, with:
// - reading 13.56MHz NFC tags with a PN532
// - get its own Id by reading pins D2...D6
// - on request sending its Id and the read NFC's to a master on a RS485 bus

#include <SPI.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>
#include <SoftwareSerial.h>

#define RS485_RX 3
#define RS485_TX 4
#define DE_RE 2
#define MAX_TAGS 50

byte deviceId = 0;

struct Tag {
  uint8_t uid[7];
  uint8_t length;
};
Tag tags[MAX_TAGS];
uint8_t tagCount = 0;

// ---------------- RS-485 ----------------
SoftwareSerial rs485(RS485_RX, RS485_TX);

// PN532 setup
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);


void setup() {
  Wire.begin();            // Initialize I2C (SDA=A4, SCL=A5 on Nano)
  Serial.begin(115200);    // Initialize serial for debug
  while (!Serial);         // Wait for Serial (optional)

  nfc.begin();             // Initialize PN532

  for (int pin=2; pin<=6; pin++) {
    pinMode(pin, INPUT_PULLUP);
  }
  deviceId = getId();

  // Check if PN532 is detected
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("PN532 not found. Check wiring!");
  } else {
    Serial.print("Found PN532, Firmware version: 0x");
    Serial.println(versiondata, HEX);
  }
  nfc.SAMConfig();
}

bool sameUID(uint8_t *a, uint8_t *b, uint8_t len) {
  for (uint8_t i = 0; i < len; i++)
    if (a[i] != b[i]) return false;
  return true;
}

void storeTag(uint8_t *uid, uint8_t len) {
  if (tagCount >= MAX_TAGS) return;
  if (tagCount >= 1) {
    if (tags[tagCount].length == len && sameUID(tags[tagCount].uid, uid, len)) return;
  }
  memcpy(tags[tagCount].uid, uid, len);
  tags[tagCount].length = len;
  tagCount++;
}

byte getId() {
  byte id = 0;
  for (int pin=2; pin<=6; pin++) {
    id = (2 * id) + (digitalRead(pin) == LOW ? 0 : 1);
  }
  return id;
}

// Send all stored tags over RS-485
void sendTags() {
  digitalWrite(DE_RE, HIGH); // TX mode
  delay(1);

  rs485.print("ID:");
  rs485.print(deviceId);
  rs485.print(",COUNT:");
  rs485.print(tagCount);

  for (uint8_t i = 0; i < tagCount; i++) {
    rs485.print(",T:");
    for (uint8_t j = 0; j < tags[i].length; j++) {
      if (tags[i].uid[j] < 0x10) rs485.print("0");
      rs485.print(tags[i].uid[j], HEX);
    }
  }

  rs485.println();
  delay(2);
  digitalWrite(DE_RE, LOW); // back to RX
}

void loop() {
  // 1️⃣ Read PN532 tags
  uint8_t uid[7];       // Buffer to store UID
  uint8_t uidLength;
  if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 100)) {
    Serial.print("D;");
    Serial.print(deviceId);
    Serial.print(";");
    for (uint8_t i = 0; i < uidLength; i++) {
      Serial.print(uid[i], HEX);
    }
    Serial.println();
    delay(100); // Small delay between reads
  }

  // 2️⃣ Listen for master commands
  if (rs485.available()) {
    int addr = rs485.read();

    if (addr == deviceId) {
      sendTags();
    }
  }

}
