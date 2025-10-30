unsigned long previousPWM_time = 0;  // Store the last time the pin was toggled
unsigned long previous_dir_time = 0;

const int STEP_SIGNAL = 12;
const int DIR_SIGNAL = 13;
int period = 500;

/** LET'S DO EVERYTHING IN MICRO SECONDS**/

#define HALF_PERIOD_DIR_SIGNAL 2000000  //the actual period is twice this (1 second), so the frequency is 1hz
#define HALF_PERIOD_STEP_SIGNAL 33    //the actual period is twice this (1000us), so the frequency is 1khz
#define TYPE_IN_PERIOD_HERE 10

void setup() {
  Serial.begin(115200);
  pinMode(STEP_SIGNAL, OUTPUT);  // Set the pin as an output
  pinMode(DIR_SIGNAL, OUTPUT);  // Set the pin as an output
                        //digitalWrite(13, 1);
}

void loop() {
  unsigned long currentMicros = micros(); //get current ti me in microseconds

  //This is pin 13, dir signal
  // if (currentMicros - previous_dir_time >= HALF_PERIOD_DIR_SIGNAL) {
  //   previous_dir_time = currentMicros;      // Store the last time the pin was toggled
  //   digitalWrite(DIR_SIGNAL, !digitalRead(DIR_SIGNAL));  // Toggle the pin state
  // }

  // //This is pin 12, step signal
  // int   half_period_step_signal =  1 / (2 * TYPE_IN_PERIOD_HERE); //convert user input frequency to a half period in microseconds
  // //timer event happens on rising and falling edges, so half a period


  //start at 1kHz and accelerate to 20kHz
  if (currentMicros - previousPWM_time >= period) {

    previousPWM_time = currentMicros;    // Store the last time the pin was toggled
    digitalWrite(STEP_SIGNAL, !digitalRead(STEP_SIGNAL));  // Toggle the pin state
  }

  if (currentMicros - previousPWM_time >= 200000) {
    //decrement wait time until period reaches 20kHz
    if (period > 25) {
    period = period - 25;
    } else {
      period = 25;
    }
  }


}