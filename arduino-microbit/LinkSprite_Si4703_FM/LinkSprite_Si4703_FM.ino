// Works with Microbit, LinkSprite Si4703 FM Radio, Line Level Convertor BSS138
//
// Line Level Convertor BSS138
// https://www.adafruit.com/product/757
// Hook up low side to microbit
// Power high side off of 5V power supply
// Run high side line levels to FM board
// Power FM board off of 3V microbit
//
// LinkSprite Si4703 FM Radio Board:
// http://store.linksprite.com/breakout-board-for-si4703-fm-tuner/
// http://linksprite.com/wiki/index.php?title=Main_Page
// although the Si4703 chip is a 3.3V chip, the LinkSprite board needs to be 
// powered at 3.3V, but expects 5V line levels for control
// 
// Getting Started:
// 1) Follow Microbit Arduino startup instructions: https://learn.adafruit.com/use-micro-bit-with-arduino/overview
//    Install Si4703_FMRadio library, https://github.com/whiteneon/Si4703_FMRadio
//    Install Adafruit GFX Library
//    Install BLE Peripheral Library
//    Install Adafruit_Microbit Library https://github.com/adafruit/Adafruit_Microbit/archive/master.zip
//    
//    
// 3) Wire up board pins: SDIO, SCLK, Reset, Ground, 3.3V
// 2) Upload code to Arduino
//
#include <Si4703_Breakout.h>
#include <Adafruit_Microbit.h>
#include <Wire.h>

int resetPin = 16;
//int SDIO = A4;
//int SCLK = A5;

int SDIO = SDA; //SDA/A4 on Arduino
int SCLK = SCL; //SCL/A5 on Arduino

Si4703_Breakout radio(resetPin, SDIO, SCLK);
int channel = 881;
int volume = 5;
char rdsBuffer[10];
Adafruit_Microbit_Matrix microbit;

void setup()
{
  Serial.begin(57600);
  Serial.println("\n\nSi4703_Breakout Test Sketch");
  Serial.println("===========================");  
  Serial.println("a b     Favourite stations");
  Serial.println("+ -     Volume (max 15)");
  Serial.println("u d     Seek up / down");
  Serial.println("r       Listen for RDS Data (15 sec timeout)");
  Serial.println("Send me a command letter.");
  
  microbit.begin();

  radio.powerOn();
//  radio.setVolume(0);

  radio.setVolume(volume);
  radio.setChannel(channel);
}

void loop() {
  microbit.print(channel);
  
  if (Serial.available())
  {
    char ch = Serial.read();
    if (ch == 'u') 
    {
      channel = radio.seekUp();
      displayInfo();
    } 
    else if (ch == 'd') 
    {
      channel = radio.seekDown();
      displayInfo();
    } 
    else if (ch == '+') 
    {
      volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == '-') 
    {
      volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
    } 
    else if (ch == 'a')
    {
      channel = 930; // Rock FM
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'b')
    {
      channel = 974; // BBC R4
      radio.setChannel(channel);
      displayInfo();
    }
    else if (ch == 'r')
    {
      Serial.println("RDS listening");
      radio.readRDS(rdsBuffer, 15000);
      Serial.print("RDS heard:");
      Serial.println(rdsBuffer);      
    }
  }
}

void displayInfo()
{
   Serial.print("Channel:"); Serial.print(channel); 
   Serial.print(" Volume:"); Serial.println(volume); 
}
