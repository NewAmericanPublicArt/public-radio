#include <Adafruit_DotStar.h>

// see https://github.com/adafruit/Adafruit_DotStar/blob/master/Adafruit_DotStar.cpp
#define PIC32 // force slower clock speed for voltage level converter SN54AHCT125

// Use Microbit's SPI, Apa102 Data -> Microbit MOSI AKA pin 15, Apa102 Clock -> Microbit SCK AKA pin 13
Adafruit_DotStar strip = Adafruit_DotStar(NUMPIXELS, DOTSTAR_BGR);


// Setup colors RGB 0-255
uint32_t offbandColor = strip.Color(3, 41, 38);
uint32_t volumeOnColor = strip.Color(20, 20, 255);
uint32_t volumeOffColor = strip.Color(0, 0, 0);
uint32_t white = strip.Color(255, 255, 255); // current station color
uint32_t gradientColor1 = strip.Color(255, 0, 0); // gradient near current station
uint32_t gradientColor2 = strip.Color(0, 0, 255); // gradient color 1/2 way away from station

/* Current station is index-0 */
uint32_t stationColors[STATION_COLORS_LENGTH];

// The moving waves coming of current station
struct LightPulse {
  int location;  // index within just the station LEDs
  unsigned long timeOfLastChange;
  unsigned long birth;
  int direction; // positive: increase station, negative: decrease station
  boolean alive;
};

// if we ever exceed MAX_LIGHT_PULSES we'll just delete the oldest pulse
#define MAX_LIGHT_PULSES 20
LightPulse lightPulses[MAX_LIGHT_PULSES];
int lightPulseIndex = 0;
boolean sendNewPulse = false;
unsigned long lastPulseCreate = 0;
int PULSE_SPEED = 1; // larger is faster
unsigned long DELAY_BETWEEN_PULSE_UPDATES = 50; // micros to wait on each LED before transitioning to the next
unsigned long MIN_WAIT_UNTIL_PULSE_START = 500000; // micros to wait until a pulse first starts after station stops
unsigned long PULSE_MAX_LIFE = 2000000; // max micros a pulse exists before it is deleted
// pulse generation is coupled to breathing animation

int PULSE_WIDTH_HALF = 6; // pulse width is twice this width + 1 (the center)
// we don't want to send pulses to the edge if we are really close to the edge
// because they won't look good
#define MIN_DISTANCE_FROM_EDGE_FOR_PULSE_START 20
/* a tick is 4-bulbs wide but is indexed off the leading edge of the tick
   so our pulse starts 2 indices below the tick and 3 indices above */
#define PULSE_START_OFFSET_LOW 2
#define PULSE_START_OFFSET_HIGH 3
// index into Dotstar library sine table at which we will send a new pulse
// we want this less then 255 value so it appears the pulse is sent as we approach 255
#define SINE_TABLE_255_SEND_PULSE 8
#define SINE_TABLE_255 62 // index of a 255 value in Dotstar library sine table
uint8_t centerPulseSineTime255 = SINE_TABLE_255;
unsigned long CENTER_TICK_SPEED = 300; // breathing speed (smaller is faster)
unsigned long centerTickLastTimeChange = 0;

unsigned long MIN_DELAY_BETWEEN_LED_UPDATES = 16000; // ~60fps, IE 1000ms/60 ~= 16 * 1000
unsigned long lastLEDUpdate = 0;

void ledsSetup() {
  // Clear light pulse array
  for (int i = 0; i < MAX_LIGHT_PULSES; i++) {
    lightPulses[i].location = 0;
    lightPulses[i].timeOfLastChange = 0;
    lightPulses[i].birth = 0;
    lightPulses[i].direction = 0;
    lightPulses[i].alive = false;
  }

  /* Sliding array of colors, used to animate current station
      pixels last,0,1,2 is always the tick representing the current station
      depending on the current station we will “slide” this array into position
      we'll surround the current station with off pixels so it stands out
  */
  // clear array
  for (int i = 0; i < STATION_COLORS_LENGTH; i++) {
    stationColors[i] = strip.Color(0, 0, 0);
  }

  // Setup sliding animation array (gradient that shifts as we change station)
  // setup colors around current station
  stationColors[0] = white;
  stationColors[1] = white;
  stationColors[2] = white;
  stationColors[3] = gradientColor1;
  stationColors[4] = gradientColor1;
  stationColors[5] = gradientColor1;

  // setup colors around current station (wrap)
  stationColors[STATION_COLORS_LENGTH - 4] = gradientColor1;
  stationColors[STATION_COLORS_LENGTH - 3] = gradientColor1;
  stationColors[STATION_COLORS_LENGTH - 2] = gradientColor1;
  stationColors[STATION_COLORS_LENGTH - 1] = white;

  // Setup actual gradient of colors (IE the colors that aren't the current station)
  for (int i = 6; i <= (STATION_COLORS_LENGTH - 5); i++) {
    if (i <= STATION_COLORS_LENGTH / 2) {
      // gradient from gradientColor1 to gradientColor2
      stationColors[i] = interpolate(i, 6, STATION_COLORS_LENGTH / 2, gradientColor1, gradientColor2);
    } else {
      // now gradient back going from gradientColor2 back to gradientColor1
      stationColors[i] = interpolate(i, (STATION_COLORS_LENGTH / 2) + 1, STATION_COLORS_LENGTH - 5, gradientColor2, gradientColor1);
    }
  }
  strip.begin();  // Initialize pins for output
  updatePixels();
}

void updateLightPulses(int currentStationIndex) {
  unsigned long updateTime = micros();

  // clear active pulses during channel changing, they are confusing
  if ((updateTime - lastStationDialChangeChangeMicros) <= MIN_WAIT_UNTIL_PULSE_START) {
    for (int i = 0; i < MAX_LIGHT_PULSES; i++) {
      lightPulses[i].alive = false;
    }
  }

  // Send new pulse?
  if (sendNewPulse) {
    sendNewPulse = false;
    lastPulseCreate = updateTime;

    // create a pulse going down
    if (currentStationIndex > MIN_DISTANCE_FROM_EDGE_FOR_PULSE_START) {
      lightPulses[lightPulseIndex].location = currentStationIndex - PULSE_START_OFFSET_LOW;
      lightPulses[lightPulseIndex].timeOfLastChange = updateTime;
      lightPulses[lightPulseIndex].birth = updateTime;
      lightPulses[lightPulseIndex].direction = -1;
      lightPulses[lightPulseIndex].alive = true;
      lightPulseIndex = (lightPulseIndex + 1) >= MAX_LIGHT_PULSES ? 0 : lightPulseIndex + 1; // overwrite oldest pulse
    }

    // create a pulse going up
    if (currentStationIndex < STATION_COLORS_LENGTH - MIN_DISTANCE_FROM_EDGE_FOR_PULSE_START) {
      lightPulses[lightPulseIndex].location = currentStationIndex + PULSE_START_OFFSET_HIGH;
      lightPulses[lightPulseIndex].timeOfLastChange = updateTime;
      lightPulses[lightPulseIndex].birth = updateTime;
      lightPulses[lightPulseIndex].direction = 1;
      lightPulses[lightPulseIndex].alive = true;
      lightPulseIndex = (lightPulseIndex + 1) >= MAX_LIGHT_PULSES ? 0 : lightPulseIndex + 1; // overwrite oldest pulse
    }
  }

  // Draw pulses
  int loc = 0;
  int locC = 0;
  int centerPix = 0;
  uint32_t c;
  uint8_t r, g, b;
  uint32_t newColor;
  int d = 0;
  int timeD = 0;
  for (int i = 0; i < MAX_LIGHT_PULSES; i++) {
    if (lightPulses[i].alive) {
      loc = lightPulses[i].location + STATION_PIXEL_START_INDEX;
      centerPix = currentStationIndex + STATION_PIXEL_START_INDEX;
      timeD = constrain((updateTime - lightPulses[i].birth) * 255 / PULSE_MAX_LIFE, 0, 255);
      if (centerPix != loc) {
        c = strip.getPixelColor(loc);
        r = c >> 16 & 0xFF;
        g = c >> 8 & 0xFF;
        b = c & 0xFF;
        newColor = strip.Color(constrain((r + 255 - timeD), 0, 255), constrain((g + 255 - timeD), 0, 255), constrain((b + 255 - timeD), 0, 255));
        strip.setPixelColor(loc, newColor);
      }
      for (int j = 1; j <= PULSE_WIDTH_HALF; j++) {
        d = constrain(255 * (PULSE_WIDTH_HALF - j) / (PULSE_WIDTH_HALF - 1) - timeD, 0, 255);

        // draw pulse part that is left of center
        locC = loc - j;
        if (centerPix != locC) {
          c = strip.getPixelColor(locC);
          r = c >> 16 & 0xFF;
          g = c >> 8 & 0xFF;
          b = c & 0xFF;
          newColor = strip.Color(constrain((r + d), 0, 255), constrain((g + d), 0, 255), constrain((b + d), 0, 255));
          strip.setPixelColor(locC, newColor);
        }

        // draw pulse part that is right of center
        locC = loc + j;
        if (centerPix != locC) {
          c = strip.getPixelColor(locC);
          r = c >> 16 & 0xFF;
          g = c >> 8 & 0xFF;
          b = c & 0xFF;
          newColor = strip.Color(constrain((r + d), 0, 255), constrain((g + d), 0, 255), constrain((b + d), 0, 255));
          strip.setPixelColor(locC, newColor);
        }
      }
    }
  }

  // Update pulse locations
  for (int i = 0; i < MAX_LIGHT_PULSES; i++) {
    if (lightPulses[i].alive && ((updateTime - lightPulses[i].timeOfLastChange) > DELAY_BETWEEN_PULSE_UPDATES)) {
      lightPulses[i].location = lightPulses[i].location + (PULSE_SPEED * lightPulses[i].direction);
      lightPulses[i].timeOfLastChange = updateTime;
      //|| (updateTime - lightPulses[i].birth) > PULSE_MAX_LIFE
      if (lightPulses[i].location >= STATION_COLORS_LENGTH || lightPulses[i].location < 0) {
        lightPulses[i].alive = false;
      }
    }
  }
}

void updatePixels() {
  // Update pixels at ~60fps
  if (micros() - lastLEDUpdate < MIN_DELAY_BETWEEN_LED_UPDATES) {
    return;
  }
  lastLEDUpdate = micros();

  // Reset Current Tick to white when moving so we can see it
  if ((lastLEDUpdate - lastStationDialChangeChangeMicros) <= MIN_WAIT_UNTIL_PULSE_START) {
    stationColors[0] = white;
    stationColors[1] = white;
    stationColors[2] = white;
    stationColors[STATION_COLORS_LENGTH - 1] = white;

    // clear breathing that happens around center pixel during channel changing
    stationColors[3] = gradientColor1;
    stationColors[4] = gradientColor1;
    stationColors[5] = gradientColor1;
    stationColors[STATION_COLORS_LENGTH - 4] = gradientColor1;
    stationColors[STATION_COLORS_LENGTH - 3] = gradientColor1;
    stationColors[STATION_COLORS_LENGTH - 2] = gradientColor1;

    // restart breathing timing
    centerPulseSineTime255 = 0;
  }

  // Update Current Station Tick brightness based on sine wave
  if ((lastLEDUpdate - lastStationDialChangeChangeMicros) > MIN_WAIT_UNTIL_PULSE_START &&
      (lastLEDUpdate - centerTickLastTimeChange) > CENTER_TICK_SPEED) {
    centerTickLastTimeChange = lastLEDUpdate;
    centerPulseSineTime255 = centerPulseSineTime255 + 1; // uint8 will automatically roll-over at 255
    uint32_t newColor = interpolate(strip.sine8(centerPulseSineTime255), 0, 255, white, gradientColor1);

    // breath the pixels around the center chanel
    stationColors[3] = newColor;
    stationColors[4] = newColor;
    stationColors[5] = newColor;
    stationColors[STATION_COLORS_LENGTH - 4] = newColor;
    stationColors[STATION_COLORS_LENGTH - 3] = newColor;
    stationColors[STATION_COLORS_LENGTH - 2] = newColor;

    if (centerPulseSineTime255 == SINE_TABLE_255_SEND_PULSE) {
      sendNewPulse = true;
    }
  }

  // Location of bulb that indicates current station
  // this index gives us the location within just our STATION LEDs
  // int lightOffsetIndex = map(channel, MINFREQ, MAXFREQ, 0, NUMPIXELS);
  int lightOffsetIndex = constrain(int(LEDS_PER_STATION * (channel - MINFREQ) / 2.0), 0, STATION_COLORS_LENGTH - 1);

  // Manually fudge factors, to get bulbs aligned with stations
  // since we don't have exactly 3 bulbs per tick
  if (lightOffsetIndex > 151 /* this is where we are now fully 1 bulb out of alignment */) {
    lightOffsetIndex = lightOffsetIndex - 1;
  }

  /* Color array is used to determine bulb colors
     we will slide it up and down the LED strip
     it loops as it gets to the end
  */
  int stationColorsIndex = 0;
  int onCount = 0;
  // set color of current station up to the last channel pixel in our LED strip
  for (int i = lightOffsetIndex + STATION_PIXEL_START_INDEX; i <= STATION_PIXEL_END_INDEX; i++) {
    strip.setPixelColor(i, stationColors[stationColorsIndex]);
    stationColorsIndex++;
  }
  // if we haven't used up our entire light array
  // loop it filling in the colors starting from bulb 0 up to but not including
  // the bulb representing the current station
  for (int i = STATION_PIXEL_START_INDEX; i < lightOffsetIndex + STATION_PIXEL_START_INDEX; i++) {
    strip.setPixelColor(i, stationColors[stationColorsIndex]);
    stationColorsIndex++;
  }

  // when we are on the first tick (86.5), don't place a white bulb before it, since this would
  // appear at the end of the track (wrap-around) which is kind of confusing
  if (lightOffsetIndex == 0) {
    // use the 2nd to last color instead of the last
    strip.setPixelColor(STATION_PIXEL_END_INDEX, stationColors[STATION_COLORS_LENGTH - 2]);
  }
  // when we are on the last tick (107.9), don't place a white bulb after it, since this would
  // appear at the beginning of the track (wrap-around) which is kind of confusing
  if (channel == MAXFREQ) {
    strip.setPixelColor(STATION_PIXEL_START_INDEX, stationColors[3]);
  }

  // Update Volume LEDs
  // note that volume is mounted upside down for layout reasons
  // IE bulb 0 is on top and bulb VOLUME_NUM_PIXELS-1 is on bottom
  // volume will radiate from center, louder, more lights
  int firstHalfLEDsOnCount = int(constrain(round(map(volume, MIN_VOLUME, MAX_VOLUME, 0, VOLUME_CENTER_PIXEL)), 0, VOLUME_CENTER_PIXEL));
  int secondHalfLEDsOnCount = firstHalfLEDsOnCount;
  if (volume == 0) {
    // turn center pixel off for 0 volume
    strip.setPixelColor(VOLUME_CENTER_PIXEL, volumeOffColor);
  } else {
    strip.setPixelColor(VOLUME_CENTER_PIXEL, volumeOnColor);
  }
  for (int i = VOLUME_CENTER_PIXEL + 1; i < VOLUME_NUM_PIXELS; i++) {
    if (firstHalfLEDsOnCount > 0) {
      strip.setPixelColor(i, volumeOnColor);
      firstHalfLEDsOnCount--;
    } else {
      strip.setPixelColor(i, volumeOffColor);
    }
  }
  for (int i = VOLUME_CENTER_PIXEL - 1; i >= 0; i--) {
    if (secondHalfLEDsOnCount > 0) {
      strip.setPixelColor(i, volumeOnColor);
      secondHalfLEDsOnCount--;
    } else {
      strip.setPixelColor(i, volumeOffColor);
    }
  }

  // color the offband pixels
  for (int i = VOLUME_NUM_PIXELS; i < STATION_PIXEL_START_INDEX; i++) {
    strip.setPixelColor(i, offbandColor);
  }
  for (int i = STATION_PIXEL_END_INDEX + 1; i < NUMPIXELS; i++) {
    strip.setPixelColor(i, offbandColor);
  }

  updateLightPulses(lightOffsetIndex);
  strip.show();
}

uint8_t r1, g1, b1, r2, g2, b2;
uint32_t interpolate(int cur, int min, int max, uint32_t c1, uint32_t c2) {
  r1 = c1 >> 16 & 0xFF;
  g1 = c1 >> 8 & 0xFF;
  b1 = c1 & 0xFF;
  r2 = c2 >> 16 & 0xFF;
  g2 = c2 >> 8 & 0xFF;
  b2 = c2 & 0xFF;
  
  // gamma it correctly
  float x = (float)strip.gamma8(round((((float)(cur - min)) / ((float)max - (float)min) * 255))) / (float)255.0;
  float xr = (float)gamma38(round((((float)(cur - min)) / ((float)max - (float)min) * 255))) / (float)255.0;
  return strip.Color(
           lerp(r1, r2, xr),
           lerp(g1, g2, x),
           lerp(b1, b2, x)
         );
}

uint8_t lerp(uint8_t a, uint8_t b, float x) {
  if (a == b) {
    return a;
  }
  if (a < b) {
    return constrain(a + (x * ((float)b - (float)a)), 0, 255);
  }
  return lerp(b, a, (float)1.0 - x);
}

// generated with Processing Sketch `tools/GenerateRedGammaTable`
const uint8_t PROGMEM _gamma38[] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,
  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  3,  3,  3,  3,
  3,  3,  3,  4,  4,  4,  4,  4,  4,  5,  5,  5,  5,  6,  6,  6,
  6,  6,  7,  7,  7,  8,  8,  8,  8,  9,  9,  9, 10, 10, 10, 11,
  11, 12, 12, 12, 13, 13, 14, 14, 15, 15, 15, 16, 16, 17, 18, 18,
  19, 19, 20, 20, 21, 21, 22, 23, 23, 24, 25, 25, 26, 27, 28, 28,
  29, 30, 31, 31, 32, 33, 34, 35, 36, 37, 38, 38, 39, 40, 41, 42,
  43, 44, 45, 47, 48, 49, 50, 51, 52, 53, 55, 56, 57, 58, 60, 61,
  62, 64, 65, 66, 68, 69, 71, 72, 74, 75, 77, 78, 80, 82, 83, 85,
  87, 88, 90, 92, 94, 96, 98, 99, 101, 103, 105, 107, 109, 111, 113, 115,
  118, 120, 122, 124, 126, 129, 131, 133, 136, 138, 141, 143, 146, 148, 151, 153,
  156, 158, 161, 164, 167, 169, 172, 175, 178, 181, 184, 187, 190, 193, 196, 199,
  203, 206, 209, 212, 216, 219, 222, 226, 229, 233, 237, 240, 244, 247, 251, 255
};

const uint8_t gamma38(uint8_t x) {
  return pgm_read_byte(&_gamma38[x]); // 0-255 in, 0-255 out
}
