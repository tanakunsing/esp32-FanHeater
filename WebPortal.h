#pragma once

// Local control panel: ESP32 hosts its own Wi-Fi AP + web server (buttons +
// live status), replacing Serial as the way to send control commands.
// Runs alongside the existing Wi-Fi station connection (AP+STA mode).

void webPortalSetup();
void webPortalService();
