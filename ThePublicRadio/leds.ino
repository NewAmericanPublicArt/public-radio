#include <Adafruit_DotStar.h>

// ASR is this DEFINE necessary?
// see https://github.com/adafruit/Adafruit_DotStar/blob/master/Adafruit_DotStar.cpp
#define PIC32 // force slower clock speed for voltage level converter SN54AHCT125

// Use Microbit's SPI, Apa102 Data -> Microbit MOSI AKA pin 15
// Apa102 Clock -> Microbit SCK AKA pin 13
Adafruit_DotStar strip = strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BGR);

/* Current station is index-0 */
uint32_t stationColors[STATION_COLORS_LENGTH];
uint32_t offbandColor = 0x033E3A;

void ledsSetup() {
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
