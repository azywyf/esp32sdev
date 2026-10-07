#pragma once

#include <Arduino.h>

struct KnownDevice {
  const char *mac;
  const char *label;
};

// Add your own devices here (MAC lookup is case-insensitive).
static const KnownDevice KNOWN_DEVICES[] = {
    {"AA:BB:CC:DD:EE:FF", "My Phone"},
};

static const size_t KNOWN_DEVICES_COUNT =
    sizeof(KNOWN_DEVICES) / sizeof(KNOWN_DEVICES[0]);

inline const char *lookupKnownDevice(const String &mac) {
  for (size_t i = 0; i < KNOWN_DEVICES_COUNT; i++) {
    if (mac.equalsIgnoreCase(KNOWN_DEVICES[i].mac)) {
      return KNOWN_DEVICES[i].label;
    }
  }
  return nullptr;
}
