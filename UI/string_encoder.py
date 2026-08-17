"""
Protocol string encoder — Python port of protocol_prototype.cpp
Builds fixed-width strings for Terry's parseProtocol() to decode.
"""

def encode_agitation(speed, duration, volume, percent_volume, pausetime, repeats):
    return f"A{speed:01d}{duration:02d}{volume:04d}{percent_volume:02d}{pausetime:02d}{repeats:02d}"

def encode_moving(initial_surface_time, speed, stop_at_sequences, sequence_pause_time):
    return f"M{initial_surface_time:03d}{speed:01d}{stop_at_sequences:01d}{sequence_pause_time:02d}"

def encode_pausing(duration):
    return f"P{duration:01d}"


if __name__ == "__main__":
    # Quick sanity check against real strings from full_protocol.ino's protocolInstructions[]
    test1 = encode_agitation(4, 12, 1100, 50, 12, 2)
    print(test1, "==", "A4121100501202", "->", test1 == "A4121100501202")

    test2 = encode_moving(60, 1, 1, 99)
    print(test2, "==", "M0601199", "->", test2 == "M0601199")