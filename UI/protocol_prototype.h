/*
 * Protocol String Encoder/Decoder
 *
 * Covers all 3 step types: AGITATION ('B'), MOVING ('M'), PAUSING ('P').

 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>

// AGITATION ('B') — 14 chars total: 1 (type) + 1+2+3+3+2+2
constexpr int B_W_SPEED         = 1; // protocol[1]
constexpr int B_W_DURATION      = 2; // protocol[2:4]
constexpr int B_W_VOLUME        = 3; // protocol[4:7]
constexpr int B_W_PERCENTVOLUME = 3; // protocol[7:10]
constexpr int B_W_PAUSETIME     = 2; // protocol[10:12]
constexpr int B_W_REPEATS       = 2; // protocol[12:14]

// MOVING ('M') — 8 chars total: 1 (type) + 3+1+1+2
constexpr int M_W_INITSURFACETIME = 3; // protocol[1:4]
constexpr int M_W_SPEED           = 1; // protocol[4]
constexpr int M_W_STOPATSEQUENCES = 1; // protocol[5]
constexpr int M_W_SEQUENCEPAUSE   = 2; // protocol[6:8]

// PAUSING ('P') — 2 chars total: 1 (type) + 1
constexpr int P_W_DURATION = 1; // protocol[1]

struct AgitationStep {
    int speed;
    int duration;
    int volume;
    int percentVolume;
    int pausetime;
    int repeats;
};


struct MovingStep {
    int initialSurfaceTime;
    int speed;
    int stopAtSequences;
    int sequencePauseTime;
};


struct PausingStep {
    int duration;
};

AgitationStep decodeAgitation(const char* str);
MovingStep decodeMoving(const char* str);
PausingStep decodePausing(const char* str);

void encodeAgitation(const AgitationStep& s, char* out, size_t outSize);
void encodeMoving(const MovingStep& s, char* out, size_t outSize);
void encodePausing(const PausingStep& s, char* out, size_t outSize); 
