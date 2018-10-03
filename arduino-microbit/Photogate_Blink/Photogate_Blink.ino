#include <Adafruit_Microbit.h>
Adafruit_Microbit_Matrix microbit;

void setup() {
  microbit.begin();

  pinMode(1, INPUT);
  pinMode(2, INPUT);
}

void loop() {
  /* If pin1 is low we have obstructed the beam
      so flash a line of LEDs
  */
  if (digitalRead(1) == LOW) {
    microbit.drawLine(0, 0, 4, 0, LED_ON);
  } else {
    microbit.drawLine(0, 0, 4, 0, LED_OFF);
  }

  /* If pin1 is low we have obstructed the beam
    so flash a line of LEDs
  */
  if (digitalRead(2) == LOW) {
    microbit.drawLine(0, 1, 4, 1, LED_ON);
  } else {
    microbit.drawLine(0, 1, 4, 1, LED_OFF);
  }

}
