/************
4/3/25
Dorjee Tenzing
Overview:
Create premade protocols that consist of three operations: agitate, pause, bind
Write a state machine that calls existing motor functions with defined inputs and outputs
*************/
const int AGITATION_MOTOR_STEP = 12;
const int AGITATION_MOTOR_DIR = 13;
const int AGITATION_MOTOR_STEP_CHANNEL = 1;
const int AGITATION_MOTOR_DIR_CHANNEL = 2;

#define MAX_LINE_LENGTH 32
#define MAX_LINES 100
// Initialize the tempBuffer with nulls ('\0')
char tempBuffer[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } };
//memset(tempBuffer, 0, sizeof(tempBuffer));
uint8_t startFlag = 0;
uint8_t i = 0;
uint8_t j = 0;

HardwareSerial mySerial(1);
//types of protocols
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

//Protocol Array
char *protocols[] = {
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

//global size of protocols
int size = sizeof(protocols) / sizeof(protocols[0]);  // Get number of elements

//init functions
Protocol parseProtocol(char *protocol);
ProtocolType getProtocolType(char *protocol);

// Initialize the tempBuffer with nulls ('\0')

void setup() {
  //Set up UART and GPIO
  Serial.begin(115200);
  pinMode(AGITATION_MOTOR_DIR, OUTPUT);
  pinMode(AGITATION_MOTOR_STEP, OUTPUT);
  // Setup Agitation Motor Step Signal
  ledcAttachChannel(AGITATION_MOTOR_STEP, 1000, 8, AGITATION_MOTOR_STEP_CHANNEL);
  ledcWrite(AGITATION_MOTOR_STEP, 0);  //set duty cycle
  //ledcChangeFrequency(AGITATION_MOTOR_STEP, 1000, 8);  //set real frequency here

  // Setup Agitation Motor Dir Signal
  ledcAttachChannel(AGITATION_MOTOR_DIR, 10, 8, AGITATION_MOTOR_DIR_CHANNEL);
  ledcWrite(AGITATION_MOTOR_DIR, 0);  //set duty cycle
  //ledcChangeFrequency(AGITATION_MOTOR_DIR, 10, 8);  //set real frequency here

  /***** SANTIY CHECK MOTOR TESTING CODE BEGIN ******/

  // delay(2000);

  // ledcWriteTone(AGITATION_MOTOR_STEP, 1000);
  // ledcWriteTone(AGITATION_MOTOR_DIR, 10);

  // delay(2000);

  // ledcWriteTone(AGITATION_MOTOR_STEP, 0);
  // ledcWriteTone(AGITATION_MOTOR_DIR, 0);

  // delay(2000);

  // ledcWriteTone(AGITATION_MOTOR_STEP, 500);
  // ledcWriteTone(AGITATION_MOTOR_DIR, 5);

  /******** SANITY CHECK MOTOR TESTING CODE FINSIH *********/

  /** USEFUL FUNCTIONS:
  * ledcWriteTone(AGITATION_MOTOR_STEP, 1000);
  * ledcChangeFrequency(AGITATION_MOTOR_DIR, 10, 8);
 */

  /*********** RUN MAIN TEST ***********/

  //Fill in tempBuffer with premade protocol
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
  //loop through array of protocols
  for (int i = 0; i < size; i++) {
    Protocol parsed = parseProtocol(protocols[i]);  // Parse protocol

    //print out list
    Serial.print("Protocol: ");
    Serial.println(protocols[i]);
    //for each parsed protocol print out its information based on type
    switch (parsed.type) {
      //call the agitation function
      case AGITATION:
        agitateMotors(parsed.speed, parsed.duration, parsed.volume, parsed.percentVolume);  //agitate the motors
                                                                                            //debug for information correctness
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
        pauseMotors(parsed.duration);
        Serial.println("Type: Pausing");
        Serial.print("Duration: ");
        Serial.println(parsed.duration);
        break;

      case MOVING:  //moving function not written yet.
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
}
/**
 * @brief: Pause the motor for a number of seconds
 * @param pauseDuration: pause duration in seconds
 * @retval: none
 */
void pauseMotors(uint8_t pauseDuration) {
  //analogWrite(AGITATION_MOTOR_STEP, 255);  //no pwm signal, use high for debugging
  ledcWriteTone(AGITATION_MOTOR_STEP, 0);
  delay(pauseDuration * 1000);  //convert from seconds to milliseconds for delay function
}

/**
 * @brief: perform the agitation operation of moving motors up and down very quickly
 * @param agitateSpeed: agitation speed on a scale of 1-9
 * @param agitateDuration: agitation duration given in seconds
 * @param percentDepth: percentage of well volume to be displaced
 * @retval: none
 */
void agitateMotors(uint8_t agitateSpeed, uint8_t agitateDuration, uint8_t totalVolume, uint8_t percentDepth) {
  //set motor speed
  int agitationFrequency = mapSpeed(agitateSpeed);  //newmap
  int depth = mapDepth(totalVolume, percentDepth);  // Number of direction changes per cycle
  int toggleDelay = 1000 / (depth);                 // milliseconds per half-cycle (back and forth)

  ledcWriteTone(AGITATION_MOTOR_STEP, agitationFrequency);  // Drive motor

  //setup for changing directions
  bool DIR = HIGH;                       //init direction
  unsigned long startTime = millis();    //capture a start time
  pinMode(AGITATION_MOTOR_DIR, OUTPUT);  //ensure pin is set to output

  while (millis() - startTime < agitateDuration * 1000) {
    digitalWrite(AGITATION_MOTOR_DIR, DIR);  //set new direction
    delay(toggleDelay);                      //delay before next iteration
    DIR = (DIR == HIGH) ? LOW : HIGH;        //change directions after every toggleDelay iteration
  }
  //turn motors off afrer delay is finished
  ledcWriteTone(AGITATION_MOTOR_STEP, 0);
}

//   /**
//  * @brief: Map agitationSpeed value to a PWM frequency
//  * @param agitateSpeed: agitation speed on a scale of 1-9
//  * @retval: frequency of a PWM signal given in Hz
//  */
//   int mapAgitationSpeed(uint8_t agitateSpeed) {
//     if (agitateSpeed == 9) {
//       return 1000;
//     } else if (agitateSpeed == 1) {
//       return 500;
//     } else {
//       return 800;
//     }
//   }

//num1 and num2 are the integer ranges of speed, num3 and num4 are the frequency ranges
unsigned int mapSpeed(float value) {
  return (value - 1) * (1000 - 400) / (9 - 1) + 400;
}
//maping ranges of depths to a fequency for direction channel
unsigned int mapDepth(float value1, float value2) {
  // Map value1 (1-6) directly to 5-20
  float mapped1 = ((value1 - 1) * (20 - 5) / (6 - 1)) + 5;

  // Map value2 (1-100) directly to 5-20
  float mapped2 = ((value2 - 1) * (20 - 5) / (100 - 1)) + 5;

  // Weighted blend (change weights if needed)
  float finalValue = (mapped1 * 0.6) + (mapped2 * 0.4);

  // Clamp to 5-20
  if (finalValue < 5) finalValue = 5;
  if (finalValue > 20) finalValue = 20;

  return (unsigned int)finalValue;
}

/**
 * @brief: iterate through the array and extract all information
 * @param char *protocol : pause duration in seconds
 * @retval: none
 */
Protocol parseProtocol(char *protocol) {
  Protocol parsed;  //object of protocol struct with the elements of all protocols contained
  parsed.type = getProtocolType(protocol);
  //for each type extract its respective information
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
  //based on the first character of the protocols you can see what type of protocol it is
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
