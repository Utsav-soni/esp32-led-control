#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Update.h>

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

size_t updateSize = 0;
size_t updateProgress = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      if (value.length() > 0) {
        if (value.substr(0, 3) == "OTA") {
          updateSize = std::stoul(value.substr(3));
          updateProgress = 0;
          Update.begin(updateSize);
        } else {
          if (Update.write((uint8_t*)value.data(), value.length()) != value.length()) {
            Serial.println("Error applying update");
          }
          updateProgress += value.length();
          if (updateProgress >= updateSize) {
            if (Update.end(true)) {
              Serial.println("Update Success");
              ESP.restart();
            } else {
              Serial.println("Error Occurred. Error #: " + String(Update.getError()));
            }
          }
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  
  BLEDevice::init("ESP32_OTA");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->setCallbacks(new MyCallbacks());
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
}

void loop() {
  delay(1000);
}
