#include <Arduino.h>

#define DE_RE 2
#define NUM_NODES 3  // Number of Nanos

void setup() {
  pinMode(DE_RE, OUTPUT);
  digitalWrite(DE_RE, LOW); // Start in receive mode

  Serial.begin(9600);       // USB for PC
  Serial1.begin(9600);      // RS-485 bus on Serial1 (Mega) or SoftwareSerial on Uno
}

void loop() {
  for (int node = 1; node <= NUM_NODES; node++) {
    // Send poll to node
    digitalWrite(DE_RE, HIGH);
    delay(1);
    Serial1.write(node);      // Address node
    delay(1);
    digitalWrite(DE_RE, LOW);

    // Wait for response
    unsigned long start = millis();
    while (millis() - start < 100) {
      if (Serial1.available()) {
        String response = Serial1.readStringUntil('\n');
        Serial.println(response); // Forward to PC
      }
    }
  }
}