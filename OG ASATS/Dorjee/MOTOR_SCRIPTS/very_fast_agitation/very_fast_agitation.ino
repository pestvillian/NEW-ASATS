const int STEP = 23;
const int DIR = 22;
const int CHANNEL = 3;

#define ACCEL_STEPS 50

/******* HOW TO USE ******
* Frequency Sweep across given range. 

*/

void setup() {
  Serial.begin(115200);

  //direction
  pinMode(DIR, OUTPUT);
  digitalWrite(DIR, 1);

  //step
  ledcAttachChannel(STEP, 12000, 8, CHANNEL);
  ledcWrite(STEP, 0);
  //delay(2000);

  // ledcChangeFrequency(12, 1000, 8);
  // delay(2000);

  // ledcChangeFrequency(12, 1500, 8);
  // delay(2000);

  //start from 2khz, increment by 500hz each time with 2 sec delay
  // for (int i = 0; i < 24; i++) {
  //     ledcChangeFrequency(12, 1000 + i * 1000, 8);
  //     delay(1);
  // }

  /*NOTES
  12khz too high on old motor. old motor worked at 3khz, turning pretty average speed
  never do 250hz
  */
}

void loop() {
  ledcChangeFrequency(STEP, 1000, 8);
  ledcWrite(STEP, 128);
  for (int i = 1; i <= 49; i++) {
    ledcChangeFrequency(STEP, 1000 + 1000 * i, 8);
    delay(1);
  }
  delay(50);
  for (int i = 49; i >= 1; i--) {
    ledcChangeFrequency(STEP, 1000 + 1000 * i, 8);
    delay(1);
  }
  ledcWrite(STEP, 0);
  digitalWrite(DIR, !digitalRead(DIR));
}

//1.) smps to increase current
//2.) why isnt potentiometer working?
//3.)
