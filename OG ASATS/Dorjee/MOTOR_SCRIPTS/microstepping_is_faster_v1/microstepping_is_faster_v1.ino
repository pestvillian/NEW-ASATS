#define DELAY 1
#define BIG_DELAY 100
//THIS IS FOR AGITATION WITH ALL THREE HOOKED UP BUT ONLY TESTING AGITAION

const int STEP = 12;
const int DIR = 13;

const int channel = 8;

void setup() {
  Serial.begin(115200);


  pinMode(17, OUTPUT);
  digitalWrite(17, 1);  //0 is up

  //set dir pin
  pinMode(DIR, OUTPUT);  //vertical motor dir pin is a digital gpio
  digitalWrite(DIR, 1);  //0 is up

  //step
  ledcAttachChannel(STEP, 1000, 8, channel);
  ledcWrite(STEP, 0);
}

int count = 0;

void loop() {
  //motor starts off and going downward (1)

  count++;
  Serial.println(count);
  ledcChangeFrequency(STEP, 1000, channel);  //start slow
  ledcWrite(STEP, 128);                     //turn on motor
  delay(DELAY);
  ledcChangeFrequency(STEP, 2000, channel);  //start slow
  delay(DELAY);
  ledcChangeFrequency(STEP, 3000, channel);  //start slow
  delay(DELAY);
  ledcChangeFrequency(STEP, 4000, channel);  //start slow
  delay(DELAY);
  ledcChangeFrequency(STEP, 5000, channel);  //start slow
  delay(DELAY);
  ledcChangeFrequency(STEP, 6000, channel);  //start slow
  delay(DELAY);
  ledcChangeFrequency(STEP, 7000, channel);  //start slow
  delay(DELAY);
  ledcChangeFrequency(STEP, 8000, channel);  //start slow
  delay(DELAY);
  ledcChangeFrequency(STEP, 9000, channel);  //start slow
  delay(DELAY);
  ledcChangeFrequency(STEP, 10000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 11000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 12000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 13000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 14000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 15000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 16000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 17000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 18000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 19000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 20000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 21000, channel);  //start slow
  // delay(DELAY);
  // ledcChangeFrequency(STEP, 22000, channel);  //start slow
  delay(BIG_DELAY + 5);

  //turn motor off
  ledcWrite(STEP, 0);                   //stop
  digitalWrite(DIR, !digitalRead(13));  //prepare direction switch (1)
  delay(10);
}


//hello
//16mm traveled down, 100hz to 1.3khz with 10ms delays. 300ms full speed delay.

//notes: 4/24/25. do we need microstepping?
//with no microstepping, the max frequency is 3.8khz