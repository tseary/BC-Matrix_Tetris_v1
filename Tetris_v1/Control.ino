
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
const byte L_BUTTON_PIN = 8; // brown wire
const byte R_BUTTON_PIN = 7; // gray wire
const byte D_BUTTON_PIN = 6; // green wire
const byte E_BUTTON_PIN = 4; // blue wire (encoder button)

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

long lastPosition = 0,
  nextPosition = 0;

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

bool isLPress() {
  return isLPressed;
}

bool isRPress() {
  return isRPressed;
}

bool isDPress() {
  return isDPressed;
}

bool isEPress() {
  return isEPressed;
}

void updateEncoder() {
  lastPosition = nextPosition;

  // Encoder detents are four counts apart
  long encoderValue = encoder.read();
  nextPosition = (encoderValue + (encoderValue >= 0 ? 2 : -2)) / 4;

#if DEBUG_SERIAL
  Serial.print("encoderValue = ");
  Serial.print(encoderValue);
  Serial.print("\tnextPosition = ");
  Serial.println(nextPosition);
#endif
}

// Gets the change in encoder position
// Positive = clockwise
long getEncoderChange() {
  return nextPosition - lastPosition;
}

