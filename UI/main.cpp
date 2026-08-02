#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

#include "protocol_prototype.h"


//test bench for the protocol encoder/decoder
//AGITATION
// void encodeAgitation(const AgitationStep& s, char* out, size_t outSize)

//AgitationStep decodeAgitation(const char* str)

// ---- MOVING ----

//void encodeMoving(const MovingStep& s, char* out, size_t outSize)
//MovingStep decodeMoving(const char* str)

// ---- PAUSING ----

//void encodePausing(const PausingStep& s, char* out, size_t outSize)
//PausingStep decodePausing(const char* str) 



int main() {
    // Test AgitationStep encoding and decoding
    AgitationStep agitationStep{5, 10, 15, 20, 25, 30};
    char agitationBuffer[15];
    encodeAgitation(agitationStep, agitationBuffer, sizeof(agitationBuffer));
    AgitationStep decodedAgitation = decodeAgitation(agitationBuffer);
    
    if(agitationStep.speed == decodedAgitation.speed &&
       agitationStep.duration == decodedAgitation.duration &&
       agitationStep.volume == decodedAgitation.volume &&
       agitationStep.percentVolume == decodedAgitation.percentVolume &&
       agitationStep.pausetime == decodedAgitation.pausetime &&
       agitationStep.repeats == decodedAgitation.repeats) {
        printf("AgitationStep encoding and decoding successful.\n");
    } else {
        printf("AgitationStep encoding and decoding failed.\n");
    }


    // Test MovingStep encoding and decoding
    MovingStep movingStep{100, 5, 1, 10};
    char movingBuffer[9];
    encodeMoving(movingStep, movingBuffer, sizeof(movingBuffer));
    MovingStep decodedMoving = decodeMoving(movingBuffer);
    if(movingStep.initialSurfaceTime == decodedMoving.initialSurfaceTime &&
       movingStep.speed == decodedMoving.speed &&
       movingStep.stopAtSequences == decodedMoving.stopAtSequences &&
       movingStep.sequencePauseTime == decodedMoving.sequencePauseTime) {
        printf("MovingStep encoding and decoding successful.\n");
    } else {
        printf("MovingStep encoding and decoding failed.\n");
    }
    // Test PausingStep encoding and decoding
    PausingStep pausingStep{50};
    char pausingBuffer[3];
    encodePausing(pausingStep, pausingBuffer, sizeof(pausingBuffer));
    PausingStep decodedPausing = decodePausing(pausingBuffer);
    if(pausingStep.duration == decodedPausing.duration) {
        printf("PausingStep encoding and decoding successful.\n");
    } else {
        printf("PausingStep encoding and decoding failed.\n");
    }
    return 0;
}