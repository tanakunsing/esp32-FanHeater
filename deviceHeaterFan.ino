/*
  ZEEP deviceHeaterFan / Heater Relay Control / MQTT + AP Web Panel + OTA
  Firmware version: 4.0.0-deviceHeaterFan

  Runs on either board; only the GPIO numbers in Config.h change:
    - ESP32 Dev Module (plain ESP32, CP2102 USB-UART) -> pins 16/17/18/19 (current)
    - ESP32-S3 N16R8 (native USB)                     -> pins 4/5/6/7

  Combines the station Wi-Fi/MQTT link to the Pi (brought back in 4.0.0)
  with the local AP + web panel + OTA update page (from 3.x) at the same
  time, using Wi-Fi AP_STA mode. The ESP32 has one radio, so the AP and
  station share a single Wi-Fi channel automatically — no config needed for
  that, and it doesn't affect clients connecting to the AP.

  Command schema (identical over MQTT and the web panel):
    {"power":"on","heat1":"on","heat2":"off","swing":"off"}
    - power=false (or missing): everything forced off, no matter what else is set.
    - heat2=on forces heat1=on too (no "heat2 alone" state).
    - Any field missing/not exactly "on" is treated as off.

  Relay outputs (Active-High) - see Config.h for the pin numbers in effect:
    Swing motor, Fan relay, Heater 1000W, Heater 1500W

  File layout:
    Config.h          - device identity, Wi-Fi/MQTT + AP credentials, GPIO pins
    RelayControl.*     - hardware layer: drives the relays, tracks their state
    JsonCommand.*       - parses the {"power":...} JSON object
    MqttHandler.*       - station Wi-Fi + MQTT lifecycle, JSON status/event
    WebPortal.*         - AP + web server: buttons and live status
    OtaUpdate.*         - GET/POST /update: browser-based firmware upload
    CommandHandler.*    - dispatch layer: JSON state -> relay actions
    SerialConsole.*     - Serial-only debug console (help/state/test)

  Required Arduino libraries:
    - PubSubClient by Nick O'Leary
    - ArduinoJson

  IMPORTANT:
    - Edit WIFI_SSID/WIFI_PASSWORD in Config.h to match the Pi's AP.
    - MQTT commands must NOT be retained.
    - Never enable Heat1/Heat2 without the fan running (enforced in code).
*/

#include <Arduino.h>
#include "Config.h"
#include "RelayControl.h"
#include "MqttHandler.h"
#include "WebPortal.h"
#include "CommandHandler.h"
#include "SerialConsole.h"

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(1000);

  relayInit();
  mqttSetup();
  startWifi();      // sets AP_STA mode and starts the station connection
  webPortalSetup();  // adds the AP on top (needs AP_STA mode set above)

  printLine();
  Serial.println(F("ZEEP deviceHeaterFan - HEATER RELAY + MQTT + AP WEB PANEL"));
  Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
  Serial.printf("Device ID: %s\n", DEVICE_ID);
  Serial.printf("Fan GPIO: %d | Heat1 GPIO: %d | Heat2 GPIO: %d | Swing GPIO: %d\n",
    PIN_FAN, PIN_HEAT1, PIN_HEAT2, PIN_SWING);
  printLine();
  printHelp();

  Serial.println();
  Serial.print(F("Current status: "));
  printStatus();
  Serial.println(F("Device READY - waiting for commands."));
}

void loop() {
  processSerialCommand();

  maintainWifi();
  maintainMqtt();
  mqttService();
  webPortalService();

  delay(1);
}
