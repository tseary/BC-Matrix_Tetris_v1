
#include <LRAS1130.h>

using namespace lr;
AS1130 ledDriver;

typedef AS1130Picture24x5 Picture;
Picture boardPicture; // The picture of the board, score, etc.
Picture blankPicture; // A picture with nothing drawn on it

void initializeDisplay() {
  // Start I2C
  Wire.begin();
  
  // Wait until the chip is ready.
  delay(100);
  
  // TODO Check if the chip is addressable
  /*if (!ledDriver.isChipConnected()) {
    Serial.println(F("Communication problem with chip."));
    Serial.flush();
    return;
  }*/

  // Set-up everything
  ledDriver.setRamConfiguration(AS1130::RamConfiguration1);
  ledDriver.setOnOffFrameAllOff(0);
  ledDriver.setBlinkAndPwmSetAll(0);
  ledDriver.setCurrentSource((AS1130::Current)0x10 /*AS1130::Current5mA*/); // 0 - 30 (in 5 mA increments)
  ledDriver.setScanLimit(AS1130::ScanLimitFull);
  ledDriver.setMovieEndFrame(AS1130::MovieEndWithFirstFrame);
  ledDriver.setMovieFrameCount(4);
  ledDriver.setFrameDelayMs(100);
  ledDriver.setMovieLoopCount(AS1130::MovieLoop6);
  ledDriver.setScrollingEnabled(true);
  ledDriver.startPicture(0);
  
  // Enable the chip
  ledDriver.startChip();
}

// Updates the board display
// drawTetramino determines whether or not to draw the active tetramino
// curtain fills all the pixels up to the given number (if positive),
//   or down to the given number (if negative). Zero means no curtain will be drawn.
void drawBoard(){
  drawBoard(true);
}
void drawBoard(bool drawTetramino){
  drawBoard(drawTetramino, 0);
}
void drawBoard(bool drawTetramino, int curtain) {
  // Render and output the current state of the board
  for (int boardY = 0; boardY < BOARD_HEIGHT; boardY++) {
    // A row of the image to draw
    uint16_t row;

    // Draw board or curtain
    if (boardY < curtain || boardY >= (BOARD_HEIGHT + curtain)) {
      // Draw curtain
      row = 0xffff;
    } else {
      // Get a row from the field
      row = field[boardY + BORDER_Y];
    }
    
    // Draw active piece
    if (drawTetramino) {
      uint16_t tetraminoShape = TETRAMINO_SHAPES[tetraminoType][tetraminoR];
      int tetraminoRowIndex = boardY + BORDER_Y - tetraminoY;
      if (tetraminoRowIndex >= 0 && tetraminoRowIndex < TETRAMINO_SIZE) {
        row |= ((tetraminoShape >> (TETRAMINO_SIZE * tetraminoRowIndex)) &
            TETRAMINO_MASK) << tetraminoX;
      }
    }

    // Draw row
    for (uint16_t rowMask = 1 << (BOARD_WIDTH + BORDER_X - 1), x = 0; (rowMask & ~BORDER_MASK) != 0; rowMask >>= 1, x++) {
      boardPicture.setPixel(BOARD_HEIGHT - 1 - boardY, BOARD_WIDTH - 1 - x, row & rowMask);
    }
  }
  ledDriver.setOnOffFrame(0, boardPicture);
  
  // DEBUG
//  printBoard();
}

// TODO possibly rename this to include other display data e.g. next piece, level number etc.
void drawScore() {
  // TODO Render and output the current player score
  Serial.print("Score: ");
  Serial.println(score);
}

void drawBlank() {
  ledDriver.setOnOffFrame(0, blankPicture);
}

/******************************************************************************
 * TEXT
 ******************************************************************************/

const byte letters5High[26][5] = {
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b11110, 0b10001, 0b11110, 0b10001, 0b11110},  // B
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b01111, 0b10001, 0b10011, 0b10000, 0b01111},  // G
  {0b10001, 0b10001, 0b11111, 0b10001, 0b10001},  // H
  {0b11111, 0b00100, 0b00100, 0b00100, 0b11111},  // I
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  {0b10001, 0b10001, 0b11111, 0b10001, 0b01110},  // A
  
  {0b11111, 0b01000, 0b00100, 0b00010, 0b11111}  // Z
  };

void drawText5High(const char* text) {
  for (char i = 0, c = text[i]; i < 4 && c != '\0'; c = text[++i]) {
    uint16_t y = FIELD_HEIGHT - 6 * (i + 1) + 1;
//    char c = text[0];
    for (byte r = 0; r < 5; r++) {
      field[y + r] = letters5High[c - 'A'][r] << BORDER_X;
    }
  }
}

// TODO Make a separate picture for this
const byte scoreText[24] = {
  0b01111,
  0b11000,
  0b00111,
  0b11110,
  0b00000,
  0b01111,
  0b10000,
  0b10000,
  0b01111,
  0b00000,
  0b01110,
  0b10001,
  0b10001,
  0b01110,
  0b00000,
  0b11110,
  0b10001,
  0b11110,
  0b10001,
  0b00000,
  0b11111,
  0b11100,
  0b10000,
  0b11111};

void drawTextScore() {
  for (byte r = 0; r < 24; r++) {
    field[FIELD_HEIGHT - 1 - r] = scoreText[r] << BORDER_X;
  }
}

/******************************************************************************
 * DEBUG
 ******************************************************************************/

void printBoard() {
  for (int y = FIELD_HEIGHT - 1; y >= BORDER_Y; y--) {
    // Get a row from the field
    uint16_t row = field[y];
    
    // Draw active piece
    uint16_t tetraminoShape = TETRAMINO_SHAPES[tetraminoType][tetraminoR];
    int tetraminoRowIndex = y - tetraminoY;
    if (tetraminoRowIndex >= 0 && tetraminoRowIndex < TETRAMINO_SIZE) {
      row |= ((tetraminoShape >> (TETRAMINO_SIZE * tetraminoRowIndex)) &
          TETRAMINO_MASK) << tetraminoX;
    }
    
    for (uint16_t rowMask = 1 << (BOARD_WIDTH + BORDER_X - 1); (rowMask & ~BORDER_MASK) != 0; rowMask >>= 1) {
      Serial.print(row & rowMask ? "[]" : "'.");
    }
    Serial.print(' ');
    Serial.println(y);
  }
  Serial.println();
}

void printField() {
  for (int y = FIELD_HEIGHT - 1; y >= 0; y--) {
    // Get a row from the field
    uint16_t row = field[y];
    
    // Draw active piece
    uint16_t tetraminoShape = TETRAMINO_SHAPES[tetraminoType][tetraminoR];
    int tetraminoRowIndex = y - tetraminoY;
    if (tetraminoRowIndex >= 0 && tetraminoRowIndex < TETRAMINO_SIZE) {
      row |= ((tetraminoShape >> (TETRAMINO_SIZE * tetraminoRowIndex)) &
          TETRAMINO_MASK) << tetraminoX;
    }
    
    for (uint16_t rowMask = 0x8000; rowMask != 0; rowMask >>= 1) {
      Serial.print(row & rowMask ? "[]" : "'.");
    }
    Serial.print(' ');
    Serial.println(y);
  }
  Serial.println();
}

