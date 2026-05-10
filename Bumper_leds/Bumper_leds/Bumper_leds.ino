#define BOARD_PURPOSE "Bumpers"

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

#define NUM_MESSAGE_LINES 6

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct PWMChannel {
  int pin;
  int fadeToMaxTime;
  int holdTimeMax;
  int fadeToMinTime;
  int holdTimeMin;
};

PWMChannel channels[] = {
  {3, 4000, 2000, 500, 300},
  {5, 4500, 2000, 750, 200},
  {6, 5000, 2000, 1000, 350},
  {9, 4000, 1000, 500, 250},
  {10, 4500, 1000, 750, 300},
  {11, 5000, 1000, 1000, 200},
};

const int numChannels = sizeof(channels) / sizeof(channels[0]);

void setup() {
  Serial.begin(9600);  // start serial communication
  for (int i = 0; i < numChannels; i++) {
    pinMode(channels[i].pin, OUTPUT);
  }

    // Setup display
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.display();
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(255);  // medium brightness
  printStatus();
}

void loop() {
  unsigned long currentMillis = millis();

  for (int i = 0; i < numChannels; i++) {
    PWMChannel &ch = channels[i];

    int cycleTime = currentMillis % (ch.fadeToMaxTime + ch.holdTimeMax + ch.fadeToMinTime + ch.holdTimeMin);
    if (cycleTime <= ch.fadeToMaxTime) {
      int val = int(255 * (float(cycleTime) / float(ch.fadeToMaxTime)));
      analogWrite(ch.pin, val);
    } else if (cycleTime <= ch.fadeToMaxTime + ch.holdTimeMax) {
      analogWrite(ch.pin, 255);
    } else if (cycleTime <= ch.fadeToMaxTime + ch.holdTimeMax + ch.fadeToMinTime) {
      int val = 255 - int(255 * (float(cycleTime - ch.fadeToMaxTime - ch.holdTimeMax) / float(ch.fadeToMinTime)));
      analogWrite(ch.pin, val);
    } else{
      analogWrite(ch.pin, 0);
    }
  }
  recvData();
}

void recvData() {
  if (Serial.available() > 0) {
      char controlChar1 = Serial.read();
      switch (controlChar1) {
        case '?':
          Serial.print("{\"id\":\"eTrack Bumpers\"}");
          break;
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

  for (int i = 0; i < numChannels; i++) {
    PWMChannel &ch = channels[i];
    display.print("using pin ");
    display.println(ch.pin);
  }

  display.display();
}
