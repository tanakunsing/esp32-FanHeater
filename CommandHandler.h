#pragma once

#include <Arduino.h>

// Dispatch layer: parses a text command (from Serial or MQTT) and drives
// RelayControl, then reports the result back through MqttHandler.

extern uint32_t commandCounter;

bool executeHeaterCommand(String command, bool fromMqtt);
