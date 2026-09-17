/*
  ZEEP ESP32-S3 N16R8 - deviceHeaterFan / Heater Relay Control / Local MQTT
  Firmware version: 1.0.0-deviceHeaterFan

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

  Required Arduino libraries:
    - PubSubClient by Nick O'Leary

  IMPORTANT:
    - Edit WIFI_SSID and WIFI_PASSWORD before upload.
    - MQTT commands must NOT be retained.
    - Never enable Heat1/Heat2 without the fan running.
*/

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// -----------------------------------------------------------------------------
// Device and network configuration
// -----------------------------------------------------------------------------

static const char* DEVICE_ID = "controlhub1-pod1";
static const char* FIRMWARE_VERSION = "1.0.0-deviceHeaterFan";

// Change these two values to match the Pi 5 POD 1 access point exactly.
static const char* WIFI_SSID = "POD 1 wifi";
static const char* WIFI_PASSWORD = "12345678";

// Pi 5 AP address, based on the supplied local MQTT setup.
static const char* MQTT_HOST = "192.168.50.1";
static const uint16_t MQTT_PORT = 1883;

static const char* MQTT_COMMAND_TOPIC = "zeep/pod1/controlhub1/command";
static const char* MQTT_STATUS_TOPIC = "zeep/pod1/controlhub1/status";
static const char* MQTT_EVENT_TOPIC = "zeep/pod1/controlhub1/event";

static const uint32_t WIFI_RETRY_MS = 10000;
static const uint32_t MQTT_RETRY_MS = 5000;
static const uint32_t STATUS_INTERVAL_MS = 30000;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

uint32_t lastWifiAttemptAt = 0;
uint32_t lastMqttAttemptAt = 0;
uint32_t lastStatusAt = 0;
uint32_t commandCounter = 0;

// -----------------------------------------------------------------------------
// Relay configuration (heater box: fan, heat1 1000W, heat2 1500W, swing)
// -----------------------------------------------------------------------------

const int PIN_FAN   = 4;
const int PIN_HEAT1 = 5;
const int PIN_HEAT2 = 6;
const int PIN_SWING = 7;

bool fanState   = false;
bool heat1State = false;
bool heat2State = false;
bool swingState = false;

bool fullTestRunning = false;

// Serial-only full test settings.
static const uint32_t STEP_WAIT_MS = 5000;

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

void printLine() {
  Serial.println(F("============================================================"));
}

void jsonEscape(const String& input, char* output, size_t outputSize) {
  size_t out = 0;
  if (outputSize == 0) return;

  for (size_t i = 0; i < input.length() && out + 1 < outputSize; i++) {
    const char c = input[i];
    if ((c == '\\' || c == '"') && out + 2 < outputSize) {
      output[out++] = '\\';
      output[out++] = c;
    } else if (static_cast<uint8_t>(c) >= 0x20) {
      output[out++] = c;
    }
  }
  output[out] = '\0';
}

void publishStatus(bool online) {
  if (!mqttClient.connected()) return;

  char payload[320];
  const String ip = WiFi.localIP().toString();
  snprintf(
    payload,
    sizeof(payload),
    "{\"schema_version\":1,\"device_id\":\"%s\",\"firmware_version\":\"%s\","
    "\"online\":%s,\"ip\":\"%s\",\"rssi\":%ld,\"uptime_ms\":%lu,"
    "\"mqtt_connected\":true,\"fan\":%s,\"heat1\":%s,\"heat2\":%s,\"swing\":%s,"
    "\"command_count\":%lu}",
    DEVICE_ID,
    FIRMWARE_VERSION,
    online ? "true" : "false",
    ip.c_str(),
    static_cast<long>(WiFi.RSSI()),
    static_cast<unsigned long>(millis()),
    fanState ? "true" : "false",
    heat1State ? "true" : "false",
    heat2State ? "true" : "false",
    swingState ? "true" : "false",
    static_cast<unsigned long>(commandCounter)
  );

  mqttClient.publish(MQTT_STATUS_TOPIC, payload, true);
  lastStatusAt = millis();
}

void publishCommandEvent(const String& command, bool ok, const char* detail) {
  if (!mqttClient.connected()) return;

  char escapedCommand[96];
  jsonEscape(command, escapedCommand, sizeof(escapedCommand));

  char payload[320];
  snprintf(
    payload,
    sizeof(payload),
    "{\"schema_version\":1,\"event\":\"heater_command\",\"device_id\":\"%s\","
    "\"command\":\"%s\",\"ok\":%s,\"detail\":\"%s\",\"command_count\":%lu,\"uptime_ms\":%lu}",
    DEVICE_ID,
    escapedCommand,
    ok ? "true" : "false",
    detail,
    static_cast<unsigned long>(commandCounter),
    static_cast<unsigned long>(millis())
  );

  mqttClient.publish(MQTT_EVENT_TOPIC, payload, false);
  if (ok) publishStatus(true);
}

// -----------------------------------------------------------------------------
// Heater relay outputs
// -----------------------------------------------------------------------------

void fan(bool state) {
  fanState = state;
  digitalWrite(PIN_FAN, state ? HIGH : LOW);
  Serial.println(state ? F("Fan ON") : F("Fan OFF"));
}

void heat1(bool state) {
  heat1State = state;
  digitalWrite(PIN_HEAT1, state ? HIGH : LOW);
  Serial.println(state ? F("Heat1 (1000W) ON") : F("Heat1 (1000W) OFF"));
}

void heat2(bool state) {
  heat2State = state;
  digitalWrite(PIN_HEAT2, state ? HIGH : LOW);
  Serial.println(state ? F("Heat2 (1500W) ON") : F("Heat2 (1500W) OFF"));
}

void swing(bool state) {
  swingState = state;
  digitalWrite(PIN_SWING, state ? HIGH : LOW);
  Serial.println(state ? F("Swing ON") : F("Swing OFF"));
}

// 1. Power On = เปิดพัดลม
void powerOn() {
  fan(true);
}

// 2. Power Off = ปิดอุปกรณ์ทั้งหมด
void powerOff() {
  fan(false);
  heat1(false);
  heat2(false);
  swing(false);
}

void printStatus() {
  Serial.print(F("Fan="));   Serial.print(fanState ? "ON " : "OFF ");
  Serial.print(F("Heat1=")); Serial.print(heat1State ? "ON " : "OFF ");
  Serial.print(F("Heat2=")); Serial.print(heat2State ? "ON " : "OFF ");
  Serial.print(F("Swing=")); Serial.println(swingState ? "ON" : "OFF");
}

// -----------------------------------------------------------------------------
// Shared command parser: Serial and MQTT use the same heater commands
// -----------------------------------------------------------------------------

bool executeHeaterCommand(String command, bool fromMqtt) {
  command.trim();
  command.toLowerCase();

  bool ok = true;
  const char* detail = "sent";

  if (command == "fan_on") {
    fan(true);
  } else if (command == "fan_off") {
    fan(false);
  } else if (command == "heat1_on") {
    heat1(true);
  } else if (command == "heat1_off") {
    heat1(false);
  } else if (command == "heat2_on") {
    heat2(true);
  } else if (command == "heat2_off") {
    heat2(false);
  } else if (command == "level1") {
    // Level 1: Fan only, no heat.
    fan(true);
    heat1(false);
    heat2(false);
  } else if (command == "level2") {
    // Level 2: Fan + Heat1 (1000W).
    fan(true);
    heat1(true);
    heat2(false);
  } else if (command == "level3") {
    // Level 3: Fan + Heat1 + Heat2 (2500W).
    fan(true);
    heat1(true);
    heat2(true);
  } else if (command == "swing_on") {
    swing(true);
  } else if (command == "swing_off") {
    swing(false);
  } else if (command == "all_off") {
    powerOff();
  } else if (command == "status") {
    detail = "status_published";
    if (mqttClient.connected()) publishStatus(true);
  } else if (command == "test") {
    ok = false;
    detail = fromMqtt ? "test_is_serial_only" : "use_serial_test_handler";
  } else {
    ok = false;
    detail = "unknown_command";
  }

  if (ok) commandCounter++;

  if (fromMqtt) {
    publishCommandEvent(command, ok, detail);
  }
  return ok;
}

// -----------------------------------------------------------------------------
// MQTT and Wi-Fi
// -----------------------------------------------------------------------------

void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, MQTT_COMMAND_TOPIC) != 0) return;

  if (length == 0 || length > 80) {
    publishCommandEvent("", false, length == 0 ? "empty_command" : "command_too_long");
    return;
  }

  char commandBuffer[81];
  memcpy(commandBuffer, payload, length);
  commandBuffer[length] = '\0';

  String command(commandBuffer);
  Serial.printf("[MQTT] RX command: %s\n", command.c_str());
  executeHeaterCommand(command, true);
}

void startWifi() {
  Serial.printf("[WiFi] Connecting to '%s'\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttemptAt = millis();
}

void maintainWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  if (mqttClient.connected()) mqttClient.disconnect();
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastWifiAttemptAt) < WIFI_RETRY_MS) return;

  Serial.println(F("[WiFi] Disconnected; retrying"));
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lastWifiAttemptAt = now;
}

void maintainMqtt() {
  if (WiFi.status() != WL_CONNECTED || mqttClient.connected()) return;

  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastMqttAttemptAt) < MQTT_RETRY_MS) return;
  lastMqttAttemptAt = now;

  char clientId[48];
  const uint64_t chipId = ESP.getEfuseMac();
  snprintf(clientId, sizeof(clientId), "%s-%04X", DEVICE_ID, static_cast<uint16_t>(chipId));

  char willPayload[160];
  snprintf(
    willPayload,
    sizeof(willPayload),
    "{\"schema_version\":1,\"device_id\":\"%s\",\"online\":false}",
    DEVICE_ID
  );

  Serial.printf("[MQTT] Connecting to %s:%u\n", MQTT_HOST, MQTT_PORT);
  const bool connected = mqttClient.connect(
    clientId,
    MQTT_STATUS_TOPIC,
    0,
    true,
    willPayload
  );

  if (!connected) {
    Serial.printf("[MQTT] Connect failed, state=%d\n", mqttClient.state());
    return;
  }

  Serial.println(F("[MQTT] Connected"));

  // Delete an old retained command before subscribing. Live commands must be
  // published without -r / retain.
  mqttClient.publish(MQTT_COMMAND_TOPIC, "", true);
  mqttClient.subscribe(MQTT_COMMAND_TOPIC, 0);
  publishStatus(true);
}

// -----------------------------------------------------------------------------
// Serial tools
// -----------------------------------------------------------------------------

void printHelp() {
  Serial.println();
  Serial.println(F("Commands (Serial and MQTT unless noted):"));
  Serial.println(F("  level1   (Fan only)"));
  Serial.println(F("  level2   (Fan + Heat1, 1000W)"));
  Serial.println(F("  level3   (Fan + Heat1 + Heat2, 2500W)"));
  Serial.println(F("  fan_on | fan_off"));
  Serial.println(F("  heat1_on | heat1_off   (1000W)"));
  Serial.println(F("  heat2_on | heat2_off   (1500W)"));
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

// -----------------------------------------------------------------------------
// Arduino entry points
// -----------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);
  delay(1000);

  pinMode(PIN_FAN, OUTPUT);
  pinMode(PIN_HEAT1, OUTPUT);
  pinMode(PIN_HEAT2, OUTPUT);
  pinMode(PIN_SWING, OUTPUT);
  powerOff();

  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(512);
  mqttClient.setKeepAlive(30);

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

  if (mqttClient.connected()) {
    mqttClient.loop();
    if (static_cast<uint32_t>(millis() - lastStatusAt) >= STATUS_INTERVAL_MS) {
      publishStatus(true);
    }
  }

  delay(1);
}
