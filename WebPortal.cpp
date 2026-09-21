#include "WebPortal.h"
#include <WiFi.h>
#include <WebServer.h>
#include "Config.h"
#include "RelayControl.h"
#include "CommandHandler.h"
#include "JsonCommand.h"

static WebServer server(80);

// Single-page control panel. Buttons always POST the full desired state
// (power/heat1/heat2/swing) built from the last known status, then re-fetch
// status to show what the device actually ended up doing (so the safety
// gate / heat1-follows-heat2 interlock is always reflected honestly, even
// if a button press didn't produce the state it asked for).
static const char PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>deviceHeaterFan</title>
<style>
  :root { color-scheme: light dark; }
  body { font-family: -apple-system, Segoe UI, Roboto, sans-serif; margin: 0; padding: 20px;
         background: #101418; color: #eef2f6; }
  h1 { font-size: 1.1rem; margin: 0 0 4px; }
  .sub { color: #8a97a6; font-size: 0.8rem; margin-bottom: 20px; }
  .grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 12px; max-width: 420px; }
  button { font-size: 1rem; padding: 18px 10px; border-radius: 12px; border: 2px solid #2a323c;
           background: #1a2027; color: #eef2f6; cursor: pointer; transition: 0.15s; }
  button.on { background: #1f7a4d; border-color: #2ecc71; }
  button:disabled { opacity: 0.5; }
  .status { margin-top: 24px; max-width: 420px; border-top: 1px solid #2a323c; padding-top: 14px;
            font-size: 0.85rem; line-height: 1.7; color: #c3ccd6; }
  .status b { color: #eef2f6; }
  .dot { display: inline-block; width: 9px; height: 9px; border-radius: 50%; margin-right: 6px; }
  .dot.on { background: #2ecc71; } .dot.off { background: #4a5461; }
</style>
</head>
<body>
  <h1>deviceHeaterFan control panel</h1>
  <div class="sub" id="meta">loading...</div>

  <div class="grid">
    <button id="btn-power" onclick="toggle('power')">Power (Fan)</button>
    <button id="btn-swing" onclick="toggle('swing')">Swing</button>
    <button id="btn-heat1" onclick="toggle('heat1')">Heat1 (1000W)</button>
    <button id="btn-heat2" onclick="toggle('heat2')">Heat2 (1500W)</button>
  </div>

  <div class="status" id="status">reading status...</div>

<script>
let current = { power: false, heat1: false, heat2: false, swing: false };

function render() {
  for (const key of ['power', 'heat1', 'heat2', 'swing']) {
    document.getElementById('btn-' + key).classList.toggle('on', current[key]);
  }
  const dot = (label, on) =>
    `<span class="dot ${on ? 'on' : 'off'}"></span>${label}: <b>${on ? 'ON' : 'OFF'}</b>`;
  document.getElementById('status').innerHTML =
    dot('Fan', current.power) + '<br>' +
    dot('Heat1', current.heat1) + '<br>' +
    dot('Heat2', current.heat2) + '<br>' +
    dot('Swing', current.swing);
}

async function refreshStatus() {
  const r = await fetch('/api/status');
  const j = await r.json();
  current = { power: j.fan, heat1: j.heat1, heat2: j.heat2, swing: j.swing };
  document.getElementById('meta').textContent =
    j.firmware_version + ' | commands: ' + j.command_count + ' | uptime: ' + Math.round(j.uptime_ms / 1000) + 's';
  render();
}

async function toggle(field) {
  const next = { ...current, [field]: !current[field] };
  const body = {
    power: next.power ? 'on' : 'off',
    heat1: next.heat1 ? 'on' : 'off',
    heat2: next.heat2 ? 'on' : 'off',
    swing: next.swing ? 'on' : 'off',
  };
  await fetch('/api/command', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  });
  await refreshStatus();
}

refreshStatus();
setInterval(refreshStatus, 2000);
</script>
</body>
</html>
)rawliteral";

static void handleRoot() {
  server.send_P(200, "text/html", PAGE_HTML);
}

static void handleStatus() {
  char payload[224];
  snprintf(
    payload,
    sizeof(payload),
    "{\"fan\":%s,\"heat1\":%s,\"heat2\":%s,\"swing\":%s,"
    "\"firmware_version\":\"%s\",\"command_count\":%lu,\"uptime_ms\":%lu}",
    fanState ? "true" : "false",
    heat1State ? "true" : "false",
    heat2State ? "true" : "false",
    swingState ? "true" : "false",
    FIRMWARE_VERSION,
    static_cast<unsigned long>(commandCounter),
    static_cast<unsigned long>(millis())
  );
  server.send(200, "application/json", payload);
}

static void handleCommand() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"ok\":false,\"detail\":\"missing_body\"}");
    return;
  }

  bool power, heat1, heat2, swing;
  if (!parseRelayStateJson(server.arg("plain"), power, heat1, heat2, swing)) {
    server.send(400, "application/json", "{\"ok\":false,\"detail\":\"invalid_json\"}");
    return;
  }

  applyRelayState(power, heat1, heat2, swing);
  server.send(200, "application/json", "{\"ok\":true}");
}

void webPortalSetup() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  Serial.print(F("[AP] SSID: "));
  Serial.println(AP_SSID);
  Serial.print(F("[AP] Control panel: http://"));
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/command", HTTP_POST, handleCommand);
  server.begin();
}

void webPortalService() {
  server.handleClient();
}
