#pragma once

#include <Arduino.h>

// -----------------------------------------------------------------------------
// Device identity
// -----------------------------------------------------------------------------

static const char* DEVICE_ID = "controlhub1-pod1";
static const char* FIRMWARE_VERSION = "4.0.1-deviceHeaterFan";

// -----------------------------------------------------------------------------
// Station Wi-Fi: connects to the Pi's AP for MQTT (brought back in 4.0.0,
// alongside — not instead of — the local AP below). Change these two to
// match the Pi 5 POD 1 access point exactly.
// -----------------------------------------------------------------------------

// static const char* WIFI_SSID = "POD 1 wifi";
// static const char* WIFI_PASSWORD = "12345678";

static const char* WIFI_SSID = "Tanakun";
static const char* WIFI_PASSWORD = "REDACTED-put-your-hotspot-password-here";

// Testing without the Pi: point this at your PC's IP while it runs Mosquitto
// on the same Wi-Fi network (see comment above). Revert to the Pi's AP
// address ("192.168.50.1") once testing against the real Pi again.
static const char* MQTT_HOST = "10.213.238.217";
static const uint16_t MQTT_PORT = 1883;

static const char* MQTT_COMMAND_TOPIC = "zeep/pod1/controlhub1/command";
static const char* MQTT_STATUS_TOPIC = "zeep/pod1/controlhub1/status";
static const char* MQTT_EVENT_TOPIC = "zeep/pod1/controlhub1/event";

static const uint32_t WIFI_RETRY_MS = 10000;
static const uint32_t MQTT_RETRY_MS = 5000;
static const uint32_t STATUS_INTERVAL_MS = 30000;

// -----------------------------------------------------------------------------
// Local control panel: ESP32 also hosts its own AP + web server (buttons +
// OTA update at /update), so the device is reachable even without the Pi's
// Wi-Fi in range. Runs alongside the station connection above (AP_STA mode
// — the ESP32 has a single radio, so the AP and station share one Wi-Fi
// channel; this is automatic and does not need any configuration here).
// -----------------------------------------------------------------------------

static const char* AP_SSID = "deviceHeaterFan-AP-test";
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
