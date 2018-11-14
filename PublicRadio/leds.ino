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
uint32_t white;

struct LightPulse {
  int location;  // index within just the station LEDs
  unsigned long timeOfLastChange;
  unsigned long birth;
  int direction; // positive: increase station, negative: decrease station
  boolean alive;
};
#define MAX_LIGHT_PULSES 20
LightPulse lightPulses[MAX_LIGHT_PULSES];
int lightPulseIndex = 0;
boolean sendNewPulse = false;
unsigned long lastPulseCreate = 0;
unsigned long PULSE_SPEED = 1; // millis to wait on each LED before transitioning to the next
unsigned long MIN_WAIT_UNTIL_PULSE_START = 500; // millis to wait until a pulse starts
unsigned long PULSE_MAX_LIFE = 2000; // millis to wait in between pulses
int PULSE_WIDTH_HALF = 6; // pulse with be twice this width + 1 (the center)
// we don't want to send pulses to the edge if we are really close to the edge
// because they won't look good
#define MIN_DISTANCE_FROM_EDGE_FOR_PULSE_START 20
/* a tick is 4-bulbs wide but is index off the leading edge of the tick
   so our pulse starts 2 indices below the tick and 3 indices above */
#define PULSE_START_OFFSET_LOW 2
#define PULSE_START_OFFSET_HIGH 3
// index into Dotstar library sine table at which we will send a new pulse
// we want this less then 255 value so it appears the pulse is sent as we approach 255
#define SINE_TABLE_255_SEND_PULSE 16
#define SINE_TABLE_255 62 // index of a 255 value in Dotstar library sine table
uint8_t centerPulseSineTime255 = SINE_TABLE_255;
int CENTER_TICK_SPEED = 1;
unsigned long centerTickLastTimeChange = 0;

unsigned long MIN_DELAY_BETWEEN_LED_UPDATES = 16; // ~60fps, IE 1000ms/60 ~= 16
unsigned long lastLEDUpdate = 0;

void ledsSetup() {
  volumeOnColor = strip.Color(20, 20, 255);
  volumeOffColor = strip.Color(0, 0, 0);
  white = strip.Color(255, 255, 255);

  // setup light pulse array
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

  // clear active pulses during channel changing, they are confusing
  if ((updateTime - lastChannelChange) <= MIN_WAIT_UNTIL_PULSE_START) {
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
    if (lightPulses[i].alive && ((updateTime - lightPulses[i].timeOfLastChange) > PULSE_SPEED)) {
      lightPulses[i].location = lightPulses[i].location + lightPulses[i].direction;
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
  if (millis() - lastLEDUpdate < MIN_DELAY_BETWEEN_LED_UPDATES) {
    return;
  }
  lastLEDUpdate = millis();

  // Update Current Station Tick brightness based on sine wave
  if ((lastLEDUpdate - lastChannelChange) > MIN_WAIT_UNTIL_PULSE_START &&
      (lastLEDUpdate - centerTickLastTimeChange) > CENTER_TICK_SPEED) {
    centerTickLastTimeChange = lastLEDUpdate;
    centerPulseSineTime255++;
    uint8_t brightness = strip.sine8(centerPulseSineTime255);
    uint32_t newColor = strip.Color(brightness, brightness, brightness);
    stationColors[0] = newColor;
    stationColors[1] = newColor;
    stationColors[2] = newColor;
    stationColors[STATION_COLORS_LENGTH - 1] = newColor;
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

  updateLightPulses(lightOffsetIndex);
  strip.show();
}
