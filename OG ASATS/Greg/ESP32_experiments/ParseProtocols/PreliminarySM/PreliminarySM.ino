
//types of protocols
enum ProtocolType {
  AGITATION,  // 'B'
  PAUSING,    // 'P'
  MOVING,      // 'M'
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

char *protocols[] = {
  "M827395", "B491762", "P3", "M615829", "B728341", "P7", "B983251", "M147693", "P5", "M283741",
  "B672849", "P2", "M518392", "B739251", "P9", "M924873", "B356721", "P4", "M837462", "B671285",
  "P6", "B924173", "M462839", "P8", "M183627", "B519284", "P1", "B832749", "M746218", "P7",
  "M382615", "B721493", "P5", "B629471", "M548293", "P9", "M192847", "B364927", "P2", "M847362",
  "B512874", "P6", "B937415", "M284759", "P4", "M682391", "B715428", "P3", "B492671", "M531827",
  "P8", "M914275", "B267843", "P5", "B839472", "M362491", "P1", "M472681", "B619427", "P9",
  "B738429", "M294783", "P6", "M483729", "B175392", "P3", "B926471", "M571839", "P7", "M318472",
  "B827149", "P2", "B571382", "M649273", "P8", "M417629", "B385291", "P4", "B927416", "M518734",
  "P6", "M742193", "B618492", "P1", "B574829", "M392847", "P9", "M673284", "B941752", "P5",
  "B738192", "M512679", "P7", "M294678", "B415926", "P3", "B689472", "M471382", "P2", "M527491",
  "X123456",  // Invalid protocol (invalid first character 'X')
  "Z9"        // Invalid protocol (invalid first character 'Z')
};
//global size of protocols
int size = sizeof(protocols) / sizeof(protocols[0]);  // Get number of elements


void setup() {
  Serial.begin(115200);
  //loop through array of protocols
  for (int i = 0; i < size; i++) {
    Protocol parsed = parseProtocol(protocols[i]);  // Parse protocol
    
    //check to ensure valid protocols
    if (parsed.type == INVALID) {
      Serial.println("Skipping invalid protocol...");
      continue;  // Skip processing this invalid entry
    }
  //print out list
    Serial.print("Protocol: ");
    Serial.println(protocols[i]);
    //for each parsed protocol print out its information based on type
    switch (parsed.type) {
      case AGITATION:
        Serial.println("Type: Agitation");
        Serial.print("Volume: "); Serial.println(parsed.volume);
        Serial.print("Percent Volume: "); Serial.println(parsed.percentVolume);
        Serial.print("Speed: "); Serial.println(parsed.speed);
        Serial.print("Duration: "); Serial.println(parsed.duration);
        break;

      case PAUSING:
        Serial.println("Type: Pausing");
        Serial.print("Duration: "); Serial.println(parsed.duration);
        break;

      case MOVING:
        Serial.println("Type: Moving");
        Serial.print("Initial Surface Time: "); Serial.println(parsed.initialSurfaceTime);
        Serial.print("Speed: "); Serial.println(parsed.speed);
        Serial.print("Stop at Sequences: "); Serial.println(parsed.stopAtSequences);
        Serial.print("Sequence Pause Time: "); Serial.println(parsed.sequencePauseTime);
        break;
    }

    Serial.println("-----------------------------");
  }
}




void loop() {
  //Serial.println("Hello World");

  delay(1000);  // Add a delay to avoid spamming the Serial Monitor
}
//function to parse thorugh the protocols
Protocol parseProtocol(char *protocol) {
  Protocol parsed; //object of protocol struct with the elements of all protocols contained
  parsed.type = getProtocolType(protocol);
  //for each type extract its respective information
  switch (parsed.type) {
    case AGITATION:
      parsed.volume = (protocol[1] - '0');            // Extract volume
      parsed.percentVolume = ((protocol[2] - '0') * 10) + (protocol[3] - '0');  // % of volume
      parsed.speed = (protocol[4] - '0');            // Speed
      parsed.duration = ((protocol[5] - '0') * 10) + (protocol[6] - '0');  // Duration
      break;

    case PAUSING:
      parsed.duration = (protocol[1] - '0');  // Only duration for pausing
      break;

    case MOVING:
      parsed.initialSurfaceTime = ((protocol[1] - '0') * 100) + ((protocol[2] - '0') * 10) + (protocol[3] - '0');  // Initial surface time
      parsed.speed = (protocol[4] - '0');  // Speed
      parsed.stopAtSequences = (protocol[5] - '0');  // Stop at sequences
      parsed.sequencePauseTime = (protocol[6] - '0');  // Sequence pause time
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
