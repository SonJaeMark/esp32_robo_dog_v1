#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <string>

// BLE Libraries for ESP32
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// OLED Settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define SCREEN_ADDRESS 0x3C

const int OLED_SDA = 21;
const int OLED_CLK = 22;

// Servos (All 180° Positional Servos)
const int NUM_SERVOS = 4;
const int SERVO_PINS[NUM_SERVOS] = {13, 12, 14, 27};
Servo servos[NUM_SERVOS];

// Standing baseline angles (All set to 90° for standing position)
const int STAND_ANGLES[NUM_SERVOS] = {90, 90, 90, 90};

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// BLE UUIDs (Nordic UART Service standard is widely compatible with MIT App Inventor BLE extensions)
#define SERVICE_UUID "6E400001-B5A3-F393-E0A9-E50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50e24dcca9e"

bool deviceConnected = false;
String receivedCommand = "";
bool newDataReceived = false;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("BLE Client Connected");
  };

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("BLE Client Disconnected");
    // Restart advertising so apps can reconnect easily
    pServer->getAdvertising()->start();
  }
};

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    // Safely converts getValue() whether it returns std::string or Arduino String
    String value = String(pCharacteristic->getValue().c_str());
    
    if (value.length() > 0) {
      receivedCommand = value;
      receivedCommand.trim();
      receivedCommand.toLowerCase();
      newDataReceived = true;
    }
  }
};
// OLED Drawing Helper
void drawEyes(const char* leftEye, const char* rightEye) {
  display.clearDisplay();
  display.setTextSize(3);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(30, 20);
  display.print(leftEye);
  display.setCursor(86, 20);
  display.print(rightEye);

  display.display();
}

// Facial Expressions
void drawCurious()   { drawEyes("O", "O"); }
void drawSqueezing() { drawEyes(">", "<"); }
void drawKnockedOut(){ drawEyes("X", "X"); }
void drawExcited()   { drawEyes("^", "^"); }
void drawCrying()    { drawEyes("T", "T"); }
void drawWink()      { drawEyes(">", "--"); }
void drawDefault1()  { drawEyes("@", "@"); }
void drawDefault0()  { drawEyes("-", "-"); }

// 180° Servo Helpers
void setServoAngle(int index, int angle) {
  if (index < 0 || index >= NUM_SERVOS) return;
  int targetAngle = constrain(angle, 0, 180);
  servos[index].write(targetAngle);
}

void setAllServosToStand() {
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(STAND_ANGLES[i]);
  }
}

void testServoAngle() {
  drawExcited();
  int targetAngles[NUM_SERVOS];
  for (int i = 0; i < NUM_SERVOS; i++) {
    targetAngles[i] = constrain(STAND_ANGLES[i] + 90, 0, 180);
    servos[i].write(targetAngles[i]);
  }
  delay(1000); 
  drawSqueezing();

  for (int step = 0; step <= 90; step += 2) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      int currentAngle = targetAngles[i] - step;
      if (currentAngle < STAND_ANGLES[i]) {
        currentAngle = STAND_ANGLES[i];
      }
      servos[i].write(currentAngle);
    }
    delay(25);
  }

  setAllServosToStand();
  drawDefault1();
}

void hello() {
  setAllServosToStand();
  setServoAngle(2, 55);
  delay(500);
  drawExcited();

  setServoAngle(0, 105);
  setServoAngle(3, 105);
  delay(500);

  setServoAngle(1, 0);
  drawSqueezing();
  delay(500);
  setServoAngle(1, 30);
  delay(500);
  setServoAngle(1, 0);
  delay(500);
  
  setAllServosToStand();
}

void playDead(){
  drawKnockedOut();
  setServoAngle(0, 180);
  setServoAngle(1, 0);
  setServoAngle(2, 0);
  setServoAngle(3, 180);
  delay(1000);
  drawDefault1();
}

void sit(){
  drawCurious();
  setServoAngle(0, 90);
  setServoAngle(1, 90);
  setServoAngle(2, 130);
  setServoAngle(3, 50);
  delay(200);
  drawWink();
  delay(1000);
}

void walk(int steps) {
  for (int cycle = 0; cycle < steps; cycle++) {
    drawExcited(); 
    servos[0].write(180); 
    servos[3].write(0);
    servos[1].write(0);
    servos[2].write(180);
    delay(250); 

    drawSqueezing(); 
    servos[0].write(0);
    servos[3].write(180);
    servos[1].write(180);
    servos[2].write(0);
    delay(250); 
  }
  setAllServosToStand();
  drawDefault1();
}

// Non-blocking timer helper
bool myCustomDelay(unsigned long &lastDelay, unsigned long interval) {
  if (millis() - lastDelay >= interval) {
    lastDelay = millis();
    return true;
  }
  return false;
}

unsigned long faceTimer = 0;
int currentFaceStep = 0;

void updateFaceSequence() {
  switch (currentFaceStep) {
    case 0:
      drawDefault1();
      if (myCustomDelay(faceTimer, 700)) currentFaceStep++;
      break;
    case 1:
      drawDefault0();
      if (myCustomDelay(faceTimer, 100)) currentFaceStep++;
      break;
    case 2:
      drawDefault1();
      if (myCustomDelay(faceTimer, 100)) currentFaceStep++;
      break;
    case 3:
      drawDefault0();
      if (myCustomDelay(faceTimer, 100)) currentFaceStep++;
      break;
    case 4:
      drawDefault1();
      if (myCustomDelay(faceTimer, 2500)) {
        currentFaceStep = 0; 
      }
      break;
  }
}

void setup() {
  Serial.begin(115200);

  // Initialize OLED Display
  Wire.begin(OLED_SDA, OLED_CLK);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  // Allocate PWM Timers for ESP32
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  // Initialize Servos
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].setPeriodHertz(50);
    servos[i].attach(SERVO_PINS[i], 500, 2500);
    servos[i].write(STAND_ANGLES[i]);
  }

  // Initialize BLE Server
  BLEDevice::init("ESP32_RoboDog");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic *pCharacteristicTX = pService->createCharacteristic(
                                        CHARACTERISTIC_UUID_TX,
                                        BLECharacteristic::PROPERTY_NOTIFY
                                      );
  pCharacteristicTX->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristicRX = pService->createCharacteristic(
                                        CHARACTERISTIC_UUID_RX,
                                        BLECharacteristic::PROPERTY_WRITE
                                      );
  pCharacteristicRX->setCallbacks(new MyCallbacks());

  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE Server Started! Ready to pair.");
}

void loop() {
  // Process incoming BLE commands asynchronously
  if (newDataReceived) {
    newDataReceived = false;
    Serial.print("Received BLE Command: ");
    Serial.println(receivedCommand);

    if (receivedCommand.indexOf("hello") != -1) {
      hello();
    } 
    else if (receivedCommand.indexOf("test servo") != -1) {
      testServoAngle();
    }
    else if (receivedCommand.indexOf("play dead") != -1) {
      playDead();
    }
    else if (receivedCommand.indexOf("sit") != -1) {
      sit();
    }
    else if (receivedCommand.indexOf("walk") != -1) {
      walk(4); 
    }
    else if (receivedCommand.indexOf("long walk") != -1) {
      walk(8); 
    }
    else if (receivedCommand.indexOf("stand") != -1){
      setAllServosToStand();
    }
  }

  // Update idle animation continuously without blocking
  updateFaceSequence();
}