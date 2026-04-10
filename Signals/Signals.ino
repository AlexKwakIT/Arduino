#define BOARD_PURPOSE "Signals"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

#define TFT_CS   10
#define TFT_DC   9
#define TFT_RST  8   // or -1 if tied to Arduino reset
#define TFT_BL   12  

#define NUM_MESSAGE_LINES 6

#define STRAIGHT LOW
#define TURNOUT  HIGH

#define MAX_NUM_SIGNALS 31
#define NUM_I2C_DEVICES 8

#define COLOR_YELLOW_FLASH  0
#define COLOR_RED           1
#define COLOR_YELLOW        2
#define COLOR_GREEN         3

byte signal_values[MAX_NUM_SIGNALS];
byte i2c_addrs[NUM_I2C_DEVICES] = {-1, -1, -1, -1, -1, -1, -1, -1};
byte i2c_values_cur[NUM_I2C_DEVICES] = {0, 0, 0, 0, 0, 0, 0, 0};
byte i2c_values_new[NUM_I2C_DEVICES] = {0, 0, 0, 0, 0, 0, 0, 0};
int max_num_signals = 0;

String messages[NUM_MESSAGE_LINES] = {"", "", "", "", "", ""};

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

long lastRefresh = 0;

String setSignal(int signalNr, int color) {
  int index = signalNr * 3 + color;
  int i2c_device = int(index / 8);
  int pin = index % 8;

  String msg = "Signal ";
  msg += signalNr;
  msg + " set to ";
  switch (color) {
    case COLOR_YELLOW_FLASH:
      msg += "YELLOW FLASH";
      break;
    case COLOR_YELLOW:
      msg += "YELLOW";
      break;
    case COLOR_RED:
      msg += "RED";
      break;
    case COLOR_GREEN:
      msg += "GREEN";
      break;
  }
  addMessage(msg);
  return msg;
}

int get_pin(int nr) {
  return nr + 4;
}

void setup() {
  Wire.begin(); // join I2C bus as master
  Serial.begin(9600);
  Serial.println("Init...");
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(TFT_BL, OUTPUT);
  analogWrite(TFT_BL, 255);

  // Setup display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLUE);
  tft.setTextColor(ST77XX_YELLOW);
  tft.setTextSize(1);
  tft.setCursor(0, 4);
  tft.println(" Init...");

  // Find I2C devices
  int addressIndex = 0;
  for (byte addr = 0; addr <= 127; addr++) {
      Wire.beginTransmission(addr);
      if (Wire.endTransmission() == 0) {
        Wire.beginTransmission(addr);
        Wire.write(0x00);             // register to read
        Wire.endTransmission(false);  // repeated start
        i2c_addrs[addressIndex] = addr;
        addressIndex += 1;
        String msg = " I2C at 0x";
        msg += String(addr, HEX);
        Serial.println(msg);
        tft.println(msg);
        addMessage(msg);
        printStatus();
      }
  }
  max_num_signals = addressIndex;

  // // Init Signals
  // for (int i = 0; i < MAX_NUM_SIGNALS; i++) {
  //   signal_values[i] = COLOR_YELLOW_FLASH;
  // }

  // Flush receive buffer
  while (Serial.available() > 0) {
    char ch = Serial.read();
  }

  printStatus();

  Serial.println(i2c_addrs[0]);

  Wire.beginTransmission(i2c_addrs[0]);
  Wire.write(0);
  Wire.endTransmission();
}

void set_I2C() {
  for (int i=0; i<max_num_signals; i++) {

  }
}

void loop() {
  // internal LED short blink
  digitalWrite(LED_BUILTIN, (millis() % 2000 <= 200 ? HIGH : LOW));

  delay(1000);
  Wire.beginTransmission(i2c_addrs[0]);
  Wire.write(255);
  Wire.endTransmission();

  delay(1000);
  Wire.beginTransmission(i2c_addrs[0]);
  Wire.write(254);
  Wire.endTransmission();

  delay(1000);
  Wire.beginTransmission(i2c_addrs[0]);
  Wire.write(253);
  Wire.endTransmission();

  delay(1000);
  Wire.beginTransmission(i2c_addrs[0]);
  Wire.write(251);
  Wire.endTransmission();

  delay(2000);
  Wire.beginTransmission(i2c_addrs[0]);
  Wire.write(255);
  Wire.endTransmission();

  for (int i=0; i<10; i++) {
    delay(400);
    Wire.beginTransmission(i2c_addrs[0]);
    Wire.write(255);
    Wire.endTransmission();

    delay(400);
    Wire.beginTransmission(i2c_addrs[0]);
    Wire.write(254);
    Wire.endTransmission();

  }

  // if (lastRefresh + 1000 > millis()) {
  //   set_I2C();
  // }

  // recvData();
  // printStatus();
}

void addMessage(String message) {
  for (int i=NUM_MESSAGE_LINES-2; i>=0; i--) {
    messages[i+1] = messages[i];
  }
  messages[0] = message;
  printStatus();
}

void recvData() {
  if (Serial.available() > 0) {
      char controlChar1 = Serial.read();
      switch (controlChar1) {
        case '?':
          Serial.print("{\"id\":\"eTrack Signals\"}");
          break;
        // case 's': {
        //   // Set switch on/off
        //   char controlChar2 = Serial.read();
        //   char controlChar3 = Serial.read();
        //   char controlChar4 = Serial.read();
        //   int switchNr = 10 * (controlChar2 - '0') + (controlChar3 - '0');
        //   String msg = setSwitch(switchNr, controlChar4);
        //   Serial.print(msg + "\n");
        // }
        // break;
      }
  }
}

void printStatus() {
  // display.setTextSize(2);      // Normal 1:1 pixel scale
  // display.setTextColor(WHITE); // Draw white text
  // display.setCursor(0, 0);     // Start at top-left corner
  // display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // display.clearDisplay();
  
  // display.setTextColor(BLACK, WHITE);
  // display.println(BOARD_PURPOSE);

  // display.setTextColor(WHITE, BLACK);
  // display.setTextSize(1);
  // for (String message : messages) {
  //   display.println(message);
  // }
  // display.display();
}

