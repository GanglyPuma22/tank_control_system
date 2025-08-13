#pragma once
#include <Arduino.h>
#include <time.h>

class TimeOfDay {
public:
  TimeOfDay(uint8_t hour = 0, uint8_t minute = 0)
      : hour(hour), minute(minute) {}

  uint8_t getHour() const { return hour; }
  uint8_t getMinute() const { return minute; }

  // Compare two times
  bool operator<(const TimeOfDay &other) const {
    return (hour < other.hour) || (hour == other.hour && minute < other.minute);
  }

  bool operator>=(const TimeOfDay &other) const { return !(*this < other); }

  // Get current local time from ESP32 NTP (handles PST/PDT if tz set)
  static TimeOfDay now() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return TimeOfDay(0, 0);
    }
    return TimeOfDay(timeinfo.tm_hour, timeinfo.tm_min);
  }

private:
  uint8_t hour;
  uint8_t minute;
};
