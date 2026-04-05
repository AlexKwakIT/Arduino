void flushData() {
  while (Serial.available() > 0) {
    char ch = Serial.read();
  }
}
