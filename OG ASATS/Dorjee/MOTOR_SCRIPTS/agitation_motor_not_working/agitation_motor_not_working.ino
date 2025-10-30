const int STEP = 12;
const int DIR = 13;
unsigned int prevTime = 0;
unsigned int curTime = 0;

/******* HOW TO USE ******
* Frequency Sweep across given range. 
*/

void setup() {
  Serial.begin(115200);

  //direction
  // pinMode(13, OUTPUT);              //vertical motor dir pin is a digital gpio
  // digitalWrite(13, 0);
  ledcAttachChannel(13, 4, 8, 12);
  ledcWrite(13, 128);
  delay(2000);

  //step
  ledcAttachChannel(12, 100, 8, 8);
  ledcWrite(12, 128);
  delay(100);

  // ledcChangeFrequency(12, 500, 8);
  // delay(100);

  // ledcChangeFrequency(12, 1000, 8);
  // delay(100);

  // ledcChangeFrequency(12, 1500, 8);
  // delay(100);

  // ledcChangeFrequency(12, 2000, 8);
  // delay(100);

  // ledcChangeFrequency(12, 2500, 8);
  // delay(2000);

  //start from 2khz, increment by 500hz each time with 2 sec delay
  // for (int i = 1; i <= 3; i++) {
  //   ledcChangeFrequency(12, 100 + i * 500, 8);
  //   delay(10);
  // }

  /*NOTES
  4hz is the lowest for dir
  */
}

void loop() {
  unsigned int curTime = millis();
  if ((curTime - prevTime) >= 250) {
    //begin 250ms period of movement

    //accelerate motor for 30ms
    for (int i = 0; i <= 3; i++) {
      ledcChangeFrequency(12, 100 + i * 500, 8);
      delay(10);
    }

    //190ms of max speed movement
    delay(190);

    //deaccelerate motor for last 30ms
    for (int i = 0; i <= 3; i++) {
      ledcChangeFrequency(12, 1600 - i * 500, 8);
      delay(10);
    }
    //switch direction happens after 250ms
    prevTime = curTime;
  }
}

//1.) smps to increase current
//2.) why isnt potentiometer working?
//3.)