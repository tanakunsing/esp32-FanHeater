#pragma once

#include <Arduino.h>

// Parses one line of Serial input as a full relay-state JSON object, e.g.:
//   {"power":"on","heat1":"on","heat2":"off","swing":"off"}
//
// A field that is missing, not a string, or not exactly "on" comes back as
// false. Returns false (and prints why to Serial) only if the line is not
// valid JSON at all.
bool parseRelayStateJson(const String& line, bool& power, bool& heat1, bool& heat2, bool& swing);
