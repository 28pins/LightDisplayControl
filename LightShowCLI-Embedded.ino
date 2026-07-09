#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include "Matrix.h"

bool LEDBuiltinOn = false;

uint16_t cycleDelay[FRAME_COUNT];
uint16_t delayLast[FRAME_COUNT];
uint16_t colors[FRAME_COUNT][LED_COUNT];
uint16_t colorsLast[FRAME_COUNT][LED_COUNT];

Adafruit_NeoPixel strip(LED_COUNT + OFFSET, LED_PIN, NEO_RGB + NEO_KHZ800);

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  toggleBuiltin();
  strip.begin();
  strip.show();
  Serial.begin(9600);
  Serial.println(F("> ####################> SERIAL LIGHT SHOW <####################"));
  toggleBuiltin();
  Serial.println(F("> Version 1.1.0"));
  Serial.println(F("> **Warning:** Set line ending to `New Line` for proper parsing."));
  Serial.println(F("> Type `help` for help"));
  Serial.println("> Currently configured with " + String(LED_COUNT) + " LEDs and " + String(FRAME_COUNT) + " frames.");
  toggleBuiltin();
  if (loadEEPROM())
  {
    Serial.println(F("> Saved data found in EEPROM and loaded successfully."));
  }
  else
  {
    Serial.println(F("> No saved data found in EEPROM."));
  }
  Serial.println(F("> Ready for command \n"));
  toggleBuiltin();
}

uint16_t bm = 0;

void loop()
{
  serialHandle();
  for (uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    if(cycleDelay[i] == 254) { //Text flag
      textLoop(i);
      serialHandle();
    } else {
      if(cycleDelay[i] != 0){
        colorLoop(i);
      }
      bm = cycleDelay[i];
      if (bm > 0){
        digitalWrite(LED_BUILTIN, HIGH);
        if(bm > 500) {
          delay(500);
        }
        while(bm > 200){
          toggleBuiltin();
          delay(200);
          bm -= 200;
          serialHandle();
        }
        delay(bm);
        bm = 0;
      }
    }
  }
}
