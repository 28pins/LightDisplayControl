#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

/// @brief Loads the last saved colors and delays into the current arrays.
void loadLast()
{
  uint16_t buf = 0;
  for (uint8_t k = 0; k < FRAME_COUNT; k++)
  {
    for (uint8_t l = 0; l < LED_COUNT; l++)
    {
      buf = colorsLast[k][l];
      colorsLast[k][l] = colors[k][l];
      colors[k][l] = buf;
    }
    buf = delayLast[k];
    delayLast[k] = cycleDelay[k];
    cycleDelay[k] = buf;
  }
}

void printNumPadded(uint16_t in) {
  if(in == 255) {
    Serial.print("---"); //Special case in matrix printing
  } else if (in < 10) {
    Serial.print(" ");
    Serial.print(in);
    Serial.print(" ");
  } else if (in < 100) {
    Serial.print(" ");
    Serial.print(in);
  } else {
    Serial.print(in);
  }
}



/// @brief Saves the current colors and delays into the "last" arrays for potential later retrieval.  Call this before making changes to the current arrays if you want to be able to easily revert back to the previous state.
void saveLast()
{
  for (uint8_t k = 0; k < FRAME_COUNT; k++)
  {
    for (uint8_t l = 0; l < LED_COUNT; l++)
    {
      colorsLast[k][l] = colors[k][l];
    }
    delayLast[k] = cycleDelay[k];
  }
}

/// @brief Clears all colors and delays from the current arrays and the strip.
void clear()
{
  for (uint8_t k = 0; k < FRAME_COUNT; k++)
  {
    for (uint8_t l = 0; l < LED_COUNT; l++)
    {
      colors[k][l] = 0;
    }
    cycleDelay[k] = 0;
  }
  strip.clear();
  strip.show();
}

/// @brief Clears the colors for a specific frame.
void clear(uint8_t k)
{
  for (uint8_t l = 0; l < LED_COUNT; l++)
  {
    colors[k][l] = 0;
  }
  strip.clear();
  strip.show();
}

///@brief Generates a color.
uint32_t c(uint8_t r, uint8_t g, uint8_t b)
{
  return strip.Color(r, g, b);
}

///@brief Generates black color.
uint32_t c()
{
  return strip.Color(0, 0, 0);
}

uint32_t c(uint32_t in, bool silent = true)
{

  uint8_t r = floor(in / 100);
  uint8_t g = floor((in - (100 * r)) / 10);
  uint8_t b = floor(in - (100 * r) - (10 * g));

  r = constrain(r, 0, 9);
  g = constrain(g, 0, 9);
  b = constrain(b, 0, 9);

  uint8_t newr = 3 * r * r;
  uint8_t newg = 3 * g * g;
  uint8_t newb = 3 * b * b;

  if (r == 1)
  {
    newr = 6;
  }

  if (g == 1)
  {
    newg = 6;
  }

  if (b == 1)
  {
    newb = 6;
  }

  if (!silent)
  {
    Serial.print("> Color code `");
    Serial.print(in);
    Serial.print("`: rgb(");
    Serial.print(newr);
    Serial.print(", ");
    Serial.print(newg);
    Serial.print(", ");
    Serial.print(newb);
    Serial.print(")");
    Serial.println();
  }

  return strip.Color(newr, newg, newb);
}

/// @brief Saves the current colors and delays to EEPROM.
void saveEEPROM()
{
  EEPROM.write(0, 1); // Set a flag to indicate that data is saved
  EEPROM.put(1, colors);
  EEPROM.put(sizeof(colors) + 1, cycleDelay);
}

/// @brief Loads the colors and delays from EEPROM into the current arrays.
/// @return true if data was successfully loaded, false if no valid data was found.
bool loadEEPROM()
{
  if (EEPROM.read(0) != 1)
  {
    return false;
  }
  EEPROM.get(1, colors);
  EEPROM.get(sizeof(colors) + 1, cycleDelay);
  return true;
}

/// @brief Clears all data from EEPROM.
void clearEEPROM()
{
  for (int i = 0; i < EEPROM.length(); i++)
  {
    EEPROM.write(i, 0);
  }
}

void toggleBuiltin(){
  if (LEDBuiltinOn) {
    digitalWrite(LED_BUILTIN, LOW);
  } else {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  LEDBuiltinOn = !LEDBuiltinOn;
}

/// @brief Transforms an x and y value into an LED index based on the MATRIX array.
uint8_t xyToIndex(int x, int y)
{
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT)
  {
    Serial.print(F("> Warning: xyToIndex: 'cell ("));
    Serial.print(x);
    Serial.print(F(","));
    Serial.print(y);
    Serial.println(F(") is out of bounds'"));
    return 255;
  }

  uint8_t index = pgm_read_byte(&(MATRIX[y][x]));
  if (index == 255)
  {
    Serial.print(F("> Warning: xyToIndex: 'cell ("));
    Serial.print(x);
    Serial.print(F(","));
    Serial.print(y);
    Serial.println(F(") is empty'"));
    return 255;
  }
  return index;
}

uint8_t indToX(uint8_t ind){
  for(uint8_t y = 0; y < MATRIX_HEIGHT; y++){
    for(uint8_t x = 0; x < MATRIX_WIDTH; x++){
      if(pgm_read_byte(&(MATRIX[y][x])) == ind){
        return x;
      }
    }
  }
  return 255;
}

uint8_t indToY(uint8_t ind){
  for(uint8_t y = 0; y < MATRIX_HEIGHT; y++){
    for(uint8_t x = 0; x < MATRIX_WIDTH; x++){
      if(pgm_read_byte(&(MATRIX[y][x])) == ind){
        return y;
      }
    }
  }
  return 255;
}

String extractCmd(const String& in){
  String out = in;
  while (out.indexOf('\n') != -1) {
    out = out.substring(0, out.indexOf('\n'));
  }
  uint8_t idxStr = out.indexOf(' ');
  if(idxStr == -1){
    return out;
  } else {
    return out.substring(0, idxStr);
  }
}

uint8_t parseIntCharsToBurn(const String& s) {
  uint8_t parseIntIdx = 0;
  bool shouldFinish = false;
  while (!shouldFinish) {
    if (isDigit(s.charAt(parseIntIdx))) {
      parseIntIdx++;
    } else {
      shouldFinish = true;
    }
  }
  return parseIntIdx;
}

int parseInt(const String& s) {
  uint8_t parseIntIdx = 0;
  int out = 0;
  bool shouldFinish = false;
  while (!shouldFinish) {
    if (isDigit(s.charAt(parseIntIdx))) {
      out *= 10;
      out += s.charAt(parseIntIdx) - '0';
      parseIntIdx++;
    } else {
      shouldFinish = true;
    }
  }
  return out;
}
