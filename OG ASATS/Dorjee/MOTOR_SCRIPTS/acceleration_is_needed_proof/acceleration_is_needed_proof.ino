#define DELAY 10
#define BIG_DELAY 30
//THIS IS FOR AGITATION WITH ALL THREE HOOKED UP BUT ONLY TESTING AGITAION

void setup() {
  //set dir pin
  pinMode(13, OUTPUT);  //vertical motor dir pin is a digital gpio
  digitalWrite(13, 1);  //0 is up

  //step
  ledcAttachChannel(12, 100, 8, 8);
  ledcWrite(12, 0);

  //   //turn motor on
  // ledcChangeFrequency(12, 1000, 8);  //start slow
  // ledcWrite(12, 128);
  // delay(BIG_DELAY);

  // //turn motor off and prep switch directions (0)
  // ledcWrite(12, 0);
  // digitalWrite(13, 0);
  // delay(10);
}

void loop() {
  //motor starts off and going downward (1)

  ledcChangeFrequency(12, 3600, 8);  //start slow
  ledcWrite(12, 128);               //turn on motor
  delay(BIG_DELAY + 5);

  //turn motor off
  ledcWrite(12, 0);     //stop
  digitalWrite(13, 1);  //prepare direction switch (1)
  delay(10);

  //turn motor on and go up
  ledcChangeFrequency(12, 3600, 8);  //start slow
  ledcWrite(12, 128);               //turn on motor
  delay(BIG_DELAY);

  //turn motor off and prep switch directions (0)
  ledcWrite(12, 1);
  digitalWrite(13, 0);
  delay(10);
}


//hello
//16mm traveled down, 100hz to 1.3khz with 10ms delays. 300ms full speed delay.