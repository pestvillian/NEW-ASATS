const int VERTICAL_STEP_PIN = 27;
const int VERTICAL_DIR_PIN = 14;

const int HORIZONTAL_STEP_PIN = 25;
const int HORIZONTAL_DIR_PIN = 26;  //dorjee, are horizontal and vertical flipped?

void setup() {
  Serial.begin(115200);

  //vertical motor
  pinMode(VERTICAL_DIR_PIN, OUTPUT);  // Set the pin as an output
  ledcAttachChannel(VERTICAL_STEP_PIN, 100, 8, 9);
  ledcWrite(VERTICAL_STEP_PIN, 0);

  //horizontal motor
  pinMode(HORIZONTAL_DIR_PIN, OUTPUT);  // Set the pin as an output
  digitalWrite(HORIZONTAL_DIR_PIN, 1);  //its either 1 or 0, we are only going rightwards
  ledcAttachChannel(HORIZONTAL_STEP_PIN, 100, 8, 10);
  ledcWrite(HORIZONTAL_STEP_PIN, 0);

  //move vertical motor up
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
}

void loop() {
}