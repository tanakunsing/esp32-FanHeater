#pragma once

#include <Arduino.h>

// Hardware layer: drives the fan/heater/swing relays and tracks their state.

extern bool fanState;
extern bool heat1State;
extern bool heat2State;
extern bool swingState;

void relayInit();
void fan(bool state);
void heat1(bool state);
void heat2(bool state);
void swing(bool state);
void powerOn();
void powerOff();
void printStatus();
