#include "SerialConsole.h"
#include <Arduino.h>
#include "Config.h"
#include "RelayControl.h"
#include "CommandHandler.h"
#include "JsonCommand.h"

static bool fullTestRunning = false;

void printLine() {
  Serial.println(F("============================================================"));
}

void printHelp() {
  Serial.println();
  Serial.println(F("Serial input is a full relay-state JSON object:"));
  Serial.println(F("  {\"power\":\"on\",\"heat1\":\"on\",\"heat2\":\"off\",\"swing\":\"off\"}"));
  Serial.println(F("Missing/non-\"on\" fields default to off. power=off forces"));
  Serial.println(F("everything off regardless of heat1/heat2/swing (safety gate)."));
  Serial.println(F("heat2=on forces heat1=on too (no \"heat2 alone\" state)."));
  Serial.println(F("Serial-only plain-text utilities:"));
  Serial.println(F("  state | test | help"));
  Serial.println(F("MQTT command topic uses the same JSON object (no plain-text"));
  Serial.println(F("commands anymore):"));
  Serial.printf("  %s\n", MQTT_COMMAND_TOPIC);
}

void runFullTest() {
  if (fullTestRunning) return;
  fullTestRunning = true;

  Serial.println(F("STARTING SERIAL-ONLY FULL RELAY TEST"));

  Serial.println(F("Step 1: Fan only"));
  powerOn();
  delay(STEP_WAIT_MS);

  Serial.println(F("Step 2: Fan + Heat1 (1000W)"));
  fan(true);
  heat1(true);
  delay(STEP_WAIT_MS);

  Serial.println(F("Step 3: Fan + Heat2 (1500W) + Swing"));
  heat1(false);
  heat2(true);
  swing(true);
  delay(STEP_WAIT_MS);

  Serial.println(F("Step 4: Cool down (fan only)"));
  heat1(false);
  heat2(false);
  swing(false);
  fan(true);
  delay(STEP_WAIT_MS);

  Serial.println(F("Step 5: All off"));
  powerOff();

  fullTestRunning = false;
  Serial.println(F("FULL TEST FINISHED"));
}

void processSerialCommand() {
  if (!Serial.available()) return;

  String line = Serial.readStringUntil('\n');
  line.trim();
  if (line.length() == 0) return;

  String normalized = line;
  normalized.toLowerCase();

  if (normalized == "help") {
    printHelp();
    return;
  }
  if (normalized == "state") {
    printStatus();
    return;
  }
  if (normalized == "test") {
    runFullTest();
    return;
  }

  bool power, wantHeat1, wantHeat2, wantSwing;
  if (!parseRelayStateJson(line, power, wantHeat1, wantHeat2, wantSwing)) {
    Serial.println(F("Expected JSON: {\"power\":\"on\",\"heat1\":\"on\",\"heat2\":\"off\",\"swing\":\"off\"}"));
    return;
  }

  applyRelayState(power, wantHeat1, wantHeat2, wantSwing);
}
