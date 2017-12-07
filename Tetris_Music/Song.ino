
// Note frequencies in Hertz (rounded to nearest integer)
const uint16_t NOTE_FREQUENCIES[26] = {
  0, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415,
  440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880};

// Note durations in milliseconds
uint16_t NOTE_DURATIONS[6];

// The silence at the end of each note in milliseconds
uint16_t DURA_DECAY = 0;

// TODO create Note struct with pitch and value

// The number of music tracks
const uint8_t TRACK_COUNT = 3;

// The number of notes and rests in each track
// track 0 = theme song
// track 1 = game over
// track 2 = high score
const uint16_t TRACK_LENGTHS[TRACK_COUNT] = {
  (8 + 6 + 5 + 4 +
  7 + 5 + 5 + 4 +
  2 + 2 + 2 + 3 +
  2 + 2 + 3 + 2), 5, 7;

// The pitches of notes in the theme song track
Pitch themeSongPitches[SONG_LENGTH] = {
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
  A4b, B4, REST,
  E4, C4,
  D4, B4,
  C4, E4, A5n,
  A5b, REST};

// The pitches of notes in the game over track
Pitch gameOverPitches[SONG_LENGTH] = {
  E4, D4, C4, B4, A4n};

// The pitches of notes in the high score track
Pitch highScorePitches[SONG_LENGTH] = {
  C4, C4, C4, D4, C4, D4, E4};

// Array of pointers to track pitch arrays
Pitch *trackPitches[TRACK_COUNT] = {&songPitches, &gameOverPitches, &highScorePitches};

// The values of notes in the theme song track
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
  v4TH, v4TH, v2ND,
  v2ND, v2ND};

// The values of notes in the game over track
Value gameOverValues[SONG_LENGTH] = {
  v8TH, v8TH, v8TH, v8TH, v2ND};

// The values of notes in the high score track
Value highScoreValues[SONG_LENGTH] = {
  v2ND, v8TH, v4TH_DOT, v4TH, v4TH, v4TH, v2ND};

// Array of pointers to track value arrays
Value *trackValues[TRACK_COUNT] = {&songValues, &gameOverValues, &highScoreValues};

/* GAME OVER ********************************************************/

// The number of notes and rests in the song
const uint16_t GAME_OVER_LENGTH = 3;

// The pitches of notes in the song
Pitch gameOverPitches[GAME_OVER_LENGTH] = {
  D4, C4, A4n};
  
// The values of notes in the song
Value gameOverValues[GAME_OVER_LENGTH] = {
  v4TH, v4TH, v2ND};

uint16_t getTrackLength(uint8_t track) {
  return TRACK_LENGTHS[track];
}

uint16_t getNoteFrequency(uint8_t track, uint16_t noteIndex) {
  return NOTE_FREQUENCIES[trackPitches[track][noteIndex]];
}

uint16_t getNoteDuration(uint8_t track, uint16_t noteIndex) {
  return NOTE_DURATIONS[getNoteValue(track, noteIndex)];
}

Value getNoteValue(uint8_t track, uint16_t noteIndex) {
  return trackValues[track][noteIndex];
}

uint16_t getDecayDuration() {
  return DURA_DECAY;
}

// Calculate note values using the game level to determine tempo
void calculateTempo() {
  calculateTempo(100 - 6 * gameLevel);
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

