#pragma once
// These need to be defined before including FirebaseClient.h
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#include "../../devices/Device.h"
#include <FirebaseClient.h>
#include <WiFiClientSecure.h>

class FirebaseWrapper {
public:
  FirebaseWrapper(const char *apiKey, const char *email, const char *password,
                  const char *dbUrl);

  void begin(const char *stream_path = nullptr);
  void loop();

  // High-level API for DB interaction

  // Explicit overloads for setting values of type const char* and float
  void setValue(const char *path, const char *value);
  void setValue(const char *path, float value);

  // Async get and stream subscribe functions
  void subscribeValue(const char *path);
  void getValue(const char *path);

  // Publish the reported state of all devices to Firebase
  void publishReportedStates();

private:
  static void onSetResultStatic(AsyncResult &r); // static callback
  static void onGetResultStatic(AsyncResult &result);
  static void streamCallback(AsyncResult &result);
  UserAuth userAuth;
  FirebaseApp app;
  WiFiClientSecure sslClient, streamSslClient;
  using AsyncClient = AsyncClientClass;
  AsyncClient asyncClient;
  AsyncClient streamClient;
  RealtimeDatabase database;
  const char *databaseUrl;
};
