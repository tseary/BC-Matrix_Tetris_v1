
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
const uint16_t TETRAMINO_MASK = 0x000f;

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
	{0x4444, 0x0f00, 0x4444, 0x0f00},	// I
	{0x44C0, 0x0e20, 0x6440, 0x08e0},	// J
	{0x4460, 0x2e00, 0xC440, 0x0e80},	// L
	{0x6600, 0x6600, 0x6600, 0x6600},	// O
	{0x6c00, 0x4620, 0x6c00, 0x4620},	// S
	{0x4e00, 0x4c40, 0x0e40, 0x4640},	// T
	{0xc600, 0x2640, 0xc600, 0x2640}	// Z
};

