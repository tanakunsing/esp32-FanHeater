#include "SerialConsole.h"
#include <Arduino.h>
#include "Config.h"
#include "RelayControl.h"
#include "CommandHandler.h"

static bool fullTestRunning = false;

void printLine() {
  Serial.println(F("============================================================"));
}

void printHelp() {
  Serial.println();
  Serial.println(F("Commands (Serial and MQTT unless noted):"));
  Serial.println(F("  level1   (Fan only)"));
  Serial.println(F("  level2   (Fan + Heat1, 1000W)"));
  Serial.println(F("  level3   (Fan + Heat1 + Heat2, 2500W)"));
  Serial.println(F("  swing_on | swing_off"));
  Serial.println(F("  all_off"));
  Serial.println(F("  status"));
  Serial.println(F("Serial only:"));
  Serial.println(F("  state | test | help"));
  Serial.printf("MQTT command topic: %s\n", MQTT_COMMAND_TOPIC);
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

  String command = Serial.readStringUntil('\n');
  command.trim();
  String normalized = command;
  normalized.toLowerCase();

  if (normalized.length() == 0) return;
  if (normalized == "help") {
    printHelp();
  } else if (normalized == "state") {
    printStatus();
  } else if (normalized == "test") {
    runFullTest();
  } else if (!executeHeaterCommand(normalized, false)) {
    Serial.println(F("Unknown or invalid command. Type 'help'."));
  }
}
