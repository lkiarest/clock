#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include "alarm.h"

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyCallbacks: public BLECharacteristicCallbacks {
    private:
      AlarmSetting *alarmSetting;

    public:
      MyCallbacks(AlarmSetting *alarmSetting) {
        this->alarmSetting = alarmSetting;
      }

      void onWrite(BLECharacteristic *pCharacteristic) {
        std::string value = pCharacteristic->getValue();

        int hour, minute;
        sscanf(value.c_str(), "%d %d", &hour, &minute);
        this->alarmSetting->on = 1;
        this->alarmSetting->hour = hour;
        this->alarmSetting->minute = minute;
        writeAlarmSetting(this->alarmSetting);
      }
};

void startBLEService(AlarmSetting *alarmSetting) {
  BLEDevice::init("ESP32-Clock");
  BLEServer *pServer = BLEDevice::createServer();

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setCallbacks(new MyCallbacks(alarmSetting));

  pCharacteristic->setValue("Hello World");
  pService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}
