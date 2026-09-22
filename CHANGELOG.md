# Changelog

All notable changes to the deviceHeaterFan firmware are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/), versioning follows [Semantic Versioning](https://semver.org/).

## [4.0.1] - 2026-09-22

### Fixed
- `printHelp()` no longer prints a literal JSON example next to the MQTT topic name — it looked like a command that had already run (it hadn't; it was just help text), which caused confusion during testing.

### Added
- `setup()` now prints the actual relay status and a clear `Device READY - waiting for commands.` line right after boot, so the Serial log shows what really happened instead of only static help text.

## [4.0.0] - 2026-09-22

### Added
- Station Wi-Fi + MQTT restored (`MqttHandler.h/.cpp`, brought back from the pre-3.0.0 design) — `WIFI_SSID`/`WIFI_PASSWORD`, `MQTT_HOST/PORT`, and the three MQTT topics are back in `Config.h`.

### Changed
- **Breaking:** Wi-Fi mode is now `WIFI_AP_STA` instead of `WIFI_AP` — the device connects to the Pi's Wi-Fi for MQTT *and* runs its own AP (web panel + OTA) at the same time. Single radio, so AP and station share one Wi-Fi channel automatically; this doesn't affect AP clients.
- `WebPortal.cpp` no longer calls `WiFi.mode()` itself — `startWifi()` (MqttHandler) sets AP_STA before `webPortalSetup()` adds the AP on top.
- MQTT command topic accepts the same JSON schema as the web panel (`{"power":...}`), reusing `parseRelayStateJson()` + `applyRelayState()` — same safety gate and heat1/heat2 interlock regardless of source.

### Notes
- This reintroduces the same MQTT topics/`DEVICE_ID` as before 3.0.0, so the earlier Pi interface spec applies again unchanged.

## [3.1.0] - 2026-09-21

### Added
- Browser-based OTA firmware update, WiFiManager-style (`OtaUpdate.h/.cpp`), attached to the existing web server:
  - `GET /update` — upload page with a file picker and progress bar.
  - `POST /update` — streams the `.bin` straight into the `Update` library (`UPDATE_SIZE_UNKNOWN`, no full-file buffering), then `ESP.restart()` on success.
  - Uses the built-in `Update` library (ships with the esp32 core, no new dependency). Confirmed the board's `default` partition scheme already has the dual OTA app partitions this needs.
  - Linked from the control panel's front page ("firmware update (OTA)").

## [3.0.1] - 2026-09-21

### Changed
- Replaced the 4 independent toggle buttons (power/heat1/heat2/swing) on the web panel with 4 fixed presets: **Fan Only**, **Heat 1**, **Heat 2**, **Close** — each POSTs one constant JSON payload instead of a computed one.
  - `Heat 2` omits `heat1` in its payload; the page highlights the button based on the *resulting* state (heat1 forced on too), not the payload sent, since `applyRelayState()`'s heat2-requires-heat1 rule still applies.
  - No swing control by design — every preset leaves swing off.
- Backend (`/api/command`) unchanged — still the same JSON schema and dispatcher; this was a page-only change.

## [3.0.0] - 2026-09-21

### Removed
- MQTT entirely (`MqttHandler.h/.cpp` deleted, `PubSubClient` no longer a dependency).
- Station Wi-Fi connection to the Pi's AP (`WIFI_SSID`/`WIFI_PASSWORD`, `WIFI_RETRY_MS`) — device no longer joins any external network.
- **Breaking:** the Raspberry Pi can no longer control this device at all. The MQTT interface spec given to the Pi team is obsolete.

### Changed
- The device now only runs its own Wi-Fi AP (`deviceHeaterFan-AP`) with the web control panel (added in 2.1.0) as the sole way to control it.

## [2.1.0] - 2026-09-21

### Added
- Local AP + web control panel (`WebPortal.h/.cpp`): 4 toggle buttons (power/heat1/heat2/swing) and live status, served from the device's own Wi-Fi AP alongside the existing MQTT station connection.
- `POST /api/command` reuses the same `{"power":...}` JSON schema and `applyRelayState()` dispatcher as MQTT, so the safety gate and heat1/heat2 interlock apply identically.

### Removed
- Serial no longer accepts relay commands (JSON-over-Serial from 2.0.0 removed); only `help`/`state`/`test` utilities remain.

## [2.0.1] - 2026-09-21

### Changed
- Documented which GPIO pin set targets which board (16/17/18/19 = ESP32 Dev Module, 4/5/6/7 = ESP32-S3) in `Config.h` and the `.ino` header.

## [2.0.0] - 2026-09-19

### Changed
- **Breaking:** unified Serial and MQTT on one JSON relay-state schema, `{"power":"on","heat1":"on","heat2":"off","swing":"off"}`, replacing the plain-text `level1/level2/level3/...` command vocabulary on both channels.
- `power=false` (or missing) forces everything off; `heat2=on` forces `heat1=on` too.

### Removed
- `executeHeaterCommand()` and the old plain-text command vocabulary.

## [1.1.0] - 2026-09-17

### Removed
- Granular relay commands `fan_on/off`, `heat1_on/off`, `heat2_on/off` — confirmed nothing on the Pi/backend side sends them directly. Only the `level1/2/3` presets and `swing_on/off`/`all_off` remain as ways to drive the relays.

### Fixed
- `printHelp()` no longer advertises the removed granular commands.

## [1.0.1] - 2026-09-17

### Changed
- Split the monolithic `.ino` into modules by responsibility, no behavior change:
  - `Config.h` — device/network constants, GPIO pin numbers
  - `RelayControl.h/.cpp` — hardware layer (fan/heat1/heat2/swing relay control)
  - `MqttHandler.h/.cpp` — network layer (Wi-Fi/MQTT lifecycle, JSON status/event)
  - `CommandHandler.h/.cpp` — dispatch layer (text command -> relay actions)
  - `SerialConsole.h/.cpp` — Serial-only debug console
  - `deviceHeaterFan.ino` — now only `setup()`/`loop()`

## [1.0.0] - 2026-09-17

### Added
- Initial tracked version of the heater/fan relay controller (Fan GPIO4, Heat1 1000W GPIO5, Heat2 1500W GPIO6, Swing GPIO7).
- MQTT command topic `zeep/pod1/controlhub1/command`, status/event topics for reporting back to the Raspberry Pi.
- Combined usage-level commands `level1` (fan only), `level2` (fan + heat1), `level3` (fan + heat1 + heat2).
- Granular commands: `fan_on/off`, `heat1_on/off`, `heat2_on/off`, `swing_on/off`, `all_off`, `status`.
- Serial-only tools: `state`, `test`, `help`.

### Known issues
- `heat1_on`/`heat2_on` do not verify the fan is running first; only the `level2`/`level3` presets guarantee that ordering.
