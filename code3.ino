#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Update.h>

#define SERVICE_UUID            "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define OTA_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define LED_CHARACTERISTIC_UUID "62c3efa9-f131-4c4d-a352-d5a6b2952afc"

BLEServer* pServer = NULL;
BLECharacteristic* pOTACharacteristic = NULL;
BLECharacteristic* pLEDCharacteristic = NULL;
bool deviceConnected = false;
bool ledState = false;

const int ledPin = 2; // GPIO pin for the internal LED

size_t updateSize = 0;
size_t updateProgress = 0;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("BLE device connected");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("BLE device disconnected");
    }
};

class OTA_Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      if (value.length() > 0) {
        if (value.substr(0, 3) == "OTA") {
          updateSize = std::stoul(value.substr(3));
          updateProgress = 0;
          Update.begin(updateSize);
          Serial.println("Starting OTA update");
        } else {
          if (Update.write((uint8_t*)value.data(), value.length()) != value.length()) {
            Serial.println("Error applying update");
          }
          updateProgress += value.length();
          Serial.printf("Received %d/%d bytes\n", updateProgress, updateSize);
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

class LED_Callbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      
      if (value.length() > 0) {
        if (value == "ON") {
          digitalWrite(ledPin, HIGH);
          ledState = true;
          Serial.println("LED turned ON");
        } else if (value == "OFF") {
          digitalWrite(ledPin, LOW);
          ledState = false;
          Serial.println("LED turned OFF");
        }
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  
  BLEDevice::init("ESP32_OTA");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pOTACharacteristic = pService->createCharacteristic(
                      OTA_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
  pOTACharacteristic->setCallbacks(new OTA_Callbacks());
  pOTACharacteristic->addDescriptor(new BLE2902());
  
  pLEDCharacteristic = pService->createCharacteristic(
                      LED_CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pLEDCharacteristic->setCallbacks(new LED_Callbacks());

  pService->start();
  Serial.println("BLE service started with UUID: " + String(SERVICE_UUID));

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE advertising started");
}

void loop() {
  delay(1000);
}
