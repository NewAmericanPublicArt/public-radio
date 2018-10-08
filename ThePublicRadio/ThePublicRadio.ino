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
#include <Si4703_Breakout.h>
#include <Adafruit_DotStar.h>
#include <Adafruit_Microbit.h>
#include <Wire.h>
#include <SPI.h>

const int PHOTOGATE1_PIN = 1;
const int PHOTOGATE2_PIN = 2;
const int buttonA = 5;     // the number of the pushbutton pin
const int buttonB = 11;     // the number of the pushbutton pin

float speed1 = 0;
float speed2 = 0;

int resetPin = 16;
//int SDIO = A4;
//int SCLK = A5;

int SDIO = SDA; //SDA/A4 on Arduino
int SCLK = SCL; //SCL/A5 on Arduino

Si4703_Breakout radio(resetPin, SDIO, SCLK);
// U.S. FM Broadcast is 879—107.9 (101 Stations)
#define NUM_STATIONS 101
#define MINFREQ 879   // https://en.wikipedia.org/wiki/87.9_FM#United_States_(Channel_200)
#define MAXFREQ 1079
int channel = MINFREQ;
int volume = 5;
char rdsBuffer[10];
Adafruit_Microbit_Matrix microbit;

#define LEDS_PER_STATION 3
#define STATION_COLORS_LENGTH (NUM_STATIONS * LEDS_PER_STATION)
#define NUMPIXELS 432 // Number of LEDs in strip
#define STATION_PIXEL_START_INDEX (17 + 21)
#define STATION_PIXEL_END_INDEX (STATION_PIXEL_START_INDEX + STATION_COLORS_LENGTH - 1)
/* Current station is index-0 */
uint32_t stationColors[STATION_COLORS_LENGTH];
uint32_t offbandColor = 0x033E3A;

// ASR is this DEFINE necessary?
// see https://github.com/adafruit/Adafruit_DotStar/blob/master/Adafruit_DotStar.cpp
#define PIC32 // force slower clock speed for voltage level converter SN54AHCT125

// Use Microbit's SPI, Apa102 Data -> Microbit MOSI AKA pin 15
// Apa102 Clock -> Microbit SCK AKA pin 13
Adafruit_DotStar strip = strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BGR);


void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  /* Sliding array of colors, used to animate current station
      pixels 0,1,2 is alwyas he tick representing the current station
      depending on the current station we will “slide” this array into position
  */
  stationColors[0] = 0xFFFFFF;
  stationColors[1] = 0xFFFFFF;
  stationColors[2] = 0x000000;
  for (int i = 3; i < STATION_COLORS_LENGTH; i++) {
    if (i % 3 == 2) {
      /* every 3rd pixel will be off AKA Black
          we have 144*3 pixels, and (144*3-3) % 3 == 0, so the last pixel will be black
          so we have a nice loop, IE the current station tick is centered at Black,White,White,Black
      */
      stationColors[i] = 0x000000;
    } else {
      stationColors[i] = 0xFF0000;
    }
  }
  strip.begin();  // Initialize pins for output
  updatePixels();

  Serial.begin(57600);
  Serial.println("\n\nSi4703_Breakout Test Sketch");
  Serial.println("===========================");
  Serial.println("a b     Favourite stations");
  Serial.println("+ -     Volume (max 15)");
  Serial.println("u d     Seek up / down");
  Serial.println("r       Listen for RDS Data (15 sec timeout)");
  Serial.println("Send me a command letter.");

  pinMode(buttonA, INPUT);
  pinMode(buttonB, INPUT);

  // Photogates for volume/channel wheel
  pinMode(PHOTOGATE1_PIN, INPUT);
  pinMode(PHOTOGATE2_PIN, INPUT);

  // for Microbit LED matrix
  microbit.begin();

  // Radio
  radio.powerOn();
  radio.setVolume(volume);
  radio.setChannel(channel);
}

void loop() {
  //  microbit.print(channel);

  measure();

  // Use Microbit buttons to seek up and down
  if (!digitalRead(buttonA)) {
    channel = channel - 2;
    if (channel < MINFREQ) {
      channel = MAXFREQ;
    }
    radio.setChannel(channel);
    updatePixels();
  }
  if (!digitalRead(buttonB)) {
    channel = channel + 2;
    if (channel > MAXFREQ) {
      channel = MINFREQ;
    }
    radio.setChannel(channel);
    updatePixels();
  }

  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'u') {
      seekUp();
    } else if (ch == 'd') {
      seekDown();
    } else if (ch == '+') {
      volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
    } else if (ch == '-') {
      volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
    } else if (ch == 'a') {
      channel = 930; // Rock FM
      radio.setChannel(channel);
      displayInfo();
    } else if (ch == 'b') {
      channel = 974; // BBC R4
      radio.setChannel(channel);
      displayInfo();
    } else if (ch == 'r') {
      Serial.println("RDS listening");
      radio.readRDS(rdsBuffer, 15000);
      Serial.print("RDS heard:");
      Serial.println(rdsBuffer);
    }
  }
}

void seekUp() {
  channel = radio.seekUp();
  displayInfo();
}

void seekDown() {
  channel = radio.seekDown();
  displayInfo();
}

void displayInfo() {
  Serial.print("Channel:"); Serial.print(channel);
  Serial.print(" Volume:"); Serial.println(volume);
}

void updatePixels() {
  // Location of bulb that indicates current station
  // this index gives us the location within our entire LED strip
  // int lightOffsetIndex = map(channel, MINFREQ, MAXFREQ, 0, NUMPIXELS);
  int lightOffsetIndex = constrain(int(LEDS_PER_STATION * (channel - MINFREQ) / 2.0), 0, STATION_COLORS_LENGTH - 1);

  /* Color array is used to determine bulb colors
     we will slide it up and down the LED strip
     it loops as it gets to the end
  */
  int stationColorsIndex = 0;
  int onCount = 0;
  // set color of current station up to the last pixel in our LED strip
  for (int i = lightOffsetIndex; i < STATION_COLORS_LENGTH; i++) {
    strip.setPixelColor(i + STATION_PIXEL_START_INDEX, stationColors[stationColorsIndex]);
    stationColorsIndex++;
  }
  // if we haven't used up our entire light array
  // loop it filling in the colors starting from bulb 0 up to but not including
  // the bulb representing the current station
  for (int i = 0; i < lightOffsetIndex; i++) {
    strip.setPixelColor(i + STATION_PIXEL_START_INDEX, stationColors[stationColorsIndex]);
    stationColorsIndex++;
  }

  // color the offband pixels
  for (int i = 0; i < STATION_PIXEL_START_INDEX; i++) {
    strip.setPixelColor(i, offbandColor);
  }
  for (int i = STATION_PIXEL_END_INDEX + 1; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, offbandColor);
  }

  strip.show();
}
