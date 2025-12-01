#include <AccelStepper.h>

// --- Pin Assignments ---
#define MOTOR_X_STEP 5
#define MOTOR_X_DIR 4
#define MOTOR_Y_STEP 2
#define MOTOR_Y_DIR 3
#define AGITATION_MOTOR_STEP 19
#define AGITATION_MOTOR_DIR 18
//channels of futures past
const int AGITATION_MOTOR_STEP_CHANNEL = 2;
const int MOTOR_X_STEP_CHANNEL = 1;
const int MOTOR_Y_STEP_CHANNEL = 0;

//limit switches
#define HORIZONTAL_SWITCH 7
#define VERTICAL_SWITCH 6
#define AGITATION_SWITCH 15
//enable pin of motherboard
#define MOTOR_ENABLE 22

// --- Create Stepper Instances ---
AccelStepper stepperX(AccelStepper::DRIVER, MOTOR_X_STEP, MOTOR_X_DIR);
AccelStepper stepperY(AccelStepper::DRIVER, MOTOR_Y_STEP, MOTOR_Y_DIR);
AccelStepper stepperA(AccelStepper::DRIVER, AGITATION_MOTOR_STEP, AGITATION_MOTOR_DIR);

// types of protocolInstructions
enum ProtocolType {
  AGITATION,  // 'B'
  PAUSING,    // 'P'
  MOVING,     // 'M'
  INVALID     // For unknown protocol types
};
//parameters of said protocols
struct Protocol {
  ProtocolType type;
  uint8_t volume; //volume of liquid in a given well
  uint8_t percentVolume; // amount of liquid to be displaced
  uint8_t speed; //speed for motors to run at
  uint8_t duration; // time for agitation to occur
  uint8_t initialSurfaceTime; // time to let liquid drip off into next well
  uint8_t stopAtSequences;//number of sections to pause at, in a given well
  uint8_t sequencePauseTime; // time spent at each point in the well
  uint8_t pausetime; // amount of rest time in between agitations
  uint8_t repeats; // number of repeated agitations
};

// Protocol Array
char *protocolInstructions[] = {
  "B1123501000602",
  "M0061199",
  "B1063500950602",
  "M0011199",
  "B1123500950602",
  "M0611199",
  "B1063500950602",
  "M0011199",
  "B9993501000102"  //last step
};


// global size of protocolInstructions
int size = sizeof(protocolInstructions) / sizeof(protocolInstructions[0]);  // Get number of elements


void setup() {
  Serial.begin(115200);
  delay(500);

  // Enable motors
  pinMode(MOTOR_ENABLE, OUTPUT);
  digitalWrite(MOTOR_ENABLE, LOW);  // LOW = enabled

  // Configure limit switches
  pinMode(HORIZONTAL_SWITCH, INPUT_PULLDOWN);
  pinMode(VERTICAL_SWITCH, INPUT_PULLDOWN);
  pinMode(AGITATION_SWITCH, INPUT_PULLDOWN);
  //direction Pins
  pinMode(AGITATION_MOTOR_DIR, OUTPUT);
  pinMode(MOTOR_Y_DIR, OUTPUT);
  pinMode(MOTOR_X_DIR, OUTPUT);
  // // Enable outputs
  stepperX.enableOutputs();
  stepperY.enableOutputs();
  stepperA.enableOutputs();



  // Setup Agitation Motor Step Signal




  Serial.println("Starting homing sequence...");
  autoHome();  //home the system over the rack

  //agitateMotors(1, 99, 350, 100);
  // moveSample(020,2,3,5);
  // agitateMoto

  for (int i = 0; i < size; i++) {
    Protocol parsed = parseProtocol(protocolInstructions[i]);  // Parse protocol

    // print out list
    Serial.print("Protocol: ");
    Serial.println(protocolInstructions[i]);
    // for each parsed protocol print out its information based on type
    switch (parsed.type) {
      // call the agitation function
      case AGITATION:
        // agitate for as many times as the parameter sets
        //before you loop through the number of repeats you need to insert the combs into the wells
        moveMotorY(1, 1, 12);  //insert body into well
        for (int i = 0; i < parsed.repeats; i++) {       
          agitateMotors(parsed.speed, parsed.duration, parsed.volume, parsed.percentVolume);  // agitate the motors
          delay(1000 * parsed.pausetime);                                                     //delay time inbetween repeats
        }

        break;

      case PAUSING:
        pauseMotors(parsed.duration);

        break;

      case MOVING:  // moving function not tested yet.
        moveSample(parsed.initialSurfaceTime, parsed.speed, parsed.stopAtSequences, parsed.sequencePauseTime);

        break;
      case INVALID:
        Serial.println("Invalid Command");
        break;
    }
  }

  Serial.println("Homing complete.");
}


void moveSample(uint8_t initialSurfaceTime, uint8_t speed, uint8_t stopAtSequences, uint8_t sequencePauseTime) {
  // uint16_t yspeed = mapSpeedY(speed);
  // uint16_t xspeed = mapSpeedX(speed);
 
  homeAgitation();  // home the agitation motor and wait
  //moveMotorY(-1, 1, 12);  //back to home height

  pauseMotors(initialSurfaceTime);
  // fulltraydepth is about 41mm
  uint8_t segs = 33.6 / stopAtSequences;  // franctions of segment distances
  moveMotorY(1, 1, 33.6);                 // move all the way to the bottom then go back to home to prevent clustering
  delay(2000);                            //wait just 2 secinds
  moveMotorY(-1, 1, 33.6);                // back to home
  delay(1000);
  // incrementaly lower magnets
  for (int i = 0; i < stopAtSequences; i++) {
    moveMotorY(1, 1, segs);
    delay(1000 * sequencePauseTime);
  }

  moveMotorY(-1, speed, 45.6);  // back to idle hieght
  delay(1000);                  // delay for smoothness
  moveMotorX(1, speed, 9);      // increased from 10 to 9
  delay(1000);
  moveMotorY(1, speed, 20);  // back to idle hieght
  delay(1000);
  moveMotorA(1, speed, 10);  // positive 1 go down
  delay(1000);
  moveMotorY(-1, speed, 20);
  homeAgitation();
}


/**
 * @brief: move agitaton motor up and down rapidly
 * @param agitateSpeed: speed of motor from 1-9
 * @param agitateDuration: duration of agitation from 1-?
 * @param totalVolume: irrelevant parameter?
 * @param percentDepth: how far the agitation goes up and down from 0-100
 * @retval: 1 if finished, 0 if error
 * @author: Gregory Ziegler
 */
uint8_t agitateMotors(uint16_t agitateSpeed, uint8_t agitateDuration, uint8_t totalVolume, uint8_t percentDepth) {

  delay(200);
  homeAgitation();       // bodies clamped
  stepperA.enableOutputs();
  //30 mm is the distance between the tip of the combs inserted into the wells and the bottom of the wells
  // Convert input values to physical parameters
  uint16_t agitationFrequency = mapSpeedAgitation(agitateSpeed);  // Frequency in steps/sec

                                                                  // Define positions
  //float depth_mm = 29.0 - (29.0 * (totalVolume / 350.0)); //mapping of volume given in ul to a distance from hope position in mm
  //   //float distance = 33.30;  //350ul = 33.3mm away from home position higher volumes make lower distances
  //   //float depth_mm = 38.0 * (100 / 100.0);// THIS MUST SET THE BEGINING HEIGHT OF THE COMBS //check me
  //float depth_mm = 32 * (totalVolume / 100.0);                    // Total immersion depth in mm

  float depth_mm = (40.0 * (totalVolume / 350.0)); //mapping of volume given in ul to a distance from home position in mm

  float stroke_mm = (40.0 - depth_mm) * (percentDepth / 100.0);  // Agitation stroke depth in mm
  //uint32_t stroke_steps = stroke_mm / 0.00625;                    // Convert mm to steps // HOW tf did we get this number???
  uint32_t stroke_steps = stroke_mm * ((3200.0 / 20.0) + 0.5f);  //I think this is right

  // Define positions
  //long top = 400;  // Home = 0 //distance away from primary body in steps top_steps = (topDistance_mm * (3200.0 / 20.0) + 0.5f);
  long top = (depth_mm * (3200.0 / 20.0) + 0.5f);  //distance in mm
  long bottom = stroke_steps;
  bool movingDown = true;  //am i breaking shit

  // Setup stepper
  stepperA.setMaxSpeed(20000);        // initial slow speed for positioning
  stepperA.setAcceleration(3000000);  // Very aggressive acceleration

  // Move to top first
  stepperA.moveTo(top);
  while (stepperA.distanceToGo() != 0) {
    stepperA.run();
  }

  // Move to bottom second
  stepperA.moveTo(bottom);
  while (stepperA.distanceToGo() != 0) {
    stepperA.run();
  }

  stepperA.setMaxSpeed(agitationFrequency);  // High speed target
  stepperA.setAcceleration(3000000);         // Very aggressive acceleration
  // Start timed agitation loop
  unsigned long startTime = millis();

  stepperA.moveTo(top);  // First move up

  while (millis() - startTime < (agitateDuration * 1000)) {
    stepperA.run();                      // run the motor
    if (stepperA.distanceToGo() == 0) {  //check if we hit the desired agitation depth
      stepperA.moveTo(movingDown ? top : bottom);
      movingDown = !movingDown;
    }
  }

  stepperA.stop();  //hault


  homeAgitation();        //clamp bodies
  //moveMotorY(-1, 1, 12);  //back to home height
  return 1;
}



// i am the work in progress version
// void agitateMotors(uint16_t agitateSpeed, uint8_t agitateDuration, uint8_t totalVolume, uint8_t percentDepth) {

//   delay(200);
//   homeAgitation();

//   stepperA.enableOutputs();

//    // Define positions
//   float topDistance_mm = 38.0 - (38.0 * (totalVolume / 350.0)); //mapping of volume given in ul to a distance from hope position in mm
//   //float distance = 33.30;  //350ul = 33.3mm away from home position higher volumes make lower distances
//   //float depth_mm = 38.0 * (100 / 100.0);// THIS MUST SET THE BEGINING HEIGHT OF THE COMBS //check me
//   //42 remeber me
//   float stroke_mm = (38.0) * (percentDepth / 100.0);            // Agitation stroke depth in mm
//   uint32_t stroke_steps = stroke_mm / 0.00625;                    // Convert mm to steps

//   uint32_t top_steps = (topDistance_mm * (3200.0 / 20.0) + 0.5f);

//   long top = top_steps;  // Home = 0 changed from 6 to 8

//   long bottom = stroke_steps;
//   bool movingDown = true;


//     // Setup stepper
//   // Convert input values to physical parameters
//   uint16_t agitationFrequency = mapSpeedAgitation(agitateSpeed);  // Frequency in steps/sec
//   stepperA.setMaxSpeed(20000);        // initial slow speed for positioning
//   stepperA.setAcceleration(3000000);  // Very aggressive acceleration

//   // Move to top first
//   stepperA.moveTo(top);
//   while (stepperA.distanceToGo() != 0) {
//     stepperA.run();
//   }

//   // Move to bottom second
//   stepperA.moveTo(bottom);
//   while (stepperA.distanceToGo() != 0) {
//     stepperA.run();
//   }

//   stepperA.setMaxSpeed(agitationFrequency);  // High speed target
//   stepperA.setAcceleration(3000000);         // Very aggressive acceleration
//   // Start timed agitation loop
//   unsigned long startTime = millis();
//   stepperA.moveTo(top);  // First move up

//   while (millis() - startTime < (agitateDuration * 1000)) {
//     stepperA.run();                      // run the motor
//     if (stepperA.distanceToGo() == 0) {  //check if we hit the desired agitation depth
//       stepperA.moveTo(movingDown ? top : bottom);
//       movingDown = !movingDown;
//     }
//   }

//   stepperA.stop();  //hault
//   homeAgitation();  //reset Agitation Motor
// }


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
    case AGITATION:  // B 9 30 100 100 20 15
      parsed.speed = (protocol[1] - '0');
      parsed.duration = ((protocol[2] - '0') * 10) + (protocol[3] - '0');                                       // Duration
      parsed.volume = ((protocol[4] - '0') * 100) + ((protocol[5] - '0') * 10) + ((protocol[6] - '0'));         //total volume                              // Extract volume
      parsed.percentVolume = ((protocol[7] - '0') * 100) + ((protocol[8] - '0') * 10) + ((protocol[9] - '0'));  // % of volume
      parsed.pausetime = ((protocol[10] - '0') * 10) + ((protocol[11] - '0'));
      parsed.repeats = ((protocol[12] - '0') * 10) + ((protocol[13] - '0'));

      break;

    case PAUSING:
      parsed.duration = (protocol[1] - '0');  // Only duration for pausing
      break;

    case MOVING:
      parsed.initialSurfaceTime = ((protocol[1] - '0') * 100) + ((protocol[2] - '0') * 10) + (protocol[3] - '0');  // Initial surface time
      parsed.speed = (protocol[4] - '0');                                                                          // Speed
      parsed.stopAtSequences = (protocol[5] - '0');                                                                // Stop at sequences
      parsed.sequencePauseTime = ((protocol[6] - '0') * 10) + (protocol[7] - '0');                                 //changed from 1 integer to 2                                                           // Sequence pause time
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
 * @brief: Pause the motor for a number of seconds
 * @param pauseDuration: pause duration in seconds
 * @retval: none
 */
void pauseMotors(uint8_t pauseDuration) {
  stepperX.stop();
  stepperY.stop();
  stepperA.stop();
  homeAgitation();  // home the agitator
  // analogWrite(AGITATION_MOTOR_STEP, 255);  //no pwm signal, use high for debugging

  delay(pauseDuration * 1000);  // convert from seconds to milliseconds for delay function
}




void homeAgitation() {
  //Agitation
  stepperA.setMaxSpeed(10000);
  stepperA.setAcceleration(30000);
  stepperA.moveTo(-999999);

  //home agitation motor
  while (1) {
    //run the A motor
    stepperA.run();
    uint16_t Astop = digitalRead(AGITATION_SWITCH);

    //Serial.println("I'm in the loop");
    if (Astop == LOW) {
      stepperA.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
  stepperA.setCurrentPosition(0);
}


/**
 * @brief:send each motor one at a time to the the home direction until the limit switch is hit
 * @param none
 * @retval none
 */
void autoHome() {
  //initialize motor parameters
  //Horizontal
  stepperX.setMaxSpeed(800);
  stepperX.setAcceleration(30000);
  stepperX.moveTo(-999999);  // minus goes left
  //Agitation
  stepperA.setMaxSpeed(10000);
  stepperA.setAcceleration(30000);
  stepperA.moveTo(-999999);  // minus goes up
  //Vertical
  stepperY.setMaxSpeed(800);
  stepperY.setAcceleration(30000);
  stepperY.moveTo(-700);  //posotive goes down

  //home agitation motor
  while (1) {
    //run the A motor
    stepperA.run();
    uint16_t Astop = digitalRead(AGITATION_SWITCH);

    //Serial.println("I'm in the loop");
    if (Astop == LOW) {
      stepperA.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
  //Move Y axis above test rack
  //Vertical init settings for out of way
  while (1) {
    //run the Y motor
    stepperY.run();

    if (stepperY.distanceToGo() == 0) {  //go up a little bit
      stepperY.stop();                   //stop there for now
      break;
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
  //home X motor
  while (1) {
    //run the X motor
    stepperX.run();
    uint16_t Xstop = digitalRead(HORIZONTAL_SWITCH);

    //Serial.println("I'm in the loop");
    if (Xstop == LOW) {
      stepperX.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
  //Vertical
  //new homeing setings
  stepperY.setMaxSpeed(800);
  stepperY.setAcceleration(30000);
  stepperY.moveTo(9999999);  //posotive goes down
  while (1) {
    //run the A motor
    stepperY.run();
    uint16_t Ystop = digitalRead(VERTICAL_SWITCH);

    //Serial.println("I'm in the loop");
    if (Ystop == LOW) {
      stepperY.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
  //create initial position
  stepperA.setCurrentPosition(0);
  stepperY.setCurrentPosition(0);
  stepperX.setCurrentPosition(0);

  //move chassis to above the test rack

  //position over test rack
  moveMotorY(-1, 9, 44.6);  // move Y up
  moveMotorX(1, 6, 55);     // move X right
  //moveMotorA(1, 6, 20);   // move A down posotive goes down
}





void moveMotorA(int DIR, uint32_t speed, float distance) {  //
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsA(distance);
  uint16_t stepFrequency = mapSpeedA(speed);  // adjust to control speed (Hz)
  uint32_t stepdir = steps * DIR;
  //configure motor parameters
  stepperA.setMaxSpeed(stepFrequency);
  stepperA.setAcceleration(300000);
  stepperA.move(stepdir);
  //move motor to location
  while (1) {
    //run the A motor
    stepperA.run();

    //Serial.println("I'm in the loop");
    if (stepperA.distanceToGo() == 0) {
      stepperA.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
}

uint32_t distanceToStepsA(float distance)  // about 8.75mm
{
  //return distance * 200 * 16; // one rotation
  return (uint32_t)(distance * (3200.0 / 20.0) + 0.5f);
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedA(float value) {
  return (value - 1) * (12000 - 5000) / (9 - 1) + 5000;
}

void moveMotorY(uint32_t DIR, uint32_t speed, float distance) {  // 1 step is 1.8 degrees
                                                                 // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsY(distance);
  uint16_t stepFrequency = mapSpeedY(speed);  // adjust to control speed (Hz)
  uint32_t stepdir = steps * DIR;             //DIR == 1 goes down, DIR == -1 goes up


  //configure motor parameters
  stepperY.setMaxSpeed(stepFrequency);
  stepperY.setAcceleration(3000);
  stepperY.move(stepdir);
  //move motor to location
  while (1) {
    //run motor
    stepperY.run();

    //Serial.println("I'm in the loop");
    if (stepperY.distanceToGo() == 0) {
      stepperY.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
}
// angle (degrees) = (arc length / radius) * (180 / π)
uint32_t distanceToStepsY(float distance)  // about 8.75mm
{
  //return distance * 200;
  return (uint32_t)(distance * (200.0 / 8.0) + 0.5f);
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedY(float value) {
  return (value - 1) * (1000 - 200) / (9 - 1) + 200; //changed from 400 to 350 hz as bottom of frequency range
}

void moveMotorX(int DIR, uint32_t speed, float distance) {  // 1 step is 1.8 degrees
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsX(distance);
  uint16_t stepFrequency = mapSpeedX(speed);  // adjust to control speed (Hz)
  uint32_t stepdir = steps * DIR;

  //configure motor parameters
  stepperX.setMaxSpeed(stepFrequency);
  stepperX.setAcceleration(30000);
  stepperX.move(stepdir);
  //move motor to location
  while (1) {
    //run motor
    stepperX.run();

    //Serial.println("I'm in the loop");
    if (stepperX.distanceToGo() == 0) {
      stepperX.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
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
  return (uint32_t)(distance * (200.00 / 31.24) + 0.5f);
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedX(float value) {
  return (value - 1) * (1000 - 300) / (9 - 1) + 300;
}

// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedAgitation(float value) {
  return (value - 1) * (55000 - 5000) / (9 - 1) + 5000;
}



void loop() {
  // Nothing here
  //here the edge cases should go
}
