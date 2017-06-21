
#include "Tetramino.h"

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

void setup() {
  // DEBUG
  Serial.begin(115200);
  
  // Initialize the RNG
  randomSeed(analogRead(A0));
  
  initializeControl();
  initializeDisplay();
}

void loop() {
  newGame();
  playGame();
  gameOver();
}

/******************************************************************************
 * Game State
 ******************************************************************************/

void newGame() {
  score = 0;
  linesCleared = 0;
  level = getLevel(linesCleared);
  fallPeriod = getFallPeriod(level);
  
  clearBoard();
  drawBoard();
}

void playGame() {
  while (true) {  // TODO while not game over
    // Spawn a new piece
    setTetramino(random(TETRAMINO_COUNT));
    lastFallMillis = millis();
    
    drawBoard();
  
    // TODO check for collision (game over)
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
        updateButtons();
        updateEncoder();
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
        //if (isUClick()) {
        long positionChange = getPositionChange();
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

        // Drop
        /*if (isDClick()) {
          dropTetramino();
          draw = true;
          // TODO break drop loop so that piece cannot move after drop
        }*/
  
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
      
      // Calculate score
      score += getLineScore(lineCount);
      drawScore();
      
      // Level up
      level = getLevel(linesCleared);
      fallPeriod = getFallPeriod(level);
    }
  }
}

void gameOver() {
  // Clear the active tetramino
  tetraminoType = TETRAMINO_NONE;
  
  const uint16_t CURTAIN_MILLIS = 1000 / BOARD_HEIGHT;
  
  // Fill animation
  for (byte y = 1; y <= BOARD_HEIGHT; y++) {
    drawBoard(true, y);
    delay(CURTAIN_MILLIS);
  }
//  delay(500);

  // TODO Allow score etc. animation to be interrupted by user input
  
  // Display score
  clearBoard();
  uint16_t scoreCopy = score;
  for (byte i = 0; i < 4 && scoreCopy != 0; i++) {
    setDisplayDigit(scoreCopy % 10, 1, BORDER_Y + 1 + 6 * i);
    scoreCopy /= 10;
  }
  
  // Raise curtain animation
  for (byte y = 1; y <= BOARD_HEIGHT; y++) {
    drawBoard(false, y - BOARD_HEIGHT);
    delay(CURTAIN_MILLIS);
  }
  delay(1000);
  
  
  /*byte y = BOARD_HEIGHT + BORDER_Y;
  field[y - 1] = 0x0000;
  uint16_t scoreDivider = 1;
  byte scoreDigits = 0;
  while (score / scoreDivider >= 10) {
    scoreDivider *= 10;
  }
  uint16_t scoreCopy = score;
  uint16_t scoreChange;
  while (scoreDivider != 0) {
    scoreChange = scoreCopy / scoreDivider;
    
    y -= 6;
    field[y - 1] = 0x0000;
    setDisplayDigit(scoreChange, 1, y);
    drawBoard();
    Serial.println(scoreChange);
    
    scoreCopy -= scoreChange * scoreDivider;
    scoreDivider /= 10;

    delay(500);
  }*/
  
  // TODO Display high score etc
  Serial.println("Game Over!");
  delay(1000);
}

// Clear the board (fills border)
void clearBoard() {
  for (byte y = 0; y < FIELD_HEIGHT; y++) {
    field[y] = y < BORDER_Y ? 0xffff : BORDER_MASK;
  }
}

/******************************************************************************
 * Game Stats
 ******************************************************************************/

uint16_t getLineScore(byte lines) {
  return lines * (lines + 1) / 2;
}

uint16_t getLevel(uint16_t linesCleared) {
  return linesCleared / 10 + 1;
}

uint16_t getFallPeriod(uint16_t level) {
  return round(1000 * pow(0.774264, level - 1));  // 10x speed in level 10
}

/******************************************************************************
 * Teramino Movement
 ******************************************************************************/

// TODO Make CW and CCW general
bool tryRotateTetraminoCW() {
  bool canRotateCW = canTetraminoRotateCW();
  if (canRotateCW) {
    tetraminoR = (tetraminoR + 3) % 4;
  }
  return canRotateCW;
}

bool tryRotateTetraminoCCW() {
  bool canRotateCCW = canTetraminoRotateCCW();
  if (canRotateCCW) {
    tetraminoR = (tetraminoR + 1) % 4;
  }
  return canRotateCCW;
}

bool tryMoveTetraminoLeft() {
  bool canMoveLeft = canTetraminoMoveLeft();
  if (canMoveLeft) {
    tetraminoX++;
  }
  return canMoveLeft;
}

bool tryMoveTetraminoRight() {
  bool canMoveRight = canTetraminoMoveRight();
  if (canMoveRight) {
    tetraminoX--;
  }
  return canMoveRight;
}

void dropTetramino() {
  while (tryMoveTetraminoDown());
}

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

// Creates a tetramino at the top of the screen
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

// Adds the active tetramino to the field
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

const uint32_t BOARD_DIGITS[5] = {
  0b00110010100010110001110111111010,
  0b00001101100101001001001100010101,
  0b00011010010110110111110010010101,
  0b00101101001100100101001001110101,
  0b00010010111011111101110110010010
};

void setDisplayText(String str) {
  for (byte i = 0; i < str.length(); i++) {
    byte stringY = BOARD_HEIGHT - 6 * (i + 1) + BORDER_Y;
    char stringChar = str[i];

    if (isDigit(stringChar)) {
      setDisplayDigit(stringChar - '0', 1, stringY);
    } else {
      for (byte r = 0; r < 5; r++) {
        field[stringY + r] = BORDER_MASK | (0x01 << r) << BORDER_X;
      }
    }
  }
  drawBoard();
}

void setDisplayDigit(byte digit, byte x, byte y) {
  for (byte r = 0; r < 5; r++) {
    field[y + r] = ((BOARD_DIGITS[r] >> (3 * digit)) & 0b111) << (BORDER_X + x);
  }
}

