/*
  ZEEP deviceHeaterFan / Heater Relay Control / Local MQTT
  Firmware version: 2.1.0-deviceHeaterFan

  Runs on either board; only the GPIO numbers in Config.h change:
    - ESP32 Dev Module (plain ESP32, CP2102 USB-UART) -> pins 16/17/18/19 (current)
    - ESP32-S3 N16R8 (native USB)                     -> pins 4/5/6/7

  Data path:
    Raspberry Pi 5 -> Mosquitto -> POD 1 Wi-Fi -> ESP32 -> Relay -> Fan / Heater / Swing motor

  Control (same JSON object accepted by both):
    - Local web panel hosted on the device's own AP (see WebPortal.cpp) -
      buttons, no typing required.
    - MQTT command topic: {"power":"on","heat1":"on","heat2":"off","swing":"off"}
    Rules enforced regardless of source:
    - power=false (or missing): everything forced off, no matter what else is set.
    - heat2=on forces heat1=on too (no "heat2 alone" state).
    - Any field missing/not exactly "on" is treated as off.
    Serial no longer accepts commands (see printHelp() for the web panel address).

  Relay outputs (Active-High) - see Config.h for the pin numbers in effect:
    Swing motor, Fan relay, Heater 1000W, Heater 1500W

  File layout:
    Config.h          - device/network constants, GPIO pin numbers
    RelayControl.*     - hardware layer: drives the relays, tracks their state
    JsonCommand.*       - parses the {"power":...} JSON object
    MqttHandler.*       - network layer: Wi-Fi/MQTT lifecycle, JSON status/event
    WebPortal.*         - local AP + web server: buttons and live status
    CommandHandler.*    - dispatch layer: JSON state -> relay actions
    SerialConsole.*     - Serial-only debug console (help/state/test)

  Required Arduino libraries:
    - PubSubClient by Nick O'Leary
    - ArduinoJson

  IMPORTANT:
    - Edit WIFI_SSID and WIFI_PASSWORD in Config.h before upload.
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
  startWifi();      // sets AP+STA mode and starts the station connection
  webPortalSetup();  // starts the AP + web server (needs AP+STA mode set above)

  printLine();
  Serial.println(F("ZEEP deviceHeaterFan - HEATER RELAY + LOCAL MQTT"));
  Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
  Serial.printf("Device ID: %s\n", DEVICE_ID);
  Serial.printf("Fan GPIO: %d | Heat1 GPIO: %d | Heat2 GPIO: %d | Swing GPIO: %d\n",
    PIN_FAN, PIN_HEAT1, PIN_HEAT2, PIN_SWING);
  printLine();
  printHelp();
}

void loop() {
  processSerialCommand();

  maintainWifi();
  maintainMqtt();
  mqttService();
  webPortalService();

  delay(1);
}
