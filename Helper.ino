#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>

/// @brief Helper functions for the LED strip.

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
    strip.clear();
    strip.show();
  }
}

/// @brief Clears the colors for a specific frame.
/// @param k The frame index to clear.
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
///@param r Red [0-255]
///@param g Green [0-255]
///@param b Blue [0-255]
uint32_t c(uint8_t r, uint8_t g, uint8_t b)
{
  return strip.Color(r, g, b);
}

///@brief Generates black color.
///@param void
uint32_t c()
{
  return strip.Color(0, 0, 0);
}

///@brief Generates a color from a three digit number (0-999).
///@param in The number to break down [digit 1 r, 2 g, 3 b]
///@param silent Silence Serial output; default true
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
    newr = 10;
  }

  if (g == 1)
  {
    newg = 10;
  }

  if (b == 1)
  {
    newb = 10;
  }

  if (!silent)
  {
    Serial.print("> Color code `");
    Serial.print(in);
    Serial.print(": rgb(");
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

bool burnSerial(const char *shouldMatch, unsigned long timeoutMs = 500)
{
  if (!shouldMatch)
    return false;

  size_t len = strlen(shouldMatch);
  if (len == 0)
    return true;

  unsigned long start = millis();
  size_t idx = 0;

  while (idx < len && (millis() - start) < timeoutMs)
  {
    if (Serial.available() == 0)
      continue; // wait for data until timeout

    int c = Serial.peek();
    if (c < 0)
      continue;

    if ((char)c == shouldMatch[idx])
    {
      idx++;
      Serial.read();
    }
    else
    {
      return false;
    }
  }

  return (idx == len);
}

bool burnSerialSeparator(unsigned long timeoutMs = 500)
{
  unsigned long start = millis();

  while ((millis() - start) < timeoutMs)
  {
    if (Serial.available() == 0)
      continue; // wait for data until timeout

    int c = Serial.peek();
    if (c < 0)
      continue;

    if ((char)c == '0' || (char)c == '1' || (char)c == '2' || (char)c == '3' || (char)c == '4' || (char)c == '5' || (char)c == '6' || (char)c == '7' || (char)c == '8' || (char)c == '9')
    {
      return false;
    }
    else
    {
      Serial.read();
      return true;
    }
  }

  return true;
}

/// @brief Transforms an x and y value into an LED index based on the MATRIX array.
/// @param x The x value
/// @param y The y value
/// @return The LED index corresponding to the x and y values; 0 and Serial warning if the cell is empty or out of bounds.
uint8_t xyToIndex(int x, int y)
{
  if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT)
  {
    Serial.println("> Warning: xyToIndex: 'cell (" + String(x) + "," + String(y) + ") is out of bounds'");
    return 0;
  }

  uint8_t index = pgm_read_byte(&(MATRIX[x][y]));
  if (index == 255)
  {
    Serial.println("> Warning: xyToIndex: 'cell (" + String(x) + "," + String(y) + ") is empty'");
    return 0;
  }
  return index;
}

/// @brief Adds a command and its parameters to the command history.
/// @param command The command string to add to the history.
void addToCommandHistory(const char command[3], const char *params)
{
  String fullCommand = String(command) + " " + String(params);
  fullCommand.trim();
  strncpy(commandHistory[commandHistoryIndex], fullCommand.c_str(), HISTORY_MAX_LEN - 1);
  commandHistory[commandHistoryIndex][HISTORY_MAX_LEN - 1] = '\0'; // Ensure null termination
  commandHistoryIndex = (commandHistoryIndex + 1) % MAX_HISTORY;
  if (commandHistoryCount < MAX_HISTORY)
  {
    commandHistoryCount++;
  }
  else
  {
    Serial.println("> Command history full. New commands will overwrite the latest.");
  }
}