#include "esp32-hal-timer.h"

//**hi dorjee. the bind functiond doesnt have accelerationr rn. you might need it. 4/21/25 night **//

hw_timer_t *timer = NULL;  // declaration of timer
static int timerTicks = 0;

// function call by the timer interruption
void IRAM_ATTR onTimer() {
  timerTicks++;
}

#define LEDC_RESOLUTION 8

const int AGITATION_STEP_PIN = 23;
const int AGITATION_DIR_PIN = 22;

const int HORIZONTAL_STEP_PIN = 21;
const int HORIZONTAL_DIR_PIN = 20;

const int VERTICAL_STEP_PIN = 19;
const int VERTICAL_DIR_PIN = 18;

const int ENABLE_SIGNAL = 15;

typedef enum {
  UP = 0,
  DOWN = 1
} verticalDirection;  //i think is ground is up, vdd is down. need to test it

#define MAX_LINE_LENGTH 32
#define MAX_LINES 100

#define MAX_TIME 1000
#define MINIMUM_FREQUENCY 300

// Initialize the tempBuffer with nulls ('\0')
char tempBuffer[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } };
uint8_t i = 0;
uint8_t j = 0;

typedef enum {
  waitingState,
  runState
} operationState;

operationState currentState = waitingState;

void setup() {
  Serial.begin(115200);

  //agitation motor
  pinMode(AGITATION_DIR_PIN, OUTPUT);  //vertical motor dir pin is a digital gpio
  digitalWrite(AGITATION_DIR_PIN, 1);
  ledcAttachChannel(AGITATION_STEP_PIN, MINIMUM_FREQUENCY, 8, 8);
  ledcWrite(AGITATION_STEP_PIN, 0);

  //vertical motor
  pinMode(VERTICAL_DIR_PIN, OUTPUT);  // Set the pin as an output
  ledcAttachChannel(VERTICAL_STEP_PIN, MINIMUM_FREQUENCY, 8, 9);
  ledcWrite(VERTICAL_STEP_PIN, 0);

  //horizontal motor
  pinMode(HORIZONTAL_DIR_PIN, OUTPUT);  // Set the pin as an output
  digitalWrite(HORIZONTAL_DIR_PIN, 1);  //its either 1 or 0, we are only going rightwards
  ledcAttachChannel(HORIZONTAL_STEP_PIN, MINIMUM_FREQUENCY, 8, 10);
  ledcWrite(HORIZONTAL_STEP_PIN, 0);

  // Timer initialisation at a frequency of 1 MHz (1 µs per tick)
  timer = timerBegin(1000000);

  if (timer == NULL) {
    Serial.println("Error with the start of the timer");
    while (1)
      ;
  }

  // Attaches the interrupt function to the timer
  timerAttachInterrupt(timer, &onTimer);

  // Configure an alarm to trigger the interrupt every 1 ms (1000 µs)
  timerAlarm(timer, 150, true, 0);  // number represents microseconds. 150 means every 150 us interrupt triggers

  // Start of the timer
  timerStart(timer);

  Serial.println("Start Test");
}

void loop() {
  //state machine to handle the machine operation
  switch (currentState) {
    case waitingState:
      // fill up tempBuffer with serial data
      if (Serial.available()) {
        // Read the incoming byte
        char incomingByte = Serial.read();
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
          i = 0;
          j = 0;
          currentState = runState;
          Serial.println(tempBuffer[0]);
          Serial.println(tempBuffer[1]);
          Serial.println(tempBuffer[2]);
          Serial.println(tempBuffer[3]);
          Serial.println(tempBuffer[4]);
          Serial.println(tempBuffer[5]);
          Serial.println(tempBuffer[6]);
        }
      }
      break;
    case runState:
      //go line by line through tempBuffer and execute the protocol
      for (int a = 1; a < MAX_LINES; a++) {  //skip first line cuz it is the title
        //handle pause
        if (tempBuffer[a][0] == 'P') {
          Serial.println("Pausing");
          pauseMotors(tempBuffer[a][1]);
        }
        //handle binding
        if (tempBuffer[a][0] == 'B') {
          Serial.println("Binding");
          beginBinding(tempBuffer[a][1]);
        }
        //handle agitation
        if (tempBuffer[a][0] == 'A') {
          Serial.println("Agitating");
          startAgitating(tempBuffer[a][1], tempBuffer[a][2], tempBuffer[a][3]);
        }
        //handle stopping motors
        if (Serial.available()) {
          char incomingByte = Serial.read();
          if (incomingByte == 'S') {
            //stop motors
            Serial.println("STOP! Zhonyas");
            currentState = waitingState;
            memset(tempBuffer, 0, sizeof(tempBuffer));
            break;  //will this work? i want to get out of for loop and change states instantly
          }
        }
      }
      currentState = waitingState;
      memset(tempBuffer, 0, sizeof(tempBuffer));
      //shitty solution to the issue of uart data being sent while motors move
      // while (Serial.available()) {
      //   Serial.read();
      // }
      Serial.println("Finished Protocol");
      break;
  }
}

/**
 * @brief: pause the motor for specified time in milliseconds
 * @param input_duration, integer between 0-9 represents seconds to pause the motors
 * @retval: Failure if UART signal is received to stop the motors
 */
uint8_t pauseMotors(char input_duration) {
  int intDuration = input_duration - '0';  //convert from ascii to int
  int pause_duration_ms = intDuration * 1000;

  //stall motor
  ledcWrite(AGITATION_STEP_PIN, 255);

  //start timer to pause the motors
  uint32_t curTime = millis();
  while ((millis() - curTime) < pause_duration_ms) {
    //handle stopping motors
    if (Serial.available()) {
      char incomingByte = Serial.read();
      if (incomingByte == 'S') {
        //stop motors
        Serial.println("STOP! Zhonyas");
        currentState = waitingState;
        memset(tempBuffer, 0, sizeof(tempBuffer));
        return 0;
        //break;  //will this work? i want to get out of for loop and change states instantly
      }
    }
  }

  ledcWrite(AGITATION_STEP_PIN, 0);  //this is just for debugging to see the pause function bring the pin high and then low
  return 1;
}

//three timing constraints: total agitation time, direction switching time, acceleration time
/**
 * @brief: pause the motor for specified time in milliseconds
 * @param mix_time, integer between 0-9 representing mix_time in seconds
 * @param mix_speed, integer between 0-9 representing mix_speed
 * @param mix_depth, integer between 0-9 representing how far up and down the agitation goes
 * @retval: Failure if UART signal is received to stop the motors
 */
uint8_t startAgitating(unsigned long mix_time, uint8_t mix_speed, uint8_t mix_depth) {
  //ascii to int conversions
  int int_mix_time = mix_time - '0';    //convert from ascii to int
  int int_mix_speed = mix_speed - '0';  //convert from ascii to int
  int int_mix_depth = mix_depth - '0';  //convert from ascii to int

  //map inputs to usable numbers
  int mix_time_ms = int_mix_time * 1000;  //get the mix_time in milliseconds
  int agitateStepFrequency = mapSpeedtoFreq(int_mix_speed);
  int agitateHalfPeriod = mapDepthtoTime(int_mix_depth);  //half period cuz thats what it is? what do i name this

  /** begin actual agitation process ***/

  //start the interrupt for chef time
  timerTicks = 0;
  timerRestart(timer);

  //get start time and start the motor up
  uint32_t curTime = millis();
  ledcChangeFrequency(AGITATION_STEP_PIN, 10000, LEDC_RESOLUTION);
  ledcWrite(AGITATION_STEP_PIN, 128);

  //stay in this agitation loop until the total agitation time is finished
  while ((millis() - curTime) < mix_time_ms) {
    if ((timerTicks == MAX_TIME)) {
      ledcChangeFrequency(AGITATION_STEP_PIN, 10000, LEDC_RESOLUTION);
      digitalWrite(AGITATION_DIR_PIN, !digitalRead(AGITATION_DIR_PIN));
      timerTicks = 0;
    }

    //handle stopping motors
    if (Serial.available()) {
      char incomingByte = Serial.read();
      if (incomingByte == 'S') {
        //stop motors
        Serial.println("STOP! Zhonyas");
        currentState = waitingState;
        memset(tempBuffer, 0, sizeof(tempBuffer));
        return 0;
        //break;  //will this work? i want to get out of for loop and change states instantly
      }
    }
  }
  //finished agitating, turn the motor off
  ledcWrite(AGITATION_STEP_PIN, 0);
  return 1;
}

/**
 * @brief: move the gantry head up, to the right, and then down to a certain depth level
 * @param bindDepth, integer between 0-99
 * @retval: Failure if UART signal is received to stop the motors
 */
uint8_t beginBinding(char bindDepth) {
  //move vertical motor up
  digitalWrite(VERTICAL_DIR_PIN, 1);  //is this up or down?
  ledcChangeFrequency(VERTICAL_STEP_PIN, 500, LEDC_RESOLUTION);
  ledcWrite(VERTICAL_STEP_PIN, 128);

  //start timer for moving motor up
  uint32_t curTime = millis();
  while ((millis() - curTime) < 500) {
    //handle stopping motors
    if (Serial.available()) {
      char incomingByte = Serial.read();
      if (incomingByte == 'S') {
        //stop motors
        Serial.println("STOP! Zhonyas");
        currentState = waitingState;
        memset(tempBuffer, 0, sizeof(tempBuffer));
        return 0;
        //break;  //will this work? i want to get out of for loop and change states instantly
      }
    }
  }
  //stall vertical motor
  ledcWrite(VERTICAL_STEP_PIN, 0);


  //move horizontal motor right
  ledcChangeFrequency(HORIZONTAL_STEP_PIN, 700, LEDC_RESOLUTION);
  ledcWrite(HORIZONTAL_STEP_PIN, 128);

  //start timer for moving motor to the right
  curTime = millis();
  while ((millis() - curTime) < 200) {
    //handle stopping motors
    if (Serial.available()) {
      char incomingByte = Serial.read();
      if (incomingByte == 'S') {
        //stop motors
        Serial.println("STOP! Zhonyas");
        currentState = waitingState;
        memset(tempBuffer, 0, sizeof(tempBuffer));
        return 0;
        //break;  //will this work? i want to get out of for loop and change states instantly
      }
    }
  }
  //stall horizontal motor
  ledcWrite(HORIZONTAL_STEP_PIN, 0);

  //move vertical motor down
  digitalWrite(VERTICAL_DIR_PIN, 0);  //is this up or down?
  ledcChangeFrequency(VERTICAL_STEP_PIN, 700, LEDC_RESOLUTION);
  ledcWrite(VERTICAL_STEP_PIN, 128);

  //start timer for moving motor down
  curTime = millis();
  while ((millis() - curTime) < 500) {
    //handle stopping motors
    if (Serial.available()) {
      char incomingByte = Serial.read();
      if (incomingByte == 'S') {
        //stop motors
        Serial.println("STOP! Zhonyas");
        currentState = waitingState;
        memset(tempBuffer, 0, sizeof(tempBuffer));
        return 0;
        //break;  //will this work? i want to get out of for loop and change states instantly
      }
    }
  }
  //stall vertical motor
  ledcWrite(VERTICAL_STEP_PIN, 0);

  return 1;
}

int mapSpeedtoFreq(int input_speed) {
  int output_frequency = 820 + (input_speed * 20);  //input speed of 9 maxes out at 10khz
  return output_frequency;
}

int mapDepthtoTime(int input_depth) {
  int output_time = 55 + (input_depth * 5);  //input depth of 9 maxes out at 100ms of movement. min depth is 55ms of movement
  return output_time;
}