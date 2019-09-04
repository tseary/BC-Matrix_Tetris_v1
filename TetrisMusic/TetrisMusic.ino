
/*
 * ATTINY program to play the Tetris theme music.
 */

 // The ATTINY is connected to the main processor as follows:
 // ATTINY85 | ATMEGA328P
 // RESET    - digital 10
 // MOSI     - digital 11
 // MISO     - digital 12
 // SCK      - digital 13
 // This is the same connection used by the Arduino-as-ISP programmer,
 // therefore no additional hardware is necessary to program the ATTINY.

 // The speaker pin connects, in series, to a 100 uF capacitor, 120 Ohm resistor,
 // 5 kOhm rheostat (volume control), and 8 Ohm speaker to ground.

#include "PitchValue.h"

// Differential audio output pins
// Both pins are low when idle
const byte SPEAKER_P_PIN = 4; // Non-inverted output
const byte SPEAKER_N_PIN = 3; // Inverted output

// The track currently being played
// Track 0 is special:
// - it will not play while the game level is zero
// - the track number is set to 0 after any other track is finished
uint8_t trackNumber = 0;
// Flag to indicate that the track was changed
bool trackChangeFlag = false;
// Flag to indicate that the game level was set to 1
bool gameStartFlag = false;

// The current game level, which determines the song tempo (level 0 = silence)
uint8_t gameLevel = 0;

// If false, all sounds are muted
bool soundOn = true;

// If true, the track waits to play the next note
bool paused = false;

void setup() {
	// Set the audio output pins
	pinMode(SPEAKER_P_PIN, OUTPUT);
	pinMode(SPEAKER_N_PIN, OUTPUT);

	// Calculate the length of each note value based on the 16th note
	calculateTempo();

	initializeCommand();
}

void loop() {
	// Wait in silence
	// This loop breaks when:
	// - the sound is on, AND
	// - the game level is not zero OR the track number is not zero
	while (!soundOn || (gameLevel == 0 && trackNumber == 0)) {
		// TODO low-power sleep if !soundOn
		delay(1);
		checkCommand();
	}

	// Unpause before entering the play loop
	paused = false;

	// Clear the track change flag before entering the play loop
	trackChangeFlag = false;
	gameStartFlag = false;

	// Play track
	// This loop breaks when:
	// - the last note in the track is played, OR
	// - the sound is off, OR
	// - the game level is zero AND the track number is zero, OR
	// - the track changes
	uint16_t decayDuration;
	for (uint16_t i = 0; i < getTrackLength(trackNumber) && soundOn &&
		!(gameLevel == 0 && trackNumber == 0); i++) {
		// Slur 16th notes
		decayDuration = getNoteValue(trackNumber, i) != Value::v16TH ?
			getDecayDuration() : getDecayDuration() / 2;

		// Play note
		badToneDifferential(SPEAKER_P_PIN, SPEAKER_N_PIN,
			getNoteFrequency(trackNumber, i),
			getNoteDuration(trackNumber, i) - decayDuration);

		// Silence between notes
		if (decayDuration > 0) {
			delay(decayDuration);
		}

		// Wait while paused
		do {
			checkCommand();

			// If the track changes, break the loop without resetting the track number
			if (trackChangeFlag || gameStartFlag) {
				return;
			}
		} while (paused);
	}

	// Return to default track after play is completed
	setTrackNumber(0);
}

void setGameLevel(uint8_t level) {
	gameStartFlag = level == 1;
	gameLevel = level;
	calculateTempo();
}

void setTrackNumber(uint8_t newTrack) {
	trackChangeFlag = newTrack != trackNumber;
	trackNumber = newTrack;
}

// Bit-bangs a tone on two pins with opposite phase
void badToneDifferential(byte pPin, byte nPin, uint16_t frequency, uint16_t duration) {
	// Rest
	if (frequency == 0) {
		if (duration != 0) {
			delay(duration);
		}
		return;
	}

	// Play note
	uint32_t period = max(2, 1000000 / frequency);  // Clamp to prevent 0 delay
	uint32_t halfPeriod = period / 2;
	uint32_t endTime = millis() + duration;
	do {
		digitalWrite(pPin, HIGH);
		delayMicroseconds(halfPeriod);
		digitalWrite(pPin, LOW);
		digitalWrite(nPin, HIGH);
		delayMicroseconds(period - halfPeriod);
		digitalWrite(nPin, LOW);
	} while (millis() < endTime);
}

