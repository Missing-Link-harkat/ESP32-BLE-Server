#include "main.h"

BleGamepad bleGamepad("ESP32 Gamepad", "ESP32", 100);
BleGamepadConfiguration bleGamepadConfiguration;
Bounce debounce = Bounce();

static NimBLEServer* pServer;
ServerCallbacks serverCallbacks;
CharacteristicCallbacks chrCallbacks;

Preferences preferences;

TFT_eSPI tft = TFT_eSPI();

bool programState; // true - gamepad, false - BLE server

void ServerCallbacks::onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
  Serial.printf("Client address: %s\n", connInfo.getAddress().toString().c_str());
  drawConnection(connInfo.getAddress().toString().c_str());
}

void ServerCallbacks::onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
  Serial.printf("Client disconnected, start advertising\n");
  tft.fillRect(0, 0, tft.width(), 48, TFT_BLACK);
  NimBLEDevice::startAdvertising();
}

void CharacteristicCallbacks::onRead(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
  Serial.printf("%s : onRead(), value: %s\n",
    pCharacteristic->getUUID().toString().c_str(),
    pCharacteristic->getValue().c_str());
}

void CharacteristicCallbacks::onWrite(NimBLECharacteristic* pCharacteristic, NimBLEConnInfo& connInfo) {
  String newValue = pCharacteristic->getValue().c_str();
  Serial.printf("%s : onWrite(), value: %s\n", pCharacteristic->getUUID().toString().c_str(), newValue);
  drawReceived(newValue);
  // TODO: Parse received data and control motors
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, newValue);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  String key = doc["key"];

  if (key.equals("a")) {
    digitalWrite(RED_LED_PIN, 255);
    digitalWrite(GREEN_LED_PIN, 0);
    digitalWrite(BLUE_LED_PIN, 0);

  }
}

void savePreferences(bool state) {
  preferences.begin("stateVariable", false);
  preferences.putBool("state", state);
  preferences.end();
}

bool loadPreferences() {
  bool state;
  preferences.begin("stateVariable", true);
  if (!preferences.isKey("state")) {
    Serial.println("State not found, setting default state to false...");
    savePreferences(false);
    state = false;
  } else {
    Serial.println("State successfully found...");
    state = preferences.getBool("state", false);
  }
  preferences.end();
  return state;
}

void drawConnection(String message) {
  tft.fillRect(0, 0, tft.width(), 48, TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.print("Client connected:\n");
  tft.print(message);
}

void drawReceived(String received) {
  tft.fillRect(0, tft.height() - 32, tft.width(), 32, TFT_BLACK);
  tft.setCursor(0, tft.height() - 32);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.print("Last received:\n");
  tft.print(received);
}

void drawStatus() {
  uint32_t color = programState ? TFT_YELLOW : TFT_GREEN;

  int radius;
  for (int i = 1; i < tft.width() / 2; i += 15) {
    tft.drawCircle(tft.width() / 2, tft.height() / 2, i, color);
    radius = i;
    vTaskDelay(75 / portTICK_PERIOD_MS);
  }

  for (int i = radius; i > 0; i -= 15) {
    tft.drawCircle(tft.width() / 2, tft.height() / 2, i, TFT_BLACK);
    vTaskDelay(75 / portTICK_PERIOD_MS);
  }
}

void initGamepadMode() {
  bleGamepadConfiguration.setAutoReport(false);
  bleGamepadConfiguration.setControllerType(CONTROLLER_TYPE_GAMEPAD);

  bleGamepadConfiguration.setEnableOutputReport(true);
  bleGamepadConfiguration.setOutputReportLength(OUTPUT_REPORT_LENGTH);

  bleGamepadConfiguration.setHidReportId(0x05);

  bleGamepadConfiguration.setButtonCount(1);

  bleGamepadConfiguration.setAxesMin(0x0000);
  bleGamepadConfiguration.setAxesMax(0x0FFF);

  bleGamepadConfiguration.setVid(0x1234);

  bleGamepadConfiguration.setPid(0x0001);
  bleGamepad.begin(&bleGamepadConfiguration);
}

void receiveHIDReport() {
  if (bleGamepad.isOutputReceived()) {
    uint8_t* buffer = bleGamepad.getOutputBuffer();

    Serial.print("Received: ");
    for (int i = 0; i < OUTPUT_REPORT_LENGTH; i++) {
      Serial.printf("0x%X ", buffer[i]);
    }

    Serial.println();

    int leftMotor = buffer[1];
    int rightMotor = buffer[2];
    int length = buffer[3];
    int isImmidiate = buffer[4];

    Serial.printf("Left motor: %d, Right motor: %d, isImmidiate: %d\n", leftMotor, rightMotor, isImmidiate);
  }
}

void initBLEDevice() {
  Serial.println("Starting NimBLE Server");

  NimBLEDevice::init("ESP32");
  NimBLEDevice::setSecurityAuth(false, false, true);
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);

  NimBLEService* pService = pServer->createService(SERVICE_UUID);
  NimBLECharacteristic* pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE
  );
  pCharacteristic->setValue("Hello World");
  pCharacteristic->setCallbacks(&chrCallbacks);

  NimBLE2904* pDesc2904 = pCharacteristic->create2904();
  pDesc2904->setFormat(NimBLE2904::FORMAT_UTF8);

  pService->start();

  NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setName("ESP32 BLE Server");
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->enableScanResponse(true);
  pAdvertising->start();

  Serial.println("Advertising started");
}

void switchLed() {
  if (programState) {
    Serial.println("Led yellow");
    digitalWrite(RED_LED_PIN, 255);
    digitalWrite(GREEN_LED_PIN, 255);
    digitalWrite(BLUE_LED_PIN, 0);
  } else {
    Serial.println("Led green");
    digitalWrite(RED_LED_PIN, 0);
    digitalWrite(GREEN_LED_PIN, 255);
    digitalWrite(BLUE_LED_PIN, 0);
  }
}

void ledNotification(int red, int green, int blue) {
  digitalWrite(RED_LED_PIN, red);
  digitalWrite(GREEN_LED_PIN, green);
  digitalWrite(BLUE_LED_PIN, blue);
  delay(500);
  if (programState) {
    switchLed();
  }
}

void buttonLoop() {
  debounce.update();
  int debounceState = debounce.read();
  if (debounceState == LOW && debounce.currentDuration() > 3000) {
    programState = !programState;
    savePreferences(programState);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BLE...");
  tft.init();
  tft.fillScreen(TFT_BLACK);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  debounce.attach(BUTTON_PIN);
  debounce.interval(5);

  programState = loadPreferences();
  switchLed();
  drawStatus();

  if (programState) {
    Serial.println("Staring in gamepad mode...");
    initGamepadMode();
  } else {
    Serial.println("Starting in BLE server mode...");
    initBLEDevice();
  }

  if (xTaskCreatePinnedToCore(
    [] (void* pvParameters) {
      while (true) {
        drawStatus();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
      }
    },
    "drawStatusTask",
    2048,
    NULL,
    1,
    NULL,
    0
  ) != pdPASS) {
    Serial.println("Failed to create drawStatusTask");
  } 
}

void loop() {
  if (programState) {
    if (bleGamepad.isConnected()) {
      receiveHIDReport();
    }
  }
  buttonLoop();
}
