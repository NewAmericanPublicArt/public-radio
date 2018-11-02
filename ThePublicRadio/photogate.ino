float speed1 = 0;
float speed2 = 0;

const long READ_DELAY = 5;

long lastRead = millis();
boolean ready = false;
long readings = 0;

// Store current and previous photogate reading
boolean photogate1A = false;
boolean photogate1A_prev = false;
long timeOfTransition1A = 0;
long timeOfTransition1A_prev = 0;
boolean photogate1B = false;
boolean photogate1B_prev = false;
boolean photogate2A = false;
boolean photogate2A_prev = false;
long timeOfTransition2A = 0;
long timeOfTransition2A_prev = 0;
boolean photogate2B = false;
boolean photogate2B_prev = false;
int reading1_TwoBitGrayCode = 0; // combination of 1A and 1B
int reading1_TwoBitGrayCode_prev = 0;
int reading2_TwoBitGrayCode = 0; // combination of 2A and 2B
int reading2_TwoBitGrayCode_prev = 0;

int channelDirection = 1;
// Volume direction can change based on Dan's avocado volume (IE change direction when hitting limits)
// or by the user actually changing the direction of the wheel. whaaaaah :), I know fun.
int volumeAvocadoDirection = 1;
int volumeWheelDirection = 1;

void readPhotogatesForTuningAndVolume() {
  // Read our data
  if (millis() - lastRead > READ_DELAY) {
    lastRead = millis();

    // save off previous reading
    photogate1A_prev = photogate1A;
    photogate1B_prev = photogate1B;
    photogate2A_prev = photogate2A;
    photogate2B_prev = photogate2B;
    readings++;
    if (readings > 2) {
      ready = true;
    }

    // take a reading of all 4 sensors
    photogate1A = digitalRead(PHOTOGATE1A_PIN) == HIGH;
    photogate1B = digitalRead(PHOTOGATE1B_PIN) == HIGH;
    photogate2A = digitalRead(PHOTOGATE2A_PIN) == HIGH;
    photogate2B = digitalRead(PHOTOGATE2B_PIN) == HIGH;

    // measure any edge of the slit for phase/direction
    // see https://en.wikipedia.org/wiki/Rotary_encoder, https://commons.wikimedia.org/wiki/File:Incremental_directional_encoder.gif
    // https://www.ethz.ch/content/dam/ethz/special-interest/mavt/dynamic-systems-n-control/idsc-dam/Lectures/Embedded-Control-Systems/LectureNotes/3_Position_and_Velocity_Measurements.pdf
    // AKA a 2-bit Gray code pattern
    reading1_TwoBitGrayCode_prev = reading1_TwoBitGrayCode;
    reading2_TwoBitGrayCode_prev = reading2_TwoBitGrayCode;
    reading1_TwoBitGrayCode = photogate1A * 2 + photogate1B;
    reading2_TwoBitGrayCode = photogate2A * 2 + photogate2B;

    // update speed based on just the readings from the A sensors
    if (ready) {
      // only measure one edge of the slit for speed
      // so we have a reliably consistent speed
      speed1 = 0;
      speed2 = 0;
      if (photogate1A == HIGH && photogate1A_prev == LOW) {
        timeOfTransition1A = millis();
        speed1 = 1; // min-speed
      }
      if (photogate2A == HIGH && photogate2A_prev == LOW) {
        timeOfTransition2A = millis();
        speed2 = 1; // min-speed
      }

      int reading1_Change = reading1_TwoBitGrayCode - reading1_TwoBitGrayCode_prev;
      int reading2_Change = reading2_TwoBitGrayCode - reading2_TwoBitGrayCode_prev;
      if (reading1_Change == 1) {
        channelDirection = -1;
      } else if (reading1_Change == -1) {
        channelDirection = 1;
      }
      if (reading2_Change == 1) {
        volumeWheelDirection = -1;
      } else if (reading2_Change == -1) {
        volumeWheelDirection = 1;
      }

      channelChange(speed1); // update channel based on speed
      volumeChange(speed2); // update volume based on speed
    }
  }
}

void volumeChange(float speed) {
  if (speed > 0) {
    volume = volume + (speed * volumeAvocadoDirection * volumeWheelDirection);
    if (volume > MAX_VOLUME) {
      volume = MAX_VOLUME;
      volumeAvocadoDirection *= -1; // change direction, AKA Dan's avocado volume
    }
    if (volume < MIN_VOLUME) {
      volume = MIN_VOLUME;
      volumeAvocadoDirection *= -1; // change direction, AKA Dan's avocado volume
    }
    radio.setVolume(int(round(volume)));
  }
}

void channelChange(float speed) {
  if (speed > 0) {
    // multiply by 2, U.S. stations are only on odd station numbers
    channel = int(channel + 2 * speed * channelDirection);

    // always make sure we have an odd channel
    if (channel % 2 != 1) {
      channel = channel + 1 * channelDirection;
    }

    // loop around channel
    if (channel > MAXFREQ) {
      channel = MINFREQ;
    }
    if (channel < MINFREQ) {
      channel = MAXFREQ;
    }
    radio.setChannel(channel);
  }
}
