#ifndef MAIN_H
#define MAIN_H

#include "Arduino.h"
#include "Preferences.h"
#include "ArduinoJson.h"
#include "NimBLEDevice.h"
#include "TFT_eSPI.h"
#include "SPI.h"
#include "BleGamepad.h"
#include "Bounce2.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define NAME_CHARACTERISTIC_UUID "73c05096-b481-41aa-b8e9-b86bf434d298"

#define BUTTON_PIN          13
#define RED_LED_PIN         27
#define GREEN_LED_PIN       26
#define BLUE_LED_PIN        25

#define OUTPUT_REPORT_LENGTH 5

void drawConnection(String message);
void drawReceived(String received);
void savePreferences(const char* nameSpace);
void loadPreferences(const char* nameSpace);
void drawStatus();
void initGamepadMode();
void receiveHIDReport();
void initBLEDevice();
void switchLed(int red, int green, int blue);
void ledNotification(int red, int green, int blue);
void buttonLoop();

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override;
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override;
};
  
class CharacteristicCallbacks : public NimBLECharacteristicCallbacks {
  void onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
};

class NameWriteCallback : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) override;
};
  
#endif // MAIN_H