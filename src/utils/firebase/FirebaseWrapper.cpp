#include "FirebaseWrapper.h"

FirebaseWrapper::FirebaseWrapper(const char *apiKey, const char *email,
                                 const char *password, const char *dbUrl)
    : userAuth(apiKey, email, password), asyncClient(sslClient),
      databaseUrl(dbUrl) {}

// FirebaseWrapper::StringCallback FirebaseWrapper::lastStringCallback =
// nullptr; FirebaseWrapper::FloatCallback FirebaseWrapper::lastFloatCallback =
// nullptr; FirebaseWrapper::ValueType FirebaseWrapper::lastValueType =
//     FirebaseWrapper::ValueType::STRING;

void FirebaseWrapper::begin(const char *stream_path) {
  // Attach ssl clients to their respective async clients
  asyncClient.setClient(sslClient);
  streamClient.setClient(streamSslClient);

  sslClient.setInsecure(); // skip cert check for now
  sslClient.setHandshakeTimeout(5);
  streamSslClient.setInsecure();
  streamSslClient.setHandshakeTimeout(5);

  // Initialize app with async client and user auth, no callback
  // Callbacks are handled at setValue and getValue calls
  initializeApp(asyncClient, app, getAuth(userAuth));

  app.getApp<RealtimeDatabase>(database);
  database.url(databaseUrl);
  Serial.println("🔥 Firebase initialized 🔥");

  if (!app.ready()) {
    Serial.println("App not ready, auth issue?");
  } else {
    Serial.printf("Token: %s\n", app.getToken().c_str());
  }

  streamClient.setSSEFilters("get,put,patch,keep-alive,cancel,auth_revoked");
  if (stream_path) {
    Serial.printf("Starting stream at path: %s\n", stream_path);
    subscribeValue(stream_path);
  }
}

void FirebaseWrapper::loop() {
  app.loop();
  // database.loop();
}

void FirebaseWrapper::setValue(const char *path, const char *value) {
  if (app.ready()) {
    database.set<const char *>(asyncClient, path, value,
                               &FirebaseWrapper::onSetResultStatic,
                               "dbSetTask");
  }
}

void FirebaseWrapper::setValue(const char *path, float value) {
  if (app.ready()) {
    database.set<float>(asyncClient, path, value,
                        &FirebaseWrapper::onSetResultStatic, "dbSetTask");
  }
}

void FirebaseWrapper::getValue(const char *path) {
  if (app.ready()) {

    database.get(asyncClient, path, onGetResultStatic,
                 false /* only for Stream */, "dbGetTask");
  }
}

void FirebaseWrapper::subscribeValue(const char *path) {
  database.get(streamClient, path, streamCallback,
               true /* SSE mode (HTTP Streaming) */, "dbStreamTask");
}

void FirebaseWrapper::streamCallback(AsyncResult &aResult) {
  // Exits when no result is available when calling from the loop.
  if (!aResult.isResult())
    return;

  if (aResult.isEvent()) {
    Firebase.printf("Event task: %s, msg: %s, code: %d\n",
                    aResult.uid().c_str(), aResult.eventLog().message().c_str(),
                    aResult.eventLog().code());
  }

  if (aResult.isDebug()) {
    Firebase.printf("Debug task: %s, msg: %s\n", aResult.uid().c_str(),
                    aResult.debug().c_str());
  }

  if (aResult.isError()) {
    Firebase.printf("Error task: %s, msg: %s, code: %d\n",
                    aResult.uid().c_str(), aResult.error().message().c_str(),
                    aResult.error().code());
  }

  if (aResult.available()) {
    RealtimeDatabaseResult &stream = aResult.to<RealtimeDatabaseResult>();
    if (stream.isStream()) {
      Serial.println("----------------------------");
      Firebase.printf("task: %s\n", aResult.uid().c_str());
      Firebase.printf("event: %s\n", stream.event().c_str());
      Firebase.printf("path: %s\n", stream.dataPath().c_str());
      Firebase.printf("data: %s\n", stream.to<const char *>());
      Firebase.printf("type: %d\n", stream.type());

      String path = stream.dataPath();
      String end_identifier = "/desired";
      Serial.printf("Full path recieved for stream: %s\n", path.c_str());
      if (path.endsWith(end_identifier)) {
        String devName = path.substring(
            1, path.length() -
                   end_identifier.length()); // strip /devices/ and /desired
        Serial.printf("Received desired stat for device name: %s\n",
                      devName.c_str());
        auto it = Device::getDevice(devName.c_str());
        if (it) {                                        // nullptr check
          String payloadStr = stream.to<const char *>(); // get raw JSON
          JsonDocument doc; // adjust size as needed
          DeserializationError err = deserializeJson(doc, payloadStr);
          if (!err) {
            it->applyState(doc.as<JsonVariantConst>());
          } else {
            Serial.printf("Failed to parse JSON: %s\n", err.c_str());
          }
        }
      }
    } else {
      Serial.println("----------------------------");
      Firebase.printf("task: %s, payload: %s\n", aResult.uid().c_str(),
                      aResult.c_str());
    }

#if defined(ESP32) || defined(ESP8266)
    Firebase.printf("Free Heap: %d\n", ESP.getFreeHeap());
#endif
  }
}

void FirebaseWrapper::publishReportedStates() {
  const auto &allDevices = Device::getAllDevices();

  for (const auto &[name, dev] : allDevices) {
    JsonDocument doc;
    dev->reportState(doc);

    String path = "/devices/" + String(name.c_str()) + "/reported";
    Serial.printf("Publishing reported state for device %s to path %s\n",
                  name.c_str(), path.c_str());
    // TODO MOVE TO HELPER
    String jsonStr;
    serializeJson(doc, jsonStr);
    object_t json(jsonStr.c_str()); // convert ArduinoJson → Firebase object_t

    database.set<object_t>(asyncClient, path, json, onSetResultStatic,
                           "publishState");
  }
}

void FirebaseWrapper::onSetResultStatic(AsyncResult &result) {
  if (!result.isResult())
    return;

  if (result.isError()) {
    Firebase.printf("[Firebase Error] - Set -  %s code: %d\n",
                    result.error().message().c_str(), result.error().code());
  }
  if (result.available()) {
    // Firebase.printf("[Firebase Data] %s\n", result.c_str());
  }
}

// // Static callback for both String and float
void FirebaseWrapper::onGetResultStatic(AsyncResult &result) {
  if (!result.isResult() || result.isError()) {
    Firebase.printf("[Firebase Error] - Get - %s\n",
                    result.error().message().c_str());
    return;
  }

  if (result.available()) {
    Serial.println("----------------------------");
    Firebase.printf("task: %s, payload: %s\n", result.uid().c_str(),
                    result.c_str());
  }
}