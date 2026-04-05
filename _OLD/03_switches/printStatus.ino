void printStatus() {
  display.setTextSize(2);      // Normal 1:1 pixel scale
  display.setTextColor(WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  display.clearDisplay();
  if (millis() - controlCharTimestamp < 1000) {
    int c1 = controlChar1;
    int c2 = controlChar2;
    int c3 = controlChar3;
    int c4 = controlChar4;
    display.print(c1);
    display.print(" ");
    display.print(c2);
    display.print(" ");
    display.print(c3);
    display.print(" ");
    display.println(c4);
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
  display.setTextSize(1);
  for (String action : actions) {
    display.println(action);
  }
  display.display();
}
