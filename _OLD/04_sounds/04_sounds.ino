#define BOARD_NR      4
#define BOARD_TYPE    "Mega"
#define BOARD_PURPOSE "Sounds"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define PIN_SOUND_ON_OFF A0
int SOUND_ON = 1;
int SOUND_OFF = 0;
int SOUND_STATUS = SOUND_OFF;

bool BOARD_RECOGNIZED = false;

String actions[6] = {"", "", "", "", "", ""};
bool playing[10] = { true, true, true, true, true, true, true, true, true, true };
long setHighTimestamp[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
long setTriStateTimestamp[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const int SHORT_DELAY = 100;
const int LONG_DELAY = 1000;

const int cmd_previous   = 1;
const int cmd_volumeDown = 2;
const int cmd_play_pause = 3;
const int cmd_next       = 4;
const int cmd_volumeUp   = 5;

long startTimestamp = 0;
long initTimestamp = 0;
int initFase = 0;
bool readyForCommands = false;

long controlCharTimestamp = 0;
char controlChar1 = 0;
char controlChar2 = 0;
char controlChar3 = 0;

void command(int soundNr, int cmd) {
  int pad_previous =   0;
  int pad_volumeDown = 0;
  int pad_play_pause = 1;
  int pad_next =       2;
  int pad_volumeUp =   2;

  int delay_previous =   SHORT_DELAY;
  int delay_volumeDown = LONG_DELAY;
  int delay_play_pause = SHORT_DELAY;
  int delay_next =       SHORT_DELAY;
  int delay_volumeUp =   LONG_DELAY;

  int pad = 0;
  int delay = 0;
  String action = "#";
  action += soundNr;
  switch (cmd) {
    case cmd_volumeUp:
      pad = pad_volumeUp;
      delay = delay_volumeUp;
      action += " volume up";
      break;
    case cmd_volumeDown:
      pad = pad_volumeDown;
      delay = delay_volumeDown;
      action += " volume down";
      break;
    case cmd_next:
      pad = pad_next;
      delay = delay_next;
      action += " next";
      break;
    case cmd_previous:
      pad = pad_previous;
      delay = delay_previous;
      action += "previous";
      break;
    case cmd_play_pause:
      playing[soundNr] = !playing[soundNr];
      pad = pad_play_pause;
      delay = delay_play_pause;
      action += (playing[soundNr] ? " play" : " pause");
      break;
  }
  addAction(action);

  int pin = getPin(soundNr, pad);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  setHighTimestamp[soundNr] = millis() + delay;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // Intro screen
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.display();

  startTimestamp = millis();

  // Init all pins
  for (int pin=19; pin<=48; pin++) {
    pinMode(pin, INPUT); // TriState
  }

  pinMode(PIN_SOUND_ON_OFF, OUTPUT);
  toggleSoundStatus();
  flushData();
}

void toggleSoundStatus() {
  SOUND_STATUS = (SOUND_STATUS == SOUND_ON ? SOUND_OFF : SOUND_ON);
  digitalWrite(PIN_SOUND_ON_OFF, SOUND_STATUS == SOUND_ON ? HIGH : LOW);
}

void loop() {
  // internal LED blink
  digitalWrite(LED_BUILTIN, (millis() % 2000 <= 200 ? HIGH : LOW));

  if (readyForCommands) {
    recvData();
  }

  if (initFase < 99) {
    switch(initFase) {
      case 0 ... 1:
        delay(100);
        initFase++;
        break;
      default:
        if (millis() > initTimestamp + 1000) {
          // Stop all music
          for (int i=0; i<10; i++) {
            command(i, cmd_play_pause);
          }
          initFase = 99;
        }
        break;
    }
  }

  for (int i=0; i<10; i++) {
    if (setHighTimestamp[i] > 0 && millis() > setHighTimestamp[i]) {
      for (int j=0; j<=2; j++) {
        digitalWrite(getPin(i, j), HIGH);
      }
      setHighTimestamp[i] = 0;
      setTriStateTimestamp[i] = millis() + 100;
    }
    if (setTriStateTimestamp[i] > 0 && millis() > setTriStateTimestamp[i]) {
      for (int j=0; j<=2; j++) {
        pinMode(getPin(i, j), INPUT); // TriState
      }
      setTriStateTimestamp[i] = 0;
      readyForCommands = true;
    }
  }

  if (millis() > startTimestamp + 1000) {
    printStatus();
  }
}

void recvData() {
  if (Serial.available() > 0) {
      controlChar1 = Serial.read();
      controlChar2 = 0;
      controlCharTimestamp = millis();
      switch (controlChar1) {
        case '?':
          Serial.print(BOARD_NR);
          Serial.print("|");
          Serial.print(BOARD_TYPE);
          Serial.print("|");
          Serial.print(BOARD_PURPOSE);
          Serial.print("|");
          Serial.print("{"
                        "\"actions\": ["
                        "  {\"type\": \"get\", \"format\": \"text\", \"item\": \"status\", \"action\": \"s?\"},"
                        "  {\"type\": \"put\", \"text\": \"Play/pause station sound\", \"cmd\": \"s#p\"},"
                        "  {\"type\": \"put\", \"text\": \"Next station sound\", \"cmd\": \"s#>\"},"
                        "  {\"type\": \"put\", \"text\": \"Station sound louder\", \"cmd\": \"s#+\"},"
                        "  {\"type\": \"put\", \"text\": \"Station sound softer\", \"cmd\": \"s#-\"}"
                        "]"
                      "}");
          break;
        case '!':
          BOARD_RECOGNIZED = true;
          break;
        case 'x':
          toggleSoundStatus();
          if (SOUND_STATUS == SOUND_ON) {
            initTimestamp = millis();
            initFase = 1;
          }
          break;
        case 's':
          controlChar2 = Serial.read(); // Sound number 0 .. 9, or '?'
          if (controlChar2 == '?') {
            String status = "";
            for (int i=0; i<10; i++) {
              if (playing[i]) {
                if (status.length() > 0) {
                  status += ", ";
                }
                status += "#" + String(i);
              }
            }
            if (status.length() == 0) {
              status = "No sounds playing";
            } else {
              status = "Playing: " + status;
            }
            Serial.print(status);
          } else {
            int soundNr = int(controlChar2 - int('0'));
            controlChar3 = Serial.read(); // Command
            switch (controlChar3) {
              case '+':
                command(soundNr, cmd_volumeUp);
                break;
              case '-':
                command(soundNr, cmd_volumeDown);
                break;
              case 'p':
                command(soundNr, cmd_play_pause);
                break;
              case 'x':
                playing[soundNr] = false;
                command(soundNr, cmd_play_pause);
                break;
              case '>':
                command(soundNr, cmd_next);
                break;
              case '<':
                command(soundNr, cmd_previous);
                break;
            }
          }
          break;
      }
  }
}

int getPin(int soundNr, int pad) {
  return 22 + soundNr * 3 + pad;
}

int getOnOffPin(int soundNr) {
  return 43 + soundNr;
}
