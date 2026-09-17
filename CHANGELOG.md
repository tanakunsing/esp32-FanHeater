# Changelog

All notable changes to the deviceHeaterFan firmware are documented in this file.
Format follows [Keep a Changelog](https://keepachangelog.com/), versioning follows [Semantic Versioning](https://semver.org/).

## [1.0.0] - 2026-09-17

### Added
- Initial tracked version of the heater/fan relay controller (Fan GPIO4, Heat1 1000W GPIO5, Heat2 1500W GPIO6, Swing GPIO7).
- MQTT command topic `zeep/pod1/controlhub1/command`, status/event topics for reporting back to the Raspberry Pi.
- Combined usage-level commands `level1` (fan only), `level2` (fan + heat1), `level3` (fan + heat1 + heat2).
- Granular commands: `fan_on/off`, `heat1_on/off`, `heat2_on/off`, `swing_on/off`, `all_off`, `status`.
- Serial-only tools: `state`, `test`, `help`.

### Known issues
- `heat1_on`/`heat2_on` do not verify the fan is running first; only the `level2`/`level3` presets guarantee that ordering.
