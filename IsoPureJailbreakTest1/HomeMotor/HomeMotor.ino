#include <AccelStepper.h>

#define DRIVER 1

// Motor pins
const int stepPin = 3;
const int dirPin = 2;

// Light switch pin
const int sensorPin = 7;

AccelStepper stepper(DRIVER, stepPin, dirPin);

bool homed = false;

void setup() {
  Serial.begin(9600);

  pinMode(sensorPin, INPUT_PULLUP);  //10Kohm pullup resistor from output of sensor to 5V

  stepper.setMaxSpeed(800);
  stepper.setAcceleration(9999);

  Serial.println("Starting homing...");

  if (!homed) { // home the motor
    homeMotor();
    homed = true;

    Serial.println("Homing complete!");
    delay(1000);
  }
  notHome();//one revolution
}
void oneRev() {
  stepper.setMaxSpeed(600);
  stepper.setAcceleration(9999);

  stepper.moveTo(200);  //200 steps away from home direction should be 1 revolution.
  while (stepper.distanceToGo() != 0) { 
    stepper.run();
  }
  stepper.stop(); //hault
}
void notHome(){
  stepper.setMaxSpeed(600);
  stepper.setAcceleration(9999);

  stepper.moveTo(376);  //376 away from home is the end of the rail
  while (stepper.distanceToGo() != 0) { 
    stepper.run();
  }
  stepper.stop(); //hault
}

void homeMotor() {
  //       STEP 1: Move toward switch 
  stepper.setMaxSpeed(600);
  stepper.setAcceleration(9999);

  stepper.moveTo(-100000);  //negative is home direction for horizontal
  //negative is the down direction for the combs

  while (digitalRead(sensorPin) == HIGH) {  //high because it outputs a 1 when the sensor is not tripped
    stepper.run();
  }

  Serial.println("Switch hit!");
  //     STEP 2: Stop immediately 
  stepper.stop();
  while (stepper.isRunning()) {
    stepper.run();
  }

  delay(200);  // debounce

  // -------- STEP 3: Back off switch --------
  stepper.moveTo(stepper.currentPosition() + 3);  //nudge away
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  delay(200);


  Serial.println("Precise home reached");
  stepper.stop();                 // ult
  stepper.setCurrentPosition(0);  //now the system knows where to 0 the Horizontal axis
}


void loop() {
  ;
}