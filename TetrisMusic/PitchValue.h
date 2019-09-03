#pragma once

// Symbols for note pitches (and indexes for NOTE_FREQUENCIES[])
enum Pitch : byte {
	REST, A3n, B3b, B3, C3, D3b, D3, E3b, E3, F3, G3b, G3, A4b,
	A4n, B4b, B4, C4, D4b, D4, E4b, E4, F4, G4b, G4, A5b, A5n
};

// Symbols for note values (and indexes for NOTE_DURATIONS[])
enum Value : byte {
	v16TH, v8TH, v8TH_DOT, v4TH, v4TH_DOT, v2ND, v2ND_DOT, v1ST,
	VALUE_COUNT
};
