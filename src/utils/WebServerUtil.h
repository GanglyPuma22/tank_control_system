#pragma once
#include <WebServer.h>
#include <WiFi.h>

namespace WebServerUtil {

inline WebServer server(80); // Port 80 HTTP server

// Generic type for action handlers
using Handler = std::function<void()>;

// Store dynamic handlers for devices or other actions
struct Route {
  String path;
  Handler handler;
};

inline std::vector<Route> routes;

inline void addRoute(const String &path, Handler handler) {
  routes.push_back({path, handler});
  server.on(path.c_str(), [handler]() {
    handler();
    server.send(200, "text/plain", "OK");
  });
}
// todo: Make html handling more generic
inline void begin() {
  // Default root page (can edit later)
  server.on("/", []() {
    String page = "<h1>ESP32 Tank Control</h1>";
    page += "<p><a href='/heatlamp/on'>Heat Lamp ON</a></p>";
    page += "<p><a href='/heatlamp/off'>Heat Lamp OFF</a></p>";
    page += "<p><a href='/light/on'>Light ON</a></p>";
    page += "<p><a href='/light/off'>Light OFF</a></p>";
    server.send(200, "text/html", page);
  });

  // Status endpoint
  server.on("/status", []() {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  });

  // Register all dynamic routes
  for (auto &r : routes) {
    // Already handled in addRoute
  }

  server.begin();
  Serial.println("Web server started on port 80");
}

inline void handleClient() { server.handleClient(); }

} // namespace WebServerUtil
