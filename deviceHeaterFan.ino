/*
  ZEEP deviceHeaterFan / Heater Relay Control / Standalone AP Web Panel
  Firmware version: 3.0.0-deviceHeaterFan

  Runs on either board; only the GPIO numbers in Config.h change:
    - ESP32 Dev Module (plain ESP32, CP2102 USB-UART) -> pins 16/17/18/19 (current)
    - ESP32-S3 N16R8 (native USB)                     -> pins 4/5/6/7

  No station Wi-Fi, no MQTT, no Pi connectivity (removed in 3.0.0). The
  device only runs its own AP; control is entirely local via the web panel
  hosted on it (see WebPortal.cpp). Serial no longer accepts commands
  either (see printHelp() for the web panel address).

  Command schema the web panel sends internally:
    {"power":"on","heat1":"on","heat2":"off","swing":"off"}
    - power=false (or missing): everything forced off, no matter what else is set.
    - heat2=on forces heat1=on too (no "heat2 alone" state).
    - Any field missing/not exactly "on" is treated as off.

  Relay outputs (Active-High) - see Config.h for the pin numbers in effect:
    Swing motor, Fan relay, Heater 1000W, Heater 1500W

  File layout:
    Config.h          - device identity, AP credentials, GPIO pin numbers
    RelayControl.*     - hardware layer: drives the relays, tracks their state
    JsonCommand.*       - parses the {"power":...} JSON object
    WebPortal.*         - AP + web server: buttons and live status
    CommandHandler.*    - dispatch layer: JSON state -> relay actions
    SerialConsole.*     - Serial-only debug console (help/state/test)

  Required Arduino libraries:
    - ArduinoJson

  IMPORTANT:
    - Never enable Heat1/Heat2 without the fan running (enforced in code).
*/

#include <Arduino.h>
#include "Config.h"
#include "RelayControl.h"
#include "WebPortal.h"
#include "CommandHandler.h"
#include "SerialConsole.h"

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(1000);

  relayInit();
  webPortalSetup();

  printLine();
  Serial.println(F("ZEEP deviceHeaterFan - HEATER RELAY + AP WEB PANEL"));
  Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
  Serial.printf("Device ID: %s\n", DEVICE_ID);
  Serial.printf("Fan GPIO: %d | Heat1 GPIO: %d | Heat2 GPIO: %d | Swing GPIO: %d\n",
    PIN_FAN, PIN_HEAT1, PIN_HEAT2, PIN_SWING);
  printLine();
  printHelp();
}

void loop() {
  processSerialCommand();
  webPortalService();

  delay(1);
}
