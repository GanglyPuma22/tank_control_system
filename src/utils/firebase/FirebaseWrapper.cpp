#include "FirebaseWrapper.h"

// Static member initialization
String FirebaseWrapper::lastUpdatedDeviceName = "";

FirebaseWrapper::FirebaseWrapper(const char *apiKey, const char *email,
                                 const char *password, const char *dbUrl)
    : userAuth(apiKey, email, password), asyncClient(sslClient),
      databaseUrl(dbUrl) {}

void FirebaseWrapper::begin(const char *dataStreamPath) {
  // Attach ssl clients to their respective async clients
  asyncClient.setClient(sslClient);
  dataStreamClient.setClient(dataStreamSslClient);

  sslClient.setInsecure(); // skip cert check for now
  sslClient.setHandshakeTimeout(5);
  dataStreamSslClient.setInsecure();
  dataStreamSslClient.setHandshakeTimeout(5);

  // Initialize app with async client and user auth, no callback
  initializeApp(asyncClient, app, getAuth(userAuth));
  app.getApp<Firestore::Documents>(firestoreDocs);
  app.getApp<RealtimeDatabase>(database);
  database.url(databaseUrl);
  Serial.println("🔥 Firebase initialized 🔥");

  if (!app.ready()) {
    Serial.println("App not ready, auth issue?");
  } else {
    Serial.printf("Token: %s\n", app.getToken().c_str());
  }

  dataStreamClient.setSSEFilters(
      "get,put,patch,keep-alive,cancel,auth_revoked");
  if (dataStreamPath) {
    Serial.printf("Starting data stream listener path: %s\n", dataStreamPath);
    subscribeValue(dataStreamPath);
  }
}

String FirebaseWrapper::getBaseLogPath() {
  return "user_logs/" + String(FIREBASE_USER_NAME) + "/tanks/" +
         String(FIREBASE_TANK_NAME);
};

void FirebaseWrapper::logSensorEvent(
    const char *sensorName, std::optional<std::tuple<float, float>> sensorData,
    const char *label1, const char *label2) {
  String documentPath =
      getBaseLogPath() + "/sensors/" + String(sensorName) + "/events/";

  // Use current time in seconds since epoch
  time_t now;
  time(&now);
  Values::TimestampValue timeStampV(getTimestampString(now, 0));

  Document<Values::Value> doc("timeString", Values::Value(timeStampV));

  Values::MapValue map;
  if (sensorData) {
    auto [sensorValue, sensorValue2] = *sensorData;
    Values::DoubleValue sensorValueV(sensorValue);
    Values::DoubleValue sensorValue2V(sensorValue2);
    map.add(label1, Values::Value(sensorValueV))
        .add(label2, Values::Value(sensorValue2V));
  } else {
    Values::StringValue errorMsgV("sensor read failed");
    map.add("error", Values::Value(errorMsgV));
  }

  doc.add("data", Values::Value(map));

  // Async call with callback function.
  this->firestoreDocs.createDocument(
      this->asyncClient, Firestore::Parent(FIREBASE_PROJECT_ID), documentPath,
      DocumentMask(), doc, &FirebaseWrapper::onLogResultStatic,
      "createDocumentTask");
}

// void FirebaseWrapper::logStatusEvent(const char *statusMessage,
//                                      const char *status_type) {

//   String documentPath = getBaseLogPath() + "/status/";

//   Values::TimestampValue timeStampV(TimeOfDay::currentTimeStringISO());
//   Values::StringValue statusMsgV(statusMessage);
//   Values::StringValue statusTypeV(status_type);

//   Document<Values::Value> doc("timeString", Values::Value(timeStampV));
//   doc.add("statusMessage", Values::Value(statusMsgV))
//       .add("statusType", Values::Value(statusTypeV));
//   // Async call with callback function.
//   this->firestoreDocs.createDocument(
//       this->asyncClient, Firestore::Parent(FIREBASE_PROJECT_ID),
//       documentPath, DocumentMask(), doc, &FirebaseWrapper::onLogResultStatic,
//       "createDocumentTask");
// }

void FirebaseWrapper::logDeviceEvent(Values::MapValue &map,
                                     const char *deviceName,
                                     const char *event_type,
                                     const char *event_desc) {

  String documentPath =
      getBaseLogPath() + "/devices/" + String(deviceName) + "/events/";

  // Use current time in seconds since epoch
  time_t now;
  time(&now);
  Values::TimestampValue timeStampV(getTimestampString(now, 0));

  Values::StringValue eventTypeV(event_type);
  Values::StringValue eventDescV(event_desc);

  Document<Values::Value> doc("timeString", Values::Value(timeStampV));
  doc.add("eventType", Values::Value(eventTypeV))
      .add("eventDesc", Values::Value(eventDescV))
      .add("data", Values::Value(map));
  // Async call with callback function.
  this->firestoreDocs.createDocument(
      this->asyncClient, Firestore::Parent(FIREBASE_PROJECT_ID), documentPath,
      DocumentMask(), doc, &FirebaseWrapper::onLogResultStatic,
      "createDocumentTask");
}

void FirebaseWrapper::loop() {
  app.loop();

  // One time initialization to update devices to last desired state in database
  // Struggled getting this done in setup since the database connections dont
  // have time to settle
  if (app.ready() && !devicesInitialized) {
    fetchAndApplyDesiredStates();
    devicesInitialized = true;
  }

  if (app.ready() && !FirebaseWrapper::lastUpdatedDeviceName.isEmpty()) {
    auto device = Device::getDevice(
        FirebaseWrapper::lastUpdatedDeviceName.c_str()); // get device pointer
    if (device) {
      Values::MapValue map;
      device->logState(map);
      logDeviceEvent(map, FirebaseWrapper::lastUpdatedDeviceName.c_str(),
                     "update_state", "");
    }

    FirebaseWrapper::lastUpdatedDeviceName = "";
  }
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

void FirebaseWrapper::subscribeValue(const char *path) {
  database.get(dataStreamClient, path, dataStreamCallback,
               true /* SSE mode (HTTP Streaming) */, "dbStreamTask");
}

void FirebaseWrapper::dataStreamCallback(AsyncResult &aResult) {
  // Exits when no result is available when calling from the loop.
  if (!aResult.isResult())
    return;

  if (aResult.isEvent()) {
    Firebase.printf("Event task: %s, msg: %s, code: %d\n",
                    aResult.uid().c_str(), aResult.eventLog().message().c_str(),
                    aResult.eventLog().code());
  }

  if (aResult.isError()) {
    Firebase.printf("Error task: %s, msg: %s, code: %d\n",
                    aResult.uid().c_str(), aResult.error().message().c_str(),
                    aResult.error().code());
  }

  if (aResult.available()) {
    RealtimeDatabaseResult &streamResult = aResult.to<RealtimeDatabaseResult>();
    if (streamResult.isStream()) {
      String path = streamResult.dataPath();
      String end_identifier = "/desired";
      Serial.printf("Full path recieved for streamResult: %s\n", path.c_str());
      if (path.endsWith(end_identifier)) {
        String deviceName = path.substring(
            1, path.length() -
                   end_identifier.length()); // strip /devices/ and /desired
        Serial.printf("Received desired state for device name: %s\n",
                      deviceName.c_str());
        auto it = Device::getDevice(deviceName.c_str());
        if (it) {                                              // nullptr check
          String payloadStr = streamResult.to<const char *>(); // get raw JSON
          JsonDocument doc; // adjust size as needed
          DeserializationError err = deserializeJson(doc, payloadStr);
          if (!err) {
            it->applyState(doc.as<JsonVariantConst>());
            lastUpdatedDeviceName = deviceName;
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
    String jsonStr;
    serializeJson(doc, jsonStr);
    object_t json(jsonStr.c_str()); // convert ArduinoJson → Firebase object_t

    database.set<object_t>(asyncClient, path, json, onSetResultStatic,
                           "publishState");
  }
}

void FirebaseWrapper::fetchAndApplyDesiredStates() {
  if (!app.ready()) {
    return;
  }
  const auto &allDevices = Device::getAllDevices();

  for (const auto &[name, device] : allDevices) {
    String path = "/devices/" + String(name.c_str()) + "/desired";
    Serial.printf("Fetching desired state for device %s from path %s\n",
                  name.c_str(), path.c_str());

    // Using await get method since this function is called at setup
    const char *desiredState = database.get<const char *>(asyncClient, path);
    JsonDocument doc; // adjust size as needed
    Values::MapValue map;
    DeserializationError err = deserializeJson(doc, desiredState);
    if (!err) {
      device->applyState(doc.as<JsonVariantConst>());
      device->logState(map);
      logDeviceEvent(map, name.c_str(), "initial_state",
                     "Initial desired state applied to device successfully");
    } else {
      logDeviceEvent(map, name.c_str(), "error", err.c_str());
    }
  }
}

void FirebaseWrapper::onSetResultStatic(AsyncResult &result) {
  if (!result.isResult())
    return;

  if (result.isError()) {
    Firebase.printf("[Firebase Error] - Set -  %s code: %d\n",
                    result.error().message().c_str(), result.error().code());
  }
}

void FirebaseWrapper::onLogResultStatic(AsyncResult &result) {
  if (!result.isResult())
    return;

  if (result.isError()) {
    Firebase.printf("[Firebase Error] - Log -  %s code: %d\n",
                    result.error().message().c_str(), result.error().code());
  }
}

// Add helper function for proper timestamp formatting
String FirebaseWrapper::getTimestampString(uint64_t sec, uint32_t nano) {
  if (sec > 0x3afff4417f)
    sec = 0x3afff4417f;

  if (nano > 0x3b9ac9ff)
    nano = 0x3b9ac9ff;

  time_t now;
  struct tm ts;
  char buf[80];
  now = sec;
  ts = *gmtime(&now); // Use gmtime for UTC

  String format = "%Y-%m-%dT%H:%M:%S";

  if (nano > 0) {
    String fraction = String(double(nano) / 1e9, 9);
    fraction.remove(0, 1);
    format += fraction;
  }
  format += "Z"; // Always end with Z for UTC

  strftime(buf, sizeof(buf), format.c_str(), &ts);
  return buf;
}