#include "Matrix.h"
/// @brief Runs the inner loop for a given frame index, displaying the colors and applying the delay.
/// @param i The frame index to run.
void colorLoop(uint8_t i)
{
  for (uint8_t k = 0; k < LED_COUNT; k++)
  {
    strip.setPixelColor(k + OFFSET, c(colors[i][k]));
  }
  strip.show();
}

void textLoop(uint8_t i) {
  uint16_t renderWidth = MATRIX_WIDTH;
  for (uint8_t ind = 1; ind < LED_COUNT; ind++) {
    if (colors[i][ind] == '-') {
      renderWidth += 3;
    } else if (colors[i][ind] != 0) {
      uint8_t charIdx = charToIndex(colors[i][ind]);
      for (int8_t shift = 5; shift >= 0; shift--) {
        bool hasSeen1ThisShift = false;
        for (uint8_t height = 0; height < MATRIX_HEIGHT; height++) {
          uint8_t glyphRow = pgm_read_byte(&(characters[charIdx][height]));
          if ((glyphRow >> shift) & 1) {
            hasSeen1ThisShift = true;
          }
        }
        if (hasSeen1ThisShift) {
          renderWidth++;
        }
      }
      renderWidth++;
    } else {
      break;
    }
  }

  bool *rendered = new bool[MATRIX_HEIGHT * renderWidth]();
  uint16_t renderStart = MATRIX_WIDTH;
  for (uint8_t ind = 1; ind < LED_COUNT; ind++) {
    if(colors[i][ind] == '-'){
      renderStart += 3;
    } else if (colors[i][ind] != 0) {
          uint8_t charIdx = charToIndex(colors[i][ind]);
          for (int8_t shift = 5; shift >= 0; shift--) {
        bool hasSeen1ThisShift = false;
        for (uint8_t height = 0; height < MATRIX_HEIGHT; height++) {
          uint16_t colIdx = renderStart;
              uint8_t glyphRow = pgm_read_byte(&(characters[charIdx][height]));
              if ((glyphRow >> shift) & 1) {
            hasSeen1ThisShift = true;
            if (colIdx < renderWidth) {
              rendered[(MATRIX_HEIGHT - height - 1) * renderWidth + colIdx] = 1; //Assumes (0, 0) is top left; use `rendered[height][colIdx] for bottom left (0, 0)`
            }
          }
        }
        if(hasSeen1ThisShift) {
          renderStart++;
        }
      }
      renderStart++; //Space
    } else {
      break;
    }
  }
  renderWidth = renderStart;
  for (uint16_t h = 0; h < renderWidth + MATRIX_WIDTH; h++) {
    strip.clear();
    for (uint16_t j = 0; j < MATRIX_WIDTH; j++) {
      for (uint8_t k = 0; k < MATRIX_HEIGHT; k++) {
        if ((h + j) < renderWidth && rendered[k * renderWidth + h + j] && (h < renderWidth) && colors[i][0] != 0) {
          strip.setPixelColor(xyToIndex(j, k) + OFFSET, c(colors[i][0]));
        } else {
          strip.setPixelColor(xyToIndex(j, k) + OFFSET, 0);
        }
      }
    }
    strip.show();
    delay(200);
    serialHandle();
  }

  delete[] rendered;
}

void serialHandle(){
  if(Serial.available() <= 0) return;
  delay(200);
  String cmd = "";
  String cmdNext = "";
  cmd.reserve(192);
  cmdNext.reserve(192);
  uint8_t parseIdx = 0;
  bool mustReParse = false;
  while(Serial.available() > 0 || mustReParse){
    char in = '\n';
    if (!mustReParse) {
      in = (char)Serial.read();
      cmd += in;
    }
    if(in == '\n' || mustReParse){
      String cmdM = cmd; //Modifiable
      int indNextCmd = cmd.indexOf(" && ");
      if(indNextCmd > 0) {
        mustReParse = true;
        cmdNext = cmd;
        cmdNext.remove(0, indNextCmd + 4);
        cmdM = cmd;
        cmdM.remove(indNextCmd);
      } else {
        mustReParse = false;
        cmdNext = "";
      }
      Serial.print(F("< "));
      Serial.print(cmdM);
      //Crawl command!
      if (cmdM.startsWith("help")) {
        Serial.println(F(R"(
> Help: --------------------------
> <param> is entered as param!
> * - All available options in a range
> set <[o/e]frame[-frame2]> ([o/e]<x[-x2]>, [o/e]<y[-y2]>) <color 0-999> - Set a specific LED or series in a specific frame to a color.  Use o/e before a series to only apply to the odd or even indexed LEDs.  Use - to specify a range of frames or LEDs.
> clr <[o/e]frame[-frame2]> ([o/e]<x[-x2]>, [o/e]<y[-y2]>) - Clear a specific LED or series in a specific frame.  Use o/e before a series to only apply to the odd or even indexed LEDs.  Use - to specify a range of frames or LEDs.
> clr <[o/e]frame[-frame2]> - Clear all LEDs in a specific frame
> dly <[o/e]frame[-frame2]> <delay> - Set the delay for a specific frame or series of frames.  Enter in ms.  Use o/e before a series to only apply to the odd or even indexed frames.  Use - to specify a range of frames.
> text <frame> <color 0-999> <text> - Display scrolling text on the display in place of a specific frame.  Use '-' as a space.  All letters are displayed as uppercase.  use only [a-z][A-Z][0-9] and !.
> print - Prints all previous commands since the last `clr` command.
> mtx [-c <frame>][-n] - Display an ASCII representation of the LED matrix (-n gives LED IDs)
> export [—program] - Exports a C array of the current colors and delays for all frames, which can be copied into the program for static displays. (optionally as a program)
> help - Displays this help message.
> idx n - (x, y) of LED n
> idx (x, y) - number of LED at (x, y)
> undo - revert changes
> clr - Clear all frames and delays.
> clr -e - Clear EEPROM
> && - Used to chain multiple commands together.  Example: `dly 0-4 500 && txt 0 Hello 555` sets frames 0-4 to a delay of 500ms and sets frame 0 to display "Hello" in color code 555.
> wrt - Writes the current frames and delays to EEPROM for retrieval on the next startup.
> rgb <color 0-999> - Replies with the RGB values for a specific color code.
> swap <[o/e]frame[-frame2]> <from> <to> - replaces all instances of <from> leds on a given frame with <to> leds
> copy <frameFrom> <[o/e]frameTo[-frameToEnd]> - duplicates a frame to a given frame range 
)"));
      } else if (cmdM.startsWith("text")) {
        cmdM.remove(0, 5);
        uint8_t f = parseInt(cmdM);
        cmdM.remove(0, parseIntCharsToBurn(cmdM)+1);
        for(uint8_t x = 0; x < LED_COUNT; x++){
          colors[f][x] = 0;
        }
        colors[f][0] = parseInt(cmdM);
        cmdM.remove(0, parseIntCharsToBurn(cmdM)+1);
        cycleDelay[f] = 254; //Special code
        uint8_t in = 1;
        char c = 'a';
        Serial.print("> Recording text: ");
        while (c != '\n') {
          c = cmdM[0];
          cmdM.remove(0, 1);
          if(c != 10){
            colors[f][in] = c;
          }
          in++;
          const char cx = c;
          Serial.print(cx);
        }
        Serial.println();
      } else if (cmdM.startsWith("copy")) {
        uint8_t from = 0;
        uint8_t to = 0;
        uint8_t parity = 2;
        cmdM.remove(0, 5);
        uint8_t source = parseInt(cmdM);
        cmdM.remove(0, parseIntCharsToBurn(cmdM)+1);
        if(cmdM.startsWith("o")) {
          parity = 1;
          cmdM.remove(0, 1);
        } else if (cmdM.startsWith("e")) {
          parity = 0;
          cmdM.remove(0, 1);
        }
        if (cmdM.startsWith("*")){
          from = 0;
          to = FRAME_COUNT;
          cmdM.remove(0, 1);
        } else {
          from = parseInt(cmdM);
          cmdM.remove(0, parseIntCharsToBurn(cmdM));
          if (cmdM.startsWith("-")) {
            cmdM.remove(0, 1);
            to = parseInt(cmdM);
            cmdM.remove(0, parseIntCharsToBurn(cmdM));
            if (to < from) {
              to = from;
            }
          } else {
            to = from;
          }
          Serial.println("\n> Cleared requested LEDs.");
        }
        saveLast();
        for (uint8_t id = from; id <= to; id++) {
          if((parity == 2) || (parity == id % 2)){
            for (uint8_t co = 0; co < LED_COUNT; co++) {
              colors[id][co] = colors[source][co];
            }
          }
          delay(100);
        }
        Serial.println("> Done.");
      } else if (cmdM.startsWith("swap")) {
        uint8_t from = 0;
        uint8_t to = 0;
        uint8_t parity = 2;
        cmdM.remove(0, 5);
        if(cmdM.startsWith("o")) {
          parity = 1;
          cmdM.remove(0, 1);
        } else if (cmdM.startsWith("e")) {
          parity = 0;
          cmdM.remove(0, 1);
        }
        if (cmdM.startsWith("*")){
          from = 0;
          to = FRAME_COUNT;
          cmdM.remove(0, 1);
        } else {
          from = parseInt(cmdM);
          cmdM.remove(0, parseIntCharsToBurn(cmdM));
          if (cmdM.startsWith("-")) {
            cmdM.remove(0, 1);
            to = parseInt(cmdM);
            cmdM.remove(0, parseIntCharsToBurn(cmdM));
            if (to < from) {
              to = from;
            }
          } else {
            to = from;
          }
        }
        saveLast();
        cmdM.remove(0, 1); //burn space
        uint16_t from2 = parseInt(cmdM);
        cmdM.remove(0, parseIntCharsToBurn(cmdM)+1); //burn space
        uint16_t to2 = parseInt(cmdM);
        for (uint8_t id = from; id <= to; id++) {
          if((parity == 2) || (parity == id % 2)){
            for (uint8_t co = 0; co < LED_COUNT; co++) {
              if (colors[id][co] == from2) {
                colors[id][co] = to2;
              }
            }
          }
          delay(100);
        }
        Serial.println("> Done.");
      } else if (cmdM.startsWith("clr")) {
        if (cmdM.length() < 6) {
          saveLast();
          clear();
          Serial.println("> All light data cleared; `undo` to revert.");
        } else if (cmdM.length() >= 6 && cmdM.charAt(4) == '-' && cmdM.charAt(5) == 'e') {
          clearEEPROM();
        } else {
          saveLast();
          uint8_t from = 0;
          uint8_t to = 0;
          uint8_t parity = 2;
          uint8_t fromX = 0;
          uint8_t toX = MATRIX_WIDTH - 1;
          uint8_t parityX = 2;
          uint8_t fromY = 0;
          uint8_t toY = MATRIX_HEIGHT - 1;
          uint8_t parityY = 2;
          cmdM.remove(0, 4);
          if(cmdM.startsWith("o")) {
            parity = 1;
            cmdM.remove(0, 1);
          } else if (cmdM.startsWith("e")) {
            parity = 0;
            cmdM.remove(0, 1);
          }
          if (cmdM.startsWith("*")){
            from = 0;
            to = FRAME_COUNT;
            cmdM.remove(0, 1);
          } else {
            from = parseInt(cmdM);
            cmdM.remove(0, parseIntCharsToBurn(cmdM));
            if (cmdM.startsWith("-")) {
              cmdM.remove(0, 1);
              to = parseInt(cmdM);
              cmdM.remove(0, parseIntCharsToBurn(cmdM));
              if (to < from) {
                to = from;
              }
            } else {
              to = from;
            }
          }

          cmdM.remove(0, 1); //burn space
          if (cmdM.startsWith("(")) {
            cmdM.remove(0, 1);
            if(cmdM.startsWith("o")) {
              parityX = 1;
              cmdM.remove(0, 1);
            } else if (cmdM.startsWith("e")) {
              parityX = 0;
              cmdM.remove(0, 1);
            }
            if (cmdM.startsWith("*")){
              cmdM.remove(0, 1);
            } else {
              fromX = parseInt(cmdM);
              cmdM.remove(0, parseIntCharsToBurn(cmdM));
              if (cmdM.startsWith("-")) {
                cmdM.remove(0, 1);
                toX = parseInt(cmdM);
                cmdM.remove(0, parseIntCharsToBurn(cmdM));
                if (toX < fromX) {
                  toX = fromX;
                }
              } else {
                toX = fromX;
              }
            }

            cmdM.remove(0, 2); //burn comma and space
            if(cmdM.startsWith("o")) {
              parityY = 1;
              cmdM.remove(0, 1);
            } else if (cmdM.startsWith("e")) {
              parityY = 0;
              cmdM.remove(0, 1);
            }
            if (cmdM.startsWith("*")){
              cmdM.remove(0, 1);
            } else {
              fromY = parseInt(cmdM);
              cmdM.remove(0, parseIntCharsToBurn(cmdM));
              if (cmdM.startsWith("-")) {
                cmdM.remove(0, 1);
                toY = parseInt(cmdM);
                cmdM.remove(0, parseIntCharsToBurn(cmdM));
                if (toY < fromY) {
                  toY = fromY;
                }
              } else {
                toY = fromY;
              }
            }
          } else {
            to = from;
          }

          Serial.print("> Clearing");
          for (uint8_t id = from; id <= to; id++) {
            if((parity == 2) || (parity == id % 2)){
              for (uint8_t idX = fromX; idX <= toX; idX++) {
                if((parityX == 2) || (parityX == idX % 2)){
                  for (uint8_t idY = fromY; idY <= toY; idY++) {
                    if((parityY == 2) || (parityY == idY % 2)){
                      colors[id][xyToIndex(idX, idY)] = 0;
                      Serial.print(".");
                    }
                    delay(20);
                  }
                }
                delay(50);
              }
            }
            delay(100);
          }
          Serial.println("\n> Cleared requested LEDs.");
        }
      } else if (cmdM.startsWith("dly")) {
        uint8_t from = 0;
        uint8_t to = 0;
        uint8_t parity = 2;
        cmdM.remove(0, 4);
        if(cmdM.startsWith("o")) {
          parity = 1;
          cmdM.remove(0, 1);
        } else if (cmdM.startsWith("e")) {
          parity = 0;
          cmdM.remove(0, 1);
        }
        if (cmdM.startsWith("*")){
          from = 0;
          to = FRAME_COUNT;
          cmdM.remove(0, 1);
        } else {
          from = parseInt(cmdM);
          cmdM.remove(0, parseIntCharsToBurn(cmdM));
          if (cmdM.startsWith("-")) {
            cmdM.remove(0, 1);
            to = parseInt(cmdM);
            cmdM.remove(0, parseIntCharsToBurn(cmdM));
            if (to < from) {
              to = from;
            }
          } else {
            to = from;
          }
        }
        saveLast();
        cmdM.remove(0, 1); //burn space
        uint16_t del = parseInt(cmdM);
        for (uint8_t id = from; id <= to; id++) {
          if((parity == 2) || (parity == id % 2)){
            cycleDelay[id] = del;
            Serial.print("> Delay for cycle ");
            Serial.print(id);
            Serial.print(" set to ");
            Serial.print(del);
            Serial.println(".");
          }
          delay(100);
        }
      } else if (cmdM.startsWith("wrt")) {
        saveEEPROM();
        Serial.println("> Done.");
      } else if (cmdM.startsWith("rgb")) {
        cmdM.remove(0, 4);
        c(parseInt(cmdM), false);
      } else if (cmdM.startsWith("set")) {
        saveLast();
        uint8_t from = 0;
        uint8_t to = 0;
        uint8_t parity = 2;
        uint8_t fromX = 0;
        uint8_t toX = MATRIX_WIDTH - 1;
        uint8_t parityX = 2;
        uint8_t fromY = 0;
        uint8_t toY = MATRIX_HEIGHT - 1;
        uint8_t parityY = 2;
        cmdM.remove(0, 4);
        if(cmdM.startsWith("o")) {
          parity = 1;
          cmdM.remove(0, 1);
        } else if (cmdM.startsWith("e")) {
          parity = 0;
          cmdM.remove(0, 1);
        }
        if (cmdM.startsWith("*")){
          from = 0;
          to = FRAME_COUNT;
          cmdM.remove(0, 1);
        } else {
          from = parseInt(cmdM);
          cmdM.remove(0, parseIntCharsToBurn(cmdM));
          if (cmdM.startsWith("-")) {
            cmdM.remove(0, 1);
            to = parseInt(cmdM);
            cmdM.remove(0, parseIntCharsToBurn(cmdM));
            if (to < from) {
              to = from;
            }
          } else {
            to = from;
          }
          cmdM = cmdM.substring(2);
        }

        cmdM.remove(0, 1); //burn space
        if (cmdM.startsWith("(")) {
          cmdM.remove(0, 1);
          if(cmdM.startsWith("o")) {
            parityX = 1;
            cmdM.remove(0, 1);
          } else if (cmdM.startsWith("e")) {
            parityX = 0;
            cmdM.remove(0, 1);
          }
          if (cmdM.startsWith("*")){
            cmdM.remove(0, 1);
          } else {
            fromX = parseInt(cmdM);
            cmdM.remove(0, parseIntCharsToBurn(cmdM));
            if (cmdM.startsWith("-")) {
              cmdM.remove(0, 1);
              toX = parseInt(cmdM);
              cmdM.remove(0, parseIntCharsToBurn(cmdM));
              if (toX < fromX) {
                toX = fromX;
              }
            } else {
              toX = fromX;
            }
          }

          cmdM.remove(0, 2); //burn comma and space
          if(cmdM.startsWith("o")) {
            parityY = 1;
            cmdM.remove(0, 1);
          } else if (cmdM.startsWith("e")) {
            parityY = 0;
            cmdM.remove(0, 1);
          }
          if (cmdM.startsWith("*")){
            cmdM.remove(0, 1);
          } else {
            fromY = parseInt(cmdM);
            cmdM.remove(0, parseIntCharsToBurn(cmdM));
            if (cmdM.startsWith("-")) {
              cmdM.remove(0, 1);
              toY = parseInt(cmdM);
              cmdM.remove(0, parseIntCharsToBurn(cmdM));
              if (toY < fromY) {
                toY = fromY;
              }
            } else {
              toY = fromY;
            }
          }
          cmdM.remove(0, 2);
        }

        uint16_t col = parseInt(cmdM);

        Serial.print("> Setting");
        for (uint8_t id = from; id <= to; id++) {
          if((parity == 2) || (parity == id % 2)){
            for (uint8_t idX = fromX; idX <= toX; idX++) {
              if((parityX == 2) || (parityX == idX % 2)){
                for (uint8_t idY = fromY; idY <= toY; idY++) {
                  if((parityY == 2) || (parityY == idY % 2)){
                    colors[id][xyToIndex(idX, idY)] = col;
                    Serial.print(".");
                  }
                  delay(20);
                }
              }
              delay(50);
            }
          }
          delay(100);
        }
        Serial.print("\n> Set requested LEDs to color ");
        Serial.print(col);
        Serial.println(".");
      } else if (cmdM.startsWith("export")) {
        Serial.println("> Export: ");
        if(cmdM.startsWith("export --program")) {
          Serial.print("#define LED_COUNT ");
          Serial.println(LED_COUNT);
          Serial.print("#define FRAME_COUNT ");
          Serial.println(FRAME_COUNT);
          Serial.println(F("#include <Adafruit_NeoPixel.h>\n#define LED_PIN 2\nAdafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800"));
        }
        Serial.print("uint16_t cycleDelay[FRAME_COUNT] = {");
        for (uint16_t d = 0; d < FRAME_COUNT; d++) {
          Serial.print(cycleDelay[d]);
          if (d != FRAME_COUNT - 1) {
            Serial.print(", ");
          }
        }
        Serial.print("};\nuint16_t colors[FRAME_COUNT] = {\n");
        for (uint16_t d = 0; d < FRAME_COUNT; d++) {
          Serial.print("{");
          for (uint16_t e = 0; e < LED_COUNT; e++) {
            Serial.print(colors[d][e]);
            if (e != LED_COUNT - 1) {
              Serial.print(", ");
            }
          }
          Serial.print(d != FRAME_COUNT - 1 ? "},\n" : "}\n");
        }
        Serial.println("};");
        if(cmdM.startsWith("export --program")) {
          Serial.print(F(R"(
void setup()
{
  strip.begin();
  strip.show();
  strip.setBrightness(50);
}
void loop()
{
  for (uint8_t i = 0; i < FRAME_COUNT; i++) {
    if (cycleDelay[i] != 0)
      {
        for (uint8_t k = 0; k < LED_COUNT; k++)
        {
          strip.setPixelColor(k + OFFSET, c(colors[i][k]));
        }
        strip.show();
        delay(cycleDelay[i]);
      }
    }
  }
}
          )"));
        }
      } else if (cmdM.startsWith("undo") || cmdM.startsWith("redo")) {
        loadLast();
        Serial.println("> Last changes undone.");
      } else if (cmdM.startsWith("idx")) {
        cmdM.remove(0, 4);
        uint8_t x, y, ind = 0;
        if(cmdM.startsWith("(")) {
          cmdM.remove(0, 1);
          x = parseInt(cmdM);
          cmdM.remove(0, parseIntCharsToBurn(cmdM) + 2);
          y = parseInt(cmdM);
          ind = xyToIndex(x, y);
        } else {
          ind = parseInt(cmdM);
          x = indToX(ind);
          y = indToY(ind);
        }
        Serial.print(F("> LED with index "));
        Serial.print(ind);
        Serial.print(F(" is at ("));
        Serial.print(x);
        Serial.print(F(", "));
        Serial.print(y);
        Serial.println(").");
      } else if (cmdM.startsWith("mtx")) {
        bool printNums = false;
        bool printCols = false;
        uint8_t printFrame = 0;
        if(cmdM[4] == '-' && cmdM[5] == 'n'){
          printNums = true;
        }
        if(cmdM[4] == '-' && cmdM[5] == 'c'){
          printCols = true;
          cmdM.remove(0, 7);
          printFrame = parseInt(cmdM);
        }
        Serial.println("> Matrix:");
        for (uint8_t l = 0; l < MATRIX_HEIGHT; l++){
          Serial.print("> ");
          for (uint8_t m = 0; m < MATRIX_WIDTH; m++) {
            if(printNums) {
              printNumPadded(pgm_read_byte(&(MATRIX[l][m])));
              Serial.print(" ");
            } else if (printCols) {
              printNumPadded(colors[printFrame][xyToIndex(m, l)]);
              Serial.print(" ");
            } else {
              Serial.print(pgm_read_byte(&(MATRIX[l][m])) == 255 ? " " : "o");
            }
          }
          Serial.println();
        }
      } else {
        Serial.print(F("> Command `"));
        Serial.print(extractCmd(cmd));
        Serial.println(F("` not recognized."));
      }
      cmd = cmdNext;
      Serial.println(mustReParse ? "&&" : "");
    }
  }
}
