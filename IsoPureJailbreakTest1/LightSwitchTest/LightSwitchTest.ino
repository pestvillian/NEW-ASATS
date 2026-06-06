 

const int sensorPin = 7; // 

void setup() {
  pinMode(sensorPin, INPUT_PULLUP); // 10Kohm pullup resistor going from output pin of sensor to 5V
  Serial.begin(9600); //baudrate for serial terminal
}

void loop() {
  int state = digitalRead(sensorPin);
  Serial.println(state);
  if(!state){ // when the sensor is not tripped it outputs a 1 because we are in pullup configuration
    Serial.println("SENSOR TRIPPED: ");
    Serial.println(state);
  }else{
   
  }
  delay(100); //wait a little bit
}