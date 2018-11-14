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
uint32_t volumeOnColor;
uint32_t volumeOffColor;
uint32_t green;

struct LightPulse {
  int location;  // index within just the station LEDs
  unsigned long timeOfLastChange;
  int direction; // positive: increase station, negative: decrease station
  boolean alive;
};
#define MAX_LIGHT_PULSES 20
LightPulse lightPulses[MAX_LIGHT_PULSES];
int lightPulseIndex = 0;
unsigned long PULSE_SPEED = 120; // millis to wait on each LED before transitioning to the next
unsigned long MIN_WAIT_UNTIL_PULSE_START = 800; // millis to wait until a pulse starts
// we don't want to send pulses to the edge if we are really close to the edge
// because they won't look good
#define MIN_DISTANCE_FROM_EDGE_FOR_PULSE_START 5

unsigned long MIN_DELAY_BETWEEN_LED_UPDATES = 16; // ~60fps, IE 1000ms/60 ~= 16
unsigned long lastLEDUpdate = 0;

void ledsSetup() {
  volumeOnColor = strip.Color(20, 20, 255);
  volumeOffColor = strip.Color(0, 0, 0);
  green = strip.Color(0, 255, 0);

  // setup light pulse array
  for (int i = 0; i < MAX_LIGHT_PULSES; i++) {
    lightPulses[i] = (LightPulse) {
      0, 0, 0, false
    };
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
  for (int i = 6; i <= (STATION_COLORS_LENGTH - 5); i++) {
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

void updateLightPulses(int currentStationIndex) {
  unsigned long updateTime = millis();

  // Send new pulse?
  if (millis() - lastRadioUpdate > MIN_WAIT_UNTIL_PULSE_START) {
    // create a pulse going down
    if (currentStationIndex > MIN_DISTANCE_FROM_EDGE_FOR_PULSE_START) {
      lightPulses[lightPulseIndex] = (LightPulse) {
        currentStationIndex - 1, updateTime, -1, true
      };
      lightPulseIndex = (lightPulseIndex + 1) >= MAX_LIGHT_PULSES ? 0 : lightPulseIndex + 1; // overwrite oldest pulse
    }

    // create a pulse going up
    if (currentStationIndex < STATION_COLORS_LENGTH - MIN_DISTANCE_FROM_EDGE_FOR_PULSE_START) {
      lightPulses[lightPulseIndex] = (LightPulse) {
        currentStationIndex + 1, updateTime, 1, true
      };
      lightPulseIndex = (lightPulseIndex + 1) >= MAX_LIGHT_PULSES ? 0 : lightPulseIndex + 1; // overwrite oldest pulse
    }
  }

  // Draw pulses
  for (int i = 0; i < MAX_LIGHT_PULSES; i++) {
    if (lightPulses[i].alive) {
      // TODO set as opposite color of non-pulse color of current location
      strip.setPixelColor(lightPulses[i].location + STATION_PIXEL_START_INDEX, green);
    }
  }

  // Update pulse locations
  for (int i = 0; i < MAX_LIGHT_PULSES; i++) {
    if (lightPulses[i].alive && updateTime - lightPulses[i].timeOfLastChange > PULSE_SPEED) {
      lightPulses[i].location = lightPulses[i].location * lightPulses[i].direction;
      lightPulses[i].timeOfLastChange = updateTime;
      if (lightPulses[i].location > STATION_COLORS_LENGTH || lightPulses[i].location < 0) {
        lightPulses[i].alive = false;
      }
    }
  }
}

void updatePixels() {
  // Update pixels at ~60fps
  if (millis() - lastLEDUpdate < MIN_DELAY_BETWEEN_LED_UPDATES) {
    return;
  }
  lastLEDUpdate = millis();

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
  // min volume will be 1 LED (never 0 leds, that could be confusing)
  // volume will radiate from center, louder, more lights
  int firstHalfLEDsOnCount = int(constrain(round(map(volume, MIN_VOLUME, MAX_VOLUME, 0, VOLUME_CENTER_PIXEL)), 0, VOLUME_CENTER_PIXEL));
  int secondHalfLEDsOnCount = firstHalfLEDsOnCount;
  strip.setPixelColor(VOLUME_CENTER_PIXEL, volumeOnColor); // center Pixel is always on
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

//  updateLightPulses(lightOffsetIndex);
  strip.show();
}
