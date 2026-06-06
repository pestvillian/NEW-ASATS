#include <SoftwareSerial.h>

// RX pin 0, TX pin 1
SoftwareSerial mySerial(0, 1); 

const int HORIZONTAL = 7;  //
const int MAGNET = 8;      //j 12 vertical
const int COMB = 9;        // agitation
//middle switch is number 10

void setup() {
  //set switch pins to input pullup
  // 10Kohm pullup resistor going from output pin of sensor to 5V
  pinMode(HORIZONTAL, INPUT_PULLUP);
  pinMode(MAGNET, INPUT_PULLUP);
  pinMode(COMB, INPUT_PULLUP);
  Serial.begin(115200);    // Initialize the Serial monitor for debugging
    // Start software serial for the external device
  mySerial.begin(9600); 
  
}

void loop() {
  int state0 = digitalRead(HORIZONTAL);
  int state1 = digitalRead(MAGNET);
  int state2 = digitalRead(COMB);
  // when the sensor is not tripped it outputs a 1 because we are in pullup configuration
  if (!state0) {  //println delimits the message so the esp can get the full string and not just single bytes

    Serial.println("limHorizontal");  // send signal to computer
    mySerial.println("limHorizontal");//send to esp
  }
  if (!state1) {

    Serial.write("limMagnet");  // send signal to ESP that the MAGNET swithc has been triggered
    mySerial.println("limMagnet");
  }
  if (!state2) {

    Serial.write("limComb");  // send signal to ESP that the MAGNET swithc has been triggered
    mySerial.println("limComb");
  }
  Serial.flush();//
  delay(200);  //wait a little bit
}