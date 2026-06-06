#include <AccelStepper.h>

//#include <SoftwareSerial.h>

// --- Pin Assignments ---
#define HORIZONTAL_STEP 5
#define HORIZONTAL_DIR 4
#define MAGNET_STEP 2
#define MAGNET_DIR 3
#define COMB_STEP 19
#define COMB_DIR 18
//channels of futures past


//enable pin of motherboard
#define MOTOR_ENABLE 22

// --- Create Stepper Instances ---
AccelStepper HORIZONTAL(AccelStepper::DRIVER, HORIZONTAL_STEP, HORIZONTAL_DIR);
AccelStepper MAGNET(AccelStepper::DRIVER, MAGNET_STEP, MAGNET_DIR);
AccelStepper COMB(AccelStepper::DRIVER, COMB_STEP, COMB_DIR);

// logic for switchs
bool horizontalTriggered = false;
bool magnetTriggered = false;
bool combTriggered = false;

bool homingX = true;   //home horizontal init
bool homingM = false;  //home megnet init
bool homingC = false;  //hom comb init
//fuck UART
String incomingMessage = "";  //for data from esp


void setup() {

  // Serial.begin(9600,SERIAL_8N1, 30,31); //changing pin numbers for rx tx
  Serial.begin(9600);

  delay(500);

  // Enable motors
  pinMode(MOTOR_ENABLE, OUTPUT);
  digitalWrite(MOTOR_ENABLE, LOW);  // LOW = enabled
  //direction Pins
  pinMode(HORIZONTAL_DIR, OUTPUT);
  pinMode(MAGNET_DIR, OUTPUT);
  pinMode(COMB_DIR, OUTPUT);
  //step pin
  pinMode(HORIZONTAL_STEP, OUTPUT);
  pinMode(MAGNET_STEP, OUTPUT);
  pinMode(COMB_STEP, OUTPUT);

  pinMode(7, INPUT_PULLUP);   //horizontal switch
  pinMode(6, INPUT_PULLUP);   //magnet switch
  pinMode(15, INPUT_PULLUP);  //comb switch
  // // Enable outputs
  HORIZONTAL.enableOutputs();
  MAGNET.enableOutputs();
  COMB.enableOutputs();
  // Setup Agitation Motor Step Signal
  Serial.println("Hello world");
  
  
  HORIZONTAL.setMaxSpeed(700);
  HORIZONTAL.setAcceleration(9999);
  HORIZONTAL.moveTo(-200);  //one rev = 200

  MAGNET.setMaxSpeed(800);
  MAGNET.setAcceleration(9999);
  MAGNET.moveTo(200);  //one rev = 200

  COMB.setMaxSpeed(1000);
  COMB.setAcceleration(999999);
  COMB.moveTo(-1600);  //one rev = 1600
}
//200 full
//400 half
//800 quarter
//1600 eighth
//3200 sixteenth
void loop(){
  //COMB.run();//run the comb motor one revolution. one revolution is 20mm
  MAGNET.run(); //5mm
  //HORIZONTAL.run();
}

