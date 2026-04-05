#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>

// ---------------------------
// CONFIGURATION
// ---------------------------

#define TCA_START 0x70
#define TCA_END   0x77

// Maximum devices we expect
#define MAX_DEVICES 50 // Max number of PN532s

PN532_I2C* pn532i2cArray[MAX_DEVICES];
PN532* nfcArray[MAX_DEVICES];

struct I2CDevice {
  uint8_t tcaAddress;    // Multiplexer address
  uint8_t channel;       // Channel 0-7
  uint8_t deviceAddress; // Found device address
};

I2CDevice devices[MAX_DEVICES];
int deviceCount = 0;

PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);	

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(100); // allow I2C bus to settle

  Serial.println("Starting I2C scan for TCA9548A multiplexers...");

  // Scan all possible TCA9548A addresses
  for (uint8_t tcaAddr = TCA_START; tcaAddr <= TCA_END; tcaAddr++) {
    Wire.beginTransmission(tcaAddr);
    if (Wire.endTransmission() == 0) { // multiplexer found
      Serial.print("TCA9548A found at 0x");
      Serial.println(tcaAddr, HEX);

      // Scan each channel
      for (uint8_t ch = 0; ch < 8; ch++) {
        tcaSelect(tcaAddr, ch);
        scanI2CDevices(tcaAddr, ch);
      }

      // Deselect all channels after scan
      tcaSelect(tcaAddr, 0xFF);
    }
  }

  Serial.println("Scan complete!");
  Serial.println("Results:");
  for (int i = 0; i < deviceCount; i++) {
    Serial.print("TCA 0x");
    Serial.print(devices[i].tcaAddress, HEX);
    Serial.print(" | Channel ");
    Serial.print(devices[i].channel);
    Serial.print(" | Device 0x");
    Serial.println(devices[i].deviceAddress, HEX);
  }

  Serial.println("Initializing the PCA9548A multiplexer...");
  Wire.beginTransmission(0x70);
  Wire.write(1);
  Wire.endTransmission();

  Serial.println("Initializing PN532 over I2C...");
  nfc.begin();
  delay(100);

  // Try to get firmware version
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.println("Failed to communicate with PN532!");
    Serial.println("Check I2C mode, wiring, pull-ups, and address (0x24).");
    while (1); // stop here
  }

  // Print firmware info
  Serial.print("Found PN532! Firmware version: 0x");
  Serial.println(versiondata, HEX);

  // Configure board to read NFC tags
  nfc.SAMConfig();
  Serial.println("PN532 is ready to read cards...");
}

void loop() {
  uint8_t uid[7];       // Buffer to store UID
  uint8_t uidLength;

  for (int i=0; i<deviceCount; i++) {
    // Initializing the PCA9548A multiplexer...
    int adr = devices[i].tcaAddress;
    int channel = 1 << devices[i].channel;
    Wire.beginTransmission(adr);
    Wire.write(channel);
    Wire.endTransmission();
    delay(100);
    if (nfcArray[i]->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 50)) {
      Serial.print("Card UID: ");
      for (uint8_t i = 0; i < uidLength; i++) {
        if (uid[i] < 16) Serial.print("0");
        Serial.print(uid[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      //delay(1000); // avoid multiple reads in a row
    } else {
      // Serial.print(adr);
      // Serial.print(":");
      // Serial.print(channel);
      // Serial.println(" ");
    }
  }
}

// Select a channel on a specific TCA9548A
void tcaSelect(uint8_t tcaAddr, uint8_t channel) {
  Wire.beginTransmission(tcaAddr);
  if (channel > 7) {
    Wire.write(0); // deselect all if invalid
  } else {
    Wire.write(1 << channel);
  }
  Wire.endTransmission();
}

// Scan for NFC devices on the selected channel
void scanI2CDevices(uint8_t tcaAddr, uint8_t channel) {
  for (uint8_t addr = 1; addr < 127; addr++) {
    if (addr < TCA_START || addr > TCA_END) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) { // device found
        Serial.print("  Found device 0x");
        Serial.print(addr, HEX);
        Serial.print(" on channel ");
        Serial.println(channel);

        // PN532_I2C pn532i2c(Wire);
        // PN532 nfc(pn532i2c);	
        Serial.print("  Trying to determine if this is a PN532...  ");
        pn532i2cArray[deviceCount] = new PN532_I2C(Wire);
        nfcArray[deviceCount] = new PN532(*pn532i2cArray[deviceCount]);
        nfcArray[deviceCount]->begin();
        delay(100);
        // Try to get firmware version
        uint32_t versiondata = nfc.getFirmwareVersion();
        if (!versiondata) {
          Serial.println("No PN532");
        } else if (deviceCount < MAX_DEVICES) {

          PN532_I2C pn532i2c(Wire);
          devices[deviceCount].tcaAddress = tcaAddr;
          devices[deviceCount].channel = channel;
          devices[deviceCount].deviceAddress = addr;
          deviceCount++;
          Serial.println("PN532 added");
        }
      }
    }
  }
}
