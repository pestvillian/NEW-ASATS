#define LED_PIN 4       // Define the pin connected to the LED as pin 4
#define POT_PIN 15      // Define the pin connected to the potentiometer as pin 15

void setup() {
  pinMode(LED_PIN, OUTPUT);     // Set the LED pin as an output
  Serial.begin(9600);           // Initialize serial communication at a baud rate of 9600
}

void loop() {
  int potValue = analogRead(POT_PIN);        // Read the analog value from the potentiometer (range: 0-4095)
  int brightness = map(potValue, 0, 4095, 0, 255);  // Map the potentiometer value to a brightness level (range: 0-255)
  analogWrite(LED_PIN, brightness);          // Set the LED brightness based on the mapped value
  Serial.println(brightness);                // Print the brightness value to the Serial Monitor for debugging
  delay(10);                                 // Short delay to allow for smooth transitions in brightness
}
