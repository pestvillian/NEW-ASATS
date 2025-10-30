#include <AccelStepper.h>

const int MOTOR_Y_STEP = 19;  //19
const int MOTOR_Y_DIR = 18;   // 18

const int MOTOR_X_STEP = 21;  //19
const int MOTOR_X_DIR = 20;   // 18

const int AGITATION_STEP = 23;  //19
const int AGITATION_DIR = 22;   // 18

bool dir = HIGH;
AccelStepper MOTORY(AccelStepper::DRIVER, MOTOR_Y_STEP, MOTOR_Y_DIR);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(MOTOR_Y_STEP, OUTPUT);
  pinMode(MOTOR_X_STEP,OUTPUT);
  pinMode(AGITATION_STEP,OUTPUT);

  // //attach channel
  ledcAttachChannel(MOTOR_Y_STEP, 1000, 8, 4);
  ledcAttachChannel(MOTOR_X_STEP, 1000, 8, 5);
  ledcAttachChannel(AGITATION_STEP, 1000, 8, 3);

  // //writeFrequency
  ledcWriteTone(MOTOR_Y_STEP, 700);
  ledcWriteTone(MOTOR_X_STEP, 700);
  ledcWriteTone(AGITATION_STEP, 9000);


  digitalWrite(MOTOR_Y_DIR, HIGH);
  digitalWrite(MOTOR_X_DIR, HIGH);
  digitalWrite(AGITATION_DIR, HIGH);

  // while (1) {
  //   //write direction
  //   digitalWrite(MOTOR_Y_DIR, dir);
  //   digitalWrite(MOTOR_X_DIR, dir);
  //   digitalWrite(AGITATION_DIR, dir);
    
  //   //vertical
  //   ledcWriteTone(MOTOR_Y_STEP, 700);
  //   delay(500);
  //   ledcWriteTone(MOTOR_Y_STEP, 0);
  //   delay(500);
  //   //horizontal
  //   ledcWriteTone(MOTOR_X_STEP, 700);
  //   delay(500);
  //   ledcWriteTone(MOTOR_X_STEP, 0);
  //   delay(500);
  //   //Agitate
  //   ledcWriteTone(AGITATION_STEP, 700);
  //   delay(500);
  //   ledcWriteTone(AGITATION_STEP, 0);
  //   delay(500);

  //   //update direction
  //   if(dir == HIGH){
  //     dir = LOW;
  //   } else{
  //     dir = HIGH;
  //   }

  // }



  // MOTORY.setSpeed(700);
  // MOTORY.setAcceleration(900);

  // MOTORY.move(600000);
}

void loop() {
  // put your main code here, to run repeatedly:

  // MOTORY.run();
}
