#pragma once

#include <Arduino.h>

// Network layer: Wi-Fi + MQTT connection lifecycle and JSON status/event reporting.

void mqttSetup();
void startWifi();
void maintainWifi();
void maintainMqtt();
void mqttService();
bool mqttIsConnected();
void publishStatus(bool online);
void publishCommandEvent(const String& command, bool ok, const char* detail);
