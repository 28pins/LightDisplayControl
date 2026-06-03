/// @brief Runs the inner loop for a given frame index, displaying the colors and applying the delay.
/// @param i The frame index to run.
void innerLoop(int i)
{
  if (cycleDelay[i] != 0)
  {
    if (cycleDelay[i] == 254)
    {
      // special text code
      uint16_t color = colors[i][0];
      // TODO!
    }
    else
    {
      for (uint8_t j = 0; j < LED_COUNT; j++)
      {
        strip.setPixelColor(j, c(colors[i][j]));
      }
      strip.show();
      if (!Serial.available())
      {
        delay(cycleDelay[i] * 10);
      }
      else
      {
        delay(50);
      }
    }
  }
}

const char helpText[] PROGMEM = R"(
> Help: --------------------------

Commands:
set <[o/e]frame[-frame2]> ([o/e]<x[-x2]>, [o/e]<y[-y2]>) <color 0-999> - Set a specific LED or series in a specific frame to a color.  Use o/e before a series to only apply to the odd or even indexed LEDs.  Use - to specify a range of frames or LEDs.
clr <[o/e]frame[-frame2]> ([o/e]<x[-x2]>, [o/e]<y[-y2]>) - Clear a specific LED or series in a specific frame.  Use o/e before a series to only apply to the odd or even indexed LEDs.  Use - to specify a range of frames or LEDs.
clr - Clear all frames and LEDs.
clr <[o/e]frame[-frame2]> - Clear all LEDs in a specific frame
dly <[o/e]frame[-frame2]> <delay> - Set the delay for a specific frame or series of frames.  Enter in 100ths of a second. (1: 100ms, 10: 1s, 1000: 10s)  Use o/e before a series to only apply to the odd or even indexed frames.  Use - to specify a range of frames.
txt <frame> <color 0-999> <text> - Display scrolling text on the display in place of a specific frame.  Text will scroll at a speed determined by 1/10 the frame's delay per column.
rgb <color 0-999> - Replies with the RGB values for a specific color code.
prt - Prints all previous commands since the last `clr` command.
mtx [-n] - Display an ASCII representation of the LED matrix (-n gives LED IDs)
exp - Exports a C array of the current colors and delays for all frames, which can be copied into the program for static displays.
xep - Clears EEPROM memory, erasing all saved frames and delays.  Use with caution.
wrt - Writes the current frames and delays to EEPROM for retrieval on the next startup.
&& - Used to chain multiple commands together.  Example: `dly 0-4 500 && txt 0 Hello 555` sets frames 0-4 to a delay of 500ms and sets frame 0 to display "Hello" in color code 555.
hlp - Displays this help message.
)";

enum Command
{
  SET = 0,
  CLR = 1,
  DLY = 2,
  TXT = 3,
  RGB = 4,
  PRT = 5,
  MTX = 6,
  HLP = 7,
  UNKNOWN = 8,
  EXP = 9,
  XEP = 10,
  UND = 11,
  WRT = 12
};

/*
Flag:
$ x $ x $ 0 0 0 0 0 0 0
x $ x $ x - - - - - - -
$ x $ x $ 0 0 0 0 0 0 0
- - - - - - - - - - - -
0 0 0 0 0 0 0 0 0 0 0 0
- - - - - - - - - - - -

set <frame> (e0-2, 5-11) 400
set <frame> (o0-2, 5-11) 333
set <frame> (o3-5, 0-11) 333
set <frame> (4, 0-11) 400
set <frame> (0-2, 0-4) 4
set <frame> (e0-2, e0-4) 444
set <frame> (1, o1-3) 444
*/

/*
TX:
8 8 8 8 8 x x x x x x x
8 8 . 8 8 x x x x x x x
8 . . . 8 x x x x x x x
8 * . * 8 - - - - - - -
8 . 8 . 8 - - - - - - -
8 8 8 8 8 - - - - - - -

set <frame> (0-2, 4-11) 333
set <frame> (3-5, 4-11) 400
set <frame> (0-5, 0-3) 4
set <frame> (e2-4, o1-3) 444
set <frame> (3, o1-3) 224
set <frame> (1-3, 3) 444
*/

void serialLoop()
{
  if (Serial.available())
  {
    switch (matchCommand())
    {
    case Command::SET:
      Serial.print("< set");
      saveLast();
      RangeData frameData = {0, FRAME_COUNT - 1, PARITY_ALL};
      RangeData xData = {0, MATRIX_WIDTH - 1, PARITY_ALL};
      RangeData yData = {0, MATRIX_HEIGHT - 1, PARITY_ALL};
      frameData = parseRange(FRAME_COUNT - 1);
      Serial.print(" ");
      burnSerial(" ");
      if (burnSerial("("))
      {
        Serial.print("(");
        xData = parseRange(MATRIX_WIDTH - 1);
        if (burnSerial(","))
        {
          Serial.print(", ");
          burnSerial(" ");
          yData = parseRange(MATRIX_HEIGHT - 1);
          if (burnSerial(")"))
          {
            Serial.print(") ");
          }
          else
          {
            Serial.println();
            Serial.println("> Warning: Expected `)` after y range");
            return;
          }
        }
        else
        {
          Serial.println();
          Serial.println("> Warning: Expected `,` after x range");
          return;
        }
      }
      else
      {
        Serial.println();
        Serial.println("> Warning: Expected `(` after frame range");
        return;
      }
      uint16_t color = Serial.parseInt();
      Serial.println();
      Serial.println("> Set frames " + String(frameData.from) + (frameData.to != frameData.from ? "-" + String(frameData.to) : "") + " to color " + String(color) + " for LEDs " + (xData.from != 255 ? "(" + String(xData.from) + (xData.to != xData.from ? "-" + String(xData.to) : "") + ")" : "") + (yData.from != 255 ? "(" + String(yData.from) + (yData.to != yData.from ? "-" + String(yData.to) : "") + ")" : "") + (xData.parity != PARITY_ALL || yData.parity != PARITY_ALL ? " selectively" : ""));
      for (int8_t i = frameData.from; i <= frameData.to; i++)
      {
        for (uint8_t x = xData.from; x <= xData.to; x++)
        {
          for (uint8_t y = yData.from; y <= yData.to; y++)
          {
            if ((i % 2 == 0) && (frameData.parity == PARITY_ODD || xData.parity == PARITY_ODD || yData.parity == PARITY_ODD))
              continue; // skip even
            if ((i % 2 == 1) && (frameData.parity == PARITY_EVEN || xData.parity == PARITY_EVEN || yData.parity == PARITY_EVEN))
              continue; // skip odd
            if (xyToIndex(x, y) == 255)
              continue;
            colors[i][xyToIndex(x, y)] = color;
          }
        }
      }
      break;
    case Command::CLR:
      saveLast();
      Serial.print("< clr ");
      if (Serial.peek() == '-' || Serial.peek() == 'o' || Serial.peek() == 'e' || Serial.peek() == ' ')
      {
        bool keepDelay = false;
        RangeData frameData = {0, FRAME_COUNT - 1, PARITY_ALL};
        RangeData xData = {0, MATRIX_HEIGHT - 1, PARITY_ALL};
        RangeData yData = {0, MATRIX_WIDTH - 1, PARITY_ALL};
        if (burnSerial("-d"))
        {
          keepDelay = true;
          Serial.print(" -d ");
        }
        frameData = parseRange(FRAME_COUNT - 1);
        if (burnSerial("("))
        {
          Serial.print(" (");
          xData = parseRange(MATRIX_WIDTH - 1);
          if (burnSerial(","))
          {
            burnSerial(" ");
            Serial.print(", ");
            yData = parseRange(MATRIX_HEIGHT - 1);
            if (burnSerial(")"))
            {
              Serial.print(")");
            }
            else
            {
              Serial.println();
              Serial.println("> Warning: Expected `)` after y range");
              return;
            }
          }
          else
          {
            Serial.println();
            Serial.println("> Warning: Expected `,` after x range");
            return;
          }
        }
        else
        {
          Serial.println();
          Serial.println("> Warning: Expected `(` after frame range");
          return;
        }
        Serial.println();
        for (int8_t i = frameData.from; i <= frameData.to; i++)
        {
          for (uint8_t x = xData.from; x <= xData.to; x++)
          {
            for (uint8_t y = yData.from; y <= yData.to; y++)
            {
              if ((i % 2 == 0) && (frameData.parity == PARITY_ODD || xData.parity == PARITY_ODD || yData.parity == PARITY_ODD))
                continue; // skip even
              if ((i % 2 == 1) && (frameData.parity == PARITY_EVEN || xData.parity == PARITY_EVEN || yData.parity == PARITY_EVEN))
                continue; // skip odd
              if (xyToIndex(x, y) == 255)
                continue;
              colors[i][xyToIndex(x, y)] = 0;
            }
          }
          if (!keepDelay)
          {
            cycleDelay[i] = 0;
          }
        }
        Serial.println("> Frames " + String(frameData[0]) + (frameData[1] != frameData[0] ? "-" + String(frameData[1]) : "") + (xData[0] != 255 ? " (" + String(xData[0]) + "-" + String(xData[1]) + ")" : "") + (yData[0] != 255 ? " (" + String(yData[0]) + "-" + String(yData[1]) + ")" : "") + (xData[2] != 0 || yData[2] != 0 ? " selectively" : "") + " cleared" + (keepDelay ? " (delays kept)" : ""));
      }
      else
      {
        Serial.println();
        clear();
        Serial.println("> All frames and LEDs cleared");
      }
      break;
    case Command::DLY:
      saveLast();
      Serial.println("< dly");
      RangeData frameData = {0, FRAME_COUNT - 1, PARITY_ALL};
      frameData = parseRange(FRAME_COUNT - 1);
      uint16_t delay = Serial.parseInt();
      Serial.println();
      for (int8_t i = frameData.from; i <= frameData.to; i++)
      {
        if ((i % 2 == 0) && (frameData.parity == PARITY_ODD))
          continue; // skip even
        if ((i % 2 == 1) && (frameData.parity == PARITY_EVEN))
          continue; // skip odd
        cycleDelay[i] = delay;
      }
      Serial.println("> Set delay of frames " + String(frameData.from) + (frameData.to != frameData.from ? "-" + String(frameData.to) : "") + " to " + String(delay * 10) + "s" + (frameData.parity != PARITY_ALL ? (frameData.parity == PARITY_ODD ? " on odd frames." : " on even frames.") : "."));
      break;
    case Command::TXT:
      saveLast();
      Serial.print("< txt ");
#ifndef TXT
      Serial.println();
      Serial.println("> Warning: TXT command not enabled. Recompile with `#define TXT` and supply a `bool text[26][h][w] PROGMEM` array to enable.");
#else
      uint8_t frame = Serial.parseInt();
      if (frame < 0 || frame >= FRAME_COUNT)
      {
        Serial.println("> Warning: Frame index out of range. Must be between 0 and " + String(FRAME_COUNT - 1));
        return;
      }
      burnSerial(" ");
      uint16_t color = Serial.parseInt();
      color = constrain(color, 0, 999);
      Serial.print(String(color) + " ");
      burnSerial(" ");
      String text = Serial.readStringUntil('\n');
      text.trim();
      Serial.print(text);
      Serial.println();
      colors[frame][0] = color;
      cycleDelay[frame] = 254; // special delay value to indicate text
      for (uint8_t i = 0; i < LED_COUNT; i++)
      {
        colors[frame][i] = 0;
      }
      for (uint8_t i = 0; i < text.length() && i < LED_COUNT - 1; i++)
      {
        colors[frame][i + 1] = text.charAt(i);
      }

#endif
      break;
    case Command::RGB:
      Serial.print("< rgb");
      uint16_t code = Serial.parseInt();
      Serial.println("> RGB for code `" + String(code) + "`: " + String(c(code)));
      Serial.println("Verbose RGB for code `" + String(code) + "`:");
      c(code, false);
      break;
    case Command::PRT:
      Serial.print("< prt");
      if (burnSerial("-&"))
      {
        Serial.print(" -&");
        Serial.println();
        for (uint8_t i = 0; i < commandHistoryCount; i++)
        {
          Serial.print(String(commandHistory[i]));
          if (i < commandHistoryCount - 1)
          {
            Serial.print(" && ");
          }
        }
      }
      else
      {
        Serial.println();
        for (uint8_t i = 0; i < commandHistoryCount; i++)
        {
          Serial.println("> " + String(i + 1) + ": " + String(commandHistory[i]));
        }
      }
      break;
    case Command::MTX:
      Serial.println("< mtx");
      for (uint8_t y = 0; y < MATRIX_HEIGHT; y++)
      {
        String line = "";
        for (uint8_t x = 0; x < MATRIX_WIDTH; x++)
        {
          uint8_t idx = xyToIndex(x, y);
          if (idx == 255)
          {
            line += "   ";
          }
          else
          {
            line += String(idx) + (idx < 10 ? " " : "") + (idx < 100 ? " " : "");
          }
        }
        Serial.println(line);
      }
      break;
    case Command::HLP:
      Serial.println("< hlp");
      Serial.println((const __FlashStringHelper *)helpText);
      break;
    case Command::EXP:
      Serial.println("< exp");
      Serial.println("> ");
      Serial.println("const uint16_t colors[" + String(FRAME_COUNT) + "][" + String(LED_COUNT) + "] = {");
      for (uint8_t k = 0; k < FRAME_COUNT; k++)
      {
        Serial.print("  {");
        for (uint8_t l = 0; l < LED_COUNT; l++)
        {
          Serial.print(colors[k][l]);
          if (l < LED_COUNT - 1)
          {
            Serial.print(", ");
          }
        }
        Serial.println("}" + String(k < FRAME_COUNT - 1 ? "," : "") + " // Frame " + String(k));
      }
      Serial.println("};");
      Serial.println();
      Serial.print("const uint16_t cycleDelay[" + String(FRAME_COUNT) + "] = {");
      for (uint8_t k = 0; k < FRAME_COUNT; k++)
      {
        Serial.print("  " + String(cycleDelay[k]) + String(k < FRAME_COUNT - 1 ? "," : ""));
      }
      Serial.println("};");
      break;
    case Command::XEP:
      Serial.println("< xep");
      clearEEPROM();
      Serial.println("> EEPROM cleared; all saved frames and delays erased");
      break;
    case Command::UND:
      Serial.println("< und");
      loadLast();
      Serial.println("> Last changes undone; und again to redo");
      break;
    case Command::WRT:
      saveEEPROM();
      Serial.println("< wrt");
      Serial.println("> Current frames and delays written to EEPROM for retrieval on next startup");
      break;
    default:
      Serial.print("> Error: Command ` ");
      Serial.print(Serial.readString());
      Serial.println("` not found.  Type `hlp` for a list of commands.");
      break;
    }
    while (Serial.available() && (Serial.peek() == ' ' || Serial.peek() == '&'))
    {
      Serial.read();
    }
    Serial.println("User@LightShowCLI: % ");
  }
}

uint8_t matchCommand()
{
  if (burnSerial("set"))
  {
    return Command::SET;
  }
  else if (burnSerial("clr"))
  {
    return Command::CLR;
  }
  else if (burnSerial("dly"))
  {
    return Command::DLY;
  }
  else if (burnSerial("txt"))
  {
    return Command::TXT;
  }
  else if (burnSerial("rgb"))
  {
    return Command::RGB;
  }
  else if (burnSerial("prt"))
  {
    return Command::PRT;
  }
  else if (burnSerial("mtx"))
  {
    return Command::MTX;
  }
  else if (burnSerial("hlp"))
  {
    return Command::HLP;
  }
  else if (burnSerial("exp"))
  {
    return Command::EXP;
  }
  else if (burnSerial("xep"))
  {
    return Command::XEP;
  }
  else if (burnSerial("und"))
  {
    return Command::UND;
  }
  else if (burnSerial("wrt"))
  {
    return Command::WRT;
  }
  else
  {
    return Command::UNKNOWN;
  }
}