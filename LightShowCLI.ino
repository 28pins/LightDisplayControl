#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

#define LED_PIN 6
#define FRAME_COUNT 15

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

uint16_t cycleDelay[FRAME_COUNT];
uint16_t delayLast[FRAME_COUNT];
uint16_t colors[FRAME_COUNT][LED_COUNT];
uint16_t colorsLast[FRAME_COUNT][LED_COUNT];

#define MAX_HISTORY 30
#define HISTORY_MAX_LEN 34

char commandHistory[MAX_HISTORY][HISTORY_MAX_LEN];
uint8_t commandHistoryIndex = 0;
uint8_t commandHistoryCount = 0;

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