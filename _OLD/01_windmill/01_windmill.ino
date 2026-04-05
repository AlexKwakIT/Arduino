#define BOARD_NR      1
#define BOARD_PURPOSE "Windmill"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

bool BOARD_RECOGNIZED = false;
const int PIN_FLASH_LIGHT = 10;
const int PIN_WINDMILL_MOTOR = 11;

byte WINDMILL_SPEED = 100;
int WINDMILL_LIGHT = true;

long startTimestamp = 0;

long controlCharTimestamp = 0;
char controlChar1 = 0;
char controlChar2 = 0;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_FLASH_LIGHT, OUTPUT);
  pinMode(PIN_WINDMILL_MOTOR, OUTPUT);

  // Intro screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.display();

  startTimestamp = millis();

  flushData();
}

void loop() {
  // internal LED blink
  digitalWrite(LED_BUILTIN, (millis() % 2000 <= 200 ? HIGH : LOW));

  if (millis() > startTimestamp + 1000) {
    printStatus();
  }

  // windmill flash light
  int m = millis() % 4000;
  if (!WINDMILL_LIGHT || m > 2000) {
    analogWrite(PIN_FLASH_LIGHT, 0);
  } else {
    int val = 255 - int(abs((m - 1000.0) * 255.0 / 1000.0));
    analogWrite(PIN_FLASH_LIGHT, val);
  }

  analogWrite(PIN_WINDMILL_MOTOR, WINDMILL_SPEED);

  recvData();
}

void printStatus() {
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  display.clearDisplay();

  if (millis() - controlCharTimestamp < 1000) {
    int c1 = controlChar1;
    int c2 = controlChar2;
    display.print("* ");
    display.print(c1);
    display.print(" ");
    display.println(c2);
  } else {
    if (BOARD_RECOGNIZED) {
      display.setTextColor(BLACK, WHITE);
    } else {
      display.setTextColor(WHITE, BLACK);
    }
    display.print(BOARD_NR);
    display.print(" ");
    display.println(BOARD_PURPOSE);
  }
  display.setTextColor(WHITE, BLACK);
  display.print("light: ");
  display.println(WINDMILL_LIGHT ? "on" : "off");
  display.print("speed: ");
  display.println(WINDMILL_SPEED);
  display.display();
}

void flushData() {
  while (Serial.available() > 0) {
    char ch = Serial.read();
  }
}

void recvData() {
  if (Serial.available() > 0) {
      controlChar1 = Serial.read();
      controlChar2 = 0;
      controlCharTimestamp = millis();
      switch (controlChar1) {
        case '?':
          Serial.print("{\"nr\":");
          Serial.print(BOARD_NR);
          Serial.print(", \"purpose\":\"");
          Serial.print(BOARD_PURPOSE);
          Serial.print("\", \"actions\": ["
                        "{\"type\": \"get\", \"format\": \"boolean\", \"item\": \"light\", \"action\": \"l?\"},"
                        "{\"type\": \"get\", \"format\": \"int\", \"item\": \"speed\", \"action\": \"s?\"},"
                        "{\"type\": \"put\", \"text\": \"Set windmill light\", \"cmd\": \"l%\"},"
                        "{\"type\": \"put\", \"text\": \"Set windmill speed\", \"cmd\": \"s#\"},"
                        "{\"type\": \"put\", \"text\": \"Set windmill speed higher\", \"cmd\": \"s+\"},"
                        "{\"type\": \"put\", \"text\": \"Set windmill speed slower\", \"cmd\": \"s-\"}"
                      "]}");
          break;
        case '!':
          BOARD_RECOGNIZED = true;
          break;
        case 'l':
          controlChar2 = Serial.read();
          switch (controlChar2) {
            case '?':
              Serial.print(WINDMILL_LIGHT ? "on" : "off");
              break;
            case '+':
              WINDMILL_LIGHT = true;
              break;
            case '-':
              WINDMILL_LIGHT = false;
              break;
            case 'x':
              WINDMILL_LIGHT = !WINDMILL_LIGHT;
              break;
          }
          break;
        case 's':
          controlChar2 = Serial.read();
          switch (controlChar2) {
            case '?':
              Serial.print(WINDMILL_SPEED);
              break;
            case '+':
              WINDMILL_SPEED = WINDMILL_SPEED + 1;
              break;
            case '-':
              WINDMILL_SPEED = WINDMILL_SPEED - 1;
              break;
            case '0' ... '9':
              WINDMILL_SPEED = int((controlChar2 - int('0')) * 255 / 10);
              break;
          }
          break;
      }
  }
}
