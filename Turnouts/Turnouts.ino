#define BOARD_PURPOSE "Turnouts"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define NUM_MESSAGE_LINES 6

#define STRAIGHT HIGH
#define TURNOUT  LOW

#define ACTIVATE_SWITCHES_PIN 11

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

String messages[NUM_MESSAGE_LINES] = {"", "", "", "", "", ""};

bool isPrinting = false;

String setSwitch(int pinNr, char direction, bool activate=true) {
  pinMode(pinNr, OUTPUT);
  digitalWrite(pinNr, direction == 'S' ? STRAIGHT : TURNOUT);
  if (activate) {
    activateTurnouts();
  }
  String msg = "Turnout ";
  msg += pinNr;
  msg += direction == 'S' ? " straight" : " diverge";
  addMessage(msg);
  return msg;
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(ACTIVATE_SWITCHES_PIN, OUTPUT);
  digitalWrite(ACTIVATE_SWITCHES_PIN, HIGH);

  // Setup display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.display();

  // Flush receive buffer
  while (Serial.available() > 0) {
    char ch = Serial.read();
  }
}

void loop() {
  // internal LED short blink
  // digitalWrite(LED_BUILTIN, (millis() % 2000 <= 200 ? HIGH : LOW));
  digitalWrite(LED_BUILTIN, HIGH);

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

void activateTurnouts() {
  digitalWrite(ACTIVATE_SWITCHES_PIN, LOW);
  delay(250);
  digitalWrite(ACTIVATE_SWITCHES_PIN, HIGH);
}

void recvData() {
  if (Serial.available() > 0) {
    // char controlChar1 = Serial.read();
    String cmd = Serial.readStringUntil('\n');
    switch (cmd[0]) {
      case '?':
        Serial.println("{\"id\":\"eTrack Turnouts\"}");
        break;
      case 's': {
          if (cmd.length() == 4) {
            int switchNr = 10 * (cmd[1] - '0') + (cmd[2] - '0');
            char direction = cmd[3];

            String msg = setSwitch(switchNr, direction);
            Serial.println(msg);
          } else {
            Serial.print("Invalid command ");
            Serial.println(cmd);
          }
      }
      break;
      case 'a': {
        // Set all switches
        //activateTurnouts();
        Serial.println("Activated all turnouts");
      }
      break;
    }
    // Flush receive buffer
    while (Serial.available() > 0) {
      char ch = Serial.read();
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

