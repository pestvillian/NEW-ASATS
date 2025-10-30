const int STEP = 12;
const int DIR = 13;
unsigned int prevTime = 0;
unsigned int curTime = 0;

/******* HOW TO USE ******
* A4988 lower frequency code for vertical and horizontal motors
* Testing conditions: 15V, 1khz for nominal speed. blk green, blue red are pairs for polulu 1200.
* dir high is down, ground should be up
*/

void setup() {
  Serial.begin(115200);

  //direction
  pinMode(14, OUTPUT);  //vertical motor dir pin is a digital gpio
  digitalWrite(13, 0);
  //step
  ledcAttachChannel(12, 100, 8, 8);
  ledcWrite(12, 128);
  delay(10);

  ledcChangeFrequency(12, 300, 8);
  delay(10);
  ledcChangeFrequency(12, 500, 8);
  delay(10);
  ledcChangeFrequency(12, 700, 8);
  delay(10);
  ledcChangeFrequency(12, 1000, 8);

  // delay(2000);

  // //start from 100hz for 10ms, increment by 500hz each time with 10ms sec interval
  // for (int i = 0; i < 6; i++) {
  //   ledcChangeFrequency(12, 5000 + i * 1000, 8);
  //   delay(10);
  // }
}

void loop() {
}
