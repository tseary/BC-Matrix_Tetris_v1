
#include <EEPROM.h>
#include <LowPower.h>
#include <LRAS1130.h>
#include "Tetramino.h"

// Uncomment these to enable debug behaviour
//#define DEBUG_SERIAL
//#define ALL_THE_SAME_TETRAMINO TETRAMINO_I

// Game board
const uint8_t BOARD_WIDTH = 5;   // The width of the play area
const uint8_t BOARD_HEIGHT = 24; // The height of the play area
const uint8_t BORDER_X = 3;  // Padding on the right side of the board
const uint8_t BORDER_Y = 3;  // Padding on the bottom of the board
//const uint8_t FIELD_WIDTH = 16;
const uint8_t FIELD_HEIGHT = BOARD_HEIGHT + BORDER_Y;
const uint16_t BORDER_MASK = ~(~(0xffff << BOARD_WIDTH) << BORDER_X);
uint16_t field[FIELD_HEIGHT];
uint16_t collisionLine;	// Set by isTetraminoCollision()

// The active tetramino
uint8_t tetraminoType = TETRAMINO_NONE;
uint8_t tetraminoR;  // Rotation 0-3
uint8_t tetraminoX;  // x position in the field (zero is rightmost column, x increases to the left)
uint8_t tetraminoY;  // y position in the field (zero is bottom row of board, y increases upwards)

// The stored tetramino
uint8_t storedType = TETRAMINO_NONE;
uint8_t storedR = 0;

// The game field is represented by an array of uint16_t, with one bit
// representing each block. The game board is the part of the field that
// fits on the screen. There is a minimum 3-bit-wide border around the
// game board on the sides and bottom. All of the field bits outside the
// board are ones, making them behave like solid walls and floor. This
// facilitates the game engine math.

// Similarly, each tetramino is represented by an array of four uint16_t.
// The 16 bits are interpreted as blocks in a 4 x 4 grid, and there is a
// separate uint16_t for each rotation of the same tetramino.
// See Tetramino.h for more details.

// @@ = field origin
// ## = tetramino origin
// '. = empty tetramino bit
// :; = empty board bit
//
// tetramino coordinates
//          3 2 1 0
//   []:;:;:;:;:;:;:;:;[][][] 10...
// 3 []:;:;'.'.[]'.:;:;[][][] 9
// 2 []:;:;'.'.[]'.:;:;[][][] 8
// 1 []:;:;'.[][]'.:;:;[][][] 7
// 0 []:;:;'.'.'.##:;:;[][][] 6
//   []:;:;:;:;:;:;:;:;[][][] 5
//   []:;:;:;:;:;:;:;:;[][][] 4
//   []:;:;:;:;:;;::;:;[][][] 3
//   [][][][][][][][][][][][] 2
//   [][][][][][][][][][][][] 1
//   [][][][][][][][][][][]@@ 0
//  ...10 9 8 7 6 5 4 3 2 1 0
//        field coordinates

// Game stats
uint16_t score = 0;
uint16_t linesCleared = 0;
uint16_t level = 1;
uint16_t fallPeriod = 1000;
const uint16_t MINIMUM_FALL_PERIOD = 10;
//uint32_t nextFallMillis = 0;
uint32_t lastFallMillis = 0;
const uint16_t SWAP_BLINK_MILLIS = 50;

const uint8_t HIGH_SCORE_COUNT = 3;
const uint8_t INITIALS_COUNT = 3;
uint16_t highScores[] = {0, 0, 0};	// The 1st place high score is always stored at index 0.
char highScoreInitials[HIGH_SCORE_COUNT][INITIALS_COUNT + 1] = {
	{'A', 'A', 'A', '\0'},
	{'A', 'A', 'A', '\0'},
	{'A', 'A', 'A', '\0'}};  // Three letters and NULL (NULL is used for printing as string)

const uint16_t DISPLAY_MILLIS = 3000; // The amount of time to show scores, names, etc.

// Music commands - top four bits = counter, bottom four bits = opcode
const uint8_t
COMMAND_SILENCE = 0x00,
COMMAND_LEVEL_ONE = 0x01,
COMMAND_LEVEL_UP = 0x02,
COMMAND_HIGH_SCORE = 0x03,
COMMAND_PAUSE = 0x08,
COMMAND_UNPAUSE = 0x09,
COMMAND_SOUND_ON = 0x0b,
COMMAND_SOUND_OFF = 0x0c,
COMMAND_GAME_OVER = 0x0d;

// EEPROM addresses
const uint8_t
EEPROM_RANDOM_SEED = 0,				// uint32_t
EEPROM_HIGH_SCORE = 100,			// uint16_t * 3
EEPROM_HIGH_SCORE_INITIALS = 102,	// 3 chars * 3
EEPROM_LED_CURRENT = 10,				// uint8_t
EEPROM_PIXEL_BRIGHTNESS = 11;		// uint32_t
const uint8_t
HIGH_SCORE_SIZE = 5;	// bytes needed to hold one score and initials

void setup() {
#ifdef DEBUG_SERIAL
	Serial.begin(115200);
	Serial.println(F("Compiled: " __DATE__ " " __TIME__));
#endif

	// Initialize the RNG, and change the seed value for next time
	uint32_t eepromRandomSeed;
	EEPROM.get(EEPROM_RANDOM_SEED, eepromRandomSeed);
	randomSeed(eepromRandomSeed);
	eepromRandomSeed = random() ^ analogRead(A0);
	EEPROM.put(EEPROM_RANDOM_SEED, eepromRandomSeed);

	// Initialize functions
	initializeControl();
	initializeDisplay();
	initializeMusic();

	// Power-on settings
	updateControl();
	bool soundOn = !isDPress();
	bool adjustBrightnessNow = isRPress();
	bool seriousBusiness = isLPress() && isDPress();
	if (seriousBusiness) {
		delay(1000);
		updateControl();
		seriousBusiness &= isLPress() && isDPress();
	}
	if (seriousBusiness) {
		delay(1000);
		updateControl();
		seriousBusiness &= isLPress() && isDPress();
	}
	bool highScoreReset = seriousBusiness && !isRPress();
	bool doFactoryReset = seriousBusiness && isRPress();

	// Enable/disable the sound
	delay(75);  // Short delay before sending first music command
	sendMusicCommand(soundOn ? COMMAND_SOUND_ON : COMMAND_SOUND_OFF);

	// Factory reset if necessary
	if (doFactoryReset) {
		factoryReset();
		return;
	}

	// Adjust the brightness with the encoder
	if (adjustBrightnessNow) {
		adjustBrightness();
	}

	// Load the high score data and perform sanity check
	if (!highScoreReset) {
		bool dataValid = loadHighScoreData();
		highScoreReset |= !dataValid;
	}

	// Reset the high scores if necessary
	if (highScoreReset) {
		resetHighScoreData();
	}
}

void adjustBrightness() {
	// Get the display current
	int16_t currentInput = getLEDCurrent();

	// Draw something to show the brightness
	drawText5High("XXXX");
	drawBoard();

	// Change the current with the encoder
	do {
		updateControl();
		currentInput += 0x10 * getEncoderChange();
		currentInput = constrain(currentInput, 0x0f, 0xff);
		setLEDCurrent((uint8_t)currentInput, true);
		LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
	} while (!isRClick());
}

void loop() {
	newGame();
	playGame();
	gameOver();
}

/******************************************************************************
 * Game State
 ******************************************************************************/

 // Starts a new game.
void newGame() {
	score = 0;
	linesCleared = 0;
	level = getLevel(linesCleared);
	fallPeriod = getFallPeriod(level);

	clearBoard();
	drawBoard(false);

	sendMusicCommand(COMMAND_LEVEL_ONE);
}

// Plays a full game. This function does not return until game over, when a
// newly spawned tetramino collides with existing blocks.
void playGame() {
	while (true) {
		// Spawn a new piece
		spawnTetramino(random(TETRAMINO_COUNT));
		lastFallMillis = millis();

		drawBoard();

		// Check for collision (game over)
		if (isTetraminoCollision()) {
			return; // Game over
		}

		// Flag to stop dropping when a new piece is added
		bool canDropPiece = false;

		// Flag to store only once per key combo
		bool canStorePiece = true;

		// Fall loop
		while (true) {

			// User input loop
			do {
				// Read buttons
				updateControl();
				bool draw = false;  // Only draw if something changed

				// Do pause
				if (isEClick()) {
					// Pause music
					sendMusicCommand(COMMAND_PAUSE);

					// Dim display by reducing current
					uint8_t originalCurrent = getLEDCurrent();
					setLEDCurrent(originalCurrent / 3, false);

					// Wait for any input
					do {
						LowPower.powerDown(SLEEP_15MS, ADC_OFF, BOD_OFF);
						updateControl();
					} while (!isAnyClick() && !getEncoderChange());

					// Restore display brightness
					setLEDCurrent(originalCurrent, false);
					draw = true;

					// Unpause music
					sendMusicCommand(COMMAND_UNPAUSE);
				}

				// Store by pressing any two buttons together
				uint8_t pressCount = (isLPress() ? 1 : 0) + (isRPress() ? 1 : 0) + (isDPress() ? 1 : 0);
				canStorePiece |= pressCount == 0;	// Allow storing if all the buttons are released
				canDropPiece &= pressCount <= 1;	// Forbid dropping while multiple buttons are pressed

				// Store this piece and recall the previously stored piece
				bool store = pressCount >= 2 && isAnyClick();
				if (store && canStorePiece) {
					// Swap the active piece with the stored piece
					swapStoredTetramino();

					// If the active piece is empty, spawn a new piece
					if (tetraminoType == TETRAMINO_NONE) {
						// TODO Go back to top of spawn loop
						spawnTetramino(random(TETRAMINO_COUNT));
					} else {
						// Do collision check on the piece that was swapped in
						if (!resolveCollision()) {
							// Could not resolve collision with the swapped-in piece
							// We are forced to un-swap
							swapStoredTetramino();
						}
					}

					// Blink out to show the pieces swapping (or not)
					drawBoard(false);
					delay(SWAP_BLINK_MILLIS);
					draw = true;

					canStorePiece = false;
				} else {
					// Don't move while swapping

					// Move left
					if (isLClick()) {
						tryMoveTetraminoLeft();
						draw = true;
					}

					// Move right
					if (isRClick()) {
						tryMoveTetraminoRight();
						draw = true;
					}
				}

				// TODO use resolveCollision()
				uint8_t originalX = tetraminoX;

				// Rotate CCW
				long rotationChange = getEncoderChange();
				if (rotationChange != 0) {
					for (uint8_t i = 0; i < 4; i++) {  // This should work as an infinite loop, but we use a for loop for safety

						// TODO rotate the piece and let a collsion happen, then use resolveCollision().
						// if resolveCollision() fails, undo the rotation.

						if (!(rotationChange > 0 ? tryRotateTetraminoCW() : tryRotateTetraminoCCW())) {
							// Couldn't rotate, check where the collision is and try to move away from the walls
							if (isCollisionOnRight()) {
								if (!tryMoveTetraminoLeft()) {
									// Couldn't move left, break
									tetraminoX = originalX;	// Undo move
									break;
								}
							} else if (isCollisionOnLeft()) {
								if (!tryMoveTetraminoRight()) {
									// Couldn't move right, break
									tetraminoX = originalX;	// Undo move
									break;
								}
							} else {
								// Collision in center
								tetraminoX = originalX;	// Undo move
								break;
							}
						} else {
							break;  // Rotation succeeded
						}
					}
					draw = true;	// TODO maybe move to "Rotation succeeded"
				}

				if (draw) {
					drawBoard();
				}

				// Allow dropping if D is released
				canDropPiece |= !isDPress();
			} while (millis() < lastFallMillis +
				(canDropPiece && isDPress() ? MINIMUM_FALL_PERIOD : fallPeriod));

			// Move the piece down
			if (canTetraminoMoveDown()) {
				tetraminoY--;
				lastFallMillis = millis();
				drawBoard();
			} else {
				// Piece landed
				break;
			}
		}

		// The active piece has landed, so assimilate it onto the board
		assimilateTetramino();

		// Count completed rows and drop remaining rows
		uint8_t lineCount = 0;
		for (uint8_t y = BORDER_Y; y < FIELD_HEIGHT; y++) {
			// Check whether this row is full
			if ((field[y] | BORDER_MASK) == 0xffff) {
				// Row is full
				lineCount++;
			} else {
				// Row is not full
				if (lineCount != 0) {
					field[y - lineCount] = field[y];
					field[y] = BORDER_MASK;
				}
			}
		}

		if (lineCount != 0) {
			// Increase total count
			linesCleared += lineCount;
#ifdef DEBUG_SERIAL
			Serial.print("Lines: ");
			Serial.println(linesCleared);
#endif

			// Calculate score
			score += getLineScore(lineCount);
#ifdef DEBUG_SERIAL
			Serial.print("Score: ");
			Serial.println(score);
#endif

			// Level up
			uint16_t previousLevel = level;
			level = getLevel(linesCleared);
			fallPeriod = getFallPeriod(level);
			if (level > previousLevel) {
				sendMusicCommand(COMMAND_LEVEL_UP);
#ifdef DEBUG_SERIAL
				Serial.print("Level: ");
				Serial.println(level);
#endif
			}
		}
	}
}

// Draws the game over animation, displays the player's score, etc.
void gameOver() {
	sendMusicCommand(COMMAND_GAME_OVER);

	// Fill animation
	const uint16_t CURTAIN_MILLIS = 1000 / BOARD_HEIGHT;
	for (uint8_t y = 1; y <= BOARD_HEIGHT; y++) {
		drawBoard(true, y);
		delay(CURTAIN_MILLIS);
	}

	// Compare the score to the high scores
	uint8_t newHighScoreIndex = 255;
	bool newHighScore = false;
	for (uint8_t i = 0; i < HIGH_SCORE_COUNT; i++) {
		if (score > highScores[i]) {
			newHighScoreIndex = i;
			newHighScore = true;

			shiftHighScores(newHighScoreIndex);
			highScores[newHighScoreIndex] = score;

			break;
		}
	}

	// TODO Forbid skipping score display if true

	// Display score (behind curtain)
	clearBoard();
	drawUInt16(score);

	// Raise curtain animation
	for (uint8_t y = 1; y <= BOARD_HEIGHT; y++) {
		drawBoard(false, y - BOARD_HEIGHT);
		delay(CURTAIN_MILLIS);
	}

	// Make the score flash if it is a new high score
	if (newHighScore) {
		sendMusicCommand(COMMAND_HIGH_SCORE);
		for (uint8_t i = 0; i < DISPLAY_MILLIS / 200; i++) {
			drawBlank(); // Hide
			delay(100);
			drawBoard(false); // Show
			delay(100);
		}
	} else {
		// Display the score non-flashing
		if (breakableDelay(DISPLAY_MILLIS)) {
			return;
		}
	}

	if (newHighScore) {
		// Display "HIGH"
		clearBoard();
		drawText5High("HIGH");
		drawBoard(false);
		delay(1000);

		// Display "SCORE"
		drawTextScore();
		drawBoard(false);
		delay(1000);

		// Enter initials
		for (uint8_t a = 0; a < INITIALS_COUNT; a++) {
			highScoreInitials[newHighScoreIndex][a] = 'A';  // Clear the old initials
		}
		char hiddenLetter;
		uint8_t letterIndex = 0;
		uint32_t flashStartMillis = millis();
		bool flashOn, wasFlashOn = false;
		bool redraw;  // The display is only updated if this is true
		updateControl();  // One extra call to clear the last encoder rotation
		do {
			updateControl();
			redraw = false;

			// Change letter
			long encoderChange = getEncoderChange();
			if (encoderChange != 0) {
				// Change letter and wrap around
				highScoreInitials[newHighScoreIndex][letterIndex] += encoderChange;
				if (highScoreInitials[newHighScoreIndex][letterIndex] < 'A') {
					highScoreInitials[newHighScoreIndex][letterIndex] += 26;
				} else if (highScoreInitials[newHighScoreIndex][letterIndex] > 'Z') {
					highScoreInitials[newHighScoreIndex][letterIndex] -= 26;
				}

				// Reset flashing
				flashStartMillis = millis();
				redraw = true;
			}

			// Select letter or finish
			if (isLClick() && letterIndex > 0) {
				// L is previous
				letterIndex--;
				redraw = true;
			} else if (isRClick() && letterIndex < INITIALS_COUNT - 1) {
				// R is next only
				letterIndex++;
				redraw = true;
			} else if (isDClick()) {
				// D is next/finish
				letterIndex++;
				redraw = true;
			}

			// Make selected letter flash
			flashOn = (millis() - flashStartMillis + 333) % 1000 < 667;
			redraw |= flashOn ^ wasFlashOn;
			wasFlashOn = flashOn;

			if (redraw) {
				// Hide the selected letter while it is flashed off
				if (!flashOn && letterIndex < INITIALS_COUNT) {
					hiddenLetter = highScoreInitials[newHighScoreIndex][letterIndex];
					highScoreInitials[newHighScoreIndex][letterIndex] = ' ';
				}

				// Draw initials
				clearBoard();
				drawText5High(highScoreInitials[newHighScoreIndex]);
				drawBoard(false);

				// Unhide the selected letter
				if (!flashOn && letterIndex < INITIALS_COUNT) {
					highScoreInitials[newHighScoreIndex][letterIndex] = hiddenLetter;
				}
			}
		} while (letterIndex < INITIALS_COUNT);

		// Save the high score data immediately
		saveHighScoreData();

		// Show completed initials
		delay(1000);

		// Show score again
		/*clearBoard();
		drawUInt16(score);
		drawBoard(false);
		delay(DISPLAY_MILLIS);*/

	}

	// Show all high scores until interrupted by user
	for (uint8_t i = 0; i < HIGH_SCORE_COUNT; i++) {
		// Show high score initials
		clearBoard();
		drawText5High(highScoreInitials[i]);
		drawBoard(false);
		if (breakableDelay(1000)) {
			return;
		}

		// Show high score
		clearBoard();
		drawUInt16(highScores[i]);
		drawBoard(false);
		if (breakableDelay(1000)) {
			return;
		}
	}

#ifdef DEBUG_SERIAL
	Serial.println("Game Over!");
#endif
}

// Helper
// Returns true if the user clicked a button to break the delay
bool breakableDelay(uint32_t milliseconds) {
	uint32_t delayStartMillis = millis();
	while (millis() - delayStartMillis < milliseconds) {
		updateControl();
		if (isLClick() || isRClick() || isDClick()) {
			return true;
		}
		delay(1);
	}
	return false;
}

// Clear the board (also fills the border).
void clearBoard() {
	for (uint8_t y = 0; y < FIELD_HEIGHT; y++) {
		field[y] = y < BORDER_Y ? 0xffff : BORDER_MASK;
	}
}

void factoryReset() {
	for (uint16_t i = 0; i < EEPROM_HIGH_SCORE; i++) {
		EEPROM.write(i, 0);
	}
	resetHighScoreData();

	// Show reset text and 
	drawText5High("RST");
	drawBoard(false);
	LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
	while (true);
}

/******************************************************************************
 * Game Stats
 ******************************************************************************/

 // Gets the number of points earned for clearing the given number of lines.
uint16_t getLineScore(uint8_t lines) {
	return lines * (lines + 1) / 2;
}

// Gets the current level. The level is implied by the number of lines cleared.
uint16_t getLevel(uint16_t linesCleared) {
	return linesCleared / 10 + 1;
}

// Gets the number of milliseconds between steps of a falling piece.
// The period is initially 1000 and decreases exponentially as the level increases. 
uint16_t getFallPeriod(uint16_t level) {
	return max(MINIMUM_FALL_PERIOD,
		round(1000 * pow(0.774264, level - 1)));  // 10x speed in level 10
}

// Moves all high scores down one slot, starting at the given index.
void shiftHighScores(uint8_t startIndex) {
#ifdef DEBUG_SERIAL
	Serial.print("Shift High Scores from index ");
	Serial.println(startIndex);
#endif

	for (int toIndex = HIGH_SCORE_COUNT - 1; toIndex > startIndex; toIndex--) {
		highScores[toIndex] = highScores[toIndex - 1];
		for (uint8_t a = 0; a < INITIALS_COUNT; a++) {
			highScoreInitials[toIndex][a] = highScoreInitials[toIndex - 1][a];
		}
	}
}

// Clears the high score and initials from EEPROM
void resetHighScoreData() {
#ifdef DEBUG_SERIAL
	Serial.println("Reset High Scores");
#endif

	for (uint8_t i = 0; i < HIGH_SCORE_COUNT; i++) {
		highScores[i] = 0;
		for (uint8_t a = 0; a < INITIALS_COUNT; a++) {
			highScoreInitials[i][a] = 'A';
		}
	}
	saveHighScoreData();
}

// Returns true if the loaded data passes sanity check
bool loadHighScoreData() {
#ifdef DEBUG_SERIAL
	Serial.println("Load High Scores");
#endif

	bool dataValid = true;

	for (uint8_t i = 0; i < HIGH_SCORE_COUNT; i++) {
		EEPROM.get(EEPROM_HIGH_SCORE + i * HIGH_SCORE_SIZE, highScores[i]);
		for (uint8_t a = 0; a < INITIALS_COUNT; a++) {
			highScoreInitials[i][a] = (char)EEPROM.read(EEPROM_HIGH_SCORE_INITIALS + i * HIGH_SCORE_SIZE + a);
			dataValid &= highScoreInitials[i][a] >= 'A' && highScoreInitials[i][a] <= 'Z';
		}

#ifdef DEBUG_SERIAL
		Serial.print("High score initials: ");
		Serial.println(highScoreInitials[i]);
		Serial.print("High score: ");
		Serial.println(highScores[i]);
#endif
	}

	return dataValid;
}

void saveHighScoreData() {
#ifdef DEBUG_SERIAL
	Serial.println("Save High Scores");
#endif

	for (uint8_t i = 0; i < HIGH_SCORE_COUNT; i++) {
		EEPROM.put(EEPROM_HIGH_SCORE + i * HIGH_SCORE_SIZE, highScores[i]);
		for (uint8_t a = 0; a < INITIALS_COUNT; a++) {
			EEPROM.write(EEPROM_HIGH_SCORE_INITIALS + i * HIGH_SCORE_SIZE + a, highScoreInitials[i][a]);
		}

#ifdef DEBUG_SERIAL
		Serial.print("High score initials: ");
		Serial.println(highScoreInitials[i]);
		Serial.print("High score: ");
		Serial.println(highScores[i]);
#endif
	}
}

/******************************************************************************
 * Tetramino Movement
 ******************************************************************************/

 // TODO Combine tryRotateTetraminoCW() with tryRotateTetraminoCCW().
 // Rotates the active tetramino clockwise by 90 degrees if possible.
 // Returns true if the tetramino was rotated successfully.
bool tryRotateTetraminoCW() {
	if (!canTetraminoRotateCW()) return false;
	rotateTetraminoCW();
	return true;
}

// Rotates the active tetramino clockwise by 90 degrees.
// Does not check for collisions.
void rotateTetraminoCW() {
	tetraminoR = (tetraminoR + 3) % 4;
}

// Rotates the active tetramino counterclockwise by 90 degrees if possible.
// Returns true if the tetramino was rotated successfully.
bool tryRotateTetraminoCCW() {
	if (!canTetraminoRotateCCW())return false;
	rotateTetraminoCCW();
	return true;
}

// Rotates the active tetramino counterclockwise by 90 degrees.
// Does not check for collisions.
void rotateTetraminoCCW() {
	tetraminoR = (tetraminoR + 1) % 4;
}

// Moves the active tetramino one step to the left if possible.
// Returns true if the tetramino was moved successfully.
bool tryMoveTetraminoLeft() {
	if (!canTetraminoMoveLeft())return false;
	tetraminoX++;
	return true;
}

// Moves the active tetramino one step to the right if possible.
// Returns true if the tetramino was moved successfully.
bool tryMoveTetraminoRight() {
	if (!canTetraminoMoveRight()) return false;
	tetraminoX--;
	return true;
}

// Drops the active tetramino instantaneously.
void dropTetramino() {
	while (tryMoveTetraminoDown());
}

// Moves the active tetramino one step down if possible.
// Returns true if the tetramino was moved successfully.
bool tryMoveTetraminoDown() {
	if (!canTetraminoMoveDown()) return false;
	tetraminoY--;
	return true;
}

/******************************************************************************
 * Active Tetramino
 ******************************************************************************/

 // Spawns a tetramino at the top of the screen
void spawnTetramino(uint8_t type) {
#ifndef ALL_THE_SAME_TETRAMINO
	tetraminoType = type;
#else
	tetraminoType = ALL_THE_SAME_TETRAMINO;
#endif
	tetraminoR = 0;
	tetraminoX = (BOARD_WIDTH - TETRAMINO_SIZE) / 2 + BORDER_X;
	tetraminoY = BOARD_HEIGHT - TETRAMINO_SIZE + BORDER_Y;
}

// Swaps the stored tetramino with the active one.
// Type and rotation are swapped, x and y are unchanged.
void swapStoredTetramino() {
	uint8_t temp = storedType;
	storedType = tetraminoType;
	tetraminoType = temp;

	temp = storedR;
	storedR = tetraminoR;
	tetraminoR = temp;
}

// Attempts to resolve a collision with the walls or other pieces by moving the
// active tetramino left or right any number of spaces, or down by at most one space.
// If the tetramino is overlapping one wall, it will be pushed toward the center
// of the board. Any attempted moves are undone if the collision cannot be resolved.
// Returns true if successful.
bool resolveCollision() {
	uint8_t originalX = tetraminoX;
	while (isTetraminoCollision()) {
		if (isCollisionOnRight()) {
			if (!tryMoveTetraminoLeft()) {
				// Couldn't move left, break
				tetraminoX = originalX;	// Undo move
				return false;
			}
		} else if (isCollisionOnLeft()) {
			if (!tryMoveTetraminoRight()) {
				// Couldn't move right, break
				tetraminoX = originalX;	// Undo move
				return false;
			}
		} else {
			// Collision in center
			uint8_t originalY = tetraminoY;
			if (!tryMoveTetraminoDown()) {
				// Couldn't move down, break
				tetraminoX = originalX;	// Undo move
				// tetraminoY has not been changed if we come here, so no need to undo
				return false;
			}

			// Moving down succeeded
			// We don't attempt multiple moves down, so if there is still a collision, fail
			if (isTetraminoCollision()) {
				tetraminoX = originalX;	// Undo move
				tetraminoY = originalY;	// Undo move
				return false;
			} else {
				// Success
				return true;
			}
		}
	}

	// Success
	return true;
}

bool canTetraminoRotateCCW() {
	return !isTetraminoCollision(tetraminoType, (tetraminoR + 1) % 4, tetraminoX, tetraminoY);
}

bool canTetraminoRotateCW() {
	return !isTetraminoCollision(tetraminoType, (tetraminoR + 3) % 4, tetraminoX, tetraminoY);
}

// TODO This should allow collisions with the right wall, in case a tetramino overlaps the right wall by >1
bool canTetraminoMoveLeft() {
	return !isTetraminoCollision(tetraminoType, tetraminoR, tetraminoX + 1, tetraminoY);
}

// TODO This should allow collisions with the left wall, in case a tetramino overlaps the left wall by >1
bool canTetraminoMoveRight() {
	return !isTetraminoCollision(tetraminoType, tetraminoR, tetraminoX - 1, tetraminoY);
}

bool canTetraminoMoveDown() {
	return !isTetraminoCollision(tetraminoType, tetraminoR, tetraminoX, tetraminoY - 1);
}

// Returns true if any of the tetramino's bits overlap those already on the board
bool isTetraminoCollision() {
	return isTetraminoCollision(tetraminoType, tetraminoR, tetraminoX, tetraminoY);
}

bool isTetraminoCollision(uint8_t type, uint8_t r, uint8_t x, uint8_t y) {
	const uint16_t tetraminoShape = TETRAMINO_SHAPES[type][r];
	for (uint8_t i = 0; i < TETRAMINO_SIZE; i++) {
		uint16_t tetraminoLine = ((tetraminoShape >> (TETRAMINO_SIZE * i)) & TETRAMINO_MASK) << x;
		uint16_t fieldLine = field[y + i];
		collisionLine = tetraminoLine & fieldLine;

		if (collisionLine != 0) {
			return true;
		}
	}

	return false;
}

// After calling isTetraminoCollision(), returns true if the active tetramino is
// intersecting the right wall.
bool isCollisionOnRight() {
	return collisionLine & ~(0xffff << BORDER_X);
}

// After calling isTetraminoCOllision(), returns true if the active tetramino is
// intersecting the left wall.
bool isCollisionOnLeft() {
	return collisionLine & (0xffff << (BOARD_WIDTH + BORDER_X));
}

// Adds the active tetramino to the field.
void assimilateTetramino() {
	const uint16_t tetraminoShape = TETRAMINO_SHAPES[tetraminoType][tetraminoR];
	for (uint8_t i = 0; i < TETRAMINO_SIZE; i++) {
		uint16_t tetraminoLine = ((tetraminoShape >> (TETRAMINO_SIZE * i))& TETRAMINO_MASK) << tetraminoX;
		field[tetraminoY + i] |= tetraminoLine;
	}
}

/******************************************************************************
 * Text
 ******************************************************************************/

 // 3-wide digits 0 to 9
const uint32_t BOARD_DIGITS[5] = {
	0b00110010100010110001110111111010,
	0b00001101100101001001001100010101,
	0b00011010010110110111110010010101,
	0b00101101001100100101001001110101,
	0b00010010111011111101110110010010};

// 5-wide digits 0 to 5
const uint32_t BOARD_DIGITS_05[5] = {
	0b00111100000111110111111111101110,
	0b00000010000100001100000010010001,
	0b00111101111100110011100010010001,
	0b00100001000100001000011110010001,
	0b00111111000111111111100010001110};

// 5-wide digits 6 to 9
const uint32_t BOARD_DIGITS_69[5] = {
	0b00000000000011110011100010001110,
	0b00000000000000001100010010010001,
	0b00000000000001111011100001011110,
	0b00000000000010001100010000110000,
	0b00000000000001110011101111101111};

void setDisplayText(String str) {
	for (uint8_t i = 0; i < str.length(); i++) {
		uint8_t stringY = BOARD_HEIGHT - 6 * (i + 1) + BORDER_Y;
		char stringChar = str[i];

		if (isDigit(stringChar)) {
			setDisplayDigit3Wide(stringChar - '0', 1, stringY);
		} else {
			// TODO Implement letters
			for (uint8_t r = 0; r < 5; r++) {
				field[stringY + r] = BORDER_MASK | (0x01 << r) << BORDER_X;
			}
		}
	}
	drawBoard();
}

void setDisplayDigit3Wide(uint8_t digit, uint8_t x, uint8_t y) {
	for (uint8_t r = 0; r < 5; r++) {
		field[y + r] = ((BOARD_DIGITS[r] >> (3 * digit)) & 0b111) << (BORDER_X + x);
	}
}

void setDisplayDigit5Wide(uint8_t digit, uint8_t x, uint8_t y) {
	for (uint8_t r = 0; r < 5; r++) {
		field[y + r] = (((digit <= 5 ? BOARD_DIGITS_05[r] : BOARD_DIGITS_69[r]) >> (5 * (digit % 6))) & 0b11111) << (BORDER_X + x);
	}
}

