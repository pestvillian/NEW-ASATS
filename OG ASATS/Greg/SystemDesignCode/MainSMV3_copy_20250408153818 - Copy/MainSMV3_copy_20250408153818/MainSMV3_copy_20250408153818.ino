/************
4/3/25
Gregory Ziegler + Dorjee Tenzing
Overview:
Create premade protocolInstructions that consist of three operations: agitate, pause, bind
Write a state machine that calls existing motor functions with defined inputs and outputs
IE host a diddy party in each well specified by diddy. Check on the hoes, then go to the next house.
*************/

#include <ezButton.h>
#include <AccelStepper.h>
#include <Adafruit_NeoPixel.h>


#define MAX_LINE_LENGTH 32
#define MAX_LINES 100

// Define the pin and number of LEDs
#define LED_PIN 8
#define NUM_LEDS 1
// Create a NeoPixel object
Adafruit_NeoPixel pixels(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

const int DIAG = 15;  // diag pin on TMC

const int AGITATION_MOTOR_STEP = 23;
const int AGITATION_MOTOR_DIR = 22;

const int MOTOR_X_STEP = 21;
const int MOTOR_X_DIR = 20;

const int MOTOR_Y_STEP = 19;  //
const int MOTOR_Y_DIR = 18;   //
const int AGITATION_MOTOR_STEP_CHANNEL = 2;
const int MOTOR_X_STEP_CHANNEL = 4;
const int MOTOR_Y_STEP_CHANNEL = 3;


AccelStepper stepper(AccelStepper::DRIVER, AGITATION_MOTOR_STEP, AGITATION_MOTOR_DIR);
ezButton limitSwitchX(4);          // create ezButton object that attach to ESP32 pin GPIO5
ezButton limitSwitchY(5);          // create ezButton object that attach to ESP32 pin GPIO6
ezButton limitSwitchAgitation(6);  // create ezButton object that attach to ESP32 pin GPIO7

// HardwareSerial mySerial(1);

// Initialize the tempBuffer with nulls ('\0')
char tempBuffer[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } };
// memset(tempBuffer, 0, sizeof(tempBuffer));
uint8_t startFlag = 0;
uint8_t i = 0;
uint8_t j = 0;

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
  "B930100050", "P10", "B930100050", "P10", "M010435", "B930100050", "P10"
};

// global size of protocolInstructions
int size = sizeof(protocolInstructions) / sizeof(protocolInstructions[0]);  // Get number of elements


// Initialize the tempBuffer with nulls ('\0')

void setup() {
  // Set up UART and GPIO
  Serial.begin(115200);
  pixels.begin();                            // for led side quest
  limitSwitchAgitation.setDebounceTime(50);  // set debounce time of limit switch to 50 milliseconds
  limitSwitchX.setDebounceTime(50);
  limitSwitchY.setDebounceTime(50);
  pinMode(AGITATION_MOTOR_DIR, OUTPUT);
  pinMode(MOTOR_Y_DIR, OUTPUT);
  pinMode(MOTOR_X_DIR, OUTPUT);
  //limit switch pins set to input
  pinMode(4, INPUT_PULLDOWN);
  pinMode(5, INPUT_PULLDOWN);
  pinMode(6, INPUT_PULLDOWN);

  // Setup  Motor Step Signal
  ledcAttachChannel(AGITATION_MOTOR_STEP, 1000, 8, AGITATION_MOTOR_STEP_CHANNEL);
  ledcAttachChannel(MOTOR_X_STEP, 1000, 8, MOTOR_X_STEP_CHANNEL);
  ledcAttachChannel(MOTOR_Y_STEP, 1000, 8, MOTOR_Y_STEP_CHANNEL);

  //rx pin is set to a microstepping trace
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);

  //config Enable Pin
  pinMode(15, OUTPUT);
  /** USEFUL FUNCTIONS:
   * ledcWriteTone(AGITATION_MOTOR_STEP, 1000);
   * ledcChangeFrequency(AGITATION_MOTOR_DIR, 10, 8);
   */

  /*********** RUN MAIN TEST ***********/

  // Fill in tempBuffer with premade protocol
  strcpy(tempBuffer[0], "P3");
  strcpy(tempBuffer[1], "A340532");
  uint8_t i = 0;

  // for (i = 0; i, MAX_LINES; i++) {
  //   //handle pause operation: P7. Input: Duration
  //   if (tempBuffer[i][0] == 'P') {
  //     uint8_t pauseDuration = tempBuffer[i][1] - '0';  //convert from ascii to int
  //     pauseMotors(pauseDuration);
  //   }

  //   //handle agitate operation: A74. Inputs: Agitation speed, agitation duration
  //   if (tempBuffer[i][0] == 'A') {
  //     uint8_t agitateSpeed = tempBuffer[i][4] - '0';                                         //extract speed
  //     uint8_t agitateDuration = ((tempBuffer[i][5] - '0') * 10) + (tempBuffer[i][6] - '0');  //extract duration
  //     uint8_t totalVolume = tempBuffer[i][1] - '0';                                          //extract total volume
  //     uint8_t percentVolume = ((tempBuffer[i][2] - '0') * 10) + (tempBuffer[i][3] - '0');    //extract percent volumt
  //     agitateMotors(agitateSpeed, agitateDuration, totalVolume, percentVolume);
  //   }

  //   //handle bind operation: B7. Inputs: Bind duration
  //   if (tempBuffer[i][0] == 'B') {
  //     uint8_t bindDuration = tempBuffer[i][0];
  //   }
  // }


  // /*Limit Switch Test Case*/
  //fun
  //sidequest

  // while (1) {
  //   for (int i = 0; i < 256; i++) {
  //     // Set the pixel to the current color in the wheel
  //     pixels.setPixelColor(0, Wheel(i));
  //     pixels.show();
  //     delay(100);
  //   }
  // }


  /*WORKS this block FUcking Works. 
  // digitalWrite(AGITATION_MOTOR_DIR, HIGH);
  // ledcWriteTone(AGITATION_MOTOR_STEP,5000);



  /*Test Protocol hard code functions
    */
  // Serial.println("Hello World");
  // autoHome();
  // agitateMotors(9, 30, 100, 100);
  // moveSample(010, 4, 3, 5);
  // autoHome();

  runASAT();
  // autoHome();
  // agitateMotors(9, 30, 100, 100);
  // end setup
}

void runASAT(){
   /*Main Test CASE*/

  //loop through array of protocolInstructions
  autoHome();
  delay(100);  //small delay before running the SM
  for (int i = 0; i < size; i++) {
    Serial.println("looping through Protocol List");
    Protocol parsed = parseProtocol(protocolInstructions[i]);  // Parse protocol

    // print out list
    Serial.print("Current Protocol: ");
    Serial.println(protocolInstructions[i]);
    // for each parsed protocol print out its information based on type
    switch (parsed.type) {
      // call the agitation function
      case AGITATION:  //diddy party
        Serial.println("INIT AGITATION");
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
        agitateMotors(parsed.speed, parsed.duration, parsed.volume, parsed.percentVolume);  // agitate the motors

        break;

      case PAUSING:  //let the liquid rest
        pauseMotors(parsed.duration);
        Serial.println("Type: Pausing");
        Serial.print("Duration: ");
        Serial.println(parsed.duration);
        break;

      case MOVING:  //move sample to next rack
        moveSample(parsed.initialSurfaceTime, parsed.speed, parsed.stopAtSequences, parsed.sequencePauseTime);
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

void agitateMotors(uint16_t agitateSpeed, uint8_t agitateDuration, uint8_t totalVolume, uint8_t percentDepth) {

  delay(200);
  homeAgitation();
  ledcDetach(AGITATION_MOTOR_STEP);  // Releases control from PWM
  stepper.enableOutputs();

  // Convert input values to physical parameters
  uint16_t agitationFrequency = mapSpeedAgitation(agitateSpeed);  // Frequency in steps/sec
  float depth_mm = 38.0 * (totalVolume / 100.0);                  // Total immersion depth in mm
  float stroke_mm = depth_mm * (percentDepth / 100.0);            // Agitation stroke depth in mm
  uint32_t stroke_steps = stroke_mm / 0.00625;                    // Convert mm to steps

  // Setup stepper
  stepper.setMaxSpeed(agitationFrequency);  // High speed target
  stepper.setAcceleration(3000000);         // Very aggressive acceleration
  // Define positions
  long top = 0;  // Home = 0
  long bottom = stroke_steps;
  bool movingDown = false;

  // Move to bottom first
  stepper.moveTo(bottom);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }

  // Start timed agitation loop
  unsigned long startTime = millis();
  stepper.moveTo(top);  // First move up

  while (millis() - startTime < (agitateDuration * 1000)) {
    stepper.run();
    if (stepper.distanceToGo() == 0) {
      stepper.moveTo(movingDown ? top : bottom);
      movingDown = !movingDown;
    }
  }

  stepper.stop();  //hault
  // Reattach PWM and rehome agitation motor
  ledcAttachChannel(AGITATION_MOTOR_STEP, 1000, 8, AGITATION_MOTOR_STEP_CHANNEL);
  homeAgitation();
}
// num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeedAgitation(float value) {
  return (value - 1) * (55000 - 5000) / (9 - 1) + 5000;
}



/**
  * from here, assume that the combs in question will be clamped with the magnets until the very end
 * @brief:slowly moves the magnets into the sampletray and transports sample to next well"M742193"
 * @param   initialSurfaceTime first 3 numbers
 * @param speed fourth number
 * @param stopAtSequences fifth number
 * @param sequencePauseTime sixth number
 * @retval: none
 */
void moveSample(uint8_t initialSurfaceTime, uint8_t speed, uint8_t stopAtSequences, uint8_t sequencePauseTime) {
  //fully init into the funciton
  delay(1000);
  homeAgitation();  // home the agitation motor and wait
  delay(2000);
  //we need to know at what hight inside the wells to put the combs
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
  for (int i = 0; i < stopAtSequences; i++) {
    moveMotorY(HIGH, speed, pos);     //increment motor
    delay(sequencePauseTime * 1000);  //hold here for however long specified
  }

  //moving sequence
  moveMotorY(LOW, 1, 40);  //after the beads are magnatized, hove the body un slowly
  delay(2000);             //delay to make sure the liquid stays
  //above works time for step 2, move to the right and fill the next Well
  moveMotorX(HIGH, 1, 8.75);  //move to next Well
  delay(2000);                //wait for smoothness
  moveMotorY(HIGH, 1, 25);    //position just above the wells
  //fill other well sequence
  moveMotorA(HIGH, 2, 16);
  moveMotorY(LOW, 1, 15);
  delay(initialSurfaceTime);
  homeAgitation();
}




/**
 * @brief:runs motor in home direction until a limit switch is hit
 * @param none
 * @retval: none
 */
void autoHome() {
  int motorSequence = 0;  //controls sequence of motors homing.
  Serial.println("HOMEING!!!");
  delay(200);
  moveMotorY(LOW, 4, 20);  // motor y to init position
  delay(200);              //allow Y motor to raise out of the way then allow the agitation motor to Home.
  if (motorSequence == 0) {
    // Low is Up, High is DOwn
    Serial.println("Homing Agitation");
    digitalWrite(AGITATION_MOTOR_DIR, LOW);      //LOW is the home DIrection
    ledcWriteTone(AGITATION_MOTOR_STEP, 20000);  // Drive motor
    // determine if it is pressed
    uint8_t stateA = digitalRead(6);
    //if the state of the limit switch is high that means it has been pressed
    while (1) {
      stateA = digitalRead(6);
      if (stateA != 1) {
        Serial.println("Limit switch is currently Touched");
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
  if (motorSequence == 1) {
    Serial.println("Homing Y Axis");
    //Right is Low, High is Left
    ledcWriteTone(MOTOR_X_STEP, 800);  // Drive motor
    digitalWrite(MOTOR_X_DIR, LOW);    //home direction is left = LOW
    //if the state of the limit switch is high that means it has been pressed
    while (1) {
      //the motor should be running the whole time that the limit switch is untouched
      uint8_t stateX = digitalRead(4);  //check status of limit switch
      if (stateX != 1) {
        Serial.println("Limit switch is currently Touched");
        pixels.setPixelColor(0, 0);  //first light state
        pixels.show();
        //turn off motor in this event
        digitalWrite(MOTOR_X_DIR, HIGH);
        ledcWriteTone(MOTOR_X_STEP, 0);  //
        moveMotorX(HIGH, 2, 2);          //nudge motor to the right so it's not on the limit switch
        motorSequence = 2;
        break;
      }
    }
  }
  if (motorSequence == 2) {
    Serial.println("Homing X Axis");
    //Low is Up and High is Down
    ledcWriteTone(MOTOR_Y_STEP, 800);  // Drive motor
    digitalWrite(MOTOR_Y_DIR, HIGH);   //home direction is Down = LOW

    uint8_t stateY = digitalRead(5);  //read status of limit switch

    //if the state of the limit switch is high that means it has been pressed
    while (1) {
      stateY = digitalRead(5);  //read status of limit switch
      if (stateY != 1) {
        Serial.println("Limit switch is currently Touched");
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



  //homing complete begin phase 2

  //move all three axis to there init positions

  moveMotorY(LOW, 4, 40);   // motor y to init position
  moveMotorX(HIGH, 5, 45);  //motor x to init position
  //the nudge puts it in the right place to begin with
  //moveMotorA(HIGH, 4, 2);  //move Agitation Head so that the plastic Combs are just above the test rack

  Serial.println("Homeing Complete!!");
  //rest of the homing repositioning sequence goes here:)
}

/**
 * @brief: take in a distance and move the gantry head that amount
 * note this particular motor driver is quarter microsteped.
 * @param distance
 * @retval: none
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
    digitalWrite(AGITATION_MOTOR_DIR, LOW);      //LOW is the home DIrection
    ledcWriteTone(AGITATION_MOTOR_STEP, 8000);  // Drive motor
    // determine if it is pressed
    uint8_t stateA = digitalRead(6);
    //if the state of the limit switch is high that means it has been pressed
    while (1) {
      stateA = digitalRead(6);
      if (stateA != 1) {
        Serial.println("Limit switch is currently Touched");
        //turn off motor in this event
        digitalWrite(AGITATION_MOTOR_DIR, HIGH);
        ledcWriteTone(AGITATION_MOTOR_STEP, 0);  //
        moveMotorA(HIGH, 2, 4);                  //check the nudge
        motorSequence = 1;
        break;
      } else {
        //Serial.println("Limit switch is currently Untouched");
        continue;
      }
    }
  }
  moveMotorA(HIGH,2,6);//this should be the agitation Motor's 0 point.
}





/**
 * @brief: take in a distance and move the gantry head that amount
 * note this particular motor driver is quarter microsteped.
 * @param distance
 * @retval: none
 */
void moveMotorY(int DIR, uint32_t speed, float distance) {  // 1 step is 1.8 degrees
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsY(distance);
  uint16_t stepFrequency = mapSpeedY(speed);          // adjust to control speed (Hz)
  float secondsToRun = (float)steps / stepFrequency;  // time = steps / freq
  uint32_t durationMs = (uint32_t)(secondsToRun * 1000);

  digitalWrite(MOTOR_Y_DIR, DIR);              // set direction
  ledcWriteTone(MOTOR_Y_STEP, stepFrequency);  // start step pulses

  delay(durationMs);  // run long enough to cover desired steps

  ledcWriteTone(MOTOR_Y_STEP, 0);  // stop step pulses
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
 */
void moveMotorX(int DIR, uint32_t speed, float distance) {  // 1 step is 1.8 degrees
  // convert distance to steps. for now i'm keeping it in number of revolutions
  uint32_t steps = distanceToStepsX(distance);
  uint16_t stepFrequency = mapSpeedX(speed);          // adjust to control speed (Hz)
  float secondsToRun = (float)steps / stepFrequency;  // time = steps / freq
  uint32_t durationMs = (uint32_t)(secondsToRun * 1000);


  Serial.print("Frequency => ");
  Serial.println(stepFrequency);
  Serial.print("delay => ");
  Serial.println(durationMs);

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
 * @retval: none
 */
void pauseMotors(uint8_t pauseDuration) {
  delay(100);
  homeAgitation();  // home the agitator
  ledcWriteTone(AGITATION_MOTOR_STEP, 0);
  delay(pauseDuration * 1000);  // convert from seconds to milliseconds for delay function
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
 * @brief: iterate through the array and extract all information
 * @param char *protocol : pause duration in seconds
 * @retval: none
 */
Protocol parseProtocol(char *protocol) {
  Protocol parsed;  // object of protocol struct with the elements of all protocolInstructions contained
  parsed.type = getProtocolType(protocol);
  // for each type extract its respective information
  switch (parsed.type) {
    case AGITATION:                        //"B930100100"
      parsed.speed = (protocol[1] - '0');  //you take the index and subtract by null to get the actual number in the struct
      parsed.duration = ((protocol[2] - '0') * 10) + ((protocol[3] - '0'));
      parsed.volume = ((protocol[4] - '0') * 100) + ((protocol[5] - '0') * 10) + ((protocol[6] - '0'));
      parsed.percentVolume = ((protocol[7] - '0') * 100) + ((protocol[8] - '0') * 10) + ((protocol[9] - '0'));
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
// unused
void loop() {
}
// Helper function to convert HSV to RGB for side quest
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixels.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return pixels.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  } else {
    WheelPos -= 170;
    return pixels.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}