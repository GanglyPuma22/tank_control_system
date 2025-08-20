#pragma once
#include "../devices/CameraDevice.h"
#include <WebServer.h>
#include <WiFi.h>

namespace WebServerUtil {

inline WebServer server(80);
inline CameraDevice *cameraDevice = nullptr;
inline unsigned long snapshotIntervalMs = 1000; // default 1 second
// -------------------- MJPEG --------------------
const char *mjpegBoundary = "ESP32MJPEG";
bool streaming = false;

inline void begin(CameraDevice *cam,
                  unsigned long intervalMs = snapshotIntervalMs) {
  cameraDevice = cam;
  snapshotIntervalMs = intervalMs;

  // Serve root page with automatic snapshot refresh
  server.on("/", []() {
    String page = "<h1>ESP32 Tank Control</h1>";

    // Example device controls (generic, can add more later)
    page += "<p><a href='/heatlamp/on'>Heat Lamp ON</a></p>";
    page += "<p><a href='/heatlamp/off'>Heat Lamp OFF</a></p>";
    page += "<p><a href='/light/on'>Light ON</a></p>";
    page += "<p><a href='/light/off'>Light OFF</a></p>";
    // Add stream
    page += "<h1>ESP32 MJPEG Stream</h1>";
    page += "<img src='/stream'>";
    // // Add snapshot image with auto-refresh
    // page += "<h2>Camera Snapshot at interval: " + String(snapshotIntervalMs)
    // +
    //         " ms </h2>";
    // page += "<img src='/snapshot' width='320' height='240'>";
    // page += "<script>setInterval(()=>{"
    //         "document.querySelector('img').src='/snapshot?ts='+Date.now();"
    //         "}, " +
    //         String(snapshotIntervalMs) + ");</script>";

    server.send(200, "text/html", page);
  });

  // // Serve snapshot
  // server.on("/snapshot", []() {
  //   if (cameraDevice) {
  //     cameraDevice->capture(server);
  //   } else {
  //     server.send(503, "text/plain", "No camera configured");
  //   }
  // });

  server.on("/stream", []() {
    if (cameraDevice) {
      cameraDevice->stream(server, mjpegBoundary, streaming);
    } else {
      server.send(503, "text/plain", "No camera configured");
    }
  });

  server.begin();
  Serial.println("Web server started on port 80");
}

inline void handleClient() { server.handleClient(); }

} // namespace WebServerUtil
