#include <Adafruit_DotStar.h>

// ASR is this DEFINE necessary?
// see https://github.com/adafruit/Adafruit_DotStar/blob/master/Adafruit_DotStar.cpp
#define PIC32 // force slower clock speed for voltage level converter SN54AHCT125

// Use Microbit's SPI, Apa102 Data -> Microbit MOSI AKA pin 15
// Apa102 Clock -> Microbit SCK AKA pin 13
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BGR);

/* Current station is index-0 */
uint32_t stationColors[STATION_COLORS_LENGTH];
uint32_t offbandColor = 0x032926;

long MIN_DELAY_BETWEEN_LED_UPDATES = 16; // ~60fps, IE 1000ms/60 ~= 16
long lastLEDUpdate = 0;

void ledsSetup() {
  /* Sliding array of colors, used to animate current station
      pixels last,0,1,2 is always the tick representing the current station
      depending on the current station we will “slide” this array into position
      we'll surround the current station with off pixels so it stands out
  */
  // clear array
  for(int i=0; i<STATION_COLORS_LENGTH; i++){
    stationColors[i] = strip.Color(0, 0, 0);
  }

  // setup sliding animation array
  stationColors[0] = strip.Color(255, 255, 255);
  stationColors[1] = strip.Color(255, 255, 255);
  stationColors[2] = strip.Color(255, 255, 255);
  stationColors[3] = strip.Color(100, 0, 0);
  stationColors[4] = strip.Color(100, 0, 0);
  stationColors[5] = strip.Color(100, 0, 0);

  stationColors[STATION_COLORS_LENGTH - 4] = strip.Color(100, 0, 0);
  stationColors[STATION_COLORS_LENGTH - 3] = strip.Color(100, 0, 0);
  stationColors[STATION_COLORS_LENGTH - 2] = strip.Color(100, 0, 0);
  stationColors[STATION_COLORS_LENGTH - 1] = strip.Color(255, 255, 255);
  for (int i = 6; i <= (STATION_COLORS_LENGTH-5); i++) {
    int redValue = 0;
    if (i <= (float)STATION_COLORS_LENGTH / 2.0) {
      redValue = map(i, 0, STATION_COLORS_LENGTH / 2.0, 255, 0);
    } else {
      redValue = map(i, STATION_COLORS_LENGTH / 2.0 + 1, STATION_COLORS_LENGTH - 1, 0, 255);
    }
    stationColors[i] = strip.Color(redValue, 0, 255 - redValue);
  }
  strip.begin();  // Initialize pins for output
  updatePixels();
}

void updatePixels() {
  // Update pixels at ~60fps
  if(millis() - lastLEDUpdate < MIN_DELAY_BETWEEN_LED_UPDATES){
    return;
  }
  lastLEDUpdate = millis();
  
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
