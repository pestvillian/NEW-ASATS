/************
WHAT THIS CODE DOES, nvm its outdated
This is a simple test for sending data from the STM32 to the ESP32 via UART.
The STM32 sends two lines of data. The first line is a title and the second is the data.
The data is formatted as step type, # of repeats, mix speed, mix duration and dry time.
For example, "B9954" is bind step, 9 repeats, speed of 9, mix duration of 5, and dry time of 4.
The first line is ignored by using the startFlag and checking for a newline.
The startFlag is reset after the second line is terminated by the tab character.
*************/
const int STEP_SIGNAL = 12;
const int DIR_SIGNAL = 13;

#define TYPE_IN_PERIOD_HERE 5000  //this is in hz?
#define MAX_LINE_LENGTH 32
#define MAX_LINES 100

void setup() {
  //Set up UART and GPIO
  Serial.begin(115200);
  pinMode(STEP_SIGNAL, OUTPUT);  // Set the pin as an output
  pinMode(DIR_SIGNAL, OUTPUT);   // Set the pin as an output
}

// Initialize the tempBuffer with nulls ('\0')
char tempBuffer[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } };
//memset(tempBuffer, 0, sizeof(tempBuffer));
uint8_t startFlag = 0;
uint8_t i = 0;
uint8_t j = 0;
analogWriteResolution(STEP_SIGNAL, 8);
analogWriteResolution(DIR_SIGNAL, 8);

void loop() {
  // Check if data is available to read from UART
  if (Serial.available()) {
    // Read the incoming byte
    char incomingByte = Serial.read();
    tempBuffer[i][j] = incomingByte;
    j++;
    //tab indicates the end of a protocol. reset startFlag for next protocol
    //ignore the first line
    if (incomingByte == '\n') {
      tempBuffer[i][j] = '\0';
      i++;
      j = 0;
      //startFlag = 1;
      //Serial.println("Newline");
    }
    if (incomingByte == '\t') {
      tempBuffer[i][j] = '\0';
      Serial.println(tempBuffer[0]);
      Serial.println(tempBuffer[1]);
      i = 0;
      j = 0;
      //handle pause
      if (tempBuffer[1][0] == 'P') {
        pauseMotors(tempBuffer[1][1]);
      }
      //handle binding
      if (tempBuffer[1][0] == 'B') {
        beingBinding(tempBuffer[1][1], tempBuffer[1][2]);
      }
      Serial.println("Debug: Finished Protocol");
      //reset tempBuffer for next protocol
      //memset(tempBuffer, 0, sizeof(tempBuffer));
    }
  }
}

//pause the motor for specified time in milliseconds
void pauseMotors(char input_duration) {
  int intDuration = input_duration - '0';  //convert from ascii to int
  analogWrite(STEP_SIGNAL, 255);           //no pwm signal, use high for debugging
  delay(intDuration * 1000);               //convert from seconds to milliseconds for delay function
  analogWrite(STEP_SIGNAL, 0);             //drive back low for debugging
}

//this shit was too confusing. first input is step type, second is length in seconds, third is mix speed
void startAgitating(unsigned long mix_time, uint8_t mix_speed) {
  //ascii to int conversions
  int int_mix_time = -'0';   //convert from ascii to int
  int int_mix_speed = -'0';  //convert from ascii to int

  analogWriteFrequency(STEP_SIGNAL, int_mix_speed * 1000);
  analogWrite(STEP_SIGNAL, 128);
  analogWriteFrequency(DIR_SIGNAL, int_mix_speed);  // idk how often to change direction
  analogWrite(DIR_SIGNAL, 128);
  delay(mix_time * 1000);
}

//handles the entire process of binding using delays
void startDrying(unsigned long dry_time) {
  //ascii to int conversions
  int int_dry_time = -'0';  //convert from ascii to int

  analogWrite(STEP_SIGNAL, 0);
  delay(mix_time * 100);  //max dry time is 900 milliseconds
}
