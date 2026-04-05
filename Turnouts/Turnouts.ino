#define BOARD_PURPOSE "Turnouts"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define NUM_MESSAGE_LINES 6

#define STRAIGHT LOW
#define TURNOUT  HIGH

#define NUM_TURNOUTS 7
#define MAIN_SWITCH 8

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String messages[NUM_MESSAGE_LINES] = {"", "", "", "", "", ""};

bool isPrinting = false;

String setSwitch(int switchNr, char direction, bool activate=true) {
  int pin = get_pin(switchNr);
  digitalWrite(pin, direction == '0' ? STRAIGHT : TURNOUT);
  if (activate) {
    activateSwitches();
  }
  String msg = "Turnout ";
  msg += pin;
  msg += direction == '0' ? " straight" : " diverge";
  msg += direction;
  addMessage(msg);
  return msg;
}

int get_pin(int nr) {
  return nr + 4;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  // Setup display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.display();

  // Setup Turnouts
  for (int pin=0; pin<=NUM_TURNOUTS; pin++) {
    pinMode(get_pin(pin), OUTPUT);
  }
 
  digitalWrite(get_pin(MAIN_SWITCH), LOW);

  for (int pin=0; pin<NUM_TURNOUTS; pin++) {
    pinMode(get_pin(pin), OUTPUT);
    setSwitch(pin, TURNOUT, false);
  }
  delay(500);
  activateSwitches();
  delay(500);
  for (int pin=1; pin<NUM_TURNOUTS; pin++) {
    setSwitch(pin, STRAIGHT, false);
  }
  delay(500);
  activateSwitches();

  // Flush receive buffer
  while (Serial.available() > 0) {
    char ch = Serial.read();
  }
}

void loop() {
  // internal LED short blink
  digitalWrite(LED_BUILTIN, (millis() % 2000 <= 200 ? HIGH : LOW));

  recvData();
  printStatus();
}

void addMessage(String message) {
  for (int i=NUM_MESSAGE_LINES-2; i>=0; i--) {
    messages[i+1] = messages[i];
  }
  messages[0] = message;
  printStatus();
}

void activateSwitches() {
  digitalWrite(get_pin(MAIN_SWITCH - 1), LOW);
  delay(250);
  digitalWrite(get_pin(MAIN_SWITCH - 1), HIGH);
}

void recvData() {
  if (Serial.available() > 0) {
      char controlChar1 = Serial.read();
      switch (controlChar1) {
        case '?':
          Serial.print("{\"id\":\"eTrack Turnouts\"}");
          break;
        case 's': {
          // Set switch on/off
          char controlChar2 = Serial.read();
          char controlChar3 = Serial.read();
          char controlChar4 = Serial.read();
          int switchNr = 10 * (controlChar2 - '0') + (controlChar3 - '0');
          String msg = setSwitch(switchNr, controlChar4);
          Serial.print(msg + "\n");
        }
        break;
        case 'a': {
          // Set all switches
          activateSwitches();
          Serial.print("Activated all switches\n");
        }
        break;
      }
  }
}

void printStatus() {
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  display.clearDisplay();
  
  display.setTextColor(BLACK, WHITE);
  display.println(BOARD_PURPOSE);

  display.setTextColor(WHITE, BLACK);
  display.setTextSize(1);
  for (String message : messages) {
    display.println(message);
  }
  display.display();
}

