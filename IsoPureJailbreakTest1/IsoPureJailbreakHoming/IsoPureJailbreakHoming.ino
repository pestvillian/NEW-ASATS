#include <AccelStepper.h>

//#include <SoftwareSerial.h>

// --- Pin Assignments ---
#define HORIZONTAL_STEP 5
#define HORIZONTAL_DIR 4
#define MAGNET_STEP 2
#define MAGNET_DIR 3
#define COMB_STEP 19
#define COMB_DIR 18
//Limit pins
#define H_home 7    //homing switch for horizontal
#define C_ready 15  //botom most light sensor
#define Mid 1       //misdle switch
#define M_ready 6   //topmost light sensor

#define fan_pin 14
//enable pin of motherboard
#define HORIZONTAL_EN 21
#define COMB_EN 22
#define MAGNET_EN 23


#define clearSampleDist 37.0
#define clamplingOffset 22.0  //tuning this one

// --- Create Stepper Instances ---
AccelStepper HORIZONTAL(AccelStepper::DRIVER, HORIZONTAL_STEP, HORIZONTAL_DIR);
AccelStepper MAGNET(AccelStepper::DRIVER, MAGNET_STEP, MAGNET_DIR);
AccelStepper COMB(AccelStepper::DRIVER, COMB_STEP, COMB_DIR);

// logic for switchs
bool horizontalTriggered = false;
bool magnetTriggered = false;
bool combTriggered = false;

bool homingH = false;  //home horizontal init
bool homingM = true;   //home megnet init
bool homingC = false;  //hom comb init
//fuck UART
// types of protocolInstructions
enum ProtocolType {
  AGITATION,  // 'A'
  PAUSING,    // 'P'
  MOVING,     // 'M'
  INVALID     // For unknown protocol types
};
//parameters of said protocols
struct Protocol {
  ProtocolType type;
  uint16_t volume;              //volume of liquid in a given well
  uint16_t percentVolume;       // amount of liquid to be displaced
  uint16_t speed;               //speed for motors to run at
  uint16_t duration;            // time for agitation to occur
  uint16_t initialSurfaceTime;  // time to let liquid drip off into next well
  uint16_t stopAtSequences;     //number of sections to pause at, in a given well
  uint16_t sequencePauseTime;   // time spent at each point in the well
  uint16_t pausetime;           // amount of rest time in between agitations
  uint8_t repeats;              // number of repeated agitations
};

// Protocol Array
char *protocolInstructions[] = {
  "A9101100500502",  //speed 9-duration 10 - volume 1100 - % 50 - pausetime 05 repeats 03
  "M0101199"         // checking for drip time
  // "A9101100500503"  //speed 9-duration 10 - volume 1100 - % 50 - pausetime 05 repeats 03
  // "M0601199"

};


// global size of protocolInstructions
int size = sizeof(protocolInstructions) / sizeof(protocolInstructions[0]);  // Get number of elements

uint8_t home();
uint32_t distanceToStepsC(float distance);
unsigned int mapSpeedC(float value);


void setup() {
  Serial.begin(9600);

  delay(500);
  //direction Pins
  pinMode(HORIZONTAL_DIR, OUTPUT);
  pinMode(MAGNET_DIR, OUTPUT);
  pinMode(COMB_DIR, OUTPUT);
  //step pin
  pinMode(HORIZONTAL_STEP, OUTPUT);
  pinMode(MAGNET_STEP, OUTPUT);
  pinMode(COMB_STEP, OUTPUT);

  pinMode(H_home, INPUT_PULLUP);   //horizontal switch
  pinMode(M_ready, INPUT_PULLUP);  //magnet switch
  pinMode(Mid, INPUT_PULLUP);      //middle lightswitch
  pinMode(C_ready, INPUT_PULLUP);  //comb switch
  //Enable pins output
  pinMode(HORIZONTAL_EN, OUTPUT);
  pinMode(MAGNET_EN, OUTPUT);
  pinMode(COMB_EN, OUTPUT);

  pinMode(fan_pin, OUTPUT);
  digitalWrite(fan_pin, LOW);
  // // Enable outputs
  HORIZONTAL.enableOutputs();
  MAGNET.enableOutputs();
  COMB.enableOutputs();

  //configure homing parameters for each motor
  HORIZONTAL.setMaxSpeed(700);
  HORIZONTAL.setAcceleration(9999);
  HORIZONTAL.moveTo(1600);  //far distance posotive home

  MAGNET.setMaxSpeed(800);
  MAGNET.setAcceleration(999999);
  MAGNET.moveTo(100000);  //magnet home posotive distance

  COMB.setMaxSpeed(1000);
  COMB.setAcceleration(999999);  //3200 steps to 1 rev
  COMB.moveTo(100000000);        //comb home posotive distance

  //turn on motor drivers
  digitalWrite(HORIZONTAL_EN, LOW);
  digitalWrite(COMB_EN, LOW);
  digitalWrite(MAGNET_EN, LOW);
  // home this ho
  while (1) {
    if (home() == 1) {  //should home the gantry to right above the wells
      break;
    } else {
      continue;
    }
    Serial.println("Homing!!\n");
  }



  // // //comb motor is intermittently turining on and off when the system is idle holding torque - fixed
  // agitateMotors(9, 5, 1100, 50);  // 1100 is total volume 50 = 50% of total volume

  // magnetPushComb(102.0);  //tuning distance
  // pauseMotors(5);         //wait to let the beads attach to combs
  // //                                                     //this part needs to be tested!!!!!!!
  // combPushMagnet(clearSampleDist);                    //move sample out of rack good
  // moveMotorH(-1, 1, 7.5);                             //distance between wells
  // magnetPushComb(clearSampleDist + clamplingOffset);  // for some reason the magnet axis is going upwards slightly before going back down to push on the combs
  // delay(200);                                         //slight wait
  // homeMagnet();                                       //testing...working?????
  // moveComb(-1,1,15);//15mm down
  // agitateMotors(9, 10, 1100, 50);
  Serial.println("Hello World");  //hello fucking world cant work but we can move motors????????????????????? make that make sense
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
        Serial.print("Speed=");
        Serial.print(parsed.speed);

        Serial.print(" Duration=");
        Serial.print(parsed.duration);

        Serial.print(" Volume=");
        Serial.print(parsed.volume);

        Serial.print(" Percent=");
        Serial.print(parsed.percentVolume);

        Serial.print(" Pause=");
        Serial.print(parsed.pausetime);

        Serial.print(" Repeats=");
        Serial.println(parsed.repeats);
        //call agitation for every repeat we have
        for (int i = 0; i < parsed.repeats; i++) {                                            //just for now don't get pissed
          agitateMotors(parsed.speed, parsed.duration, parsed.volume, parsed.percentVolume);  // agitate the motors
          delay(1000 * parsed.pausetime);                                                     //delay time inbetween repeats
        }

        break;

      case PAUSING:
        pauseMotors(parsed.duration);
        break;

      case MOVING:  // moving function not tested yet.
        moveSample(parsed.initialSurfaceTime, parsed.speed, parsed.stopAtSequences, parsed.sequencePauseTime);
        delay(2000);

        break;
      case INVALID:
        Serial.println("Invalid Command");
        break;
    }
  }
}


void moveSample(uint8_t initialSurfaceTime, uint8_t speed, uint8_t stopAtSequences, uint8_t sequencePauseTime) {
  //move from one well to the next

  magnetPushComb(102.0);           //push the magnets all the way down into the rack
  pauseMotors(sequencePauseTime);  //wait to let the beads attach to combs
  //this part needs to be tested!!!!!!!
  combPushMagnet(clearSampleDist);                    //move sample out of rack good
  moveMotorH(-1, speed, 7.5);                         //distance between wells //this number will likely be tuned a lot
  magnetPushComb(clearSampleDist + clamplingOffset);  // for some reason the magnet axis is going upwards slightly before going back down to push on the combs
  delay(200);                                         //slight wait
  homeMagnet();                                       //testing...working????? working!!!
}

void homeMagnet() {  // working now
  //configure magnet axis
  magnetTriggered = false;  //
  MAGNET.setMaxSpeed(800);
  MAGNET.setAcceleration(9999999);  //AGRESSIVE
  MAGNET.moveTo(9999999);           //posotive home direction
  MAGNET.enableOutputs();           //idk pmo lowk
  //home Magnet motor
  while (1) {
    //run the M motor
    if (digitalRead(M_ready) == 0) {
      magnetTriggered = true;  //var for keeping track of magnet
    }
    MAGNET.run();  //MOVE THE FUKIN MOTOR please
    //Serial.println("I'm in the loop");
    if (magnetTriggered == true) {  //magnet home switch triggered
      MAGNET.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
  MAGNET.setCurrentPosition(0);  //
}
//logic for moving horizontal axis exact distance
void moveMotorH(int DIR, uint32_t speed, float distance) {  // 1 step is 1.8 degrees
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsH(distance);
  uint16_t stepFrequency = mapSpeedH(speed);  // adjust to control speed (Hz)
  uint32_t stepdir = steps * DIR;

  //configure motor parameters
  HORIZONTAL.setMaxSpeed(stepFrequency);
  HORIZONTAL.setAcceleration(30000);
  HORIZONTAL.move(stepdir);
  //move motor to location
  while (1) {
    //run motor
    HORIZONTAL.run();

    //Serial.println("I'm in the loop");
    if (HORIZONTAL.distanceToGo() == 0) {
      HORIZONTAL.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
}
// angle (degrees) = (arc length / radius) * (180 / π)
uint32_t distanceToStepsH(float distance)  // about 8.75mm
{
  // 200 steps = 1 revolution
  //for returning # of steps per rotation use distance * 200
  //30mm per 1 revolution
  //1mm = 20/3

  //return distance * 200;
  //the number i devide 200 by is what i'm changing to get the best accuracy
  return (uint32_t)(distance * (200.00 / 31.24) + 0.5f);
}
//logic for moing the Magnet Axis exact distances
void moveMotorM(uint32_t DIR, uint32_t speed, float distance) {  // 1 step is 1.8 degrees
                                                                 // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsM(distance);
  uint16_t stepFrequency = mapSpeedM(speed);  // adjust to control speed (Hz)
  uint32_t stepdir = steps * DIR;             //DIR == 1 goes down, DIR == -1 goes up
  //configure motor parameters
  MAGNET.setMaxSpeed(stepFrequency);
  MAGNET.setAcceleration(300000);
  MAGNET.move(stepdir);
  //move motor to location
  while (1) {
    //run motor
    MAGNET.run();

    //Serial.println("I'm in the loop");
    if (MAGNET.distanceToGo() == 0) {  //break when the steps have steppec
      MAGNET.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
}
// angle (degrees) = (arc length / radius) * (180 / π)
uint32_t distanceToStepsM(float distance)  // about 8.75mm
{
  //return distance * 200;// tuning distance measurements for new screw
  return (uint32_t)(distance * (200.0 / 8.0) + 0.5f);  //shoulf convert desired distance traveld to a number of steps to send the motor
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedM(float value) {
  return (value - 1) * (1000 - 200) / (9 - 1) + 200;  //changed from 400 to 350 hz as bottom of frequency range
}

// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedH(float value) {
  return (value - 1) * (1000 - 300) / (9 - 1) + 300;
}
//logic for moving comb to axact distances
void moveComb(int DIR, uint32_t speed, float distance) {  //
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsC(distance);
  uint16_t stepFrequency = mapSpeedC(speed);  // adjust to control speed (Hz)
  uint32_t stepdir = steps * DIR;
  //configure motor parameters
  COMB.setMaxSpeed(stepFrequency);
  COMB.setAcceleration(300000);
  COMB.move(stepdir);
  //move motor to location
  while (1) {
    //run the A motor
    COMB.run();

    //Serial.println("I'm in the loop");
    if (COMB.distanceToGo() == 0) {
      COMB.stop();
      break;
      //Serial.println("I broke the loop");
    }
    delayMicroseconds(500);  // or try 100–500 µ
  }
}
uint32_t distanceToStepsC(float distance) {  // 20mm lead
  //return distance * 200 * 16; // one rotation for full step
  return (uint32_t)(distance * (1600.0 / 20.0) + 0.5f);  //changed from 3200 to 1600
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedC(float value) {
  return (value - 1) * (12000 - 5000) / (9 - 1) + 5000;
}


//logic to use the comb axis to push the magnet axis up so we can stay clamped together without losing the smaple
void combPushMagnet(float pushDist) {  //working...just kidding
  digitalWrite(COMB_EN, LOW);          //comb on
  digitalWrite(MAGNET_EN, HIGH);       //magnet off

  moveComb(1, 1, pushDist);  //move the comb up

  // while (COMB.distanceToGo() != 0) {
  //   COMB.run();  //run
  // }
  delay(200);                    //give the motor a chance to be in a fixed position...i heard a click and got scared
  digitalWrite(MAGNET_EN, LOW);  //magnet on to save its place
}
//logic to use the magnet axis to push the comb axis up so we can stay clamped together without losing the smaple
void magnetPushComb(float pushDist) {
  digitalWrite(COMB_EN, HIGH);   //comb off
  digitalWrite(MAGNET_EN, LOW);  //magnet on
  //magnet ne
  moveMotorM(-1, 9, pushDist);  //push comb axis with magnet

  // while (MAGNET.distanceToGo() != 0) {
  //   MAGNET.run();  //run
  // }
  delay(200);                  //give the motor a chance to be in a fixed position...i heard a click and got scared
  digitalWrite(COMB_EN, LOW);  //comb on to save its place
}

/**
 * @brief: Pause the motor for a number of seconds
 * @param pauseDuration: pause duration in seconds
 * @retval: none
 */
void pauseMotors(uint8_t pauseDuration) {
  HORIZONTAL.stop();
  MAGNET.stop();
  COMB.stop();
  delay(pauseDuration * 1000);  // convert from milliseconds to seconds for delay function
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
uint8_t agitateMotors(uint16_t agitateSpeed, uint8_t agitateDuration, uint16_t totalVolume, uint16_t percentDepth) {
  //42.2 = height of whole well
  delay(200);
  COMB.enableOutputs();
  //30 mm is the distance between the tip of the combs inserted into the wells and the bottom of the wells
  // Convert input values to physical parameters
  uint16_t agitationFrequency = mapSpeedC(agitateSpeed);  // Frequency in steps/sec
  COMB.setMaxSpeed(agitationFrequency);                   // High speed target
  COMB.setAcceleration(3000000);                          // Very aggressive acceleration

  // Define positions
  uint16_t top = abs((totalVolume / 50.0) - (42.2) + 0.5f);  //plus initia position??? was 50 well hright -(42.2)

  uint16_t agitDistance = (abs(top - 42.2) * (percentDepth / 100.0));  //percentage of liquid to be displaced
  uint16_t agitSteps = distanceToStepsC(agitDistance);
  int movingDown = 1;  //should initially be true

  uint16_t topSteps = distanceToStepsC(top);  //
  // Start timed agitation loop
  unsigned long startTime = millis();  //snag initial time

  //move to top of sample volume
  COMB.moveTo(-topSteps);             //topSteps
  while (COMB.distanceToGo() != 0) {  // do comb run
    COMB.run();                       // run until we are at the top of the solution
  }
  delay(2000);

  //move to percent depth of liquid
  COMB.move(-agitSteps);
  while (COMB.distanceToGo() != 0) {
    COMB.run();
  }

  //alternate direcitons
  while (millis() - startTime < (agitateDuration * 1000)) {  //millis() - startTime < (agitateDuration * 1000)
    COMB.run();                                              // run the motor
    if (COMB.distanceToGo() == 0) {                          //check if we hit the desired agitation depth
      //COMB.moveTo(movingDown ? top : agitDistance); //changed bottom to agitDistance
      COMB.move(movingDown * agitSteps);
      movingDown = movingDown * -1;
    }
  }

  COMB.stop();  //hault
  return 1;
}


//set all motor axis to a set 000 position
uint8_t home() {
  // //uart debug
  uint8_t homed = 0;
  //homing switches
  if (homingM == true) {
    if (digitalRead(M_ready) == 0) {  //home pin for magnets
      magnetTriggered = true;
    }
    if (magnetTriggered != true) {
      MAGNET.run();  // keep moving
      // keep looping
    } else {                         //when horizontalTriggerd == TRUE we stop
      MAGNET.stop();                 //hault motor
      MAGNET.setCurrentPosition(0);  //now at home position
      homingM = false;               // we are home
      homingC = true;
    }
  }
  //next motor in the sequence
  if (homingC == true) {          //checking if the midle switch is inverted
    if (digitalRead(Mid) == 0) {  //mid pin for comb
      combTriggered = true;
    }
    if (combTriggered != true) {
      COMB.run();  // keep moving
      // keep looping
    } else if (combTriggered == true) {  //when horizontalTriggerd == TRUE we stop
      COMB.stop();                       //hault motor
      COMB.setCurrentPosition(0);        //now at home position
      homingC = false;                   // we are home
      homingH = true;
    }
  }

  if (homingH == true) {
    if (digitalRead(H_home) == 0) {  //horizontal
      horizontalTriggered = true;
    }
    if (horizontalTriggered != true) {
      HORIZONTAL.run();  // keep moving
      // keep looping
    } else {                             //when horizontalTriggerd == TRUE we stop
      HORIZONTAL.stop();                 //hault motor
      HORIZONTAL.setCurrentPosition(0);  //now at home position
      homingH = false;                   // we are home
                                         // we want to home the magnet motor now
    }
  }

  //check if we are done
  if (homingH == false && homingM == false && homingC == false) {
    //we will now nudged the horizontal axis towards home slighlty more so the comb is more alligned
    moveMotorH(1, 1, 3.75);  //unlcear on distance must tune. this seems to be the most i can go without snapping the rubber stopper
    HORIZONTAL.setCurrentPosition(0);
    COMB.moveTo(-80000);  //long steps in down direction
    while (1) {
      COMB.run();                       //get stuck here          // keep moving
      if (digitalRead(C_ready) == 0) {  //comb now at the ready position
        COMB.setCurrentPosition(0);
        homed = 1;
        break;  //leave loop
                //homing done
      }
    }
    COMB.stop();  //hault motor
  } else {
    homed = 0;
  }

  return homed;
}

/**
 * @brief: iterate through the array and extract all information
 * @param char *protocol : pause duration in seconds
 * @retval: none
 */
Protocol parseProtocol(char *protocol) {
  Protocol parsed;                          // object of protocol struct with the elements of all protocolInstructions contained
  parsed.type = getProtocolType(protocol);  //for each string we update the type
  // for each type extract its respective information
  switch (parsed.type) {
    case AGITATION:                                                                                                                     //"A 9 10 1100 50 05 03"                                                                                                                // B 9 30 1000 100 20 15
      parsed.speed = (protocol[1] - '0');                                                                                               //1 digit
      parsed.duration = ((protocol[2] - '0') * 10) + (protocol[3] - '0');                                                               // Duration 2 digits
      parsed.volume = ((protocol[4] - '0') * 1000) + ((protocol[5] - '0') * 100) + ((protocol[6] - '0') * 10) + ((protocol[7] - '0'));  //total volume  4 digits                             // Extract volume
      parsed.percentVolume = ((protocol[8] - '0') * 10) + ((protocol[9] - '0'));                                                        // % of volume
      parsed.pausetime = ((protocol[10] - '0') * 10) + ((protocol[11] - '0'));                                                          // 2 digits
      parsed.repeats = ((protocol[12] - '0') * 10) + ((protocol[13] - '0'));                                                            //2 digits
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
    case 'A':
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

// main loop
void loop() {
}
