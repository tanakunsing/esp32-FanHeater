#pragma once

#include <Arduino.h>

// Dispatch layer: applies a relay-state JSON object (from the web panel or
// MQTT) to RelayControl.

extern uint32_t commandCounter;

// - power=false forces everything off (heat must never run without the fan).
// - heat2=on forces heat1=on too (no "heat2 alone" state, matches level3).
void applyRelayState(bool power, bool wantHeat1, bool wantHeat2, bool wantSwing);
