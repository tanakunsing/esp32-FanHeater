#pragma once

#include <Arduino.h>

// Network layer: Wi-Fi (station) + MQTT connection lifecycle and JSON
// status/event reporting. Sets AP_STA mode so the local AP (WebPortal.cpp)
// keeps working alongside this station connection.

void mqttSetup();
void startWifi();
void maintainWifi();
void maintainMqtt();
void mqttService();
bool mqttIsConnected();
void publishStatus(bool online);
void publishCommandEvent(const String& command, bool ok, const char* detail);
