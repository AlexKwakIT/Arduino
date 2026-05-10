// NAME: PN5180-FeliCa.ino
//
// DESC: Example usage of the PN5180 library for the PN5180-NFC Module
//       from NXP Semiconductors.
//
// Copyright (c) 2018 by Andreas Trappmann. All rights reserved.
// Copyright (c) 2019 by Dirk Carstensen.
// Copyright (c) 2020 by CrazyRedMachine.
//
// This file is part of the PN5180 library for the Arduino environment.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public 
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// BEWARE: SPI with an Arduino to a PN5180 module has to be at a level of 3.3V
// use of logic-level converters from 5V->3.3V is absolutly neccessary
// on most Arduinos for all input pins of PN5180!
// If used with an ESP-32, there is no need for a logic-level converter, since
// it operates on 3.3V already.
//
// Arduino <-> Level Converter <-> PN5180 pin mapping:
// 5V             <-->             5V
// 3.3V           <-->             3.3V
// GND            <-->             GND
// 5V      <-> HV
// GND     <-> GND (HV)
//             LV              <-> 3.3V
//             GND (LV)        <-> GND
// SCLK,13 <-> HV1 - LV1       --> SCLK
// MISO,12        <---         <-- MISO
// MOSI,11 <-> HV3 - LV3       --> MOSI
// SS,10   <-> HV4 - LV4       --> NSS (=Not SS -> active LOW)
// BUSY,9         <---             BUSY
// Reset,7 <-> HV2 - LV2       --> RST
//
// ESP-32    <--> PN5180 pin mapping:
// 3.3V      <--> 3.3V
// GND       <--> GND
// SCLK, 18   --> SCLK
// MISO, 19  <--  MISO
// MOSI, 23   --> MOSI
// SS, 16     --> NSS (=Not SS -> active LOW)
// BUSY, 5   <--  BUSY
// Reset, 17  --> RST
//

/*
 * Pins on ICODE2 Reader Writer:
 *
 *   ICODE2   |     PN5180
 * pin  label | pin  I/O  name
 * 1    +5V
 * 2    +3,3V
 * 3    RST     10   I    RESET_N (low active)
 * 4    NSS     1    I    SPI NSS
 * 5    MOSI    3    I    SPI MOSI
 * 6    MISO    5    O    SPI MISO
 * 7    SCK     7    I    SPI Clock
 * 8    BUSY    8    O    Busy Signal
 * 9    GND     9  Supply VSS - Ground
 * 10   GPIO    38   O    GPO1 - Control for external DC/DC
 * 11   IRQ     39   O    IRQ
 * 12   AUX     40   O    AUX1 - Analog/Digital test signal
 * 13   REQ     2?  I/O   AUX2 - Analog test bus or download
 *
 */


//#define WRITE_ENABLED 1

#include <PN5180.h>
//#include <PN5180ISO14443.h>

// #if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560) || defined(ARDUINO_AVR_NANO) || defined(ARDUINO_ARCH_SAM)

// #define PN5180_NSS  10
// #define PN5180_BUSY 9
// #define PN5180_RST  7

// #elif defined(ARDUINO_ARCH_ESP32)

// #define PN5180_NSS  16   // swapped with BUSY
// #define PN5180_BUSY 5  // swapped with NSS
// #define PN5180_RST  17

// #else
// #error Please define your pinout here!
// #endif

#define PIN_MISO  5
#define PIN_MOSI  6
#define PIN_SCK   4
#define PIN_BUSY  8
#define PIN_NSS   7
#define PIN_RST   9

PN5180 nfc(PIN_NSS, PIN_BUSY, PIN_RST);

void writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(PIN_NSS, LOW);

  SPI.transfer(0x00);      // WRITE command (PN5180 SPI framing)
  SPI.transfer(reg);
  SPI.transfer(value);

  digitalWrite(PIN_NSS, HIGH);
}

uint8_t readRegister(uint8_t reg) {
  digitalWrite(PIN_NSS, LOW);

  SPI.transfer(0x01);      // READ command
  SPI.transfer(reg);
  uint8_t val = SPI.transfer(0x00);

  digitalWrite(PIN_NSS, HIGH);

  return val;
}

void hardResetPN5180() {
  digitalWrite(PIN_RST, LOW);
  delay(50);
  digitalWrite(PIN_RST, HIGH);
  delay(500);  // critical boot time
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI, PIN_NSS);

  Serial.println("==================================");
  Serial.println("PN5180 FeliCa Demo Sketch");

  pinMode(PIN_BUSY, INPUT_PULLDOWN);
  digitalWrite(PIN_RST, LOW);
  delay(50);
  digitalWrite(PIN_RST, HIGH);
  delay(200);

  Serial.println("A1");
  delay(1000);

  SPI.beginTransaction(SPISettings(500000, MSBFIRST, SPI_MODE0));
  Serial.println("A2");
  delay(1000);

  hardResetPN5180();
  Serial.println("A3");
  delay(1000);

	uint32_t rxStatus;
	uint16_t len = 0;
	nfc.readRegister(RX_STATUS, &rxStatus);
  Serial.println(rxStatus, HEX);
  Serial.println("A4");
  delay(1000);

  Serial.println("B");

  uint8_t productVersion[2];
  nfc.readEEprom(PRODUCT_VERSION, productVersion, sizeof(productVersion));
  Serial.print(F("Product version="));
  Serial.print(productVersion[1]);
  Serial.print(".");
  Serial.println(productVersion[0]);

  if (0xff == productVersion[1]) { // if product version 255, the initialization failed
    Serial.println(F("Initialization failed!?"));
    Serial.println(F("Press reset to restart..."));
    Serial.flush();
    exit(-1); // halt
  }
  
  Serial.println(F("----------------------------------"));
  Serial.println(F("Reading firmware version..."));
  uint8_t firmwareVersion[2];
  nfc.readEEprom(FIRMWARE_VERSION, firmwareVersion, sizeof(firmwareVersion));
  Serial.print(F("Firmware version="));
  Serial.print(firmwareVersion[1]);
  Serial.print(".");
  Serial.println(firmwareVersion[0]);

  Serial.println(F("----------------------------------"));
  Serial.println(F("Reading EEPROM version..."));
  uint8_t eepromVersion[2];
  nfc.readEEprom(EEPROM_VERSION, eepromVersion, sizeof(eepromVersion));
  Serial.print(F("EEPROM version="));
  Serial.print(eepromVersion[1]);
  Serial.print(".");
  Serial.println(eepromVersion[0]);

  Serial.println(F("----------------------------------"));
  Serial.println(F("Enable RF field..."));
  //nfc.setupRF();
  Serial.println("X");
}

uint32_t loopCnt = 0;
bool errorFlag = false;

void loop() {
  Serial.println(F("----------------------------------"));
  Serial.print(F("Loop #"));
  Serial.println(loopCnt++);
  #if defined(ARDUINO_ARCH_ESP32)  
    Serial.println("Free heap: " + String(ESP.getFreeHeap())); 
  #endif
  uint8_t uid[20];
  // check for FeliCa card
  nfc.reset();
  //nfc.setupRF();
  uint8_t uidLength = nfc.readCardSerial(uid);
  if (uidLength > 0) {
    Serial.print(F("FeliCa card found, UID="));
    for (int i=0; i<uidLength; i++) {
      Serial.print(uid[i] < 0x10 ? " 0" : " ");
      Serial.print(uid[i], HEX);
    }
    Serial.println();
    Serial.println(F("----------------------------------"));
    delay(1000); 
    return;
  }
  
  // no card detected
  Serial.println(F("*** No card detected!"));
}
