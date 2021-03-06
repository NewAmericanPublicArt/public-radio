// Works with Microbit, LinkSprite Si4703 FM Radio, Line Level Convertor BSS138, Vernier Photogates
// See diagrams directory for a wiring guide
// This is meant to be built from the Arduino IDE then loaded onto a Microbit
//
// Microbit pinouts: https://microbit.org/guide/hardware/pins/
//
// Line Level Convertor BSS138
// https://www.adafruit.com/product/757
// Hook up low side to microbit
// Power high side off of 5V power supply
// Run high side line levels to FM board
// Power FM board off of 3V microbit
//
// LinkSprite Si4703 FM Radio Board:
// http://store.linksprite.com/breakout-board-for-si4703-fm-tuner/
// http://linksprite.com/wiki/index.php?title=Main_Page
// although the Si4703 chip is a 3.3V chip, the LinkSprite board needs to be
// powered at 3.3V, but expects 5V line levels for control
// *** Hmmm, actually it seems the board works at 3.3V?
//
// Getting Started:
// 1) Follow Microbit Arduino startup instructions: https://learn.adafruit.com/use-micro-bit-with-arduino/overview
//    Install Si4703_FMRadio library, https://github.com/whiteneon/Si4703_FMRadio
//    Install Adafruit GFX Library
//    Install BLE Peripheral Library
//    Install Adafruit_Microbit Library https://github.com/adafruit/Adafruit_Microbit/archive/master.zip
//
//
// 2) Wire up board pins: SDIO, SCLK, Reset, Ground, 3.3V
// 3) Upload code to Arduino
// 4) Turn on Serial Monitor, set to 57600
// 5) Change Volume or Station via Serial monitor
//    Change Stations via A and B buttons (A is seek down, B is seek up)
//
#include <Wire.h>
#include <SPI.h>

// PINS /////////////////////
const int PHOTOGATE1A_PIN = 0; //GPIO (general purpose digital input and output) with analogue to digital convertor (ADC)
const int PHOTOGATE1B_PIN = 1; //GPIO with ADC
const int PHOTOGATE2A_PIN = 2; //GPIO with ADC
const int PHOTOGATE2B_PIN = 8; //Dedicated GPIO, for sending and sensing digital signals.

const int buttonA = 5;     // the number of the pushbutton pin
const int buttonB = 11;     // the number of the pushbutton pin
const int RADIO_RESET_PIN = 16;
const int RADIO_SDIO = SDA; // P20 on Micro:bit
const int RADIO_SCLK = SCL; // P19 on Micro:bit

// CONSTANTS ////////////////
#define SERIAL_DEBUG false // serial control is not useful in production and has a performance hit, leave disabled
#define NUM_STATIONS 108 // we have a few extra ticks so we'll do 86.5—107.9 (108 Stations)
#define MINFREQ 865
#define MAXFREQ 1079
#define MAX_VOLUME 15 // Si-4703 API volume ranges from 0—15
#define MIN_VOLUME 0

#define LEDS_PER_STATION 3 // LED offsets will start at the leading edge of the plastic tick/finger
#define LED_FUDGE_FACTOR 2 // we don't have exactly 3 leds per station, since they get offset over time…
#define STATION_COLORS_LENGTH (NUM_STATIONS * LEDS_PER_STATION - LED_FUDGE_FACTOR)
#define NUMPIXELS 421 // Number of LEDs in strip (old 432)
#define VOLUME_NUM_PIXELS 37 // Number of LEDs for Volume Feedback
#define VOLUME_CENTER_PIXEL int(VOLUME_NUM_PIXELS/2)
#define STATION_PIXEL_START_INDEX VOLUME_NUM_PIXELS + 17
#define STATION_PIXEL_END_INDEX (STATION_PIXEL_START_INDEX + STATION_COLORS_LENGTH - 1)

// VARIABLES ////////////////
int volume = 5; // default volume on power-on
int channel = MINFREQ; // default channel on power-on
unsigned long lastStationDialChangeChangeMicros = 0;

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  ledsSetup();
#ifdef SERIAL_DEBUG
  serialControlSetup();
#endif

  pinMode(buttonA, INPUT);
  pinMode(buttonB, INPUT);

  pinMode(PHOTOGATE1A_PIN, INPUT); // channel tuning photogate
  pinMode(PHOTOGATE1B_PIN, INPUT); // channel tuning photogate
  pinMode(PHOTOGATE2A_PIN, INPUT); // volume control photogate
  pinMode(PHOTOGATE2B_PIN, INPUT); // volume control photogate

  radioSetup();

  // Read photogates on input change
  attachInterrupt(digitalPinToInterrupt(PHOTOGATE1A_PIN), readPhotogate1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PHOTOGATE1B_PIN), readPhotogate1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PHOTOGATE2A_PIN), readPhotogate2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PHOTOGATE2B_PIN), readPhotogate2, CHANGE);
}

void loop() {
  // channelTuningWithMicrobitButtons(); // add-in for debugging
  updateRadio();
  updatePixels();
#ifdef SERIAL_DEBUG
  serialControlInLoop(); // serial control is not useful in production and has a performance hit, leave disabled
#endif
}
