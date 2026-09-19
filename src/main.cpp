// First BLE app: a GATT server that advertises one service with one
// notifying characteristic. Connect to it with the "nRF Connect" app
// (Android/iOS) to see it in action.
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// Randomly generated UUIDs — any two unique UUIDs work, these just avoid
// colliding with real standardized BLE services.
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint32_t counter = 0;

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("Client disconnected, restarting advertising");
    pServer->startAdvertising();
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE work!");

  BLEDevice::init("ESP32-BLE-Demo");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setValue("0");

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();

  Serial.println("Advertising started, waiting for a client to connect...");
}

void loop() {
  if (deviceConnected) {
    counter++;
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)counter);
    pCharacteristic->setValue(buf);
    pCharacteristic->notify();
    Serial.printf("Notified value: %s\n", buf);
  }
  delay(2000);
}
