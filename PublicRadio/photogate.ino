int speed1 = 0;
int speed2 = 0;

// Store current and previous photogate reading
boolean photogate1A = false;
boolean photogate1A_prev = false;
boolean photogate1B = false;
boolean photogate1B_prev = false;
boolean photogate2A = false;
boolean photogate2A_prev = false;
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

void readPhotogate1() {
  photogate1A_prev = photogate1A;
  photogate1B_prev = photogate1B;
  photogate1A = digitalRead(PHOTOGATE1A_PIN) == HIGH;
  photogate1B = digitalRead(PHOTOGATE1B_PIN) == HIGH;

  updateChannelFromPhotogateReadings();
}

void readPhotogate2() {
  photogate2A_prev = photogate2A;
  photogate2B_prev = photogate2B;
  photogate2A = digitalRead(PHOTOGATE2A_PIN) == HIGH;
  photogate2B = digitalRead(PHOTOGATE2B_PIN) == HIGH;

  updateVolumeFromPhotogateReadings();
}

void updateChannelFromPhotogateReadings() {
  // measure any edge of the slit for phase/direction
  // see https://en.wikipedia.org/wiki/Rotary_encoder, https://commons.wikimedia.org/wiki/File:Incremental_directional_encoder.gif
  // https://www.ethz.ch/content/dam/ethz/special-interest/mavt/dynamic-systems-n-control/idsc-dam/Lectures/Embedded-Control-Systems/LectureNotes/3_Position_and_Velocity_Measurements.pdf
  // AKA a 2-bit Gray code pattern
  reading1_TwoBitGrayCode_prev = reading1_TwoBitGrayCode;
  reading1_TwoBitGrayCode = photogate1A * 2 + photogate1B;

  // update speed based on just the readings from the A sensors
  speed1 = 0;
  if (photogate1A != photogate1A_prev) {
    //      timeOfTransition1A = millis();
    speed1 = 1; // min-speed
  }

  switch (reading1_TwoBitGrayCode_prev) {
    case 0:
      switch (reading1_TwoBitGrayCode) {
        case 1:
          channelDirection = 1;
          break;
        case 2:
          channelDirection = -1;
          break;
      }
      break;
    case 1:
      switch (reading1_TwoBitGrayCode) {
        case 0:
          channelDirection = -1;
          break;
        case 3:
          channelDirection = 1;
          break;
      }
      break;
    case 2:
      switch (reading1_TwoBitGrayCode) {
        case 0:
          channelDirection = 1;
          break;
        case 3:
          channelDirection = -1;
          break;
      }
      break;
    case 3:
      switch (reading1_TwoBitGrayCode) {
        case 1:
          channelDirection = -1;
          break;
        case 2:
          channelDirection = 1;
          break;
      }
      break;
  }

#ifdef SERIAL_DEBUG
  Serial.println("1A,1B,2A,2B: " + String(photogate1A) + "," + String(photogate1B) + "," + String(photogate2A) + "," + String(photogate2B) + " — 1,2 CW/CCW?: " + String(channelDirection) + "," + String(volumeWheelDirection));
#endif

  channelChange(speed1); // update channel based on speed
}

void updateVolumeFromPhotogateReadings() {
  // measure any edge of the slit for phase/direction
  // see https://en.wikipedia.org/wiki/Rotary_encoder, https://commons.wikimedia.org/wiki/File:Incremental_directional_encoder.gif
  // https://www.ethz.ch/content/dam/ethz/special-interest/mavt/dynamic-systems-n-control/idsc-dam/Lectures/Embedded-Control-Systems/LectureNotes/3_Position_and_Velocity_Measurements.pdf
  // AKA a 2-bit Gray code pattern
  reading2_TwoBitGrayCode_prev = reading2_TwoBitGrayCode;
  reading2_TwoBitGrayCode = photogate2A * 2 + photogate2B;

  // update speed based on just the readings from the A sensors
  speed2 = 0;
  if (photogate2A != photogate2A_prev) {
    speed2 = 1; // min-speed
  }

  switch (reading2_TwoBitGrayCode_prev) {
    case 0:
      switch (reading2_TwoBitGrayCode) {
        case 1:
          volumeWheelDirection = 1;
          break;
        case 2:
          volumeWheelDirection = -1;
          break;
      }
      break;
    case 1:
      switch (reading2_TwoBitGrayCode) {
        case 0:
          volumeWheelDirection = -1;
          break;
        case 3:
          volumeWheelDirection = 1;
          break;
      }
      break;
    case 2:
      switch (reading2_TwoBitGrayCode) {
        case 0:
          volumeWheelDirection = 1;
          break;
        case 3:
          volumeWheelDirection = -1;
          break;
      }
      break;
    case 3:
      switch (reading2_TwoBitGrayCode) {
        case 1:
          volumeWheelDirection = -1;
          break;
        case 2:
          volumeWheelDirection = 1;
          break;
      }
      break;
  }

#ifdef SERIAL_DEBUG
  Serial.println("1A,1B,2A,2B: " + String(photogate1A) + "," + String(photogate1B) + "," + String(photogate2A) + "," + String(photogate2B) + " — 1,2 CW/CCW?: " + String(channelDirection) + "," + String(volumeWheelDirection));
#endif

  volumeChange(speed2); // update volume based on speed
}

void volumeChange(int speed) {
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
  }
}

void channelChange(int speed) {
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
  }
}
