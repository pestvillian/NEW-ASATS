/************
* 5/22/25
* Gregory Ziegler + Dorjee Tenzing
* Overview:
* This is the firmware for the ASATS Motor Controller Board. 
* The goal of this firmware is to automate the blood testing procedures for the ASATS device.

* This program has a state machine with describing two main types of behavior:
* - Receiving protocol instructions from an STM32 microcontroller's touchscreen through UART
* - Parsing the UART data and to perform either agitation or magnetization
*************/

/*************** library inclusions and macro definitions***************/
#include <HardwareSerial.h>
#include <ezButton.h>
#include <AccelStepper.h>

HardwareSerial MySerial(0);  // Use UART0

#define MAX_LINE_LENGTH 32
#define MAX_LINES 100
#define PWM_RESOLUTION 8
#define LIMIT_SWITCH_DEBOUNCE_TIME 50

/******************* PIN DEFINITIONS **************/

//agitation motor
const int AGITATION_MOTOR_STEP = 19;
const int AGITATION_MOTOR_DIR = 18;
const int AGITATION_MOTOR_STEP_CHANNEL = 2;

//horizontal motor
const int MOTOR_X_STEP = 5;
const int MOTOR_X_DIR = 4;
const int MOTOR_X_STEP_CHANNEL = 4;

//vertical motor
const int MOTOR_Y_STEP = 2;  //
const int MOTOR_Y_DIR = 3;   //
const int MOTOR_Y_STEP_CHANNEL = 3;

//limit switches
const int AGITATION_SWITCH = 15;
const int VERTICAL_SWITCH = 6;
const int HORIZONTAL_SWITCH = 7;
ezButton limitSwitchY(VERTICAL_SWITCH);           // create ezButton object that attach to ESP32 pin GPIO6
ezButton limitSwitchX(HORIZONTAL_SWITCH);         // create ezButton object that attach to ESP32 pin GPIO7
ezButton limitSwitchAgitation(AGITATION_SWITCH);  // create ezButton object that attach to ESP32 pin GPIO15

//motor enable
const int MOTOR_ENABLE = 22;

//accelstepper instantiation
AccelStepper stepper(AccelStepper::DRIVER, AGITATION_MOTOR_STEP, AGITATION_MOTOR_DIR);
AccelStepper stepperY(AccelStepper::DRIVER, MOTOR_Y_STEP, MOTOR_Y_DIR);

/******************* STATE MACHINE SETUP *****************/
typedef enum {
  waitingState,
  runState
} operationState;

operationState currentState = waitingState;

/****************** INITIALIZE FUNCTIONS **************/
uint8_t agitateMotors(uint16_t agitateSpeed, uint8_t agitateDuration, uint8_t mixPosition, uint8_t mixScope, uint8_t pauseDuration, uint8_t repeat);
uint8_t pauseMotors(uint8_t pauseDuration);
uint8_t moveSample(uint8_t initialSurfaceTime, uint8_t speed, uint8_t stopAtSequences, uint8_t sequencePauseTime);

/************** INITALIZE VARIABLES ****************/

char tempBuffer[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } };  // Initialize the tempBuffer with nulls ('\0')
uint8_t startFlag = 0;
uint8_t i = 0;
uint8_t j = 0;
uint8_t finishFlag = 0;


void setup() {
  /*********** Set up UART and GPIO ***********/

  //motor enable
  pinMode(MOTOR_ENABLE, OUTPUT);
  digitalWrite(MOTOR_ENABLE, 1);  //keep motors off until touchscreen enables them

  //hardware UART0
  Serial.begin(115200);
  MySerial.begin(115200);

  //limit switch debouncing
  limitSwitchAgitation.setDebounceTime(LIMIT_SWITCH_DEBOUNCE_TIME);  // set debounce time of limit switch to 50 milliseconds
  limitSwitchX.setDebounceTime(LIMIT_SWITCH_DEBOUNCE_TIME);
  limitSwitchY.setDebounceTime(LIMIT_SWITCH_DEBOUNCE_TIME);

  //limit switches are pulled down. when pressed, they connect to vcc
  pinMode(HORIZONTAL_SWITCH, INPUT_PULLDOWN);
  pinMode(VERTICAL_SWITCH, INPUT_PULLDOWN);
  pinMode(AGITATION_SWITCH, INPUT_PULLDOWN);

  //motor direction pins are digital outputs
  pinMode(AGITATION_MOTOR_DIR, OUTPUT);
  pinMode(MOTOR_Y_DIR, OUTPUT);
  pinMode(MOTOR_X_DIR, OUTPUT);

  //step signals
  ledcAttachChannel(AGITATION_MOTOR_STEP, 1000, PWM_RESOLUTION, AGITATION_MOTOR_STEP_CHANNEL);
  ledcAttachChannel(MOTOR_X_STEP, 1000, PWM_RESOLUTION, MOTOR_X_STEP_CHANNEL);
  ledcAttachChannel(MOTOR_Y_STEP, 1000, PWM_RESOLUTION, MOTOR_Y_STEP_CHANNEL);
}

/*
 *@brief: state machine is either waiting for uart data or moving motors.
 *@author: Dorjee Tenzing
*/
void loop() {
  //state machine to handle the machine operation
  switch (currentState) {
    case waitingState:
      // fill up tempBuffer with serial data
      if (MySerial.available()) {
        // Read the incoming byte
        char incomingByte = MySerial.read();
        tempBuffer[i][j] = incomingByte;
        //Serial.println(incomingByte);
        j++;
        //handle newline
        if (incomingByte == '\n') {
          //Serial.println("testing new line");
          tempBuffer[i][j] = '\0';
          i++;
          j = 0;
        }
        //handle tab operator
        if (incomingByte == '\t') {
          tempBuffer[i][j] = '\0';
          //for (int rep; rep < )
          i = 0;
          j = 0;
          //turn motors on and go to run state
          currentState = runState;
          digitalWrite(MOTOR_ENABLE, 0);
        }
      }
      break;
    case runState:
      //send the protocol name to the touchscreen so it can display the name
      // MySerial.write("T");
      // delay(20);
      // MySerial.println(tempBuffer[0]);


      //auto home first
      finishFlag = autoHome();


      //go line by line through tempBuffer and execute the protocol
      for (int a = 1; a < MAX_LINES; a++) {  //skip first line cuz it is the title
        //handle pause
        if (tempBuffer[a][0] == 'P') {
          //parse uart pause data
          uint8_t pauseDuration = (tempBuffer[a][1] - '0');  // Only duration for pausing

          //send uart data to tell stm32 which protocol is running
          MySerial.write("P");
          delay(20);
          MySerial.println(tempBuffer[a]);

          //perform the pause functon
          finishFlag = pauseMotors(pauseDuration);
        }

        //handle moving
        if (tempBuffer[a][0] == 'M') {
          //parse uart moving data
          uint8_t initialSurfaceTime = ((tempBuffer[a][1] - '0') * 100) + ((tempBuffer[a][2] - '0') * 10) + (tempBuffer[a][3] - '0');  // Initial surface time
          uint8_t speed = (tempBuffer[a][4] - '0');                                                                                    // Speed
          uint8_t stopAtSequences = (tempBuffer[a][5] - '0');                                                                          // Stop at sequences
          uint8_t sequencePauseTime = (tempBuffer[a][6] - '0');                                                                        // Sequence pause time

          //send uart data to tell stm32 which protocol is running
          MySerial.write("M");
          delay(20);
          MySerial.println(tempBuffer[a]);

          //perform the move function
          finishFlag = moveSample(initialSurfaceTime, speed, stopAtSequences, sequencePauseTime);
        }

        //handle agitation
        if (tempBuffer[a][0] == 'B') {
          //parse the agitation uart data
          uint16_t agitateSpeed = (tempBuffer[a][1] - '0');  //you take the index and subtract by null to get the actual number in the struct
          uint8_t agitateDuration = ((tempBuffer[a][2] - '0') * 10) + ((tempBuffer[a][3] - '0'));
          uint8_t totalVolume = ((tempBuffer[a][4] - '0') * 100) + ((tempBuffer[a][5] - '0') * 10) + ((tempBuffer[a][6] - '0'));
          uint8_t percentDepth = ((tempBuffer[a][7] - '0') * 100) + ((tempBuffer[a][8] - '0') * 10) + ((tempBuffer[a][9] - '0'));
          uint8_t pauseDuration = ((tempBuffer[a][10] - '0') * 10) + (tempBuffer[a][11] - '0');
          uint8_t repeat = ((tempBuffer[a][12] - '0') * 10) + (tempBuffer[a][13] - '0');

          //send uart data to tell stm32 which protocol is running
          MySerial.write("B");
          delay(20);
          MySerial.println(tempBuffer[a]);

          //perform the agitation function
          finishFlag = agitateMotors(agitateSpeed, agitateDuration, totalVolume, percentDepth, pauseDuration, repeat);
        }
      }

      //after protocol finishes, go back to waiting state
      currentState = waitingState;
      memset(tempBuffer, 0, sizeof(tempBuffer));
      //shitty solution to the issue of uart data being sent while motors move
      while (MySerial.available()) {
        MySerial.read();
      }

      //send uart data to tell stm32 protocol is finished
      if (finishFlag) {
        MySerial.write("D");
        digitalWrite(MOTOR_ENABLE, 1);
        finishFlag = 0;
      }
      break;
  }
}

/**
 * @brief: move agitaton motor up and down rapidly
 * @param agitateSpeed: speed of motor from 1-9
 * @param agitateDuration: duration of agitation up to 99
 * @param totalVolume: starting bottom location of the agitation. 100 is all the way down
 * @param percentDepth: how far up relative to the start bottom location of agitation. 100 is all the way back up.
 * @param pauseDuration: how long to pause inbetween agitation steps, up to 99
 * @param repeat: how many times the agitation is repated, up to 99
 * @retval: 1 if finished, 0 if stopped by touchscreen
 * @author: Code written by Gregory Ziegler, reviewed and commented by Dorjee Tenzing
 */
uint8_t agitateMotors(uint16_t agitateSpeed, uint8_t agitateDuration, uint8_t totalVolume, uint8_t percentDepth, uint8_t pauseDuration, uint8_t repeat) {

  //autohome and setup variables
  delay(200);
  homeAgitation();
  ledcDetach(AGITATION_MOTOR_STEP);  // Releases control from PWM
  stepper.enableOutputs();

  //some debugging stuff
  Serial.print("Current Position: ");
  Serial.println(stepper.currentPosition());
  if (stepper.currentPosition() != 0) {
    stepper.setCurrentPosition(0);
  }

  // Convert input speed to a usable frequnecy
  uint16_t agitationFrequency = mapSpeedAgitation(agitateSpeed);  // Frequency in steps/sec

  //convert input volume and depth into corresponding values in real life units of mm
  float depth_mm = 38.0 * (totalVolume / 100.0);                     //bottom of agitation relative to absolute zero
  float stroke_mm = depth_mm - (depth_mm * (percentDepth / 100.0));  //top of agitation relative to bottom

  //convert mm values to steps
  uint32_t init_depth = depth_mm / 0.00625;     //bottom of agitation relative to absolute zero in steps
  uint32_t stroke_steps = stroke_mm / 0.00625;  //top of agitation relative to bottom in steps


  // Setup stepper
  stepper.setMaxSpeed(20000);        // initial slow speed for positioning
  stepper.setAcceleration(3000000);  // Very aggressive acceleration

  // Define positions
  long top = stroke_steps;   //top of agitation is stroke steps
  long bottom = init_depth;  //bottom of agitation is init_depth
  bool movingDown = false;   //agitation starts by moving up


  // agitation starts by moving to the bottom location
  stepper.moveTo(bottom);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  //didnt we just do this?
  stepper.setMaxSpeed(agitationFrequency);  // High speed target
  stepper.setAcceleration(3000000);         // Very aggressive acceleration

  //send uart data for the stm32 to know we are starting agitation loop
  MySerial.write("B");
  delay(20);
  MySerial.write(agitateDuration);

  //begin agitation loop
  for (uint8_t rep = 0; rep < repeat; rep++) {
    unsigned long startTime = millis();  //get time
    movingDown = false;                  //always start agitation by going up
    stepper.moveTo(top);                 //First move up ????
    while (millis() - startTime < (agitateDuration * 1000)) {
      //run the motor until we hit top or bottom, then switch directions
      stepper.run();
      if (stepper.distanceToGo() == 0) {
        stepper.moveTo(movingDown ? top : bottom);
        movingDown = !movingDown;
      }

      //check for the touchscreen sending a stop signal
      if (checkStopMotorsMessage()) {
        ledcAttachChannel(AGITATION_MOTOR_STEP, 1000, PWM_RESOLUTION, AGITATION_MOTOR_STEP_CHANNEL);
        return 0;
      }
    }
    //pause motors after at top of test tube after agitation
    stepper.moveTo(top);
    while (stepper.distanceToGo() != 0) {
      stepper.run();
    }
    movingDown = true;  // why?
    pauseMotors(pauseDuration);
    //stepper.run();
  }

  stepper.stop();  //hault
  // Reattach PWM and rehome agitation motor
  ledcAttachChannel(AGITATION_MOTOR_STEP, 1000, PWM_RESOLUTION, AGITATION_MOTOR_STEP_CHANNEL);
  homeAgitation();
  return 1;
}

/**
  * from here, assume that the combs in question will be clamped with the magnets until the very end
 * @brief: maps agitation speed to frequency for motor step signal.
 * num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
 * @param value: speed value from 1-9
 * @retval: frequency for motor step signal
 * @author: Gregory Ziegler
 */

unsigned int mapSpeedAgitation(float value) {
  return (value - 1) * (55000 - 20000) / (9 - 1) + 20000;
}

/**
  * from here, assume that the combs in question will be clamped with the magnets until the very end
 * @brief:slowly moves the magnets into the sampletray and transports sample to next well"M742193"
 * @param   initialSurfaceTime first 3 numbers
 * @param speed fourth number
 * @param stopAtSequences fifth number
 * @param sequencePauseTime sixth number
 * @retval: 1 if finished, 0 if error
 * @author: Code written by Gregory Ziegler, reviewed and commented by Dorjee Tenzing
 */
uint8_t moveSample(uint8_t initialSurfaceTime, uint8_t speed, uint8_t stopAtSequences, uint8_t sequencePauseTime) {
  //fully init into the funciton
  delay(1000);
  if (checkStopMotorsMessage()) {
    return 0;
  }
  homeAgitation();  // home the agitation motor and wait
  delay(2000);
  if (checkStopMotorsMessage()) {
    return 0;
  }
  //we need to know at what height inside the wells to put the combs
  //not sure about this block yet
  // moveMotorY(HIGH, speed, 6);  // put combs at top of liquid
  // pauseMotors(initialSurfaceTime);
  //Binding sequence
  float range = 38;  //total hight of combs in mm
  if (stopAtSequences == 0) {
    stopAtSequences = 1;
  }
  float pos = range / stopAtSequences;  //number of times you wat to stop
  //there will be a range of distace the Y motor can travel there can be 9 sequences total.
  //we take the range and devide it by the number of sequences
  //the motor should be set to the very top of liquid here
  //incrementally lower the motor until we hit the final position

  for (int i = 1; i <= stopAtSequences; i++) {
    //move the motor sequentially down
    moveMotorY(HIGH, speed, pos * i);
    delay(sequencePauseTime * 1000);  //hold here for however long specified
    if (checkStopMotorsMessage()) {
      return 0;
    }
  }


  //moving sequence
  //moveMotorY(LOW, 1, 45);  //after the beads are magnatized, hove the body up slowly
  moveStepperY(0);  //replace line above
  delay(2000);      //delay to make sure the liquid stays
  if (checkStopMotorsMessage()) {
    return 0;
  }
  //above works time for step 2, move to the right and fill the next Well
  moveMotorX(HIGH, 1, 8.75);  //move to next Well
  delay(2000);                //wait for smoothness
  if (checkStopMotorsMessage()) {
    return 0;
  }

  //Dorjee: i have no idea what's going on below here. i know we need to shift wells, but idk why its going up and down so much

  //moveMotorY(HIGH, 1, 25);  //position just above the wells. greg ur highs and lows are backwards
  moveStepperY(20);  //replace line above

  //fill other well sequence
  moveMotorA(HIGH, 2, 16);  //greg how does this work. why does agitation go high and then low, shouldnt it be low and then high
  //moveMotorY(LOW, 1, 25);
  moveStepperY(0);  //replace line above
  delay(initialSurfaceTime);
  if (checkStopMotorsMessage()) {
    return 0;
  }
  //homeAgitation(); //i dont think we need this

  ledcAttachChannel(MOTOR_Y_STEP, 1000, PWM_RESOLUTION, AGITATION_MOTOR_STEP_CHANNEL);
  return 1;
}

void moveStepperY(float distance_mm) {
  uint32_t steps = distanceToStepsY(distance_mm);

  stepperY.moveTo(steps);
  while (stepperY.distanceToGo() != 0) {
    stepperY.run();
  }
}



/**
 * @brief:runs motor in home direction until a limit switch is hit
 * @param none
 * @retval: 1 if finished, 0 if error
 * @author: Gregory Ziegler
 */
uint8_t autoHome() {
  int motorSequence = 0;  //controls sequence of motors homing.
  //Serial.println("HOMEING!!!");
  delay(200);
  if (checkStopMotorsMessage()) {
    return 0;
  }
  moveMotorY(LOW, 4, 50);  // motor y to init position
  delay(200);              //allow Y motor to raise out of the way then allow the agitation motor to Home.
  if (checkStopMotorsMessage()) {
    return 0;
  }
  if (motorSequence == 0) {
    // Low is Up, High is DOwn
    //Serial.println("Homing Agitation");
    digitalWrite(AGITATION_MOTOR_DIR, LOW);      //LOW is the home DIrection
    ledcWriteTone(AGITATION_MOTOR_STEP, 10000);  // Drive motor
    // determine if it is pressed
    uint8_t stateA = digitalRead(AGITATION_SWITCH);
    //if the state of the limit switch is high that means it has been pressed
    while (1) {
      stateA = digitalRead(AGITATION_SWITCH);
      if (stateA != 1) {

        //turn off motor in this event
        digitalWrite(AGITATION_MOTOR_DIR, HIGH);
        ledcWriteTone(AGITATION_MOTOR_STEP, 0);  //
        moveMotorA(HIGH, 8, 2);
        motorSequence = 1;
        break;
      } else {
        //Serial.println("Limit switch is currently Untouched");
        continue;
      }
    }
  }
  if (checkStopMotorsMessage()) {
    return 0;
  }
  if (motorSequence == 1) {
    //Serial.println("Homing Y Axis");
    //Right is Low, High is Left
    ledcWriteTone(MOTOR_X_STEP, 800);  // Drive motor
    digitalWrite(MOTOR_X_DIR, LOW);    //home direction is left = LOW
    //if the state of the limit switch is high that means it has been pressed
    while (1) {
      //the motor should be running the whole time that the limit switch is untouched
      uint8_t stateX = digitalRead(HORIZONTAL_SWITCH);  //check status of limit switch
      // Serial.print("Limit switch X => ");
      // Serial.println(stateX);
      if (stateX != 1) {
        //Serial.println("Limit switch is currently Touched");

        //turn off motor in this event
        digitalWrite(MOTOR_X_DIR, HIGH);
        ledcWriteTone(MOTOR_X_STEP, 0);  //
        moveMotorX(HIGH, 2, 2);          //nudge motor to the right so it's not on the limit switch
        motorSequence = 2;
        break;
      }
    }
  }
  if (checkStopMotorsMessage()) {
    return 0;
  }
  if (motorSequence == 2) {
    //Serial.println("Homing X Axis");
    //Low is Up and High is Down
    ledcWriteTone(MOTOR_Y_STEP, 800);  // Drive motor
    digitalWrite(MOTOR_Y_DIR, HIGH);   //home direction is Down = LOW

    uint8_t stateY = digitalRead(VERTICAL_SWITCH);  //read status of limit switch

    //if the state of the limit switch is high that means it has been pressed
    while (1) {
      stateY = digitalRead(VERTICAL_SWITCH);  //read status of limit switch
      if (stateY != 1) {
        //Serial.println("Limit switch is currently Touched");
        //turn off motor in this event
        digitalWrite(MOTOR_Y_DIR, HIGH);
        ledcWriteTone(MOTOR_Y_STEP, 0);  //

        moveMotorY(LOW, 4, 3);  //nudge motor to the right so it's not on the limit switch

        break;
      } else {
        //Serial.println("Limit switch is currently Untouched");
        continue;
      }
    }
  }
  if (checkStopMotorsMessage()) {
    return 0;
  }



  //homing complete begin phase 2

  //move all three axis to there init positions

  moveMotorY(LOW, 4, 40);   // motor y to init position
  moveMotorX(HIGH, 5, 58);  //motor x to init position
                            //the nudge puts it in the right place to begin with
                            //moveMotorA(HIGH, 4, 2);  //move Agitation Head so that the plastic Combs are just above the test rack

  //Serial.println("Homeing Complete!!");
  //rest of the homing repositioning sequence goes here:)

  //accelstepperY initialization
  stepperY.setCurrentPosition(0);  // Set home position

  return 1;
}

/**
 * @brief: take in a distance and move the gantry head that amount
 * note this particular motor driver is quarter microsteped.
 * @param distance
 * @retval: none
 * @author: Gregory Ziegler
 */
void moveMotorA(int DIR, uint32_t speed, float distance) {  //
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsA(distance);
  uint16_t stepFrequency = mapSpeedA(speed);          // adjust to control speed (Hz)
  float secondsToRun = (float)steps / stepFrequency;  // time = steps / freq
  uint32_t durationMs = (uint32_t)(secondsToRun * 1000);

  digitalWrite(AGITATION_MOTOR_DIR, DIR);              // set direction
  ledcWriteTone(AGITATION_MOTOR_STEP, stepFrequency);  // start step pulses

  delay(durationMs);  // run long enough to cover desired steps

  ledcWriteTone(AGITATION_MOTOR_STEP, 0);  // stop step pulses
}
// angle (degrees) = (arc length / radius) * (180 / π)
uint32_t distanceToStepsA(float distance)  // about 8.75mm
{
  //return distance * 200 * 16; // one rotation
  return (uint32_t)(distance * (3200.0 / 20.0) + 0.5f);
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedA(float value) {
  return (value - 1) * (12000 - 5000) / (9 - 1) + 5000;
}

void homeAgitation() {
  int motorSequence = 0;
  delay(200);
  if (motorSequence == 0) {
    // Low is Up, High is DOwn
    digitalWrite(AGITATION_MOTOR_DIR, LOW);     //LOW is the home DIrection
    ledcWriteTone(AGITATION_MOTOR_STEP, 8000);  // Drive motor
    // determine if it is pressed
    uint8_t stateA = digitalRead(AGITATION_SWITCH);
    //if the state of the limit switch is high that means it has been pressed
    while (1) {
      stateA = digitalRead(AGITATION_SWITCH);
      // Serial.print("LIMIT SWITCH VALUE => ");
      // Serial.println(stateA);
      if (stateA != 1) {
        //Serial.println("Limit switch is currently Touched");
        //turn off motor in this event
        digitalWrite(AGITATION_MOTOR_DIR, HIGH);
        ledcWriteTone(AGITATION_MOTOR_STEP, 0);  //
        moveMotorA(HIGH, 8, 2);                  //nudge
        motorSequence = 1;
        break;
      } else {
        //Serial.println("Limit switch is currently Untouched");
        continue;
      }
    }
  }
  moveMotorA(HIGH, 2, 6);  //this should be the agitation Motor's 0 point.
}


/**
 * @brief: take in a distance and move the gantry head that amount
 * note this particular motor driver is quarter microsteped.
 * @param distance
 * @retval: none
 * @author: Gregory Ziegler
 */
void moveMotorY(int DIR, uint32_t speed, float distance) {  // 1 step is 1.8 degrees
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsY(distance);
  uint16_t stepFrequency = mapSpeedY(speed);  // adjust to control speed (Hz)

  //move the motor to the specified distance at the specified speed
  ledcDetach(MOTOR_Y_STEP);
  stepperY.setMaxSpeed(stepFrequency);
  //stepperY.setSpeed(stepFrequency);

  stepperY.moveTo(steps);
  while (stepperY.distanceToGo() != 0) {
    stepperY.run();
  }

  // float secondsToRun = (float)steps / stepFrequency;  // time = steps / freq
  // uint32_t durationMs = (uint32_t)(secondsToRun * 1000);

  // digitalWrite(MOTOR_Y_DIR, DIR);              // set direction
  // ledcWriteTone(MOTOR_Y_STEP, stepFrequency);  // start step pulses

  // delay(durationMs);  // run long enough to cover desired steps

  // ledcWriteTone(MOTOR_Y_STEP, 0);  // stop step pulses
}

// angle (degrees) = (arc length / radius) * (180 / π)
uint32_t distanceToStepsY(float distance)  // about 8.75mm
{
  //return distance * 200;
  return (uint32_t)(distance * (200.0 / 8.0) + 0.5f);
}

// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedY(float value) {
  return (value - 1) * (1000 - 400) / (9 - 1) + 400;
}

/**
 * @brief: take in a distance and move the gantry head that amount
 * note this particular motor driver is quarter microsteped.
 * @param distance
 * @retval: none
 * @author: Gregory Ziegler
 */
void moveMotorX(int DIR, uint32_t speed, float distance) {  // 1 step is 1.8 degrees
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsX(distance);
  uint16_t stepFrequency = mapSpeedX(speed);          // adjust to control speed (Hz)
  float secondsToRun = (float)steps / stepFrequency;  // time = steps / freq
  uint32_t durationMs = (uint32_t)(secondsToRun * 1000);


  // Serial.print("Frequency => ");
  // Serial.println(stepFrequency);
  // Serial.print("delay => ");
  // Serial.println(durationMs);

  digitalWrite(MOTOR_X_DIR, DIR);              // set direction
  ledcWriteTone(MOTOR_X_STEP, stepFrequency);  // start step pulses

  delay(durationMs);  // run long enough to cover desired steps

  ledcWriteTone(MOTOR_X_STEP, 0);  // stop step pulses
}

// angle (degrees) = (arc length / radius) * (180 / π)
uint32_t distanceToStepsX(float distance)  // about 8.75mm
{
  // 200 steps = 1 revolution
  //for returning # of steps per rotation use distance * 200
  //30mm per 1 revolution
  //1mm = 20/3

  //return distance * 200;
  //the number i devide 200 by is what i'm changing to get the best accuracy
  return (uint32_t)(distance * (200.0 / 31.24) + 0.5f);
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedX(float value) {
  return (value - 1) * (1000 - 400) / (9 - 1) + 400;
}


/**
 * @brief: Pause the motor for a number of seconds
 * @param pauseDuration: pause duration in seconds
 * @retval: 1 if finished, 0 if error
 * @author: Gregory Ziegler
 */
uint8_t pauseMotors(uint8_t pauseDuration) {
  //delay(100);
  //homeAgitation();  // home the agitator
  //ledcWriteTone(AGITATION_MOTOR_STEP, 0);
  stepper.stop();  //hault
  uint32_t curTime = millis();
  while ((millis() - curTime) < (pauseDuration * 1000)) {
    if (checkStopMotorsMessage()) {
      return 0;
    }
  }
  return 1;
}



// maping ranges of depths to a fequency for direction channel
unsigned int mapDepth(float value1, float value2) {
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

//return 1 if there is a message, else return 0
uint8_t checkStopMotorsMessage(void) {
  //if touchscreen sends a stop signal, abandon this protocol
  if (MySerial.available()) {
    char testByte = MySerial.read();
    if (testByte == 'S') {
      //turn motors off and go back to waiting state
      digitalWrite(MOTOR_ENABLE, 1);
      currentState = waitingState;
      memset(tempBuffer, 0, sizeof(tempBuffer));
      return 1;
    }
  }
  return 0;
}