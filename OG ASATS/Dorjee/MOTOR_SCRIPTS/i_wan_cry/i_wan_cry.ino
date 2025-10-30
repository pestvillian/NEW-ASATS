#include "esp32-hal-timer.h"

hw_timer_t *timer = NULL;  // declaration of timer
static int timerTicks = 0;

const int AGITATION_STEP_PIN = 23;
const int AGITATION_DIR_PIN = 22;
const int AGITATION_CHANNEL = 3;

const int HORIZONTAL_STEP_PIN = 21;
const int HORIZONTAL_DIR_PIN = 20;
const int HORIZONTAL_CHANNEL = 4;

const int VERTICAL_STEP_PIN = 19;
const int VERTICAL_DIR_PIN = 18;
const int VERTICAL_CHANNEL = 5;

const int HORIZONTAL_SWITCH = 4;
const int VERTICAL_SWITCH = 5;
const int AGITATION_SWITCH = 6;

const int ENABLE_SIGNAL = 15;

#define LEDC_RESOLUTION 8
#define ACCEL_STEPS 50
#define AGITATION_TIME 200
#define TIMER_INTERRUPT_MICROS 1000

typedef enum {
  UP = 0,
  DOWN = 1
} verticalDirection;  //i think is ground is up, vdd is down. need to test it

typedef enum {
  LEFT = 0,
  RIGHT = 1
} horizontalDirection;  //i think is ground is up, vdd is down. need to test it

typedef enum {
  AGITATION_DOWN = 0,
  AGITATION_UP = 1
} agitationDirection;  //i think is ground is up, vdd is down. need to test it

#define MAX_LINE_LENGTH 32
#define MAX_LINES 100

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

/******* HOW TO USE ******
* Frequency Sweep across given range. 

*/

// function call by the timer interruption
void IRAM_ATTR onTimer() {
  timerTicks++;
}

void setup() {
  Serial.begin(115200);

  //limit switches
  pinMode(HORIZONTAL_SWITCH, INPUT);
  pinMode(VERTICAL_SWITCH, INPUT);
  pinMode(AGITATION_SWITCH, INPUT);

  //agitation motor
  pinMode(AGITATION_DIR_PIN, OUTPUT);
  digitalWrite(AGITATION_DIR_PIN, 1);
  ledcAttachChannel(AGITATION_STEP_PIN, 1000, LEDC_RESOLUTION, AGITATION_CHANNEL);
  ledcWrite(AGITATION_STEP_PIN, 0);

  //vertical motor
  pinMode(VERTICAL_DIR_PIN, OUTPUT);  // Set the pin as an output
  ledcAttachChannel(VERTICAL_STEP_PIN, MINIMUM_FREQUENCY, LEDC_RESOLUTION, VERTICAL_CHANNEL);
  ledcWrite(VERTICAL_STEP_PIN, 0);

  //horizontal motor
  pinMode(HORIZONTAL_DIR_PIN, OUTPUT);  // Set the pin as an output
  digitalWrite(HORIZONTAL_DIR_PIN, 1);  //its either 1 or 0, we are only going rightwards
  ledcAttachChannel(HORIZONTAL_STEP_PIN, MINIMUM_FREQUENCY, LEDC_RESOLUTION, HORIZONTAL_CHANNEL);
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
  timerAlarm(timer, TIMER_INTERRUPT_MICROS, true, 0);  // number represents microseconds. 150 means every 150 us interrupt triggers

  // Start of the timer
  timerStart(timer);

  Serial.println("Start Test");

  /********* BEGIN TEST ***********/
  startHoming();
  startMagnetizing();
  startAgitating(5, 5, 5);
  char pauseDuration = '3';
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 2
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 3
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 4
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 5
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 6
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 7
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 8
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 9
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 10
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 11
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
  //round 12
  startMagnetizing();
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  startAgitating(5, 5, 5);
  pauseMotors(pauseDuration);
  shiftRight();
  delay(1000);
}

void loop() {
  // ledcChangeFrequency(AGITATION_STEP_PIN, 1000, 8);
  // ledcWrite(AGITATION_STEP_PIN, 128);
  // for (int i = 1; i <= 49; i++) {
  //   ledcChangeFrequency(AGITATION_STEP_PIN, 1000 + 1000 * i, 8);
  //   delay(1);
  // }
  // delay(50);
  // for (int i = 49; i >= 1; i--) {
  //   ledcChangeFrequency(AGITATION_STEP_PIN, 1000 + 1000 * i, 8);
  //   delay(1);
  // }
  // ledcWrite(AGITATION_STEP_PIN, 0);
  // digitalWrite(DIR, !digitalRead(DIR));
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
  digitalWrite(AGITATION_DIR_PIN, UP);
  ledcWrite(AGITATION_STEP_PIN, 128);

  //stay in this agitation loop until the total agitation time is finished
  while (((millis() - curTime) < 5000)) {
    // //accelerate
    // if ((timerTicks <= 50) && (timerTicks != 0)) {
    //   ledcChangeFrequency(AGITATION_STEP_PIN, 1000 * timerTicks, LEDC_RESOLUTION);
    // }
    // //decerlate
    // if ((timerTicks >= 100) && (timerTicks < AGITATION_TIME)) {
    //   ledcChangeFrequency(AGITATION_STEP_PIN, 1000 * (AGITATION_TIME - timerTicks), LEDC_RESOLUTION);
    // }
    //switch directions
    if ((timerTicks == AGITATION_TIME)) {
      //ledcChangeFrequency(AGITATION_STEP_PIN, 10000, LEDC_RESOLUTION);
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

  //if direction finished on an upstroke, go back down
  if (digitalRead(AGITATION_DIR_PIN) == UP) {
    ledcChangeFrequency(AGITATION_STEP_PIN, 10000, LEDC_RESOLUTION);
    digitalWrite(AGITATION_DIR_PIN, DOWN);
    ledcWrite(AGITATION_STEP_PIN, 128);
    delay(AGITATION_TIME);
  }

  //finished agitating, turn the motor off
  ledcWrite(AGITATION_STEP_PIN, 0);
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

int startHoming(void) {
  // //move agitation motor up until the switch gets hit
  // digitalWrite(AGITATION_DIR_PIN, UP);  //its either 1 or 0, we are only going rightwards
  // ledcChangeFrequency(AGITATION_STEP_PIN, 5000, LEDC_RESOLUTION);
  // ledcWrite(AGITATION_STEP_PIN, 128);
  // delay(200);
  // while (!digitalRead(AGITATION_SWITCH));  //when pin goes high, the switch has been triggered

  // //turn agitation motor off
  // ledcWrite(AGITATION_STEP_PIN, 0);
  // delay(100);

  // //set the agitation motor down a little bit
  // digitalWrite(AGITATION_DIR_PIN, DOWN);  //its either 1 or 0, we are only going rightwards
  // ledcChangeFrequency(AGITATION_STEP_PIN, 5000, LEDC_RESOLUTION);
  // ledcWrite(AGITATION_STEP_PIN, 128);
  // delay(700);

  //turn agitation motor off
  ledcWrite(AGITATION_STEP_PIN, 0);
  delay(100);

  //move vertical motor back up to set destination
  digitalWrite(VERTICAL_DIR_PIN, UP);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(VERTICAL_STEP_PIN, 700, LEDC_RESOLUTION);
  ledcWrite(VERTICAL_STEP_PIN, 128);
  delay(1500);

  //turn vertical motor off
  ledcWrite(VERTICAL_STEP_PIN, 0);
  delay(100);

  //move horizontal motor left until the switch gets hit
  digitalWrite(HORIZONTAL_DIR_PIN, LEFT);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(HORIZONTAL_STEP_PIN, 300, LEDC_RESOLUTION);
  ledcWrite(HORIZONTAL_STEP_PIN, 128);
  while (digitalRead(HORIZONTAL_SWITCH))
    ;  //when pin goes high, the switch has been triggered

  //turn horizontal motor off
  ledcWrite(HORIZONTAL_STEP_PIN, 0);

  //move vertical motor down until switch gets hit
  digitalWrite(VERTICAL_DIR_PIN, DOWN);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(VERTICAL_STEP_PIN, 500, LEDC_RESOLUTION);
  ledcWrite(VERTICAL_STEP_PIN, 128);
  while (digitalRead(VERTICAL_SWITCH))
    ;

  //turn vertical motor off
  ledcWrite(VERTICAL_STEP_PIN, 0);
  delay(100);

  //move vertical motor back up to set destination
  digitalWrite(VERTICAL_DIR_PIN, UP);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(VERTICAL_STEP_PIN, 700, LEDC_RESOLUTION);
  ledcWrite(VERTICAL_STEP_PIN, 128);
  delay(900);

  //turn vertical motor off
  ledcWrite(VERTICAL_STEP_PIN, 0);
  delay(100);

  //move horizontal motor right until aligned with tray
  digitalWrite(HORIZONTAL_DIR_PIN, RIGHT);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(HORIZONTAL_STEP_PIN, 300, LEDC_RESOLUTION);
  ledcWrite(HORIZONTAL_STEP_PIN, 128);
  delay(1080);

  //turn horizontal motor off
  ledcWrite(HORIZONTAL_STEP_PIN, 0);

  delay(2000);
  return 1;
}

uint8_t startMagnetizing(void) {
  //move vertical motor back up to set destination
  digitalWrite(VERTICAL_DIR_PIN, DOWN);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(VERTICAL_STEP_PIN, 300, LEDC_RESOLUTION);
  ledcWrite(VERTICAL_STEP_PIN, 128);
  delay(1500);
  ledcWrite(VERTICAL_STEP_PIN, 0);
  return 1;
}

uint8_t shiftRight(void) {
  //move vertical motor back up and overshoot a little bit
  digitalWrite(VERTICAL_DIR_PIN, UP);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(VERTICAL_STEP_PIN, 300, LEDC_RESOLUTION);
  ledcWrite(VERTICAL_STEP_PIN, 128);
  delay(2000);

  //turn vertical motor off
  ledcWrite(VERTICAL_STEP_PIN, 0);
  delay(100);

  //move horizontal motor right until aligned with tray
  digitalWrite(HORIZONTAL_DIR_PIN, RIGHT);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(HORIZONTAL_STEP_PIN, 300, LEDC_RESOLUTION);
  ledcWrite(HORIZONTAL_STEP_PIN, 128);
  delay(190);

  //turn horizontal motor off
  ledcWrite(HORIZONTAL_STEP_PIN, 0);

  //move vertical motor back down to reverse overshoot
  digitalWrite(VERTICAL_DIR_PIN, DOWN);  //its either 1 or 0, we are only going rightwards
  ledcChangeFrequency(VERTICAL_STEP_PIN, 300, LEDC_RESOLUTION);
  ledcWrite(VERTICAL_STEP_PIN, 128);
  delay(500);

  //turn vertical motor off
  ledcWrite(VERTICAL_STEP_PIN, 0);
  delay(100);

  return 1;
}



/*
1.) homing
2.) magnetize the beads: surface pause time (1-9), number of segments (1-5), segment time (1-99), repeat (1-9), magnet speed (1-9) 

*/