#define STEP_PIN 5
#define DIR_PIN 6

int DIR = HIGH;
int counter = 0;

void setup() {
    pinMode(DIR_PIN, OUTPUT);
    pinMode(STEP_PIN,OUTPUT);
    digitalWrite(DIR_PIN, DIR);  // Set initial direction

    // Set up 7 kHz PWM on STEP_PIN using LEDC hardware PWM
    ledcAttach(STEP_PIN,7000, 8);  // Attach STEP_PIN to channel 0
    //ledcSetup(0, 7000, 8);       // Channel 0, 7 kHz, 8-bit resolution
    ledcWrite(0, 128);           // 50% duty cycle (range 0-255)
}

void loop() {
    counter++;

    // Toggle direction every 10,000 pulses
    if (counter % 10000 == 0) {  
        DIR = (DIR == HIGH) ? LOW : HIGH;
        digitalWrite(DIR_PIN, DIR);
    }
}
