const int PHOTOGATE_LOG_BUFFER_SIZE = 11;
const long READ_DELAY = 2;

// sliding buffers for storing log data
long lastRead = millis();
int photogateLogIndex = 0;
boolean photogateLog1[PHOTOGATE_LOG_BUFFER_SIZE];
boolean photogateLog2[PHOTOGATE_LOG_BUFFER_SIZE];
boolean bufferReady = false;
float totalTransitions = ceil(((float)PHOTOGATE_LOG_BUFFER_SIZE) / 2.0);

void measure() {
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
    int transitions = 0;
    for (int i = 1; i < PHOTOGATE_LOG_BUFFER_SIZE; i++) {
      if (photogateLog1[i] && !photogateLog1[i - 1]) {
        transitions++;
      }
    }
    if (photogateLog1[0] && !photogateLog1[PHOTOGATE_LOG_BUFFER_SIZE - 1]) {
      transitions++;
    }
    speed1 = (float)transitions / totalTransitions;

    transitions = 0;
    for (int i = 1; i < PHOTOGATE_LOG_BUFFER_SIZE; i++) {
      if (photogateLog2[i] && !photogateLog2[i - 1]) {
        transitions++;
      }
    }
    if (photogateLog2[0] && !photogateLog2[PHOTOGATE_LOG_BUFFER_SIZE - 1]) {
      transitions++;
    }
    speed2 = (float)transitions / totalTransitions;

    // Use Photogate 1 to change channel
    // only update channel after a speed update
    if (speed1 > 0) {
      int channelSpeed = int(constrain(round(speed1 * 20), 0, 4));
      if (channelSpeed > 0) {
        channel = channel + channelSpeed;
        if (channel > 1079) {
          channel = 881;
        }
        radio.setChannel(channel);
        updatePixels();
      }
    }
  }
}
