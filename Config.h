#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Device identity
// -----------------------------------------------------------------------------

static const char* DEVICE_ID = "controlhub1-pod1";
static const char* FIRMWARE_VERSION = "3.0.0-deviceHeaterFan";

// -----------------------------------------------------------------------------
// Local control panel: ESP32 hosts its own AP + web server. This is now the
// ONLY way to control the device — no station Wi-Fi, no MQTT, no Pi
// connectivity at all (removed in 3.0.0).
// -----------------------------------------------------------------------------

static const char* AP_SSID = "deviceHeaterFan-AP";
static const char* AP_PASSWORD = "heaterfan123";

// -----------------------------------------------------------------------------
// Relay configuration (heater box: fan, heat1 1000W, heat2 1500W, swing)
//
// Pin set in use: 16/17/18/19 -> ESP32 Dev Module (plain ESP32, CP2102 USB-UART).
// If switching back to the ESP32-S3 board, use 4/5/6/7 instead and update
// this comment to say "ESP32-S3".
// -----------------------------------------------------------------------------
static const int PIN_SWING = 16;
static const int PIN_FAN   = 17;
static const int PIN_HEAT1 = 18;
static const int PIN_HEAT2 = 19;


// Serial-only full test settings.
static const uint32_t STEP_WAIT_MS = 5000;
