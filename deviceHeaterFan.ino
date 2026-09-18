/*
  ZEEP ESP32-S3 N16R8 - deviceHeaterFan / Heater Relay Control / Local MQTT
  Firmware version: 2.0.0-deviceHeaterFan

  Data path:
    Raspberry Pi 5 -> Mosquitto -> POD 1 Wi-Fi -> ESP32-S3 -> Relay -> Fan / Heater / Swing motor

  Command schema (same JSON object over Serial AND MQTT command topic):
    {"power":"on","heat1":"on","heat2":"off","swing":"off"}
    - power=false (or missing): everything forced off, no matter what else is set.
    - heat2=on forces heat1=on too (no "heat2 alone" state).
    - Any field missing/not exactly "on" is treated as off.

  Relay outputs (Active-High):
    Swing motor    -> GPIO 4
    Fan relay      -> GPIO 5
    Heater 1000W   -> GPIO 6
    Heater 1500W   -> GPIO 7

  File layout:
    Config.h          - device/network constants, GPIO pin numbers
    RelayControl.*     - hardware layer: drives the relays, tracks their state
    JsonCommand.*       - parses the {"power":...} JSON object
    MqttHandler.*       - network layer: Wi-Fi/MQTT lifecycle, JSON status/event
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
#include "CommandHandler.h"
#include "SerialConsole.h"

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(1000);

  relayInit();
  mqttSetup();

  printLine();
  Serial.println(F("ZEEP ESP32-S3 deviceHeaterFan - HEATER RELAY + LOCAL MQTT"));
  Serial.printf("Firmware: %s\n", FIRMWARE_VERSION);
  Serial.printf("Device ID: %s\n", DEVICE_ID);
  Serial.printf("Fan GPIO: %d | Heat1 GPIO: %d | Heat2 GPIO: %d | Swing GPIO: %d\n",
    PIN_FAN, PIN_HEAT1, PIN_HEAT2, PIN_SWING);
  printLine();
  printHelp();

  startWifi();
}

void loop() {
  processSerialCommand();

  maintainWifi();
  maintainMqtt();
  mqttService();

  delay(1);
}
