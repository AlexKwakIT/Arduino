#define BOARD_NR      "3"
#define BOARD_PURPOSE "Switches"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define STRAIGHT HIGH
#define TURNOUT  LOW

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int switchStatus[16] = {STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT, STRAIGHT };

String actions[6] = {"", "", "", "", "", ""};

bool BOARD_RECOGNIZED = false;

int initFase = 0;
long startTimestamp = 0;
long initTimestamp = 0;

long controlCharTimestamp = 0;
char controlChar1 = 0;
char controlChar2 = 0;
char controlChar3 = 0;
char controlChar4 = 0;

void setSwitch(int switchNr, int direction, bool activate=true) {
  int pin = get_pin(switchNr);
  digitalWrite(pin, direction);
  if (activate) {
    activateSwitches();
    String action = "Switch ";
    action += switchNr;
    action += direction == 0 ? " straight" : " turnout";
    addAction(action);
  }
}

void init(int fase) {
  switch (fase) {
    case 0:
      for (int pin=1; pin<16; pin++) {
        setSwitch(pin, TURNOUT, false);
      }
      activateSwitches();
      addAction("Switches turnout");
      break;
    case 1:
      for (int pin=1; pin<16; pin++) {
        setSwitch(pin, STRAIGHT, false);
      }
      activateSwitches();
      addAction( "Switches straight");
      break;
    default:
      initFase = 99;
      break;
  }
}

void activateSwitches() {
  digitalWrite(get_pin(0), HIGH);
  delay(250);
  digitalWrite(get_pin(0), LOW);
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
                        "  {\"type\": \"get\", \"format\": \"text\", \"item\": \"Switch status\", \"action\": \"s?\"},"
                        "  {\"type\": \"put\", \"text\": \"Set switch\", \"cmd\": \"s#?\"}"
                        "]"
                      "}");
          break;
        case '!':
          BOARD_RECOGNIZED = true;
          break;
        case 's': {
          // Set switch on/off
          controlChar2 = Serial.read();
          if (controlChar2 == '?') {
            String s0 = "";
            String s1 = "";
            for (int i=1; i<16; i++) {
              if (digitalRead(get_pin(i)) == HIGH) {
                s1 += ", ";
                s1 += i;
              } else {
                s0 += i;
                s0 += ", ";
              }
            }
            if (s0 == "") {
              s0 = "None";
            } else {
              s0 = s0.substring(2);
            }
            if (s1 == "") {
              s1 = "None";
            } else {
              s1 = s1.substring(2);
            }
            Serial.print("Straight: ");
            Serial.print(s1);
            Serial.print(". Turnout: ");
            Serial.print(s0);
            Serial.println(".");
          } else {
            controlChar3 = Serial.read();
            controlChar4 = Serial.read();
            int switchNr = 10 * (controlChar2 - '0') + (controlChar3 - '0');
            setSwitch(switchNr, (controlChar4 == '0' ? STRAIGHT : TURNOUT));
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
