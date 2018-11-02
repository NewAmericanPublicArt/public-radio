#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "RTClib.h"

// A simple data logger, logs RTC time
//
// Wiring with a Micro:bit
// after buying a datalogging board
// make sure to short 5V to IO pad (if connected) and solder 3V to IO pad (see Adafruit datalogging board documentation)
//
// Adafruit Datalogging Board Pin > Micro:bit Pin
// D10       > P10 (for Card Select)
// ICSP CLK  > P13 (for SD-card I/O)
// ICSP MISO > P14 (for SD-card I/O)
// ICSP MOSI > P15 (for SD-card I/O)
// SCL       > P19 (for RTC I/O)
// SDA       > P20 (for RTC I/O)
// IOr       > 3V  (for actually telling the board to run at 3V, dunno if this is required?)
// 3V        > 3V  (dunno if this is required?)
// 5V        > some 5V source (this powers the board, make this source is also grounded to microbit and board)
// Vin       > some 5V source (dunno if this is required?)
// Gnd       > Gnd (need to connect microbit, datalogging board and 5V power source grounds all together)
//                  we have digital communications, always connect the grounds together :)

// how many milliseconds between grabbing data and logging it. 1000 ms is once a second
#define LOG_INTERVAL  1000 // mills between entries (reduce to take more/faster data)

// how many milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 10000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port

RTC_DS1307 RTC; // define the Real Time Clock object

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// the logging file
File logfile;

void error(char *str) {
  Serial.print("error: ");
  Serial.println(str);
  while (1);
}

void setup(void) {
  SPI.beginTransaction(SPISettings(14000000, MSBFIRST, SPI_MODE0));

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  Serial.begin(9600);
  Serial.println();

  // Connect to RTC
  Wire.begin();
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL
  } else {
#if ECHO_TO_SERIAL
    Serial.println("RTC initialized.");
#endif  //ECHO_TO_SERIAL
  }

  // Initialize the SD card
  Serial.print("Initializing SD card...");

  // see if the card is present and can be initialized:
  //  if (!SD.begin(chipSelect)) {
  if (!SD.begin()) {
    error("Card failed, or not present");
  }
  Serial.println("card initialized.");

  // Create a new file
  //  char filename[10] = "000000.csv";
  char filename[10];
  //  = new char[10];
  for (uint8_t i = 0; i < 1000000; i++) {
    sprintf(filename, "%06d.csv", i);
    if (!SD.exists(filename)) {
      // only open a new file if it doesn't exist
      Serial.println(filename);
      logfile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }

  if (!logfile) {
    error("Couldnt create file");
  }

  Serial.print("Logging to: ");
  Serial.println(filename);

  logfile.println("millis, stamp, datetime, station, volume");
#if ECHO_TO_SERIAL
  Serial.println("millis, stamp, datetime, station, volume");
#endif //ECHO_TO_SERIAL
}

void loop(void) {
  DateTime now;

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL - 1) - (millis() % LOG_INTERVAL));

  //  digitalWrite(greenLEDpin, HIGH);

  // log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");
#if ECHO_TO_SERIAL
  Serial.print(m);         // milliseconds since start
  Serial.print(", ");
#endif

  // fetch the time
  now = RTC.now();
  // log time
  logfile.print(now.unixtime()); // seconds since 1/1/1970
  logfile.print(", ");
  logfile.print('"');
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print('"');
#if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // seconds since 1/1/1970
  Serial.print(", ");
  Serial.print('"');
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print('"');
#endif //ECHO_TO_SERIAL

  logfile.print(", 99.1, 12");
#if ECHO_TO_SERIAL
  Serial.print(", 99.1, 12");
#endif //ECHO_TO_SERIAL

  logfile.println("");
#if ECHO_TO_SERIAL
  Serial.println("");
#endif //ECHO_TO_SERIAL

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) > SYNC_INTERVAL) {
    syncTime = millis();
    logfile.flush();
  }
}
