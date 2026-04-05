#define BOARD_NR      2
#define BOARD_PURPOSE "Tracks"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String actions[6] = {"", "", "", "", "", ""};

bool BOARD_RECOGNIZED = false;

int initFase = 0;
long initTimestamp = 0;
long startTimestamp = 0;

long controlCharTimestamp = 0;
char controlChar1 = 0;
char controlChar2 = 0;
char controlChar3 = 0;
char controlChar4 = 0;

void init(int fase) {
  switch (fase) {
    case 0:
      for (int pin=0; pin<16; pin++) {
        pinMode(get_pin(pin), OUTPUT);
        digitalWrite(get_pin(pin), LOW);
        delay(10);
      }
      break;
    case 1:
      for (int pin=0; pin<16; pin++) {
        digitalWrite(get_pin(pin), HIGH);
        delay(10);
      }
      break;
    case 2:
      for (int pin=54; pin<=69;pin++) {
        pinMode(pin, OUTPUT);
        digitalWrite(get_pin(pin), HIGH);
      }
      break;
    case 3:
      for (int pin=54; pin<=69;pin++) {
        digitalWrite(get_pin(pin), LOW);
      }
      break;
    default:
      initFase = 99;
      break;
  }
}

void recvData() {
  if (Serial.available() > 0) {
      controlChar1 = Serial.read();
      controlChar2 = 0;
      controlChar3 = 0;
      controlChar4 = 0;
      controlCharTimestamp = millis();
      switch (controlChar1) {
        case '?':
          Serial.print("{\"nr\":");
          Serial.print(BOARD_NR);
          Serial.print(", \"purpose\":\"");
          Serial.print(BOARD_PURPOSE);
          Serial.print("\", \"actions\": ["
                        "  {\"type\": \"get\", \"format\": \"text\", \"item\": \"track\", \"action\": \"t?\"},"
                        "  {\"type\": \"get\", \"format\": \"text\", \"item\": \"light\", \"action\": \"l?\"}"
                        "]"
                      "}");
          break;
        case '!':
          BOARD_RECOGNIZED = true;
          break;
        case 't': {
          // Set track on/off
          controlChar2 = Serial.read();
          if (controlChar2 == '?') {
            for (int i=0; i<16; i++) {
              if (i != 0) {
                Serial.print("|");
              }
              Serial.print("Track ");
              Serial.print(i);
              Serial.print(digitalRead(get_pin(i)) == HIGH ? " on" : " off");
            }
          } else {
            controlChar3 = Serial.read();
            controlChar4 = Serial.read();
            int trackNr = 10 * (controlChar2 - '0') + (controlChar3 - '0');
            int pin = get_pin(trackNr);
            int val = controlChar4 == '0' ? LOW : HIGH;
            digitalWrite(pin, val);
            String action = "Set track ";
            action += trackNr;
            action += controlChar4 == '0' ? " off" : " on";
            addAction(action);
          }
        }
        break;
        case 'l': {
          // Set bump buffer light on/off
          controlChar2 = Serial.read();
          if (controlChar2 == '?') {
            for (int i=0; i<16; i++) {
              if (i != 0) {
                Serial.print("|");
              }
              Serial.print("Light  ");
              Serial.print(i);
              Serial.print(digitalRead(i + 54) == HIGH ? " on" : " off");
            }
          } else {
            controlChar3 = Serial.read();
            controlChar4 = Serial.read();
            int pin = 10 * (controlChar2 - '0') + (controlChar3 - '0') + 54;
            int val = controlChar4 == '0' ? LOW : HIGH;
            digitalWrite(pin, val);
            String action = "Set light ";
            action += (pin - 54);
            action += (val == HIGH ? " on" : " off");
            addAction(action);
          }
        }            
        break;
      }
  }
}

int get_pin(int nr) {
  int pins[] = {23, 22, 25, 24, 27, 26, 29, 28, 31, 30, 33, 32, 35, 34, 37, 36};
  return pins[nr];
}
