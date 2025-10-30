const int digitalPin = 13;  // digital input
const int analogPin = 12;   // analog output

void setup() {
  pinMode(analogPin, OUTPUT);  // Set analog pin as output
  pinMode(digitalPin, INPUT);  // Set digital pin as input
  Serial.begin(115200);        // For debugging
}

void loop() {
  uint8_t inputState = digitalRead(digitalPin);

  // if (inputState == HIGH) {
  //   // Output approx. 2.5V via PWM
  //   // For 3.3V max, 2.5V ≈ (2.5 / 3.3) * 255 ≈ 193
  //   analogWrite(analogPin, 193);
  // } else {
  //   analogWrite(analogPin, 0); // 0V
  // }
  analogWrite(analogPin,195); //out puts a solid 2.5 somthing volts


  Serial.print("Analog Value: ");
  Serial.println(inputState);

  // Serial.print("Voltage: ");
  // Serial.println(voltage);

  Serial.print("Digital Pin: ");
  if (inputState == HIGH) {
    Serial.println("HIGH");
  } else {
    Serial.println("LOW");
  }

  delay(1000);
}