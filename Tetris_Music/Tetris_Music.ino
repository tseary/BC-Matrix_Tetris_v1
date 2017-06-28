
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

// Master communication pins
const byte
  MOSI_PIN = 0, // Data input
  MISO_PIN = 1, // Data output
  SCK_PIN = 2;  // Clock input

const byte SPEAKER_PIN = 4;

// TODO Move pitches, values and song to separate file

// Symbols for note pitches (and indexes for NOTE_FREQUENCIES[])
enum Pitch : byte {
  REST, A3n, B3b, B3, C3, D3b, D3, E3b, E3, F3, G3b, G3, A4b,
  A4n, B4b, B4, C4, D4b, D4, E4b, E4, F4, G4b, G4, A5b, A5n
};

// Note frequencies in Hertz (rounded to nearest integer)
const uint16_t NOTE_FREQUENCIES[26] = {
  0, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415,
  440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880};

// Symbols for note values (and indexes for NOTE_DURATIONS[])
enum Value : byte {
  v16TH, v8TH, v8TH_DOT, v4TH, v4TH_DOT, v2ND, v1ST
  //SEMIQUAVER, QUAVER, QUAVER_DOT, CROTCHET, CROTCHET_DOT, MINIM, BREVE
};

// Note durations in milliseconds
uint16_t NOTE_DURATIONS[6];

// The silence at the end of each note
uint16_t DURA_DECAY = 0;

// The number of notes and rests in the song
const uint16_t SONG_LENGTH =
  8 + 6 + 5 + 4 +
  7 + 5 + 5 + 4 +
  2 + 2 + 2 + 3 +
  2 + 2 + 2 + 3;

// The pitches of notes in the song
Pitch songPitches[SONG_LENGTH] = {
  E4, B4, C4, D4, E4, D4, C4, B4,
  A4n, A4n, C4, E4, D4, C4,
  B4, B4, C4, D4, E4,
  C4, A4n, A4n, REST,
  REST, D4, D4, F4, A5n, G4, F4,
  E4, C4, E4, D4, C4,
  B4, B4, C4, D4, E4,
  C4, A4n, A4n, REST,
  E4, C4,
  D4, B4,
  C4, A4n,
  G3b, B4, REST,
  E4, C4,
  D4, B4,
  C4, A4n,
  G3b, B4, REST};
  
// The values of notes in the song
Value songValues[SONG_LENGTH] = {
  v4TH, v8TH, v8TH, v8TH, v16TH, v16TH, v8TH, v8TH,
  v4TH, v8TH, v8TH, v4TH, v8TH, v8TH,
  v4TH, v8TH, v8TH, v4TH, v4TH,
  v4TH, v4TH, v4TH, v4TH,
  v8TH, v8TH, v8TH, v8TH, v4TH, v8TH, v8TH,
  v4TH_DOT, v8TH, v4TH, v8TH, v8TH,
  v4TH, v8TH, v8TH, v4TH, v4TH,
  v4TH, v4TH, v4TH, v4TH,
  v2ND, v2ND,
  v2ND, v2ND,
  v2ND, v2ND,
  v2ND, v4TH, v4TH,
  v2ND, v2ND,
  v2ND, v2ND,
  v2ND, v2ND,
  v2ND, v4TH, v4TH};

void setup() {
  pinMode(SPEAKER_PIN, OUTPUT);
  
  // Calculate the length of each note value based on the 16th note
  calculateTempo(60);
}

void loop() {
  
  // DEBUG Random tempo
  calculateTempo((millis() % 4) * 20 + 40);
  
  uint16_t durationDecay = DURA_DECAY;
  for (uint16_t i = 0; i < SONG_LENGTH; i++) {
    // Slur 16th notes
    durationDecay = songValues[i] > v16TH ? DURA_DECAY : (DURA_DECAY / 2);

    // Play note
    badTone(SPEAKER_PIN, NOTE_FREQUENCIES[songPitches[i]],
        NOTE_DURATIONS[songValues[i]] - durationDecay);

    // Silence between notes
    delay(durationDecay);
  }
}

void badTone(byte pin, uint16_t frequency, uint16_t duration) {
  // Rest
  if (frequency == 0) {
    delay(duration);
    return;
  }

  // Play note
  uint32_t halfPeriod = 500000 / frequency;
  uint32_t endTime = millis() + duration;
  do {
    digitalWrite(pin, HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(pin, LOW);
    delayMicroseconds(halfPeriod);
  } while (millis() < endTime);
}

// Calculate note values based on the length of a 16th note in milliseconds
void calculateTempo(uint16_t v16thDuration) {
  NOTE_DURATIONS[0] = v16thDuration;
  NOTE_DURATIONS[1] = 2 * v16thDuration;
  NOTE_DURATIONS[2] = 3 * v16thDuration;
  NOTE_DURATIONS[3] = 4 * v16thDuration;
  NOTE_DURATIONS[4] = 6 * v16thDuration;
  NOTE_DURATIONS[5] = 8 * v16thDuration;
  DURA_DECAY = v16thDuration / 6;
}

