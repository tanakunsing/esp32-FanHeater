#include "SerialConsole.h"
#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"
#include "RelayControl.h"
#include "CommandHandler.h"

static bool fullTestRunning = false;

void printLine() {
  Serial.println(F("============================================================"));
}

void printHelp() {
  Serial.println();
  Serial.println(F("Control this device from a browser instead of Serial:"));
  Serial.print(F("  Wi-Fi: connect to \""));
  Serial.print(AP_SSID);
  Serial.println(F("\""));
  Serial.print(F("  Then open: http://"));
  Serial.println(WiFi.softAPIP());
  Serial.println(F("Serial-only plain-text utilities:"));
  Serial.println(F("  state | test | help"));
  Serial.println(F("MQTT command topic takes the same JSON object the web panel sends:"));
  Serial.println(F("  {\"power\":\"on\",\"heat1\":\"on\",\"heat2\":\"off\",\"swing\":\"off\"}"));
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
  } else if (normalized == "state") {
    printStatus();
  } else if (normalized == "test") {
    runFullTest();
  } else {
    Serial.println(F("Relay control now happens via the web panel or MQTT, not Serial."));
    Serial.println(F("Type 'help' for the web panel address."));
  }
}
