
// The game is controlled by a 5-way navigation switch, which
// is interfaced through a single analog pin. The switch connects
// a different resistor to ground for each direction:
// down   = 47k
// up     = 27k
// right  = 15k
// left   = 6.8k
// center = short
// There is also an external 10k pull-up resistor to 5 V.
// The five buttons are mutually exclusive, meaning that pressing
// multiple buttons simultaneously may cause undesired behaviour.

// This file manages debouncing and detects button clicks.

#include <Encoder.h>

// Buttons
const byte L_BUTTON_PIN = 8; // brown
const byte R_BUTTON_PIN = 7; // gray
const byte D_BUTTON_PIN = 6; // green
const byte E_BUTTON_PIN = 4; // blue

bool isLPressed = false;
bool isRPressed = false;
bool isDPressed = false;
bool isEPressed = false;

bool wasLPressed = false;
bool wasRPressed = false;
bool wasDPressed = false;
bool wasEPressed = false;

// Encoder
// The encoder pins are set to INPUT_PULLUP inside the Encoder class
const byte ENCODER_A_PIN = 3; // yellow
const byte ENCODER_B_PIN = 2; // red

Encoder encoder(ENCODER_A_PIN, ENCODER_B_PIN);

long lastPosition = 0;
long nextPosition = 0;

void initializeControl() {
  // Active-low pushbuttons
  pinMode(L_BUTTON_PIN, INPUT_PULLUP);
  pinMode(R_BUTTON_PIN, INPUT_PULLUP);
  pinMode(D_BUTTON_PIN, INPUT_PULLUP);
  pinMode(E_BUTTON_PIN, INPUT_PULLUP);
}

// Reads the buttons etc.
void updateControl() {
  // Store previous button states
  wasLPressed = isLPressed;
  wasRPressed = isRPressed;
  wasDPressed = isDPressed;
  wasEPressed = isEPressed;
  
  // Read buttons
  isLPressed = !digitalRead(L_BUTTON_PIN);
  isRPressed = !digitalRead(R_BUTTON_PIN);
  isDPressed = !digitalRead(D_BUTTON_PIN);
  isEPressed = !digitalRead(E_BUTTON_PIN);
  
  updateEncoder();
  
  // Check analog input
  /*if (analogValue > 931) {
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
  }*/
}

bool isLClick() {
  return !wasLPressed && isLPressed;
}

bool isRClick() {
  return !wasRPressed && isRPressed;
}

bool isDClick() {
  return !wasDPressed && isDPressed;
}

bool isEClick() {
  return !wasEPressed && isEPressed;
}

bool isDPress() {
  return isDPressed;
}

void updateEncoder() {
  lastPosition = nextPosition;
  nextPosition = (encoder.read() + 2) / 4;  // Encoder detents are spaces four counts apart
}

// Gets the change in encoder position (4 counts = 1 detent)
// Positive = clockwise
long getEncoderChange() {
  return nextPosition - lastPosition;
}

