// Define the UART number (e.g., UART1)
HardwareSerial mySerial(1);

void setup() {
  // Start UART communication at 9600 baud rate
  mySerial.begin(115200, SERIAL_8N1, 18, 17); // RX pin = GPIO16, TX pin = GPIO17 (Change as needed)
  
  Serial.begin(115200); // Start serial monitor
  Serial.println("hello world");
}

void loop() {
  if (mySerial.available()) {
    String data = mySerial.readString();
    Serial.println("Received from STM32: " + data);

  }
}