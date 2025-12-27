const byte numChars = 32;
char receivedChars[numChars]; // An array to store the received data
boolean newData = false;

void setup() {
  // Initialize serial communication at 9600 baud rate (common for UART)
  Serial.begin(9600); 
  Serial.println("<Arduino is ready>");
  Serial.println("Send text ending with a newline character in the Serial Monitor.");
}

void loop() {
  // Call the receiving function continuously
  recvWithEndMarker(); 
  // Process the data once a complete message is received
  showNewData();
}

void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\r\n'; // The character that marks the end of the message
  char rc;

  // Only read if data is available and we haven't already marked new data
  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (rc != endMarker) {
      // Store the character if it's not the end marker
      receivedChars[ndx] = rc;
      ndx++;
      // Prevent buffer overflow
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    } else {
      // Terminate the C-style string with a null character
      receivedChars[ndx] = '\0'; 
      // Reset the index for the next message
      ndx = 0;
      // Set the flag to true to indicate a complete message is ready
      newData = true;
    }
  }
}

void showNewData() {
  if (newData == true) {
    Serial.print("Received data: ");
    Serial.println(receivedChars);
    // Reset the flag so a new message can be received
    newData = false; 
  }
}
