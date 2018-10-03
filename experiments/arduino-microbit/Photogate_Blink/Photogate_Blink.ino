#include <Adafruit_Microbit.h>
Adafruit_Microbit_Matrix microbit;

const int PHOTOGATE1_PIN = 1;
const int PHOTOGATE2_PIN = 2;
float speed1 = 0;
float speed2 = 0;

void setup() {
  microbit.begin();

  pinMode(PHOTOGATE1_PIN, INPUT);
  pinMode(PHOTOGATE2_PIN, INPUT);
}

void loop() {
  /* If pin1 is low we have obstructed the beam
      so flash a line of LEDs
  */
  if (digitalRead(PHOTOGATE1_PIN) == LOW) {
    microbit.drawLine(0, 0, 4, 0, LED_ON);
  } else {
    microbit.drawLine(0, 0, 4, 0, LED_OFF);
  }

  /* If pin2 is low we have obstructed the beam
    so flash a line of LEDs
  */
  if (digitalRead(PHOTOGATE2_PIN) == LOW) {
    microbit.drawLine(0, 1, 4, 1, LED_ON);
  } else {
    microbit.drawLine(0, 1, 4, 1, LED_OFF);
  }

  measure();

  // turn on a scale of LEDs that increases with increasing speed
  int lastLEDIndex = 0;
  if (speed1 == 0) {
    microbit.drawLine(0, 2, 4, 2, LED_OFF);
  } else {
    lastLEDIndex = constrain(round(speed1 * 20), 0, 4);
    if (lastLEDIndex + 1 < 4) {
      microbit.drawLine(lastLEDIndex + 1, 2, 4, 2, LED_OFF);
    }
    if (lastLEDIndex >= 0) {
      microbit.drawLine(0, 2, lastLEDIndex, 2, LED_ON);
    }
  }

  if (speed2 == 0) {
    microbit.drawLine(0, 3, 4, 3, LED_OFF);
  } else {
    lastLEDIndex = constrain(round(speed2 * 20), 0, 4);
    if (lastLEDIndex + 1 < 4) {
      microbit.drawLine(lastLEDIndex + 1, 3, 4, 3, LED_OFF);
    }
    if (lastLEDIndex >= 0) {
      microbit.drawLine(0, 3, lastLEDIndex, 3, LED_ON);
    }
  }
}
