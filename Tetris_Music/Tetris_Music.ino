
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

// The speaker pin connects, in series, to a 100 uF capacitor, 150 Ohm resistor,
// 5 kOhm rheostat (volume control), and 8 Ohm speaker to ground.

// Symbols for note pitches (and indexes for NOTE_FREQUENCIES[])
enum Pitch : byte {
  REST, A3n, B3b, B3, C3, D3b, D3, E3b, E3, F3, G3b, G3, A4b,
  A4n, B4b, B4, C4, D4b, D4, E4b, E4, F4, G4b, G4, A5b, A5n
};

// Symbols for note values (and indexes for NOTE_DURATIONS[])
enum Value : byte {
  v16TH, v8TH, v8TH_DOT, v4TH, v4TH_DOT, v2ND, v1ST
  //SEMIQUAVER, QUAVER, QUAVER_DOT, CROTCHET, CROTCHET_DOT, MINIM, BREVE
};

const byte SPEAKER_PIN = 4;

// The current game level, which determines the song tempo (level 0 = silence)
byte gameLevel = 1;

// TODO Move pitches, values and song to separate file

void setup() {
  // Set the audio output pin
  pinMode(SPEAKER_PIN, OUTPUT);
  
  // Calculate the length of each note value based on the 16th note
  calculateTempo();
  
  initializeCommand();
}

void loop() {
  // Wait in silence
  while (gameLevel == 0) {
    checkCommand();
  }
  
  // Play song
  uint16_t decayDuration;
  for (uint16_t i = 0; i < getSongLength() && gameLevel > 0; i++) {
    // Slur 16th notes
    decayDuration = getNoteValue(i) > Value::v16TH ?
        getDecayDuration() : getDecayDuration() / 2;

    // Play note
    badTone(SPEAKER_PIN, getNoteFrequency(i), getNoteDuration(i) - decayDuration);
    
    // Silence between notes
    if (decayDuration > 0) {
      delay(decayDuration);
    }
    
    checkCommand();
  }
}

void badTone(byte pin, uint16_t frequency, uint16_t duration) {
  // Rest
  if (frequency == 0) {
    if (duration != 0) {
      delay(duration);
    }
    return;
  }

  // Play note
  uint32_t halfPeriod = max(1, 500000 / frequency);
  uint32_t endTime = millis() + duration;
  do {
    digitalWrite(pin, HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(pin, LOW);
    delayMicroseconds(halfPeriod);
  } while (millis() < endTime);
}

