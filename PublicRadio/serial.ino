void serialControlSetup() {
  Serial.begin(57600);
  Serial.println("\n\nSi4703_Breakout Test Sketch");
  Serial.println("===========================");
  Serial.println("a b     Favourite stations");
  Serial.println("+ -     Volume (max 15)");
  Serial.println("u d     Seek up / down");
  Serial.println("r       Listen for RDS Data (15 sec timeout)");
  Serial.println("Send me a command letter.");
}

void serialControlInLoop() {
  if (Serial.available()) {
    char ch = Serial.read();
    if (ch == 'u') {
      seekUp();
    } else if (ch == 'd') {
      seekDown();
    } else if (ch == '+') {
      volume ++;
      if (volume == 16) volume = 15;
      radio.setVolume(volume);
      displayInfo();
    } else if (ch == '-') {
      volume --;
      if (volume < 0) volume = 0;
      radio.setVolume(volume);
      displayInfo();
    } else if (ch == 'a') {
      channel = 930; // Rock FM
      radio.setChannel(channel);
      displayInfo();
    } else if (ch == 'b') {
      channel = 974; // BBC R4
      radio.setChannel(channel);
      displayInfo();
    } else if (ch == 'r') {
      Serial.println("RDS listening");
      radio.readRDS(rdsBuffer, 15000);
      Serial.print("RDS heard:");
      Serial.println(rdsBuffer);
    }
  }
}
