#include "esp32-hal-timer.h"

//**hi dorjee. the bind functiond doesnt have accelerationr rn. you might need it. 4/21/25 night **//

hw_timer_t *timer = NULL;  // declaration of timer
static int timerTicks = 0;

// function call by the timer interruption
void IRAM_ATTR onTimer() {
  timerTicks++;
}

const int AGITATION_STEP_PIN = 12;
const int AGITATION_DIR_PIN = 13;

const int VERTICAL_STEP_PIN = 27;
const int VERTICAL_DIR_PIN = 14;

typedef enum {
  UP = 0,
  DOWN = 1
} verticalDirection;  //i think is ground is up, vdd is down. need to test it


const int HORIZONTAL_STEP_PIN = 25;
const int HORIZONTAL_DIR_PIN = 26;

#define MAX_LINE_LENGTH 32
#define MAX_LINES 100

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
  digitalWrite(AGITATION_DIR_PIN, 0);
  ledcAttachChannel(AGITATION_STEP_PIN, 100, 8, 8);
  ledcWrite(AGITATION_STEP_PIN, 0);

  //vertical motor
  pinMode(VERTICAL_DIR_PIN, OUTPUT);  // Set the pin as an output
  ledcAttachChannel(VERTICAL_STEP_PIN, 100, 8, 9);
  ledcWrite(VERTICAL_STEP_PIN, 0);

  //horizontal motor
  pinMode(HORIZONTAL_DIR_PIN, OUTPUT);  // Set the pin as an output
  digitalWrite(HORIZONTAL_DIR_PIN, 1);  //its either 1 or 0, we are only going rightwards
  ledcAttachChannel(HORIZONTAL_STEP_PIN, 100, 8, 10);
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

  // Configure an alarm to trigger the interrupt every 10 ms (100000 µs)
  timerAlarm(timer, 10000, true, 0);  // 1000000 µs = 10ms = 0.01s

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
      }
      currentState = waitingState;
      memset(tempBuffer, 0, sizeof(tempBuffer));
      //shitty solution to the issue of uart data being sent while motors move
      while (Serial.available()) {
        Serial.read();
      }
      Serial.println("Finished Protocol");
      break;
  }
}

//pause the motor for specified time in milliseconds
void pauseMotors(char input_duration) {
  int intDuration = input_duration - '0';  //convert from ascii to int
  ledcWrite(AGITATION_STEP_PIN, 255);
  delay(intDuration * 1000);         //convert from seconds to milliseconds for delay function
  ledcWrite(AGITATION_STEP_PIN, 0);  //this is just for debugging to see the pause function bring the pin high and then low
}

//three timing constraints: total agitation time, direction switching time, acceleration time
void startAgitating(unsigned long mix_time, uint8_t mix_speed, uint8_t mix_depth) {
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
  ledcChangeFrequency(AGITATION_STEP_PIN, 200, 8);
  ledcWrite(AGITATION_STEP_PIN, 128);

  //stay in this agitation loop until the total agitation time is finished
  while ((millis() - curTime) < mix_time_ms) {
    if (timerTicks == 1) {
      ledcChangeFrequency(12, 200, 8);
    }
    if (timerTicks == 2) {
      ledcChangeFrequency(12, 1000, 8);
    }
    // if (timerTicks == 3) {
    //   ledcChangeFrequency(12, 2000, 8);
    // }
    // if (timerTicks == 4) {
    //   ledcChangeFrequency(12, 3000, 8);
    // }
    // if (timerTicks == 15 - 3) {
    //   ledcChangeFrequency(12, 2000, 8);
    // }
    // if (timerTicks == 15 - 2) {
    //   ledcChangeFrequency(12, 1000, 8);
    // }
    if (timerTicks == 15 - 1) {
      ledcChangeFrequency(12, 200, 8);
    }
    if ((timerTicks == 15) && (!digitalRead(13))) {
      ledcChangeFrequency(12, agitateStepFrequency, 8);
      digitalWrite(AGITATION_DIR_PIN, !digitalRead(13));
      timerTicks = 0;
    }
    if ((timerTicks == 15) && (digitalRead(13))) {
      ledcChangeFrequency(12, agitateStepFrequency, 8);
      digitalWrite(AGITATION_DIR_PIN, !digitalRead(13));
      timerTicks = 0;
    }
  }
  //finished agitating, turn the motor off
  ledcWrite(AGITATION_STEP_PIN, 0);
}

//for now, just move the motor up, to the right, and back down
void beginBinding(char bindDepth) {
  //move vertical motor up
  digitalWrite(VERTICAL_DIR_PIN, 1);  //is this up or down?
  ledcChangeFrequency(VERTICAL_STEP_PIN, 100, 9);
  ledcWrite(VERTICAL_STEP_PIN, 128);
  delay(10);
  ledcChangeFrequency(VERTICAL_STEP_PIN, 300, 9);
  delay(10);
  ledcChangeFrequency(VERTICAL_STEP_PIN, 500, 9);
  delay(10);
  ledcChangeFrequency(VERTICAL_STEP_PIN, 700, 9);
  delay(1000);
  ledcWrite(VERTICAL_STEP_PIN, 0);


  //move horizontal motor right
  ledcChangeFrequency(HORIZONTAL_STEP_PIN, 100, 10);
  ledcWrite(HORIZONTAL_STEP_PIN, 128);
  delay(10);
  ledcChangeFrequency(HORIZONTAL_STEP_PIN, 300, 10);
  delay(10);
  ledcChangeFrequency(HORIZONTAL_STEP_PIN, 500, 10);
  delay(10);
  ledcChangeFrequency(HORIZONTAL_STEP_PIN, 700, 10);
  delay(500);
  ledcWrite(HORIZONTAL_STEP_PIN, 0);

  //move vertical motor down
  digitalWrite(VERTICAL_DIR_PIN, 0);  //is this up or down?
  ledcChangeFrequency(VERTICAL_STEP_PIN, 100, 9);
  ledcWrite(VERTICAL_STEP_PIN, 128);
  delay(10);
  ledcChangeFrequency(VERTICAL_STEP_PIN, 300, 9);
  delay(10);
  ledcChangeFrequency(VERTICAL_STEP_PIN, 500, 9);
  delay(10);
  ledcChangeFrequency(VERTICAL_STEP_PIN, 700, 9);
  delay(1000);
  ledcWrite(VERTICAL_STEP_PIN, 0);
}

int mapSpeedtoFreq(int input_speed) {
  int output_frequency = 820 + (input_speed * 20);  //input speed of 9 maxes out at 10khz
  return output_frequency;
}

int mapDepthtoTime(int input_depth) {
  int output_time = 55 + (input_depth * 5);  //input depth of 9 maxes out at 100ms of movement. min depth is 55ms of movement
  return output_time;
}