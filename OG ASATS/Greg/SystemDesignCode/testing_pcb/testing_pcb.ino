#include <AccelStepper.h>

#define DELAY 10
#define BIG_DELAY 30
#define STEPS_PER_REV 200          // Change this if your motor has a different number of steps per revolution
#define ROTATIONS_BEFORE_CHANGE 3  // Change direction every 3 rotations

const int agitation_pin = 23;
const int agitation_dir = 22;
//THIS IS FOR AGITATION WITH ALL THREE HOOKED UP BUT ONLY TESTING AGITAION
AccelStepper stepper(AccelStepper::DRIVER, agitation_pin, agitation_dir);
void setup() {
  Serial.begin(115200);
  stepper.enableOutputs();
  stepper.setMaxSpeed(11000);
  //stepper.setAcceleration(100000);

  ledcAttachChannel(agitation_pin,1000,8,4);
  digitalWrite(agitation_dir, HIGH);
   ledcWriteTone(agitation_pin,11000);

  //set dir pin
  //   pinMode(agitation_dir, OUTPUT);  //vertical motor dir pin is a digital gpio
  //   digitalWrite(agitation_dir, 1);  //0 is up

  agitateMotors(1000,60,5,70);
}
/**
 * @brief: perform the agitation operation of moving motors up and down very quickly
 * @param agitateSpeed: agitation speed on a scale of 1-9
 * @param agitateDuration: agitation duration given in seconds
 * @param percentDepth: percentage of well volume to be displaced
 * @retval: none
 */
void agitateMotors(uint16_t agitateSpeed, uint8_t agitateDuration, uint8_t totalVolume, uint8_t percentDepth)
{
  //homeAgitation();
  // set motor speed
  //int agitationFrequency = mapSpeed(agitateSpeed); // newmap
 
  //int toggleDelay = 1000 / (depth);                // milliseconds per half-cycle (back and forth)

  // setup for changing directions
  bool DIR = HIGH;                      // init direction
  unsigned long startTime = millis();   // capture a start time
  pinMode(agitation_dir, OUTPUT); // ensure pin is set to output

  while (millis() - startTime < (agitateDuration * 1000))
  {
    delay(1);//delay before next iteration
    ledcWriteTone(agitation_pin, agitateSpeed); // Drive motor
    delay(300);  //controls depth of penetration
    ledcWriteTone(agitation_pin, 0); // stop motor
    digitalWrite(agitation_dir, DIR); // set new direction
                        
    DIR = (DIR == HIGH) ? LOW : HIGH;       // change directions after every toggleDelay iteration
    
  }
  // turn motors off afrer delay is finished
  ledcWriteTone(agitation_pin, 0);
}


/**
 * @brief: take in a distance and move the gantry head that amount
 * note this particular motor driver is quarter microsteped.
 * @param distance
 * @retval: none
 */
void moveMotor(int MOTOR_STEP, int MOTOR_DIR, int DIR, uint16_t speed, uint16_t distance)
{ // 1 step is 1.8 degrees
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint16_t steps = distanceToSteps(distance);
  uint16_t stepFrequency = mapSpeed(speed);          // adjust to control speed (Hz)
  float secondsToRun = (float)steps / stepFrequency; // time = steps / freq
  uint32_t durationMs = (uint32_t)(secondsToRun * 1000);

  digitalWrite(MOTOR_DIR, DIR); 
  ledcWriteTone(MOTOR_STEP,11000);            // set direction
  //accellerateMotor(MOTOR_STEP, stepFrequency,200); // start step pulses

  delay(durationMs); // run long enough to cover desired steps

  ledcWriteTone(MOTOR_STEP, 0); // stop step pulses
}
//function to accelerate motor with ledc
void accellerateMotor(int MOTOR_STEP,uint16_t finalFrequency,uint16_t accelSpeed){
  //increase frequency on step pin until you hit target
  for(int i = 0; i<finalFrequency;i = i+1000){

    ledcChangeFrequency(MOTOR_STEP,1000,8);
    delayMicroseconds(accelSpeed);//the delay is related to how quickly you want to accellerate
  
  }

}


// angle (degrees) = (arc length / radius) * (180 / π)
uint16_t distanceToSteps(uint16_t distance)
{ // 200 steps = 1 revolution

  return distance * 200 * 4;
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeed(float value)
{
  return (value - 1) * (11000 - 400) / (9 - 1) + 400;
}
// maping ranges of depths to a fequency for direction channel
unsigned int mapDepth(float value1, float value2)
{
  // Map value1 (1-6) directly to 5-20
  float mapped1 = ((value1 - 1) * (20 - 5) / (6 - 1)) + 5;

  // Map value2 (1-100) directly to 5-20
  float mapped2 = ((value2 - 1) * (20 - 5) / (100 - 1)) + 5;

  // Weighted blend (change weights if needed)
  float finalValue = (mapped1 * 0.6) + (mapped2 * 0.4);

  // Clamp to 5-20
  if (finalValue < 5)
    finalValue = 5;
  if (finalValue > 20)
    finalValue = 20;

  return (unsigned int)finalValue;
}

void loop() {
}