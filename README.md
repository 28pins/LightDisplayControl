This code lets you control a matrix or string of RGB leds from a Mega or ESP board using a Serial CLI-like command set.
Upload the code and type help in the Serial monitor to see all commands.
Edit the Matrix.h file with your LED grid setup and the order of LEDs.

**Warning: clearing your EEPROM is advised before uploading program.**

### Features:
- Create a custom matrix setup and reference pixels by (x, y)
- Set odd and even and ranges of pixels in one command
- EEPROM storage of animations
- 14 frames of animation (76 pixels, Mega2560) by default
- Type undo to revert changes
- Supports offset for repeater pixels
- Simple RGB color code system 0-999, one color per digit
- Export color arrays or complete program for use on 328 chips
- Scroll text across the display

### Roadmap
- Wifi/web server on esp
- Import array
