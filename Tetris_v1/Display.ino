
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

