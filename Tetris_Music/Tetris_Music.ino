
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

const byte SPEAKER_PIN = 4;

// Note frequencies in Hertz (rounded to nearest integer)
const uint16_t
  FREQ_REST = 0,
  FREQ_A4 = 440,
//FREQ_B4b = 466,
  FREQ_B4 = 494,
  FREQ_C4 = 523,
//FREQ_D4b = 554,
  FREQ_D4 = 587,
//FREQ_E4b = 622,
  FREQ_E4 = 659,
  FREQ_F4 = 698,
//FREQ_G4b = 740,
  FREQ_G4 = 784,
//FREQ_A4b = 831,
  FREQ_A5 = 880;

// Note durations in milliseconds
const uint16_t
  DURA_16TH = 60,               // sixteenth note (100 = slow, 40 = fast)
  DURA_8TH = 2 * DURA_16TH,     // eighth note 
  DURA_8TH_DOT = 3 * DURA_16TH, // dotted eighth note
  DURA_4TH = 2 * DURA_8TH,      // quarter note
  DURA_4TH_DOT = 3 * DURA_8TH;  // dotted quarter note

// The silence at the end of each note
const uint16_t DURA_DECAY = DURA_16TH / 6;

// The number of notes and rests in the song
const uint16_t SONG_LENGTH = 8 + 6 + 5 + 4 + 7 + 5 + 5 + 4;

// The frequencies of notes in the song
uint16_t songFrequencies[SONG_LENGTH] = {
  FREQ_E4, FREQ_B4, FREQ_C4, FREQ_D4, FREQ_E4, FREQ_D4, FREQ_C4, FREQ_B4,
  FREQ_A4, FREQ_A4, FREQ_C4, FREQ_E4, FREQ_D4, FREQ_C4,
  FREQ_B4, FREQ_B4, FREQ_C4, FREQ_D4, FREQ_E4,
  FREQ_C4, FREQ_A4, FREQ_A4, FREQ_REST,
  FREQ_REST, FREQ_D4, FREQ_D4, FREQ_F4, FREQ_A5, FREQ_G4, FREQ_F4,
  FREQ_E4, FREQ_C4, FREQ_E4, FREQ_D4, FREQ_C4,
  FREQ_B4, FREQ_B4, FREQ_C4, FREQ_D4, FREQ_E4,
  FREQ_C4, FREQ_A4, FREQ_A4, FREQ_REST};
  
// The durations of notes in the song
uint16_t songDurations[SONG_LENGTH] = {
  DURA_4TH, DURA_8TH, DURA_8TH, DURA_8TH, DURA_16TH, DURA_16TH, DURA_8TH, DURA_8TH,
  DURA_4TH, DURA_8TH, DURA_8TH, DURA_4TH, DURA_8TH, DURA_8TH,
  DURA_4TH, DURA_8TH, DURA_8TH, DURA_4TH, DURA_4TH,
  DURA_4TH, DURA_4TH, DURA_4TH, DURA_4TH,
  DURA_8TH, DURA_8TH, DURA_8TH, DURA_8TH, DURA_4TH, DURA_8TH, DURA_8TH,
  DURA_4TH_DOT, DURA_8TH, DURA_4TH, DURA_8TH, DURA_8TH,
  DURA_4TH, DURA_8TH, DURA_8TH, DURA_4TH, DURA_4TH,
  DURA_4TH, DURA_4TH, DURA_4TH, DURA_4TH};

void setup() {
  pinMode(SPEAKER_PIN, OUTPUT);
}

void loop() {
  uint16_t durationDecay = DURA_DECAY;
  for (uint16_t i = 0; i < SONG_LENGTH; i++) {
    durationDecay = songDurations[i] > DURA_16TH ? DURA_DECAY : (DURA_DECAY / 2);
    badTone(SPEAKER_PIN, songFrequencies[i], songDurations[i] - durationDecay);
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

