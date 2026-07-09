// User-definable setup definitions
#define MATRIX_WIDTH 13
#define MATRIX_HEIGHT 6
#define OFFSET 4 //LEDs to customize before the main grid
#define LED_COUNT (MATRIX_WIDTH * MATRIX_HEIGHT) + OFFSET
#define LED_PIN 25
#define FRAME_COUNT 14

const uint8_t MATRIX[MATRIX_HEIGHT][MATRIX_WIDTH] PROGMEM = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12},
    {25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13},
    {26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38},
    {51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39},
    {52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64},
    {77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65}
}; // 6 rows, 12 columns
#define TXT_ENABLED



/*
#define MATRIX_WIDTH 7
#define MATRIX_HEIGHT 7
#define LED_COUNT 32

const uint8_t MATRIX[MATRIX_HEIGHT][MATRIX_WIDTH] PROGMEM = {
    {255, 255, 255, 18, 19, 255, 255},
    {255, 5, 6, 17, 20, 31, 255},
    {255, 4, 7, 16, 21, 30, 255},
    {255, 3, 8, 15, 22, 29, 255},
    {255, 2, 9, 14, 23, 28, 255},
    {0, 1, 10, 13, 24, 26, 27},
    {255, 255, 11, 12, 25, 255, 255}
    //7x7 Christmas tree pattern. 255 indicates no LED in that cell.
}
*/

/// DO NOT USE 255 as a value in the MATRIX array. It is used to indicate an empty cell and will be ignored by the program.