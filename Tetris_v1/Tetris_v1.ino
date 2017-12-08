
#include <EEPROM.h>
#include "Tetramino.h"

#define DEBUG_SERIAL false

// Game board
const byte BOARD_WIDTH = 5;   // The width of the play area
const byte BOARD_HEIGHT = 24; // The height of the play area
const byte BORDER_X = 3;  // Padding on the right side of the board
const byte BORDER_Y = 3;  // Padding on the bottom of the board
//const byte FIELD_WIDTH = 16;
const byte FIELD_HEIGHT = BOARD_HEIGHT + BORDER_Y;
const uint16_t BORDER_MASK = ~(~(0xffff << BOARD_WIDTH) << BORDER_X);
uint16_t field[FIELD_HEIGHT];

uint16_t collisionLine;

// The active tetramino
byte tetraminoType = TETRAMINO_NONE;
byte tetraminoR;  // Rotation 0-3
byte tetraminoX;  // x position in the field (zero is rightmost column, x increases to the left)
byte tetraminoY;  // y position in the field (zero is bottom row of board, y increases upwards)

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
//uint32_t nextFallMillis = 0;
uint32_t lastFallMillis = 0;
uint16_t highScore = 0;
char highScoreInitials[] = {'A', 'A', 'A', '\0'};  // Three letters and NULL
const byte INITIALS_COUNT = 3;
const uint16_t DISPLAY_MILLIS = 3000; // The amount of time to show scores, names, etc.

// Music commands - top four bits = counter, bottom four bits = opcode
const byte
  COMMAND_SILENCE = 0x00,
  COMMAND_LEVEL_ONE = 0x01,
  COMMAND_LEVEL_UP = 0x02,
  COMMAND_HIGH_SCORE = 0x03,
  COMMAND_SOUND_ON = 0x0b,
  COMMAND_SOUND_OFF = 0x0c,
  COMMAND_GAME_OVER = 0x0d;

// EEPROM addresses
const byte
  EEPROM_RANDOM_SEED = 0, // uint32_t
  EEPROM_HIGH_SCORE = 4,  // uint16_t
  EEPROM_HIGH_SCORE_INITIALS = 6,  // 3 chars
  EEPROM_LED_CURRENT = 10;


// TODO
// - add low-power functionality
// - compose game-over sound


void setup() {
#if DEBUG_SERIAL
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
  bool resetHighScore = isLPress() && isDPress() && !isRPress();
  if (resetHighScore) {
    delay(1000);
    resetHighScore &= isLPress() && isDPress() && !isRPress();
    delay(1000);
    resetHighScore &= isLPress() && isDPress() && !isRPress();
  }

  // Enable/disable the sound
  sendMusicCommand(soundOn ? COMMAND_SOUND_ON : COMMAND_SOUND_OFF);
  
  // Adjust the brightness with the encoder
  if (adjustBrightnessNow) {
    adjustBrightness();
  }
  
  // Load the high score data and perform sanity check
  if (!resetHighScore) {
    EEPROM.get(EEPROM_HIGH_SCORE, highScore);
#if DEBUG_SERIAL
    Serial.print("highScore = ");
    Serial.println(highScore);
#endif
    for (byte i = 0; i < INITIALS_COUNT; i++) {
      highScoreInitials[i] = (char)EEPROM.read(EEPROM_HIGH_SCORE_INITIALS + i);
      resetHighScore |= !(highScoreInitials[i] >= 'A' && highScoreInitials[i] <= 'Z');
    }
  }
  
  // Reset the high score if necessary
  if (resetHighScore) {
    resetHighScoreData();
  }
}

void adjustBrightness() {
    // Get the display current
    int currentInput = getLEDCurrent();

    // Draw something to show the brightness
    drawText5High("XXXX");
    drawBoard();
    
    // Change the current with the encoder
    do {
      updateControl();
      currentInput += 0x10 * getEncoderChange();
      currentInput = constrain(currentInput, 0x0f, 0xff);
      setLEDCurrent((uint8_t)currentInput);
      delay(10);
    } while(!isRClick());
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
  drawBoard();

  sendMusicCommand(COMMAND_LEVEL_ONE);
}

// Plays a full game. This function does not return until game over, when a
// newly spawned tetramino collides with existing blocks.
void playGame() {
  while (true) {
    // Spawn a new piece
    setTetramino(random(TETRAMINO_COUNT));
    lastFallMillis = millis();
    
    drawBoard();
  
    // Check for collision (game over)
    if (isTetraminoCollision()) {
      return; // Game over
    }
    
    // Flag to stop dropping when a new piece is added
    bool canDropPiece = false;
    
    // Fall loop
    while (true) {
      
      // User input loop
      do {
        // Read buttons
        updateControl();
        bool draw = false;  // Only draw if something changed
        
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
        
        // Rotate CCW
        long positionChange = getEncoderChange();
        if (positionChange != 0) {
          for (byte i = 0; i < 4; i++) {  // This should work as an infinite loop, but we use a for loop for safety
            if (!(positionChange > 0 ? tryRotateTetraminoCW() : tryRotateTetraminoCCW())) {
              // Couldn't rotate, check where the collision is and try to move away from the walls
              if (collisionLine & ~(0xffff << BORDER_X)) {  // Collision on right
                if (!tryMoveTetraminoLeft()) {
                  // Couldn't move left, break
                  break;
                }
              } else if (collisionLine & (0xffff << (BOARD_WIDTH + BORDER_X))) {  // Collision on left
                if (!tryMoveTetraminoRight()) {
                  // Couldn't move right, break
                  break;
                }
              } else {  // Collision in center
                break;
              }
            } else {
              break;  // Rotation succeeded
            }
          }
          draw = true;
        }
        
        if (draw) {
          drawBoard();
        }
        
        // Allow dropping if D is released
        canDropPiece |= !isDPress();
      } while (millis() < lastFallMillis + (canDropPiece && isDPress() ? 10 : fallPeriod));
      
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
    byte lineCount = 0;
    for (byte y = BORDER_Y; y < FIELD_HEIGHT; y++) {
      // Check whether this row is full
      if ((field[y] | BORDER_MASK) == 0xffff ) {
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
#if DEBUG_SERIAL
      Serial.print("Lines: ");
      Serial.println(linesCleared);
#endif
      
      // Calculate score
      score += getLineScore(lineCount);
#if DEBUG_SERIAL
      Serial.print("Score: ");
      Serial.println(score);
#endif
      
      // Level up
      uint16_t previousLevel = level;
      level = getLevel(linesCleared);
      fallPeriod = getFallPeriod(level);
      if (level > previousLevel) {
        sendMusicCommand(COMMAND_LEVEL_UP);
#if DEBUG_SERIAL
        Serial.print("Level: ");
        Serial.println(level);
#endif
      }
    }
  }
}

// Draws the game over animation, displays the player's score, etc.
void gameOver() {
  // Clear the active tetramino
  tetraminoType = TETRAMINO_NONE;
  
  sendMusicCommand(COMMAND_GAME_OVER);
  
  // Fill animation
  const uint16_t CURTAIN_MILLIS = 1000 / BOARD_HEIGHT;
  for (byte y = 1; y <= BOARD_HEIGHT; y++) {
    drawBoard(true, y);
    delay(CURTAIN_MILLIS);
  }

  // TODO Allow score etc. animation to be interrupted by user input

  // Compare the score to the high score
  // TODO Forbid skipping score display if true
  bool newHighScore = score > highScore;  // True if the high score was just beaten
  if (newHighScore) {
    highScore = score;
  }
  
  // Display score
  clearBoard();
  drawUInt16(score);
  
  // Raise curtain animation
  for (byte y = 1; y <= BOARD_HEIGHT; y++) {
    drawBoard(false, y - BOARD_HEIGHT);
    delay(CURTAIN_MILLIS);
  }
  
  // Make the score flash if it is a new high score
  if (newHighScore) {
    sendMusicCommand(COMMAND_HIGH_SCORE);
    for (byte i = 0; i < DISPLAY_MILLIS / 200; i++) {
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
    for (byte i = 0; i < INITIALS_COUNT; i++) {
      highScoreInitials[i] = 'A';  // Clear the old initials
    }
    char hiddenLetter;
    byte letterIndex = 0;
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
        highScoreInitials[letterIndex] += encoderChange;
        if (highScoreInitials[letterIndex] < 'A') {
          highScoreInitials[letterIndex] += 26;
        } else if (highScoreInitials[letterIndex] > 'Z') {
          highScoreInitials[letterIndex] -= 26;
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
          hiddenLetter = highScoreInitials[letterIndex];
          highScoreInitials[letterIndex] = ' ';
        }
        
        // Draw initials
        clearBoard();
        drawText5High(highScoreInitials);
        drawBoard(false);

        // Unhide the selected letter
        if (!flashOn && letterIndex < INITIALS_COUNT) {
          highScoreInitials[letterIndex] = hiddenLetter;
        }
      }
    } while (letterIndex < INITIALS_COUNT);
    
    // Save the high score data immediately
    saveHighScoreData();
#if DEBUG_SERIAL
    Serial.print("High score initials: ");
    Serial.println(highScoreInitials);
    Serial.print("High score: ");
    Serial.println(highScore);
#endif
    
    // Show completed initials
    delay(1000);
    
    // Show score again
    clearBoard();
    drawUInt16(score);
    drawBoard(false);
    delay(DISPLAY_MILLIS);
    
  } else {
    // Show high score initials
    clearBoard();
    drawText5High(highScoreInitials);
    drawBoard(false);
    if (breakableDelay(1000)) {
      return;
    }

    // Show high score
    clearBoard();
    drawUInt16(highScore);
    drawBoard(false);
    if (breakableDelay(1000)) {
      return;
    }
  }
  
#if DEBUG_SERIAL
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
  for (byte y = 0; y < FIELD_HEIGHT; y++) {
    field[y] = y < BORDER_Y ? 0xffff : BORDER_MASK;
  }
}

/******************************************************************************
 * Game Stats
 ******************************************************************************/

// Gets the number of points earned for clearing the given number of lines.
uint16_t getLineScore(byte lines) {
  return lines * (lines + 1) / 2;
}

// Gets the current level. The level is implied by the number of lines cleared.
uint16_t getLevel(uint16_t linesCleared) {
  return linesCleared / 10 + 1;
}

// Gets the number of milliseconds between steps of a falling piece.
// The period is initially 1000 and decreases exponentially as the level increases. 
uint16_t getFallPeriod(uint16_t level) {
  return round(1000 * pow(0.774264, level - 1));  // 10x speed in level 10
}

// Clears the high score and initials from EEPROM
void resetHighScoreData() {
  highScore = 0;
  for (byte i = 0; i < INITIALS_COUNT; i++) {
    highScoreInitials[i] = 'A';
  }
  saveHighScoreData();
}

void saveHighScoreData() {
  EEPROM.put(EEPROM_HIGH_SCORE, highScore);
  for (byte i = 0; i < INITIALS_COUNT; i++) {
      EEPROM.write(EEPROM_HIGH_SCORE_INITIALS + i, highScoreInitials[i]);
  }
}

/******************************************************************************
 * Teramino Movement
 ******************************************************************************/

// TODO Combine tryRotateTetraminoCW() with tryRotateTetraminoCCW().
// Rotates the active tetramino clockwise by 90 degrees if possible.
// Returns true if the tetramino was rotated successfully.
bool tryRotateTetraminoCW() {
  bool canRotateCW = canTetraminoRotateCW();
  if (canRotateCW) {
    tetraminoR = (tetraminoR + 3) % 4;
  }
  return canRotateCW;
}

// Rotates the active tetramino counterclockwise by 90 degrees if possible.
// Returns true if the tetramino was rotated successfully.
bool tryRotateTetraminoCCW() {
  bool canRotateCCW = canTetraminoRotateCCW();
  if (canRotateCCW) {
    tetraminoR = (tetraminoR + 1) % 4;
  }
  return canRotateCCW;
}

// Moves the active tetramino one step to the left if possible.
// Returns true if the tetramino was moved successfully.
bool tryMoveTetraminoLeft() {
  bool canMoveLeft = canTetraminoMoveLeft();
  if (canMoveLeft) {
    tetraminoX++;
  }
  return canMoveLeft;
}

// Moves the active tetramino one step to the right if possible.
// Returns true if the tetramino was moved successfully.
bool tryMoveTetraminoRight() {
  bool canMoveRight = canTetraminoMoveRight();
  if (canMoveRight) {
    tetraminoX--;
  }
  return canMoveRight;
}

// Drops the active tetramino instantaneously.
void dropTetramino() {
  while (tryMoveTetraminoDown());
}

// Moves the active tetramino one step down if possible.
// Returns true if the tetramino was moved successfully.
bool tryMoveTetraminoDown() {
  bool canMoveDown = canTetraminoMoveDown();
  if (canMoveDown) {
    tetraminoY--;
  }
  return canMoveDown;
}

/******************************************************************************
 * Active Teramino
 ******************************************************************************/

// Spawns a tetramino at the top of the screen
void setTetramino(byte type) {
  tetraminoType = type;
  tetraminoR = 0;
  tetraminoX = (BOARD_WIDTH - TETRAMINO_SIZE) / 2 + BORDER_X;
  tetraminoY = BOARD_HEIGHT - TETRAMINO_SIZE + BORDER_Y;
}

bool canTetraminoRotateCCW() {
  return !isTetraminoCollision(tetraminoType, (tetraminoR + 1) % 4, tetraminoX, tetraminoY);
}

bool canTetraminoRotateCW() {
  return !isTetraminoCollision(tetraminoType, (tetraminoR + 3) % 4, tetraminoX, tetraminoY);
}

bool canTetraminoMoveLeft() {
  return !isTetraminoCollision(tetraminoType, tetraminoR, tetraminoX + 1, tetraminoY);
}

bool canTetraminoMoveRight() {
  return !isTetraminoCollision(tetraminoType, tetraminoR, tetraminoX - 1, tetraminoY);
}

bool canTetraminoMoveDown() {
  return !isTetraminoCollision(tetraminoType, tetraminoR, tetraminoX, tetraminoY - 1);
}

// Returns true if any of the tetramino's bits overlap those already on the board
// TODO Redo logic (see diagram above)
bool isTetraminoCollision() {
  return isTetraminoCollision(tetraminoType, tetraminoR, tetraminoX, tetraminoY);
}

bool isTetraminoCollision(byte type, byte r, byte x, byte y) {
  const uint16_t tetraminoShape = TETRAMINO_SHAPES[type][r];
  for (byte i = 0; i < TETRAMINO_SIZE; i++) {
    uint16_t tetraminoLine = ((tetraminoShape >> (TETRAMINO_SIZE * i)) & TETRAMINO_MASK) << x;
    uint16_t fieldLine = field[y + i];
    collisionLine = tetraminoLine & fieldLine;
    
    if (collisionLine != 0) {
      return true;
    }
  }
  
  return false;
}

// Adds the active tetramino to the field.
void assimilateTetramino() {
  const uint16_t tetraminoShape = TETRAMINO_SHAPES[tetraminoType][tetraminoR];
  for (byte i = 0; i < TETRAMINO_SIZE; i++) {
    uint16_t tetraminoLine = ((tetraminoShape >> (TETRAMINO_SIZE * i)) & TETRAMINO_MASK) << tetraminoX;
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
  0b00010010111011111101110110010010
};

// 5-wide digits 0 to 5
const uint32_t BOARD_DIGITS_05[5] = {
  0b00111100000111110111111111101110,
  0b00000010000100001100000010010001,
  0b00111101111100110011100010010001,
  0b00100001000100001000011110010001,
  0b00111111000111111111100010001110
};

// 5-wide digits 6 to 9
const uint32_t BOARD_DIGITS_69[5] = {
  0b00000000000011110011100010001110,
  0b00000000000000001100010010010001,
  0b00000000000001111011100001011110,
  0b00000000000010001100010000110000,
  0b00000000000001110011101111101111
};

void setDisplayText(String str) {
  for (byte i = 0; i < str.length(); i++) {
    byte stringY = BOARD_HEIGHT - 6 * (i + 1) + BORDER_Y;
    char stringChar = str[i];

    if (isDigit(stringChar)) {
      setDisplayDigit3Wide(stringChar - '0', 1, stringY);
    } else {
      // TODO Implement letters
      for (byte r = 0; r < 5; r++) {
        field[stringY + r] = BORDER_MASK | (0x01 << r) << BORDER_X;
      }
    }
  }
  drawBoard();
}

void setDisplayDigit3Wide(byte digit, byte x, byte y) {
  for (byte r = 0; r < 5; r++) {
    field[y + r] = ((BOARD_DIGITS[r] >> (3 * digit)) & 0b111) << (BORDER_X + x);
  }
}

void setDisplayDigit5Wide(byte digit, byte x, byte y) {
  for (byte r = 0; r < 5; r++) {
    field[y + r] = (((digit <= 5 ? BOARD_DIGITS_05[r] : BOARD_DIGITS_69[r]) >> (5 * (digit % 6))) & 0b11111) << (BORDER_X + x);
  }
}

