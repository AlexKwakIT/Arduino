void loop() {
  // internal LED blink
  digitalWrite(LED_BUILTIN, (millis() % 2000 <= 200 ? HIGH : LOW));

  recvData();

  if (initFase < 99) {
    if (millis() > initTimestamp + 500) {
      init(initFase);
      initTimestamp = millis();
      initFase += 1;
    }
  }

  if (millis() > startTimestamp + 1000) {
    printStatus();
  }
}