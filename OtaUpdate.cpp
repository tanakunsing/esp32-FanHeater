#include "OtaUpdate.h"
#include <Update.h>

static const char OTA_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>deviceHeaterFan OTA</title>
<style>
  :root { color-scheme: light dark; }
  body { font-family: -apple-system, Segoe UI, Roboto, sans-serif; margin: 0; padding: 20px;
         background: #101418; color: #eef2f6; max-width: 420px; }
  a { color: #7fb2ff; }
  h2 { font-size: 1.05rem; }
  p { color: #c3ccd6; font-size: 0.85rem; line-height: 1.5; }
  input[type=file] { color: #eef2f6; margin: 10px 0; }
  button { font-size: 1rem; padding: 12px 20px; border-radius: 10px; border: 2px solid #2a323c;
           background: #1a2027; color: #eef2f6; cursor: pointer; }
  button:disabled { opacity: 0.5; }
  #bar { width: 100%; background: #1a2027; border-radius: 8px; overflow: hidden; margin-top: 14px; display: none; }
  #fill { height: 10px; width: 0; background: #2ecc71; }
  #msg { margin-top: 14px; font-size: 0.9rem; }
</style>
</head>
<body>
  <p><a href="/">&larr; back to control panel</a></p>
  <h2>Firmware update (OTA)</h2>
  <p>Sketch &rarr; Export Compiled Binary in Arduino IDE, then pick the
     <code>.ino.bin</code> file below. The device flashes it and reboots on
     its own; you'll lose the connection to it for a few seconds, that's normal.</p>
  <form id="f">
    <input type="file" name="firmware" id="file" accept=".bin" required>
    <br>
    <button type="submit" id="go">Upload &amp; Flash</button>
  </form>
  <div id="bar"><div id="fill"></div></div>
  <div id="msg"></div>
<script>
document.getElementById('f').addEventListener('submit', function (e) {
  e.preventDefault();
  const file = document.getElementById('file').files[0];
  if (!file) return;

  document.getElementById('go').disabled = true;
  document.getElementById('bar').style.display = 'block';
  document.getElementById('msg').textContent = 'Uploading...';

  const data = new FormData();
  data.append('firmware', file);

  const xhr = new XMLHttpRequest();
  xhr.upload.onprogress = function (ev) {
    if (ev.lengthComputable) {
      document.getElementById('fill').style.width = Math.round(ev.loaded / ev.total * 100) + '%';
    }
  };
  xhr.onload = function () {
    document.getElementById('msg').textContent = xhr.status === 200
      ? 'Flashed OK - device is rebooting.'
      : 'Update failed: ' + xhr.responseText;
  };
  xhr.onerror = function () {
    document.getElementById('msg').textContent = 'Connection lost - normal if the device already rebooted successfully.';
  };
  xhr.open('POST', '/update');
  xhr.send(data);
});
</script>
</body>
</html>
)rawliteral";

static void handleOtaPage(WebServer& server) {
  server.send_P(200, "text/html", OTA_PAGE);
}

static void handleOtaUploadResult(WebServer& server) {
  const bool ok = !Update.hasError();
  server.send(ok ? 200 : 500, "text/plain", ok ? "OK" : "FAIL");
  if (ok) {
    delay(500);
    ESP.restart();
  }
}

static void handleOtaUpload(WebServer& server) {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("[OTA] Start: %s\n", upload.filename.c_str());
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("[OTA] Success: %u bytes\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

void otaAttachRoutes(WebServer& server) {
  server.on("/update", HTTP_GET, [&server]() { handleOtaPage(server); });
  server.on(
    "/update", HTTP_POST,
    [&server]() { handleOtaUploadResult(server); },
    [&server]() { handleOtaUpload(server); }
  );
}
