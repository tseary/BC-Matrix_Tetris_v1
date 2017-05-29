
const byte NAV_SWITCH_PIN = A3;

bool isDPressed = false;
bool isUPressed = false;
bool isRPressed = false;
bool isLPressed = false;
bool isCPressed = false;

bool wasDPressed = false;
bool wasUPressed = false;
bool wasRPressed = false;
bool wasLPressed = false;
bool wasCPressed = false;

void initializeControl() {
  pinMode(NAV_SWITCH_PIN, INPUT);
}

// Reads the buttons etc.
void updateControl() {
  int analogValue = analogRead(NAV_SWITCH_PIN);
  
  // Store previous button states
  wasDPressed = isDPressed;
  wasUPressed = isUPressed;
  wasRPressed = isRPressed;
  wasLPressed = isLPressed;
  wasCPressed = isCPressed;
  
  // Clear buttons
  isDPressed = false;
  isUPressed = false;
  isRPressed = false;
  isLPressed = false;
  isCPressed = false;
  
  // Check analog input
  if (analogValue > 931) {
    // No button
  } else if (analogValue > 794) {
    // Down
    isDPressed = true;
  } else if (analogValue > 683) {
    // Up
    isUPressed = true;
  } else if (analogValue > 514) {
    // Right
    isRPressed = true;
  } else if (analogValue > 206) {
    // Left
    isLPressed = true;
  } else {
    // Center
    isCPressed = true;
  }
}

bool isDClick() {
  return !wasDPressed && isDPressed;
}

bool isUClick() {
  return !wasUPressed && isUPressed;
}

bool isRClick() {
  return !wasRPressed && isRPressed;
}

bool isLClick() {
  return !wasLPressed && isLPressed;
}

bool isCClick() {
  return !wasCPressed && isCPressed;
}

bool isDPress() {
  return isDPressed;
}

