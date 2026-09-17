/*
  ZEEP ESP32-S3 N16R8 - deviceHeaterFan / Heater Relay Control / Local MQTT
  Firmware version: 1.0.1-deviceHeaterFan

  Data path:
    Raspberry Pi 5 -> Mosquitto -> POD 1 Wi-Fi -> ESP32-S3 -> Relay -> Fan / Heater / Swing motor

  Usage levels (combined Fan+Heat presets, sent as a single command):
    level1 -> Fan only
    level2 -> Fan + Heat1 (1000W)
    level3 -> Fan + Heat1 + Heat2 (2500W)

  Relay outputs (Active-High):
    Fan relay      -> GPIO 4
    Heater 1000W   -> GPIO 5
    Heater 1500W   -> GPIO 6
    Swing motor    -> GPIO 7

  File layout:
    Config.h          - device/network constants, GPIO pin numbers
    RelayControl.*     - hardware layer: drives the relays, tracks their state
    MqttHandler.*       - network layer: Wi-Fi/MQTT lifecycle, JSON status/event
    CommandHandler.*    - dispatch layer: text command -> relay actions
    SerialConsole.*     - Serial-only debug console (help/state/test)

  Required Arduino libraries:
    - PubSubClient by Nick O'Leary

  IMPORTANT:
    - Edit WIFI_SSID and WIFI_PASSWORD in Config.h before upload.
    - MQTT commands must NOT be retained.
    - Never enable Heat1/Heat2 without the fan running.
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
