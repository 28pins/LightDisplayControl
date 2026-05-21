/// @brief Runs the inner loop for a given frame index, displaying the colors and applying the delay.
/// @param i The frame index to run.
void innerLoop(int i)
{
  if (cycleDelay[i] != 0)
  {
    for (uint8_t j = 0; j < LED_COUNT; j++)
    {
      strip.setPixelColor(j, c(colors[i][j]));
    }
    strip.show();
    if (!Serial.available())
    {
      delay(cycleDelay[i]);
    }
    else
    {
      delay(50);
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
dly <[o/e]frame[-frame2]> <delay> - Set the delay for a specific frame or series of frames.  Use o/e before a series to only apply to the odd or even indexed frames.  Use - to specify a range of frames.
txt <frame> <text> <color 0-999> - Display scrolling text on the display in place of a specific frame.  Text will scroll at a speed determined by 1/10 the frame's delay per column.
rgb <color 0-999> - Replies with the RGB values for a specific color code.
prt - Prints all previous commands since the last `clr` command.
mtx [-n] - Display an ASCII representation of the LED matrix (-n gives LED IDs)
exp - Exports a C array of the current colors and delays for all frames, which can be copied into the program for static displays.
xxx - Clears EEPROM memory, erasing all saved frames and delays.  Use with caution.
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
  XXX = 10,
  UND = 11
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

/*
Commands:

x Up-down (first in arr [x][y]), y left-right (second in arr [x][y])
set <[o/e]frame[-frame2]> ([o/e]<x[-x2]>, [o/e]<y[-y2]>) <color 0-999> - Set a specific LED or series in a specific frame to a color.  Use o/e before a series to only apply to the odd or even indexed LEDs.  Use - to specify a range of frames or LEDs.
clr <[o/e]frame[-frame2]> ([o/e]<x[-x2]>, [o/e]<y[-y2]>) - Clear a specific LED or series in a specific frame.  Use o/e (0 even) before a series to only apply to the odd or even indexed LEDs.  Use - to specify a range of frames or LEDs.
clr [-d] - Clear all frames and LEDs. -d keeps the delays but clears the colors.
clr <[o/e]frame[-frame2]> [-d] - Clear all LEDs in a specific frame. -d keeps the delay but clears the colors.
dly <[o/e]frame[-frame2]> <delay> - Set the delay for a specific frame or series of frames.  **Do NOT use 254 as that is a special value.**  Use o/e before a series to only apply to the odd or even indexed frames.  Use - to specify a range of frames.
txt <frame> <text> <color 0-999> - Display scrolling text on the display in place of a specific frame.
rgb <color 0-999> - Replies with the RGB values for a specific color code.
prt - Prints all previous commands since the last `clr` command.
mtx [-n] - Display an ASCII representation of the LED matrix (-n gives LED IDs)
hlp - Displays this help message.
*/

void serialLoop()
{
  if (Serial.available())
  {
    switch (matchCommand())
    {
    case Command::SET:
      Serial.println("> Command: SET");
      // LATER
      break;
    case Command::CLR:
      Serial.print("< clr ");
      int8_t frameFrom = Serial.parseInt();
      int8_t frameTo = frameFrom;
      int8_t isEvenOdd = 0; // 0 for all, 1 for odd, 2 for even
      bool keepDelay = false;
      if (Serial.peek() == 'o' || Serial.peek() == 'e' || frameFrom != -1)
      {
        if (Serial.peek() == 'o')
        {
          Serial.read();
          isEvenOdd = 1;
          frameFrom = Serial.parseInt();
          frameTo = frameFrom;
          Serial.print("o");
        }
        else if (Serial.peek() == 'e')
        {
          Serial.read();
          isEvenOdd = 2;
          frameFrom = Serial.parseInt();
          frameTo = frameFrom;
          Serial.print("e");
        }
        else
        {
          Serial.print(frameFrom);
        }
        if (Serial.peek() == '-')
        {
          Serial.read();
          frameTo = Serial.parseInt();
          Serial.print("-" + String(frameTo));
        }
        if (Serial.peek() == ' ')
        {
          Serial.read();
        }
        if (burnSerial("-d"))
        {
          keepDelay = true;
          Serial.print(" -d");
        }
        Serial.println();
        for (int8_t i = frameFrom; i <= frameTo; i++)
        {
          if (i < 0 || i >= FRAME_COUNT)
          {
            Serial.println("> Warning: Frame " + String(i) + " is out of bounds");
            continue;
          }
          for (uint8_t j = 0; j < LED_COUNT; j++)
          {
            if (isEvenOdd == 1 && j % 2 == 0)
              continue; // skip even
            if (isEvenOdd == 2 && j % 2 == 1)
              continue; // skip odd
            colors[i][j] = 0;
          }
          if (!keepDelay)
          {
            cycleDelay[i] = 0;
          }
        }
        Serial.println("> Frames " + String(frameFrom) + (frameTo != frameFrom ? "-" + String(frameTo) : "") + " cleared" + (keepDelay ? " (delays kept)" : ""));
      }
      else
      {
        bool keepDelay = false;
        if (burnSerial("-d"))
        {
          keepDelay = true;
          Serial.print(" -d");
        }
        for (uint8_t i = 0; i < FRAME_COUNT; i++)
        {
          for (uint8_t j = 0; j < LED_COUNT; j++)
          {
            colors[i][j] = 0;
          }
          if (!keepDelay)
          {
            cycleDelay[i] = 0;
          }
        }
        Serial.println("> All frames cleared" + (keepDelay ? " (delays kept)" : ""));
      }
      break;
    case Command::DLY:
      Serial.println("> Command: DLY");
      // LATER
      break;
    case Command::TXT:
      Serial.println("> Command: TXT");
      // LATER
      break;
    case Command::RGB:
      Serial.println("> RGB for code `" + Serial.readString() + "`: " + c(Serial.parseInt(), false));
      break;
    case Command::PRT:
      Serial.println("> Command: PRT");
      break;
    case Command::MTX:
      Serial.println("> Command: MTX");
      break;
    case Command::HLP:
      Serial.println(FPSTR(helpText));
      break;
    case Command::EXP:
      Serial.println("> Command: EXP");
      break;
    case Command::XXX:
      Serial.println("< xxx");
      clearEEPROM();
      Serial.println("> EEPROM cleared; all saved frames and delays erased");
      break;
    case Command::UND:
      Serial.println("< und");
      loadLast();
      Serial.println("> Last changes undone; und to redo");
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
  else if (burnSerial("xxx"))
  {
    return Command::XXX;
  }
  else if (burnSerial("und"))
  {
    return Command::UND;
  }
  else
  {
    return Command::UNKNOWN;
  }
}