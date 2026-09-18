#include "MqttHandler.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "Config.h"
#include "RelayControl.h"
#include "CommandHandler.h"
#include "JsonCommand.h"

static WiFiClient wifiClient;
static PubSubClient mqttClient(wifiClient);

static uint32_t lastWifiAttemptAt = 0;
static uint32_t lastMqttAttemptAt = 0;
static uint32_t lastStatusAt = 0;

static void jsonEscape(const String& input, char* output, size_t outputSize) {
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

  char escapedCommand[192];
  jsonEscape(command, escapedCommand, sizeof(escapedCommand));

  char payload[384];
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

static void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, MQTT_COMMAND_TOPIC) != 0) return;

  if (length == 0 || length > 200) {
    publishCommandEvent("", false, length == 0 ? "empty_command" : "command_too_long");
    return;
  }

  char commandBuffer[201];
  memcpy(commandBuffer, payload, length);
  commandBuffer[length] = '\0';

  String line(commandBuffer);
  Serial.printf("[MQTT] RX: %s\n", line.c_str());

  bool power, heat1, heat2, swing;
  if (!parseRelayStateJson(line, power, heat1, heat2, swing)) {
    publishCommandEvent(line, false, "invalid_json");
    return;
  }

  applyRelayState(power, heat1, heat2, swing);
  publishCommandEvent(line, true, "applied");
}

void mqttSetup() {
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCallback(onMqttMessage);
  mqttClient.setBufferSize(512);
  mqttClient.setKeepAlive(30);
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

void mqttService() {
  if (mqttClient.connected()) {
    mqttClient.loop();
    if (static_cast<uint32_t>(millis() - lastStatusAt) >= STATUS_INTERVAL_MS) {
      publishStatus(true);
    }
  }
}

bool mqttIsConnected() {
  return mqttClient.connected();
}
