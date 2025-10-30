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

const int VERTICAL_MOTOR_STEP = 27;
const int VERTICAL_MOTOR_DIR = 14;
const int VERTICAL_MOTOR_STEP_CHANNEL = 3;
const int VERTICAL_MOTOR_DIR_CHANNEL = 4;

const int HORIZONTAL_MOTOR_STEP = 25;
const int HORIZONTAL_MOTOR_DIR = 26;
const int HORIZONTAL_MOTOR_STEP_CHANNEL = 5;
const int HORIZONTAL_MOTOR_DIR_CHANNEL = 6;

#define MAX_LINE_LENGTH 32
#define MAX_LINES 100
#define VERTICAL_SPEED 1000 //experimentally determine this
#define HORIZONTAL_SPEED 1000

typedef enum {
  DIR_DOWN = 0, DIR_UP = 1
} verticalDirection;

// Initialize the tempBuffer with nulls ('\0')
char tempBuffer[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } };
void setup() {
  //Set up UART and GPIO
  Serial.begin(115200);

  // Setup Agitation Motor Step and Dir Signals
  ledcAttachChannel(AGITATION_MOTOR_STEP, 1000, 8, AGITATION_MOTOR_STEP_CHANNEL); //set speed
  ledcWrite(AGITATION_MOTOR_STEP, 0);                //set duty cycle
  ledcAttachChannel(AGITATION_MOTOR_DIR, 10, 8, AGITATION_MOTOR_DIR_CHANNEL); //set speed
  ledcWrite(AGITATION_MOTOR_DIR, 0);              //set duty cycle

  // Setup Vertical Motor Step and Dir Signals
  ledcAttachChannel(VERTICAL_MOTOR_STEP, 1000, 8, VERTICAL_MOTOR_STEP_CHANNEL);//set speed
  ledcWrite(VERTICAL_MOTOR_STEP, 0);                //set duty cycle
  pinMode(VERTICAL_MOTOR_DIR, OUTPUT);              //vertical motor dir pin is a digital gpio

  // Setup Horizontal Motor Step and Dir Signals
  ledcAttachChannel(HORIZONTAL_MOTOR_STEP, 1000, 8, HORIZONTAL_MOTOR_STEP_CHANNEL); //set speed
  ledcWrite(HORIZONTAL_MOTOR_STEP, 0);                //set duty cycle
  pinMode(HORIZONTAL_MOTOR_DIR, OUTPUT);              //horiztonal motor dir pin is a digital gpio
  digitalWrite(HORIZONTAL_MOTOR_DIR, 1);              //the direction of horizontal should only be forward
  
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
  Serial.println("Start Test");
  //delay(1000);


  //Fill in tempBuffer with premade protocol
  // strcpy(tempBuffer[1], "A94");
  // strcpy(tempBuffer[2], "P5");
  // strcpy(tempBuffer[3], "A16");
  strcpy(tempBuffer[4], "B5");
  strcpy(tempBuffer[5], "B8");
  uint8_t i = 0;

  for (i = 0; i, MAX_LINES; i++) {
    //handle pause operation: P7. Input: Duration
    if (tempBuffer[i][0] == 'P') {
      Serial.println("Pause");
      uint8_t pauseDuration = tempBuffer[i][1] - '0';  //convert from ascii to int
      pauseMotors(pauseDuration);
    }

    //handle agitate operation: A74. Inputs: Agitation speed, agitation duration
    if (tempBuffer[i][0] == 'A') {
      Serial.println("Agitatate");
      uint8_t agitateSpeed = tempBuffer[i][1] - '0';
      uint8_t agitateDuration = tempBuffer[i][2] - '0';
      agitateMotors(agitateSpeed, agitateDuration);
    }

    //handle bind operation: B7. Inputs: Bind depth
    if (tempBuffer[i][0] == 'B') {
      Serial.println("Bind");
      uint8_t bindDepth = tempBuffer[i][0];
      bindMotors(bindDepth);
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
  delay(pauseDuration * 1000);             //convert from seconds to milliseconds for delay function
}

/**
 * @brief: perform the agitation operation of moving motors up and down very quickly
 * @param agitateSpeed: agitation speed on a scale of 1-9
 * @param agitateDuration: agitation duration given in seconds
 * @retval: none
 */
void agitateMotors(uint8_t agitateSpeed, uint8_t agitateDuration) {
  //set motor speed
  int agitationFrequency = mapAgitationSpeed(agitateSpeed);
  //analogWriteFrequency(AGITATION_MOTOR_STEP, agitationFrequency);
  //analogWrite(AGITATION_MOTOR_STEP, 128);
  ledcWriteTone(AGITATION_MOTOR_STEP, agitationFrequency);
  //set oscillation frequency
  //analogWriteFrequency(AGITATION_MOTOR_DIR, 10);  //for testing, have direction switch every 100ms
  //analogWrite(AGITATION_MOTOR_STEP, 128);
  ledcWriteTone(AGITATION_MOTOR_DIR, 5);
  //set agitation duration
  delay(agitateDuration * 1000);
  //turn motors off afrer delay is finished
  ledcWriteTone(AGITATION_MOTOR_STEP, 0);
  ledcWriteTone(AGITATION_MOTOR_DIR, 0);
}

void bindMotors(uint8_t depth) {
  //move vertically up. how do i know how far up to go?
  digitalWrite(VERTICAL_MOTOR_DIR, 1); //up dir
  ledcWriteTone(VERTICAL_MOTOR_STEP, VERTICAL_SPEED);//set speed
  delay(2000);
  ledcWriteTone(VERTICAL_MOTOR_STEP, 0); //stop speed


  //move laterally. experimentally determine how far to the right
  ledcWriteTone(HORIZONTAL_MOTOR_STEP, HORIZONTAL_SPEED);//turn motor on
  delay(2000);
  ledcWriteTone(HORIZONTAL_MOTOR_STEP, 0); //turn motor off


  //move vertically down
  digitalWrite(VERTICAL_MOTOR_DIR, 0); //dir down
  ledcWriteTone(VERTICAL_MOTOR_STEP, VERTICAL_SPEED); //turn motor on
  delay(2000);
  ledcWriteTone(VERTICAL_MOTOR_STEP, 0); //turn motor off
}

/**
 * @brief: Map agitationSpeed value to a PWM frequency
 * @param agitateSpeed: agitation speed on a scale of 1-9
 * @retval: frequency of a PWM signal given in Hz 
 */
int mapAgitationSpeed(uint8_t agitateSpeed) {
  if (agitateSpeed == 9) {
    return 1000;
  } else if (agitateSpeed == 1) {
    return 500;
  } else {
    return 800;
  }
}
