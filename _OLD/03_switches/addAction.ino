void addAction(String action) {
  for (int i=4; i>=0; i--) {
    actions[i+1] = actions[i];
  }
  actions[0] = action;
  printStatus();
}
