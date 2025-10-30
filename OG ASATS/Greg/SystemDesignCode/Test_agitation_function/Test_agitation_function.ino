#include <AccelStepper.h>

#define DELAY 10
#define BIG_DELAY 30
#define STEPS_PER_REV 200          // Change this if your motor has a different number of steps per revolution
#define ROTATIONS_BEFORE_CHANGE 3  // Change direction every 3 rotations

const int agitation_pin = 19;
const int agitation_dir = 18;
//THIS IS FOR AGITATION WITH ALL THREE HOOKED UP BUT ONLY TESTING AGITAION
AccelStepper stepper(AccelStepper::DRIVER, agitation_pin, agitation_dir);
void setup() {
  Serial.begin(115200);
  stepper.enableOutputs();
  stepper.setMaxSpeed(20000);
  stepper.setAcceleration(100000);
  stepper.setSpeed(20000);
  //set dir pin
  //   pinMode(agitation_dir, OUTPUT);  //vertical motor dir pin is a digital gpio
  //   digitalWrite(agitation_dir, 1);  //0 is up

  //   //step
  //   ledcAttachChannel(agitation_pin, 5000, 8, 8);
  //   ledcWrite(agitation_pin, 0);

  while (1) {
    static long stepsTaken = 0;
    static int direction = 1;  // 1 for forward, -1 for reverse

    stepper.runSpeed();
    // stepsTaken++;

    // if (stepsTaken >= STEPS_PER_REV * ROTATIONS_BEFORE_CHANGE) {
    //   direction *= -1;
    //   stepper.setSpeed(9000 * direction);
    //   stepsTaken = 0;
    // }
  }
}


// uint8_t accelerate(int motor,uint8_t start,uint8_t finish,uint8_t speed){
// //speed in steps/sec^2
// }


void loop() {
  //   // //motor starts off and going downward (1)

  //   ledcChangeFrequency(agitation_pin, 5000, 8);  //start slow
  //   ledcWrite(12, 128);               //turn on motor
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 5200, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 5400, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 5600, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 5800, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6000, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6200, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6400, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6600, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6800, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7000, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7200, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7400, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7600, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7800, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8000, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8200, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8400, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8600, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8800, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 9000, 8);  //start slow
  //   delay(BIG_DELAY + 5);
  // //turn motor off
  //   ledcWrite(agitation_pin, 0);     //stop
  //   digitalWrite(agitation_dir, 0);  //prepare direction switch (1)
  //   delay(10);
  // //turn motor on and go up
  //   ledcChangeFrequency(agitation_pin, 5000, 8);  //start slow
  //   ledcWrite(12, 128);               //turn on motor
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 5200, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 5400, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 5600, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 5800, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6000, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6200, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6400, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6600, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 6800, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7000, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7200, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7400, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7600, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 7800, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8000, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8200, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8400, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8600, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 8800, 8);  //start slow
  //   delay(DELAY);
  //   ledcChangeFrequency(agitation_pin, 9000, 8);  //start slow
  //   delay(BIG_DELAY);

  //   //turn motor off and prep switch directions (0)
  //   ledcWrite(agitation_pin, 1);
  //   digitalWrite(agitation_dir, 1);
  //   delay(10);
}


//hello
//16mm traveled down, 100hz to 1.3khz with 10ms delays. 300ms full speed delay.