#include <AccelStepper.h>
#include <ezButton.h>
// types of protocolInstructions
enum ProtocolType {
  AGITATION,  // 'B'
  PAUSING,    // 'P'
  MOVING,     // 'M'
  INVALID     // For unknown protocol types
};
// Structure to store protocol properties
struct Protocol {
  ProtocolType type;
  uint8_t volume;
  uint8_t percentVolume;
  uint8_t speed;
  uint8_t duration;
  uint8_t initialSurfaceTime;
  uint8_t stopAtSequences;
  uint8_t sequencePauseTime;
};

// Protocol Array
char *protocolInstructions[] = {
  "M827395", "B491762", "P3", "M615829", "B728341", "P7", "B983251", "M147693", "P5", "M283741",
  "B672849", "P2", "M518392", "B739251", "P9", "M924873", "B356721", "P4", "M837462", "B671285",
  "P6", "B924173", "M462839", "P8", "M183627", "B519284", "P1", "B832749", "M746218", "P7",
  "M382615", "B721493", "P5", "B629471", "M548293", "P9", "M192847", "B364927", "P2", "M847362",
  "B512874", "P6", "B937415", "M284759", "P4", "M682391", "B715428", "P3", "B492671", "M531827",
  "P8", "M914275", "B267843", "P5", "B839472", "M362491", "P1", "M472681", "B619427", "P9",
  "B738429", "M294783", "P6", "M483729", "B175392", "P3", "B926471", "M571839", "P7", "M318472",
  "B827149", "P2", "B571382", "M649273", "P8", "M417629", "B385291", "P4", "B927416", "M518734",
  "P6", "M742193", "B618492", "P1", "B574829", "M392847", "P9", "M673284", "B941752", "P5",
  "B738192", "M512679", "P7", "M294678", "B415926", "P3", "B689472", "M471382", "P2", "M527491"
};

// global size of protocolInstructions
int size = sizeof(protocolInstructions) / sizeof(protocolInstructions[0]);  // Get number of elements



// Motor pins
const int MOTOR_Y_STEP = 19;          //19
const int MOTOR_Y_DIR = 18;           //18
const int MOTOR_X_STEP = 21;          //21
const int MOTOR_X_DIR = 20;           //20
const int AGITATION_MOTOR_STEP = 23;  // 23
const int AGITATION_MOTOR_DIR = 22;   //22

// Limit switch pins
const int LIMIT_Y_PIN = 2;
const int LIMIT_X_PIN = 3;
const int LIMIT_A_PIN = 4;

// Motor objects
AccelStepper MOTORY(AccelStepper::DRIVER, MOTOR_Y_STEP, MOTOR_Y_DIR);
AccelStepper MOTORX(AccelStepper::DRIVER, MOTOR_X_STEP, MOTOR_X_DIR);
AccelStepper MOTOR_A(AccelStepper::DRIVER, AGITATION_MOTOR_STEP, AGITATION_MOTOR_DIR);

const int stepsPerRevolution = 800;

// ezButton objects
ezButton limitSwitchY(LIMIT_Y_PIN);
ezButton limitSwitchX(LIMIT_X_PIN);
ezButton limitSwitchAgitation(LIMIT_A_PIN);

void setup() {
  //configure limit switches
  limitSwitchAgitation.setDebounceTime(50);  // set debounce time of limit switch to 50 milliseconds
  limitSwitchX.setDebounceTime(50);
  limitSwitchY.setDebounceTime(50);
  //max frequency
  MOTOR_A.enableOutputs();
  MOTOR_A.setMaxSpeed(7000);
  MOTOR_A
  //horizontal debugging
  //testPCB();
  //agitation debugging

  // MOTOR_A.setSpeed(8000);
  // MOTOR_A.setAcceleration(9000);                  // Fixed acceleration
  // MOTOR_A.move(1000 * stepsPerRevolution);  // Relative move
  // delay(50);//time to be configured

  // while (MOTOR_A.distanceToGo() != 0) {
  //   MOTOR_A.run();
  // }
  // while(1){
    
  //   moveMotor(MOTORX, 600, 9);
  //   delay(500);
  //   moveMotor(MOTORY, 600, 9);
  //   delay(500);
  // }
  testPCB();
    //moveMotor(MOTOR_A, 11000, 9);
  //agitate(30);
  
}

void agitate(uint8_t agitateDuration) {
  unsigned long startTime = millis();  // capture a start time
  //current time - start time = timeagitating. if it goes greater than the desired duration, stop
  while ((millis() - startTime) < (agitateDuration * 1000)) {
    moveMotor(MOTOR_A, 7000, 1);
    moveMotor(MOTOR_A, 7000, -1);
  }
}

void testPCB(void) {
  while (1) {
    moveMotor(MOTORY, 700, -1);  // negative goes up
    delay(500);
    moveMotor(MOTORX, 500, 1);  // posotive goes to the right
    delay(500);
    moveMotor(MOTORX, 500, -1);  // posotive goes to the right
    delay(500);
    moveMotor(MOTORY, 700, 1);  // negative goes up
    delay(500);
   
  }
}

void moveMotor(AccelStepper motor, float speed, float rotations) {

  motor.setSpeed(speed);
  motor.setAcceleration(10000);                 // Fixed acceleration
  motor.move(rotations * stepsPerRevolution);  // Relative move

  while (motor.distanceToGo() != 0) {
    motor.run();
  }
}



/**
 * @brief: perform the agitation operation of moving motors up and down very quickly
 * @param agitateSpeed: agitation speed on a scale of 1-9
 * @param agitateDuration: agitation duration given in seconds
 * @param percentDepth: percentage of well volume to be displaced
 * @retval: none
 */
void agitateMotors(uint8_t agitateSpeed, uint8_t agitateDuration, uint8_t totalVolume, uint8_t percentDepth) {
  homeAgitation();
  // set motor speed
  int agitationFrequency = mapSpeed(agitateSpeed);  // newmap
  int depth = mapDepth(totalVolume, percentDepth);  // Number of direction changes per cycle
  int toggleDelay = 1000 / (depth);                 // milliseconds per half-cycle (back and forth)

  //ledcWriteTone(AGITATION_MOTOR_STEP, agitationFrequency); // Drive motor - this is the old way
  MOTOR_A.setMaxSpeed(agitationFrequency);
  MOTOR_A.setAcceleration(900);  // Fixed acceleration
  MOTOR_A.move(depth);           // Relative move

  // setup for changing directions
  bool DIR = HIGH;                       // init direction
  unsigned long startTime = millis();    // capture a start time
  pinMode(AGITATION_MOTOR_DIR, OUTPUT);  // ensure pin is set to output

  while (millis() - startTime < (agitateDuration * 1000)) {
    digitalWrite(AGITATION_MOTOR_DIR, DIR);  // set new direction
    delay(toggleDelay);                      // delay before next iteration
    DIR = (DIR == HIGH) ? LOW : HIGH;        // change directions after every toggleDelay iteration
  }
  // turn motors off afrer delay is finished
  ledcWriteTone(AGITATION_MOTOR_STEP, 0);
}


// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeed(float value) {
  return (value - 1) * (1000 - 400) / (9 - 1) + 400;
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




/**
 * @brief: Pause the motor for a number of seconds
 * @param pauseDuration: pause duration in seconds
 * @retval: none
 */
void pauseMotors(uint8_t pauseDuration) {
  //call the agitation homing function then wait for the desired amount of time
  homeAgitation();              // home the agitator
  delay(pauseDuration * 1000);  // convert from seconds to milliseconds for delay function
}


/**
 * @brief Runs each motor toward home until the limit switch is hit
 * @param none
 * @retval none
 */
void autoHome() {
  // Home Agitation Motor
  MOTOR_A.setMaxSpeed(200);
  MOTOR_A.setAcceleration(900);


  while (1) {
    limitSwitchAgitation.loop();
    MOTOR_A.move(-1);  // Small move in home direction
    MOTOR_A.run();

    if (limitSwitchAgitation.isPressed())
      Serial.println("Agitation Limit: UNTOUCHED -> TOUCHED");
    if (limitSwitchAgitation.isReleased())
      Serial.println("Agitation Limit: TOUCHED -> UNTOUCHED");

    if (limitSwitchAgitation.getState() == LOW) {
      Serial.println("Agitation Homed");
      MOTOR_A.stop();
      MOTOR_A.setCurrentPosition(0);
      break;
    }
  }

  // Home Motor X
  MOTORX.setMaxSpeed(200);
  MOTORX.setAcceleration(900);


  while (1) {
    limitSwitchX.loop();
    MOTORX.move(-1);  // Small move in home direction
    MOTORX.run();

    if (limitSwitchX.isPressed())
      Serial.println("X Limit: UNTOUCHED -> TOUCHED");
    if (limitSwitchX.isReleased())
      Serial.println("X Limit: TOUCHED -> UNTOUCHED");

    if (limitSwitchX.getState() == LOW) {
      Serial.println("X Homed");
      MOTORX.stop();
      MOTORX.setCurrentPosition(0);
      break;
    }
  }

  // Home Motor Y
  MOTORY.setMaxSpeed(200);
  MOTORY.setAcceleration(900);


  while (1) {
    limitSwitchY.loop();
    MOTORY.move(-1);  // Small move in home direction
    MOTORY.run();

    if (limitSwitchY.isPressed())
      Serial.println("Y Limit: UNTOUCHED -> TOUCHED");
    if (limitSwitchY.isReleased())
      Serial.println("Y Limit: TOUCHED -> UNTOUCHED");

    if (limitSwitchY.getState() == LOW) {
      Serial.println("Y Homed");
      MOTORY.stop();
      MOTORY.setCurrentPosition(0);
      break;
    }
  }
}

void homeAgitation() {
  // Home Agitation Motor
  MOTOR_A.setMaxSpeed(200);
  MOTOR_A.setAcceleration(900);


  while (1) {
    limitSwitchAgitation.loop();
    MOTOR_A.move(-1);  // Small move in home direction
    MOTOR_A.run();

    if (limitSwitchAgitation.isPressed())
      Serial.println("Agitation Limit: UNTOUCHED -> TOUCHED");
    if (limitSwitchAgitation.isReleased())
      Serial.println("Agitation Limit: TOUCHED -> UNTOUCHED");

    if (limitSwitchAgitation.getState() == LOW) {
      Serial.println("Agitation Homed");
      MOTOR_A.stop();
      MOTOR_A.setCurrentPosition(0);
      break;
    }
  }
}

/**
 * @brief: iterate through the array and extract all information
 * @param char *protocol : pause duration in seconds
 * @retval: none
 */
Protocol parseProtocol(char *protocol) {
  Protocol parsed;  // object of protocol struct with the elements of all protocolInstructions contained
  parsed.type = getProtocolType(protocol);
  // for each type extract its respective information
  switch (parsed.type) {
    case AGITATION:
      parsed.volume = (protocol[1] - '0');                                      // Extract volume
      parsed.percentVolume = ((protocol[2] - '0') * 10) + (protocol[3] - '0');  // % of volume
      parsed.speed = (protocol[4] - '0');                                       // Speed
      parsed.duration = ((protocol[5] - '0') * 10) + (protocol[6] - '0');       // Duration
      break;

    case PAUSING:
      parsed.duration = (protocol[1] - '0');  // Only duration for pausing
      break;

    case MOVING:
      parsed.initialSurfaceTime = ((protocol[1] - '0') * 100) + ((protocol[2] - '0') * 10) + (protocol[3] - '0');  // Initial surface time
      parsed.speed = (protocol[4] - '0');                                                                          // Speed
      parsed.stopAtSequences = (protocol[5] - '0');                                                                // Stop at sequences
      parsed.sequencePauseTime = (protocol[6] - '0');                                                              // Sequence pause time
      break;
  }

  return parsed;
}

// Function to determine protocol type
ProtocolType getProtocolType(char *protocol) {
  // based on the first character of the protocolInstructions you can see what type of protocol it is
  switch (protocol[0]) {  // Check the first character
    case 'B':
      return AGITATION;
      break;
    case 'P':
      return PAUSING;
      break;
    case 'M':
      return MOVING;
      break;
    default:
      Serial.print("Error: Invalid protocol type '");
      Serial.print(protocol[0]);
      Serial.println("'");
      return INVALID;
      break;
  }
}
/**
* @brief main State Machine that handles the ASAT Device
* @retval none
*/
void runASATS() {
  //loop through array of protocolInstructions
  for (int i = 0; i < size; i++) {
    Protocol parsed = parseProtocol(protocolInstructions[i]);  // Parse protocol

    // print out list
    Serial.print("Protocol: ");
    Serial.println(protocolInstructions[i]);
    // for each parsed protocol print out its information based on type
    switch (parsed.type) {
      // call the agitation function
      case AGITATION:
        //agitateMotors(parsed.speed, parsed.duration, parsed.volume, parsed.percentVolume); // agitate the motors
        // debug for information correctness
        Serial.println("Type: Agitation");
        Serial.print("Volume: ");
        Serial.println(parsed.volume);
        Serial.print("Percent Volume: ");
        Serial.println(parsed.percentVolume);
        Serial.print("Speed: ");
        Serial.println(parsed.speed);
        Serial.print("Duration: ");
        Serial.println(parsed.duration);
        break;

      case PAUSING:
        //pauseMotors(parsed.duration);
        Serial.println("Type: Pausing");
        Serial.print("Duration: ");
        Serial.println(parsed.duration);
        break;

      case MOVING:  // moving function not tested yet.
        //moveSample(parsed.initialSurfaceTime, parsed.speed, parsed.stopAtSequences, parsed.sequencePauseTime);
        Serial.println("Type: Moving");
        Serial.print("Initial Surface Time: ");
        Serial.println(parsed.initialSurfaceTime);
        Serial.print("Speed: ");
        Serial.println(parsed.speed);
        Serial.print("Stop at Sequences: ");
        Serial.println(parsed.stopAtSequences);
        Serial.print("Sequence Pause Time: ");
        Serial.println(parsed.sequencePauseTime);
        break;
    }
  }
}


void loop() {
  //ensuring the limit switches are always being updated
  limitSwitchAgitation.loop();
  limitSwitchX.loop();
  limitSwitchY.loop();
}
