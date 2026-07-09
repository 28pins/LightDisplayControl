#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include "Matrix.h"
 
#define PARITY_EVEN 2
#define PARITY_ODD 1
#define PARITY_ALL 0

bool LEDBuiltinOn = false;

uint16_t cycleDelay[FRAME_COUNT];
uint16_t delayLast[FRAME_COUNT];
uint16_t colors[FRAME_COUNT][LED_COUNT];
uint16_t colorsLast[FRAME_COUNT][LED_COUNT];

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

void setup()
{
  toggleBuiltin();
  pinMode(13, OUTPUT);
  strip.begin();
  strip.show();
  Serial.begin(9600);
  Serial.println(F("> ####################> SERIAL LIGHT SHOW <####################"));
  toggleBuiltin();
  Serial.println(F("> Version 0.0.0 [beta]"));
  Serial.println(F("> **Warning:** Set line ending to `New Line` for proper parsing."));
  Serial.println(F("> Type `help` for help"));
  Serial.print(F("> Currently configured with "));
  Serial.print(LED_COUNT);
  Serial.print(F(" LEDs and "));
  Serial.print(FRAME_COUNT);
  Serial.println(F(" frames.  Feature flags: cacheCmds: NO; textDisp: NO; WiFi: NO"));
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