///
/// \file  TestTEA5767.ino
/// \brief An Arduino sketch to operate a TEA5767 chip based radio using the Radio library.
///
/// \author Matthias Hertel, http://www.mathertel.de
/// \copyright Copyright (c) 2014 by Matthias Hertel.\n
/// This work is licensed under a BSD style license.\n
/// See http://www.mathertel.de/License.aspx
///
/// \details
/// This sketch implements a "as simple as possible" radio without any possibility to modify the settings after initializing the chip.\n
/// The radio chip is initialized and setup to a fixed band and frequency. These settings can be changed by modifying the 
/// FIX_BAND and FIX_STATION definitions. 
///
/// Open the Serial console with 57600 baud to see the current radio information.
///
/// Wiring
/// ------ 
/// Arduino port | Si4703 signal
/// ------------ | ---------------
///     3.3V | VCC
///      GND | GND
///       A5 | SCLK
///       A4 | SDIO
///       D2 | RST
///
/// More documentation and source code is available at http://www.mathertel.de/Arduino
///
/// History:
/// --------
/// * 15.09.2014 created.

#include <Arduino.h>
#include <Wire.h>
#include <radio.h>
#include <TEA5767.h>

/// The band that will be tuned by this sketch is FM.
#define FIX_BAND RADIO_BAND_FM

/// The station that will be tuned by this sketch is 90.30 MHz.
#define FIX_STATION 9030

RADIO_FREQ station0 = 8810;
RADIO_FREQ station1 = 8890;
RADIO_FREQ station2 = 8970;
RADIO_FREQ station3 = 9030;
RADIO_FREQ station4 = 9090;
RADIO_FREQ station5 = 9150;
RADIO_FREQ station6 = 9450;
RADIO_FREQ station = station0;
int index = 1;
TEA5767 radio;    // Create an instance of Class for Si4703 Chip

uint8_t vol = 3;
uint8_t test1;
byte test2;

/// Setup a FM only radio configuration
/// with some debugging on the Serial port
void setup() {
  // open the Serial port
  Serial.begin(57600);
  Serial.println("Radio...");
  delay(200);

  // Initialize the Radio 
  radio.init();

  // Enable information to the Serial port
  radio.debugEnable();

  // HERE: adjust the frequency to a local sender
//  radio.setBandFrequency(FIX_BAND, station);
  radio.setBandFrequency(FIX_BAND, station);
  radio.setVolume(vol);
  radio.setMono(false);
} // setup


/// show the current chip data every 3 seconds.
void loop() {
  char s[12];
  radio.formatFrequency(s, sizeof(s));
  Serial.print("Station:"); 
  Serial.println(s);
  
  Serial.print("Radio:"); 
  radio.debugRadioInfo();
  
  Serial.print("Audio:"); 
  radio.debugAudioInfo();

  delay(3000);

  index = (index + 1) % 7;
  if(index == 0){
    station = station0;
  }else if(index == 1){
    station = station1;
  }else if(index == 2){
    station = station2;
  }else if(index == 3){
    station = station3;
  }else if(index == 4){
    station = station4;
  }else if(index == 5){
    station = station5;
  }else if(index == 6){
    station = station6;
  }
  
  radio.setBandFrequency(FIX_BAND, station);
  
} // loop

// End.

