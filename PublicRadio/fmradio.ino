#include "src/Si4703/Si4703_Breakout.h"

// always use unsigned long for duration comparisions, https://arduino.stackexchange.com/a/12588
unsigned long MIN_DELAY_BETWEEN_RADIO_UPDATES = 5;
unsigned long lastRadioUpdate = 0;
unsigned long lastChannelChange = 0;

Si4703_Breakout radio(RADIO_RESET_PIN, RADIO_SDIO, RADIO_SCLK);
// U.S. FM Broadcast is 87.9—107.9 (101 Stations)
// we have a few extra ticks so we'll do 86.5—107.9 (108 Stations)
int previousVolume = volume;
char rdsBuffer[10];

int previousChannel = channel;

/* Only send updates to the radio board
    if volume or channel has actually changed
*/
void updateRadio() {
  if (millis() - lastRadioUpdate < MIN_DELAY_BETWEEN_RADIO_UPDATES) {
    return;
  }
  lastRadioUpdate = millis();

  if (channel != previousChannel) {
    lastChannelChange = millis();
    previousChannel = channel;
    radio.setChannel(channel);
  }
  if (volume != previousVolume) {
    previousVolume = volume;
    radio.setVolume(volume);
  }
}

void radioSetup() {
  radio.powerOn();
  radio.setVolume(volume);
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
  // Use Microbit buttons channel up and down
  if (!digitalRead(buttonA)) {
    channel = channel - 2;
    if (channel < MINFREQ) {
      channel = MAXFREQ;
    }
  }
  if (!digitalRead(buttonB)) {
    channel = channel + 2;
    if (channel > MAXFREQ) {
      channel = MINFREQ;
    }
  }
}
