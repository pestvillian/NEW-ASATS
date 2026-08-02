/*
 * Protocol String Encoder/Decoder
 *
 * Covers all 3 step types: AGITATION ('B'), MOVING ('M'), PAUSING ('P').

 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <stdio.h>
#include "protocol_prototype.h"
// ---- AGITATION ----
/**/
void encodeAgitation(const AgitationStep& s, char* out, size_t outSize) {
    int written = snprintf(out, outSize, "B%0*d%0*d%0*d%0*d%0*d%0*d",
        B_W_SPEED,         s.speed,
        B_W_DURATION,      s.duration,
        B_W_VOLUME,        s.volume,
        B_W_PERCENTVOLUME, s.percentVolume,
        B_W_PAUSETIME,     s.pausetime,
        B_W_REPEATS,       s.repeats
    );
    assert(written > 0 && (size_t)written < outSize);
}
/**/
AgitationStep decodeAgitation(const char* str) {
    AgitationStep s{};
    int pos = 1; // skip 'B'
    auto readField = [&](int width) -> int {
        char buf[8];
        strncpy(buf, str + pos, width);
        buf[width] = '\0';
        pos += width;
        return atoi(buf);
    };
    s.speed         = readField(B_W_SPEED);
    s.duration      = readField(B_W_DURATION);
    s.volume        = readField(B_W_VOLUME);
    s.percentVolume = readField(B_W_PERCENTVOLUME);
    s.pausetime     = readField(B_W_PAUSETIME);
    s.repeats       = readField(B_W_REPEATS);
    return s;
}

// ---- MOVING ----
/**/
void encodeMoving(const MovingStep& s, char* out, size_t outSize) {
    int written = snprintf(out, outSize, "M%0*d%0*d%0*d%0*d",
        M_W_INITSURFACETIME, s.initialSurfaceTime,
        M_W_SPEED,           s.speed,
        M_W_STOPATSEQUENCES, s.stopAtSequences,
        M_W_SEQUENCEPAUSE,   s.sequencePauseTime
    );
    assert(written > 0 && (size_t)written < outSize);
}
/**/
MovingStep decodeMoving(const char* str) {
    MovingStep s{};
    int pos = 1; // skip 'M'
    auto readField = [&](int width) -> int {
        char buf[8];
        strncpy(buf, str + pos, width);
        buf[width] = '\0';
        pos += width;
        return atoi(buf);
    };
    s.initialSurfaceTime = readField(M_W_INITSURFACETIME);
    s.speed               = readField(M_W_SPEED);
    s.stopAtSequences     = readField(M_W_STOPATSEQUENCES);
    s.sequencePauseTime   = readField(M_W_SEQUENCEPAUSE);
    return s;
}

// ---- PAUSING ----
/**/
void encodePausing(const PausingStep& s, char* out, size_t outSize) {
    int written = snprintf(out, outSize, "P%0*d",
        P_W_DURATION, s.duration
    );
    printf("encodePausing: written=%d, outSize=%zu\n", written, outSize);//debug
    assert(written > 0 && (size_t)written < outSize); //assertion failed.
   
}
/**/
PausingStep decodePausing(const char* str) {
    PausingStep s{};
    int pos = 1; // skip 'P'
    auto readField = [&](int width) -> int {
        char buf[8];
        strncpy(buf, str + pos, width);
        buf[width] = '\0';
        pos += width;
        return atoi(buf);
    };
    s.duration = readField(P_W_DURATION);
    return s;
}