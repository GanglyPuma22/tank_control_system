#pragma once
// These need to be defined before including FirebaseClient.h
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#define ENABLE_FIRESTORE
#include <FirebaseClient.h>

#include "../../config/Credentials.h"
#include "../../devices/Device.h"
#include "../TimeOfDay.h"
#include <WiFiClientSecure.h>
#include <optional>
#include <tuple>

class FirebaseWrapper {
public:
  FirebaseWrapper(const char *apiKey, const char *email, const char *password,
                  const char *dbUrl);

  void begin(const char *dataStreamPath = nullptr);
  void loop();

  // High-level API for DB interaction

  // Explicit overloads for setting values of type const char* and float
  void setValue(const char *path, const char *value);
  void setValue(const char *path, float value);

  // Async get and stream subscribe functions
  void subscribeValue(const char *path);

  // Publish the reported state of all devices to Firebase
  void publishReportedStates();

  // Fetch and apply the desired state for all devices from Firebase
  void fetchAndApplyDesiredStates();

  void logDeviceEvent(Values::MapValue &map, const char *deviceName,
                      const char *eventType, const char *message);
  // void logStatusEvent(const char *statusMessage, const char *status_type);
  void logSensorEvent(const char *sensorName,
                      std::optional<std::tuple<float, float>> sensorData,
                      const char *label1 = "value1",
                      const char *label2 = "value2");
  static String getBaseDatabasePath();

private:
  static void onLogResultStatic(AsyncResult &r); // static callback
  static void onSetResultStatic(AsyncResult &r); // static callback
  static void dataStreamCallback(AsyncResult &result);
  String getBaseLogPath();
  String getTimestampString(uint64_t sec, uint32_t nano); // Add this helper
  static String lastUpdatedDeviceName;
  // TODO Noticed I can set expiration here for token, this might be the cause
  // of the issue where i see the website freeze
  UserAuth userAuth;
  FirebaseApp app;
  WiFiClientSecure sslClient, dataStreamSslClient;
  using AsyncClient = AsyncClientClass;
  AsyncClient asyncClient;
  AsyncClient dataStreamClient;
  RealtimeDatabase database;
  Firestore::Documents firestoreDocs;
  const char *databaseUrl;
  bool devicesInitialized = false;
};
