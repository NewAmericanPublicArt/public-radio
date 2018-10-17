float speed1 = 0;
float speed2 = 0;

const int PHOTOGATE_LOG_BUFFER_SIZE = 11;
const long READ_DELAY = 5;

// sliding buffers for storing log data
long lastRead = millis();
int photogateLogIndex = 0;
boolean photogateLog1[PHOTOGATE_LOG_BUFFER_SIZE];
boolean photogateLog2[PHOTOGATE_LOG_BUFFER_SIZE];
boolean bufferReady = false;
float totalTransitions = ceil(((float)PHOTOGATE_LOG_BUFFER_SIZE) / 2.0);
int volumeDirection = 1;

void readPhotogatesForTuningAndVolume() {
  // Read our data
  if (millis() - lastRead > READ_DELAY) {
    lastRead = millis();
    photogateLog1[photogateLogIndex] = digitalRead(PHOTOGATE1_PIN) == LOW;
    photogateLog2[photogateLogIndex] = digitalRead(PHOTOGATE2_PIN) == LOW;

    // let the buffer fill out before we use it
    if (!bufferReady && photogateLogIndex == PHOTOGATE_LOG_BUFFER_SIZE - 1) {
      bufferReady = true;
    }

    // update our speed data
    // we will say that speed is number of transitions from HIGH to LOW
    // out of total possible transitions
    updateSpeedData();

    // update index for next log
    photogateLogIndex = (photogateLogIndex + 1) % PHOTOGATE_LOG_BUFFER_SIZE;
  }
}

void updateSpeedData() {
  if (bufferReady) {
    int transitions = 0; // total transitions over buffer time period
    boolean justTransitionedChannel = false; // our last sample was a transition recording
    for (int i = 1; i < PHOTOGATE_LOG_BUFFER_SIZE; i++) {
      if (photogateLog1[i] && !photogateLog1[i - 1]) {
        transitions++;
        if (i == photogateLogIndex) {
          justTransitionedChannel = true;
        }
      }
    }
    if (photogateLog1[0] && !photogateLog1[PHOTOGATE_LOG_BUFFER_SIZE - 1]) {
      transitions++;
      if (0 == photogateLogIndex) {
        justTransitionedChannel = true;
      }
    }
    speed1 = (float)transitions / totalTransitions;

    transitions = 0;
    boolean justTransitionedVolume = false;
    for (int i = 1; i < PHOTOGATE_LOG_BUFFER_SIZE; i++) {
      if (photogateLog2[i] && !photogateLog2[i - 1]) {
        transitions++;
        if (i == photogateLogIndex) {
          justTransitionedVolume = true;
        }
      }
    }
    if (photogateLog2[0] && !photogateLog2[PHOTOGATE_LOG_BUFFER_SIZE - 1]) {
      transitions++;
      if (0 == photogateLogIndex) {
        justTransitionedVolume = true;
      }
    }
    speed2 = (float)transitions / totalTransitions;

    // Use Photogate 1 to change channel
    // only update channel after a speed update
    if (justTransitionedChannel) {
      channelUp();
      updatePixels();
    }

    // Use Photogate 2 to change volume
    // only update volume after a speed update
    if (speed2 > 0) {
      volume = volume + (speed2 * volumeDirection);
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
}

void channelUp() {
  channel = channel + 2;
  if (channel % 2 != 1) {
    // make sure we have an odd channel
    channel++;
  }
  if (channel > MAXFREQ) {
    channel = MINFREQ;
  }
  radio.setChannel(channel);
}
