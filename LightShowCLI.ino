#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include "Matrix.h"

#define LED_PIN 6
#define FRAME_COUNT 14
#define MAX_HISTORY 25
#define HISTORY_MAX_LEN 34

uint16_t cycleDelay[FRAME_COUNT];
uint16_t delayLast[FRAME_COUNT];
uint16_t colors[FRAME_COUNT][LED_COUNT];
uint16_t colorsLast[FRAME_COUNT][LED_COUNT];

char commandHistory[MAX_HISTORY][HISTORY_MAX_LEN];
uint8_t commandHistoryIndex = 0;
uint8_t commandHistoryCount = 0;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

#define RAM_AVAILABLE 8192 // mega2560 // Define for your microcontroller if you encounter unknown microcontroller error.

#define STOP_IF_RAM_OVERFLOW true
#define RAM_OVERHEAD_NEEDED 2500
#if !defined(RAM_AVAILABLE) || RAM_AVAILABLE <= 0
#error "Unknown microcontroller. Please define RAM_AVAILABLE for your microcontroller to use STOP_IF_RAM_OVERFLOW."
#endif
#define REMAINING_RAM (RAM_AVAILABLE - RAM_OVERHEAD_NEEDED)
#define RAM_USED ((FRAME_COUNT * LED_COUNT * 4) + (FRAME_COUNT * 4) + (MAX_HISTORY * HISTORY_MAX_LEN) + (LED_COUNT * 4))

#if STOP_IF_RAM_OVERFLOW
#if RAM_USED > REMAINING_RAM
#error "RAM overflow: Reduce FRAME_COUNT or LED_COUNT to fit within 8000 bytes of RAM."
#endif
#elif RAM_USED > REMAINING_RAM
#warning "RAM overflow: Reduce FRAME_COUNT or LED_COUNT to fit within 8000 bytes of RAM. Continuing anyway because STOP_IF_RAM_OVERFLOW is false, but program may fail."
#endif
#pragma message "RAM used: " RAM_USED " bytes. Remaining RAM: " REMAINING_RAM " bytes.  This " RAM_USED> REMAINING_RAM ? "exceeds" : "fits within the limit of " RAM_AVAILABLE " bytes and a buffer of " RAM_OVERHEAD_NEEDED " bytes."

void setup()
{
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)
  Serial.begin(9600);
  Serial.println("> ####################> SERIAL LIGHT SHOW <####################\n\thlp--help");
  Serial.println("> Currently configured with " + String(LED_COUNT) + " LEDs and " + String(FRAME_COUNT) + " frames.");
  Serial.println("> User@LightShowCLI: % ");
  if (EEPROM.read(0) == 1)
  {
    Serial.println("> Saved data found in EEPROM. Loading...");
    loadEEPROM();
  }
  else
  {
    Serial.println("> No saved data found in EEPROM.");
  }
}

void loop()
{
  for (uint8_t i = 0; i < FRAME_COUNT; i++)
  {
    innerLoop(i);
    serialLoop();
  }
}