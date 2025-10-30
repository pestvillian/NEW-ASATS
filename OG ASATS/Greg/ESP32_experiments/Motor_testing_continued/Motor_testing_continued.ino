#define STEP 10
#define DIR 11


void setup() {
  // put your setup code here, to run once:
  pinMode(STEP, OUTPUT);
  pinMode(DIR,OUTPUT);

  digitalWrite(DIR, HIGH);

}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(STEP,HIGH);
  delay(1000);
  digitalWrite(STEP,LOW);
  delay(1000);


}
