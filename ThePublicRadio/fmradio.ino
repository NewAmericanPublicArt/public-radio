#include <Si4703_Breakout.h>

#define MAX_VOLUME 15
#define MIN_VOLUME 5

Si4703_Breakout radio(RADIO_RESET_PIN, RADIO_SDIO, RADIO_SCLK);
// U.S. FM Broadcast is 87.9—107.9 (101 Stations)
// we have a few extra ticks so we'll do 86.5—107.9 (108 Stations)
float volume = 5; // 0—15
char rdsBuffer[10];

void radioSetup() {
  radio.powerOn();
  radio.setVolume(int(round(volume)));
  radio.setChannel(channel);
}

void seekUp() {
  channel = radio.seekUp();
  displayInfo();
}

void seekDown() {
  channel = radio.seekDown();
  displayInfo();
}

void displayInfo() {
#ifdef SERIAL_DEBUG
  Serial.print("Channel:"); Serial.print(channel);
  Serial.print(" Volume:"); Serial.println(volume);
#endif
}

void channelTuningWithMicrobitButtons() {
  // Use Microbit buttons to seek up and down
  if (!digitalRead(buttonA)) {
    channel = channel - 2;
    if (channel < MINFREQ) {
      channel = MAXFREQ;
    }
    radio.setChannel(channel);
  }
  if (!digitalRead(buttonB)) {
    channel = channel + 2;
    if (channel > MAXFREQ) {
      channel = MINFREQ;
    }
    radio.setChannel(channel);
  }
}
