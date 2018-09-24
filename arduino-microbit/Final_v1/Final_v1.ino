// Works with Microbit, LinkSprite Si4703 FM Radio, Line Level Convertor BSS138
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

int resetPin = 16;
//int SDIO = A4;
//int SCLK = A5;

int SDIO = SDA; //SDA/A4 on Arduino
int SCLK = SCL; //SCL/A5 on Arduino

Si4703_Breakout radio(resetPin, SDIO, SCLK);
int channel = 881;
int volume = 5;
char rdsBuffer[10];
Adafruit_Microbit_Matrix microbit;

const int buttonA = 5;     // the number of the pushbutton pin
const int buttonB = 11;     // the number of the pushbutton pin

#define NUMPIXELS 144 // Number of LEDs in strip
// Here's how to control the LEDs from any two pins:
#define DATAPIN    6
#define CLOCKPIN   7
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DATAPIN, CLOCKPIN, DOTSTAR_BGR);

// U.S. FM Broadcast is 88.1—107.9 (100 Stations)
#define MINFREQ = 881
#define MAXFREQ = 1079
#define USFMSTATIONS = 100

/* Current station is index-0 */
uint32_t stationColors[NUMPIXELS] = {
  0xFFFFFF, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 1-10
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 11-20
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 21-30
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 31-40
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 41-50
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 51-60
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 61-70
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 71-80
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 81-90
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 91-100
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 101-110
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 111-120
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 121-130
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000, // 131-140
  0xFF0000, 0xFF0000, 0xFF0000, 0xFF0000                                                     // 141-144
};

void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 16000000L)
  clock_prescale_set(clock_div_1); // Enable 16 MHz on Trinket
#endif

  strip.begin(); // Initialize pins for output
  strip.show();  // Turn all LEDs off ASAP

  // all Pixels red
  for (int i = 0; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, 0xFF0000);
  }

  // some pixels white
  // some pixels around it off
  strip.setPixelColor(109, 0x000000);
  strip.setPixelColor(110, 0x000000);
  strip.setPixelColor(111, 0x000000);
  strip.setPixelColor(112, 0xFFFFFF);
  strip.setPixelColor(113, 0x000000);
  strip.setPixelColor(114, 0x000000);
  strip.setPixelColor(115, 0x000000);

  strip.show();

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

  //  microbit.begin();

  radio.powerOn();
  //  radio.setVolume(0);

  radio.setVolume(volume);
  radio.setChannel(channel);
}

void loop() {
  //  microbit.print(channel);

  // Use Microbit buttons to seek up and down
  if (! digitalRead(buttonA)) {
    channel = channel - 2;
    if (channel < 871) {
      channel  = 1079;
    }
    radio.setChannel(channel);
    updatePixels();
  }
  if (!digitalRead(buttonB)) {
    channel = channel + 2;
    if (channel > 1079) {
      channel  = 871;
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
  int lightOffsetIndex = map(channel, MINFREQ, MAXFREQ, 0, NUMPIXELS);

  /* Color array is used to determine bulb colors
   * we will slide it up and down the LED strip
   * it loops as it gets to the end
   */
  int colorsIndex = 0;
  // set color of current station up to the last pixel in our LED strip
  for (int i = lightOffsetIndex; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, stationColors[colorsIndex]);
    colorsIndex++;
  }
  // if we haven't used up our entire light array
  // loop it filling in the colors starting from bulb 0 up to but not including
  // the bulb representing the current station
  for (int i = 0; i < lightOffsetIndex; i++) {
    strip.setPixelColor(i, stationColors[colorsIndex]);
    colorsIndex++;
  }

  strip.show();
}
