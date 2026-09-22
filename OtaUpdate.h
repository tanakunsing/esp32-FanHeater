#pragma once

#include <WebServer.h>

// Browser-based firmware update (like WiFiManager's OTA page): a page to
// pick a .bin file, uploaded straight into the other OTA partition via the
// built-in Update library, then the device reboots into it.
//
// Attaches GET/POST /update to an already-created WebServer; call this once
// from webPortalSetup() after the server object exists.
void otaAttachRoutes(WebServer& server);
