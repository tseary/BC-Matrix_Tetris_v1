
const uint8_t TETRAMINO_COUNT = 7;

const uint8_t
		TETRAMINO_I = 0,
		TETRAMINO_J = 1,
		TETRAMINO_L = 2,
		TETRAMINO_O = 3,
		TETRAMINO_S = 4,
		TETRAMINO_T = 5,
		TETRAMINO_Z = 6;
    
const uint8_t TETRAMINO_NONE = 255;

const uint8_t TETRAMINO_SIZE = 4;
const uint16_t TETRAMINO_MASK = 0x000F;

// Geometry data for each tetramino and rotation
// Array indices are [tetramino][rotation]
// Rotation zero = spawn
// Rotation is CCW
// Favour upper left as center if actual center is not possible
// Favour minimum bounding box for all rotations
// 0x44C0 = 0b0100 0100 1100 0000 =
// '.[]'.'.
// '.[]'.'.
// [][]'.'.
// '.'.'.'.
const uint16_t TETRAMINO_SHAPES[TETRAMINO_COUNT][4] = {
	{ 0x4444, 0x0F00, 0x4444, 0x0F00 },	// I
	{ 0x2260, 0x0E20, 0x0644, 0x0470 },	// J
	{ 0x4460, 0x02E0, 0x0622, 0x0740 },	// L
	{ 0x6600, 0x6600, 0x6600, 0x6600 },	// O
	{ 0x6C00, 0x4620, 0x6c00, 0x4620 },	// S
	{ 0x4E00, 0x4C40, 0x0E40, 0x4640 },	// T
	{ 0xC600, 0x2640, 0xC600, 0x2640 }	// Z
};

