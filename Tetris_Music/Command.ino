
// Master communication pins
const byte
//  MOSI_PIN = 0, // Data input
//  MISO_PIN = 1, // Data output
  OPCODE_LSB_PIN = 0, // Data input
  OPCODE_MSB_PIN = 1, // Data input
  SCK_PIN = 2;  // Clock input (PCINT2)

// Master communication fields
volatile bool commandReady = false;
volatile byte commandOpcode = 0x00;
volatile byte commandCounter = 0;

// Commands - top four bits = counter, bottom four bits = opcode
const byte
  COMMAND_SILENCE = 0x00,
  COMMAND_NEXT_LEVEL = 0x11;

void initializeCommand() {
  // Set up master communication
  pinMode(OPCODE_LSB_PIN, INPUT);
  pinMode(OPCODE_MSB_PIN, INPUT);
  pinMode(SCK_PIN, INPUT);
  
  // Set up pin change interrupts on SCK
  // See https://thewanderingengineer.com/2014/08/11/pin-change-interrupts-on-attiny85/
  GIMSK = 0b00100000;   // turns on pin change interrupts
//PCMSK = 0b00000100;   // turn on interrupts on pin PB2
  PCMSK = 1 << SCK_PIN; // turn on interrupts on SCK pin
  sei();  // enable interrupts
}

void checkCommand() {
  // Return if there is no command ready
  if (!commandReady) {
    return;
  }
  
  noInterrupts();

  // Combine counter and opcode
  commandOpcode |= commandCounter << 4;
  
  // Do command
  switch (commandOpcode) {
    case COMMAND_SILENCE:
      gameLevel = 0;
      break;
    case COMMAND_NEXT_LEVEL:
      gameLevel++;
      calculateTempo();
      break;
  }
  
  clearCommand();
  
  interrupts();
}

void clearCommand() {
  noInterrupts();
  commandOpcode = 0;
  commandCounter = 0;
  commandReady = false;
  interrupts();
}

// SCK pin change interrupt - receive commands from master
ISR(PCINT0_vect) {
  // Receive commands from master
  if (digitalRead(SCK_PIN)) {
    // Rising edge, first half of opcode
    commandOpcode =
        (digitalRead(OPCODE_LSB_PIN) ? 0b0001 : 0) |
        (digitalRead(OPCODE_MSB_PIN) ? 0b0010 : 0);
  } else {
    // Falling edge, second half of opcode
    commandOpcode |=
        (digitalRead(OPCODE_LSB_PIN) ? 0b0100 : 0) |
        (digitalRead(OPCODE_MSB_PIN) ? 0b1000 : 0);

    // Increment the command counter, or set the command ready flag
    if (commandOpcode != 0x0f) {
      commandReady = true;
    } else {
      commandCounter++; // TODO counter >= 16 is an error
    }
  }
}

