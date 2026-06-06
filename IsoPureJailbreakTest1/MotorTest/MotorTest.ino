#include <AccelStepper.h>

// Define driver type (STEP + DIR)
#define DRIVER 1

// Pin definitions
const int stepPin = 3;
const int dirPin  = 2;


// Create stepper object
AccelStepper stepper(DRIVER, stepPin, dirPin);

long targetPosition = -200; //distance from current position

void setup() {
  Serial.begin(9600); 

  // Stepper setup
  stepper.setMaxSpeed(1000);     // steps per second
  stepper.setAcceleration(99999);  // steps per second^2

  stepper.moveTo(targetPosition);
}

void loop() {
  // Move motor
  stepper.run(); 

  // When target reached, reverse direction
  // if (stepper.distanceToGo() == 0) { 
  //   targetPosition = -targetPosition;
  //   stepper.moveTo(targetPosition);
  // }
}