#pragma once

#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#define ENABLE_FIRESTORE
#include <FirebaseClient.h>

// Re-export only the types we need
using FirebaseMapValue = Values::MapValue;
using FirebaseBooleanValue = Values::BooleanValue;
using FirebaseIntegerValue = Values::IntegerValue;
using FirebaseStringValue = Values::StringValue;
