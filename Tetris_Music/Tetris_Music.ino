
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

// Symbols for note pitches (and indexes for NOTE_FREQUENCIES[])
enum Pitch : byte {
  REST, A3n, B3b, B3, C3, D3b, D3, E3b, E3, F3, G3b, G3, A4b,
  A4n, B4b, B4, C4, D4b, D4, E4b, E4, F4, G4b, G4, A5b, A5n
};

// Symbols for note values (and indexes for NOTE_DURATIONS[])
enum Value : byte {
  v16TH, v8TH, v8TH_DOT, v4TH, v4TH_DOT, v2ND, v1ST
};

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

// The current game level, which determines the song tempo (level 0 = silence)
uint8_t gameLevel = 0;

// If false, all sounds are muted
bool soundOn = true;

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
  while (gameLevel == 0 || !soundOn) {
    checkCommand();
  }

  // Clear the track change flag before entering the play loop
  trackChangeFlag = false;
  
  // Play track
  // This loop breaks when:
  // - the last note in the track is played, OR
  // - the game level is zero and the track number is zero, OR
  // - the sound is off, OR
  // - the track was changed
  uint16_t decayDuration;
  for (uint16_t i = 0; i < getTrackLength(trackNumber) &&
      (gameLevel > 0 || trackNumber > 0) && soundOn && !trackChangeFlag; i++) {
    // Slur 16th notes
    decayDuration = getNoteValue(trackNumber, i) != Value::v16TH ?
        getDecayDuration() : getDecayDuration() / 2;

    // Play note
    badToneDifferential(SPEAKER_P_PIN, SPEAKER_N_PIN, getNoteFrequency(i), getNoteDuration(i) - decayDuration);
    
    // Silence between notes
    if (decayDuration > 0) {
      delay(decayDuration);
    }
    
    checkCommand();
  }

  // Return to default track after play is completed
  setTrackNumber(0);
}

void setTrackNumber(uint8_t track) {
  trackChangeFlag = true;
  trackNumber = track;
}

void setGameLevel(uint8_t level) {
  gameLevel = level;
  calculateTempo();
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

