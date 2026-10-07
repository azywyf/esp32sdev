#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#include "secrets.h"
#include "known_devices.h"
#include "webpage.h"

#define MAX_DEVICES 30
#define DEVICE_TIMEOUT_MS 30000UL
#define SCAN_DURATION_SEC 3

SeenDevice devices[MAX_DEVICES];
int deviceCount = 0;

bool scanning = true;
WebServer server(80);
BLEScan *pBLEScan;

int findDeviceIndex(const String &mac) {
  for (int i = 0; i < deviceCount; i++) {
    if (devices[i].mac == mac) return i;
  }
  return -1;
}

void upsertDevice(const String &mac, const String &name, int rssi) {
  int idx = findDeviceIndex(mac);
  if (idx >= 0) {
    devices[idx].name = name;
    devices[idx].rssi = rssi;
    devices[idx].lastSeenMs = millis();
    return;
  }

  SeenDevice newDevice;
  newDevice.mac = mac;
  newDevice.name = name;
  newDevice.rssi = rssi;
  newDevice.lastSeenMs = millis();
  newDevice.knownLabel = lookupKnownDevice(mac);

  if (deviceCount < MAX_DEVICES) {
    devices[deviceCount++] = newDevice;
  } else {
    int worstIdx = 0;
    for (int i = 1; i < deviceCount; i++) {
      if (devices[i].rssi < devices[worstIdx].rssi) worstIdx = i;
    }
    devices[worstIdx] = newDevice;
  }
}

void ageOutDevices() {
  unsigned long now = millis();
  for (int i = 0; i < deviceCount;) {
    if (now - devices[i].lastSeenMs > DEVICE_TIMEOUT_MS) {
      devices[i] = devices[deviceCount - 1];
      deviceCount--;
    } else {
      i++;
    }
  }
}

class ScanCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    String mac = advertisedDevice.getAddress().toString().c_str();
    String name = advertisedDevice.haveName()
                      ? String(advertisedDevice.getName().c_str())
                      : String("(unnamed)");
    int rssi = advertisedDevice.getRSSI();
    upsertDevice(mac, name, rssi);
  }
};

void handleRoot() {
  server.send(200, "text/html",
              buildDashboardHtml(devices, deviceCount, scanning, millis()));
}

void handleStart() {
  scanning = true;
  Serial.println("Scanning resumed");
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleStop() {
  scanning = false;
  Serial.println("Scanning paused");
  server.sendHeader("Location", "/");
  server.send(303);
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE scanner + dashboard");

  connectWiFi();

  server.on("/", handleRoot);
  server.on("/start", handleStart);
  server.on("/stop", handleStop);
  server.begin();
  Serial.println("Web server started");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new ScanCallbacks(), true);
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  server.handleClient();

  if (scanning) {
    pBLEScan->start(SCAN_DURATION_SEC, false);
    pBLEScan->clearResults();
    ageOutDevices();
  } else {
    delay(200);
  }
}
