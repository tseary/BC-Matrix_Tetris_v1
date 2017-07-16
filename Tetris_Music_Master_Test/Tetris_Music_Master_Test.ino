
const byte
  OPCODE_LSB_PIN = 11,  // Data output
  OPCODE_MSB_PIN = 12,  // Data output
  SCK_PIN = 13; // Clock output

// Commands - top four bits = counter, bottom four bits = opcode
const byte
  COMMAND_SILENCE = 0x00,
  COMMAND_NEXT_LEVEL = 0x01;

const uint16_t OPCODE_SETTLING_MICROS = 100;
const uint16_t OPCODE_PROCESSING_MICROS = 300;
  
void setup() {
  pinMode(OPCODE_LSB_PIN, OUTPUT);
  pinMode(OPCODE_MSB_PIN, OUTPUT);
  pinMode(SCK_PIN, OUTPUT);
  digitalWrite(SCK_PIN, LOW);
}

void loop() {
  sendCommand(COMMAND_SILENCE);
  delay(3);
//  sendCommand(COMMAND_NEXT_LEVEL);
  delay(2000);

  for (byte i = 0; i < 10; i++) {
    sendCommand(COMMAND_NEXT_LEVEL);
    delay(2000);
  }
}

// A command may be comprised of one or more opcodes
void sendCommand(byte command) {
  byte counter = command >> 4;

  for (byte i = 0; i < counter; i++) {
    sendOpcode(0x0f);
  }

  sendOpcode(command);  // Top four bits are ignored
}

void sendOpcode(byte opcode) {
  // First half
  digitalWrite(OPCODE_LSB_PIN, opcode & 0b0001);
  digitalWrite(OPCODE_MSB_PIN, opcode & 0b0010);
  delayMicroseconds(OPCODE_SETTLING_MICROS);
  digitalWrite(SCK_PIN, HIGH);
  delayMicroseconds(OPCODE_PROCESSING_MICROS);

  // Second half
  digitalWrite(OPCODE_LSB_PIN, opcode & 0b0100);
  digitalWrite(OPCODE_MSB_PIN, opcode & 0b1000);
  delayMicroseconds(OPCODE_SETTLING_MICROS);
  digitalWrite(SCK_PIN, LOW);
  delayMicroseconds(OPCODE_PROCESSING_MICROS);
}

