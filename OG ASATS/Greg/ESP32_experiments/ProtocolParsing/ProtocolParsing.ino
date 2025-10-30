#define RXD2 18  // UART1 RX Pin
#define TXD2 17  // UART1 TX Pin
#define MAX_LINE_LENGTH 32
#define MAX_LINES 100
// Initialize the tempBuffer with nulls ('\0')
char tempBuffer[MAX_LINES][MAX_LINE_LENGTH] = { { '\0' } };
//memset(tempBuffer, 0, sizeof(tempBuffer));
uint8_t startFlag = 0;
uint8_t i = 0;
uint8_t j = 0;

HardwareSerial mySerial(1);
//types of protocols
enum ProtocolType {
  AGITATION,  // 'B'
  PAUSING,    // 'P'
  MOVING,     // 'M'
  INVALID     // For unknown protocol types
};
// Structure to store protocol properties
struct Protocol {
  ProtocolType type;
  int volume;
  int percentVolume;
  int speed;
  int duration;
  int initialSurfaceTime;
  int stopAtSequences;
  int sequencePauseTime;
};

//Protocol Array
char *protocols[] = {};


//global size of protocols
int size = sizeof(protocols) / sizeof(protocols[0]);  // Get number of elements

//init functions
Protocol parseProtocol(char *protocol);
ProtocolType getProtocolType(char *protocol);


void setup() {
  mySerial.begin(11520, SERIAL_8N1, 18, 17);
  Serial.begin(115200);
  //Serial.begin(115200, SERIAL_8N1, RXD2, TXD2);  // UART1 Configuration
  Serial.println("hello world");


  //loop through array of protocols
  for (int i = 0; i < size; i++) {
    Protocol parsed = parseProtocol(protocols[i]);  // Parse protocol

    //print out list
    Serial.print("Protocol: ");
    Serial.println(protocols[i]);
    //for each parsed protocol print out its information based on type
    switch (parsed.type) {
      case AGITATION:
        Serial.println("Type: Agitation");
        Serial.print("Volume: ");
        Serial.println(parsed.volume);
        Serial.print("Percent Volume: ");
        Serial.println(parsed.percentVolume);
        Serial.print("Speed: ");
        Serial.println(parsed.speed);
        Serial.print("Duration: ");
        Serial.println(parsed.duration);
        break;

      case PAUSING:
        Serial.println("Type: Pausing");
        Serial.print("Duration: ");
        Serial.println(parsed.duration);
        break;

      case MOVING:
        Serial.println("Type: Moving");
        Serial.print("Initial Surface Time: ");
        Serial.println(parsed.initialSurfaceTime);
        Serial.print("Speed: ");
        Serial.println(parsed.speed);
        Serial.print("Stop at Sequences: ");
        Serial.println(parsed.stopAtSequences);
        Serial.print("Sequence Pause Time: ");
        Serial.println(parsed.sequencePauseTime);
        break;
  }

  //Serial.println("-----------------------------");
  //}
}



//uart
void loop() {
  // Check if data is available to read from UART
  if (mySerial.available()) {
    // Read the incoming byte
    String data = mySerial.readString();
    Serial.print(data);


    char lastchar = data.charAt(data.length() - 1);
    //tab indicates the end of a protocol. reset startFlag for next protocol
    if (lastchar == '/t') {
      for (int i = 0; i < MAX_LINES; i++) {

          //check incoming dadta
        
        strcpy(protocols[i], tempBuffer[i]);
        Serial.println(protocols[i]);
        Serial.println(tempBuffer[i]);

        //reset tempBuffer for next protocol
      }
    }
  }
}
/**
 * @brief: iterate through the array and extract all information
 * @param char *protocol : pause duration in seconds
 * @retval: none
 */
Protocol parseProtocol(char *protocol) {
  Protocol parsed;  //object of protocol struct with the elements of all protocols contained
  parsed.type = getProtocolType(protocol);
  //for each type extract its respective information
  switch (parsed.type) {
    case AGITATION:
      parsed.volume = (protocol[1] - '0');                                      // Extract volume
      parsed.percentVolume = ((protocol[2] - '0') * 10) + (protocol[3] - '0');  // % of volume
      parsed.speed = (protocol[4] - '0');                                       // Speed
      parsed.duration = ((protocol[5] - '0') * 10) + (protocol[6] - '0');       // Duration
      break;

    case PAUSING:
      parsed.duration = (protocol[1] - '0');  // Only duration for pausing
      break;

    case MOVING:
      parsed.initialSurfaceTime = ((protocol[1] - '0') * 100) + ((protocol[2] - '0') * 10) + (protocol[3] - '0');  // Initial surface time
      parsed.speed = (protocol[4] - '0');                                                                          // Speed
      parsed.stopAtSequences = (protocol[5] - '0');                                                                // Stop at sequences
      parsed.sequencePauseTime = (protocol[6] - '0');                                                              // Sequence pause time
      break;
  }

  return parsed;
}

// Function to determine protocol type
ProtocolType getProtocolType(char *protocol) {
  //based on the first character of the protocols you can see what type of protocol it is
  switch (protocol[0]) {  // Check the first character
    case 'B': return AGITATION;
    case 'P': return PAUSING;
    case 'M': return MOVING;
    default:
      Serial.print("Error: Invalid protocol type '");
      Serial.print(protocol[0]);
      Serial.println("'");
      return INVALID;
  }
}
