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

int channelDirection = 1;
int volumeDirection = 1;

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

    // update speed based on just the readings from the A sensors
    if (ready) {
      // only measure one edge of the slit
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

      channelUp(speed1); // update channel based on speed
      volumeUp(speed2); // update volume based on speed
    }
  }
}

void volumeUp(float speed) {
  if (speed > 0) {
    volume = volume + (speed * volumeDirection);
    if (volume > MAX_VOLUME) {
      volume = MAX_VOLUME;
      volumeDirection *= -1; // change direction, AKA Dan's avacado volume
    }
    if (volume < MIN_VOLUME) {
      volume = MIN_VOLUME;
      volumeDirection *= -1; // change direction, AKA Dan's avacado volume
    }
    radio.setVolume(int(round(volume)));
  }
}

void channelUp(float speed) {
  if (speed > 0) {
    channel = int(channel + 2 * speed);
    if (channel % 2 != 1) {
      // make sure we have an odd channel
      channel++;
    }
    if (channel > MAXFREQ) {
      channel = MINFREQ; // loop around
    }
    radio.setChannel(channel);
  }
}
