#define STEP_PIN 5
#define DIR_PIN 6

int DIR = HIGH;
int counter = 0;

void setup() {
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    digitalWrite(DIR_PIN, DIR);  // Set initial direction
}

void loop() {
    // counter++;

    // Step pulse
    digitalWrite(STEP_PIN, HIGH);
    delayMicroseconds(71); // 7ms period
    digitalWrite(STEP_PIN, LOW);
    delayMicroseconds(71);

    // Toggle direction every 1000 steps (adjust as needed)
    // if (counter % 1000 == 0) {  
    //     DIR = (DIR == HIGH) ? LOW : HIGH;  // Toggle between HIGH and LOW
    //     digitalWrite(DIR_PIN, DIR);  // Apply new direction
    // }
}