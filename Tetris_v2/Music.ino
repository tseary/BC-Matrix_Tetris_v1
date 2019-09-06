
const uint8_t
MUSIC_RESET_PIN = 10,	// ATTINY reset pin
OPCODE_LSB_PIN = 11,	// Data output (mosi)
OPCODE_MSB_PIN = 12,	// Data output (miso)
SCK_PIN = 13;			// Clock output

const uint16_t OPCODE_SETTLING_MICROS = 100;
const uint16_t OPCODE_PROCESSING_MICROS = 300;

void initializeMusic() {
	// Reset the music controller
	pinMode(MUSIC_RESET_PIN, OUTPUT);
	digitalWrite(MUSIC_RESET_PIN, LOW);
	delayMicroseconds(5);	// Minimum reset pulse width is 2.5 us
	digitalWrite(MUSIC_RESET_PIN, HIGH);

	pinMode(OPCODE_LSB_PIN, OUTPUT);
	pinMode(OPCODE_MSB_PIN, OUTPUT);

	pinMode(SCK_PIN, OUTPUT);
	digitalWrite(SCK_PIN, LOW);
}

// A command may be comprised of one or more opcodes
void sendMusicCommand(uint8_t command) {
	uint8_t counter = command >> 4;
	for (uint8_t i = 0; i < counter; i++) {
		sendOpcode(0x0f);
	}
	sendOpcode(command);  // Top four bits are ignored
}

void sendOpcode(uint8_t opcode) {
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

