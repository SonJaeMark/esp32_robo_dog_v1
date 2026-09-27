#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>
#include <string>
#include "BluetoothSerial.h"

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
BluetoothSerial SerialBT;

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

void setAllServosAngle(int angle) {
  int targetAngle = constrain(angle, 0, 180);
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].write(targetAngle);
  }
}

// Test Servo Angle: Rotates all feet 90 deg clockwise from standing (90 + 90 = 180), then returns slowly
void testServoAngle() {
  drawExcited();
  
  // 1. Move all servos 90 degrees clockwise from their standing baseline (90 -> 180)
  int targetAngles[NUM_SERVOS];
  for (int i = 0; i < NUM_SERVOS; i++) {
    targetAngles[i] = constrain(STAND_ANGLES[i] + 90, 0, 180);
    servos[i].write(targetAngles[i]);
  }
  
  delay(1000); // Hold the test position briefly
  drawSqueezing();

  // 2. Slowly return each servo back to its starting standing angle (90)
  for (int step = 0; step <= 90; step += 2) {
    for (int i = 0; i < NUM_SERVOS; i++) {
      int currentAngle = targetAngles[i] - step;
      if (currentAngle < STAND_ANGLES[i]) {
        currentAngle = STAND_ANGLES[i];
      }
      servos[i].write(currentAngle);
    }
    delay(25); // Control the slowness of the return motion
  }

  // Ensure all servos are precisely back at baseline
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

  // Number of walking steps/cycles
  for (int cycle = 0; cycle < steps; cycle++) {
    // Phase 1: Stretch leg 0 (FL) & leg 3 (BR) forward/back
    // Leg 0 -> 180° (stretch front), Leg 3 -> 0° (stretch back relative to standing)
    drawExcited(); // Show animation during walking
    servos[0].write(180); 
    servos[3].write(0);
    // Corresponding alternate legs return or shift
    servos[1].write(0);
    servos[2].write(180);
    delay(250); // Adjust speed of step

    // Phase 2: Switch diagonal pair positions
    drawSqueezing(); // Show animation during walking
    servos[0].write(0);
    servos[3].write(180);
    servos[1].write(180);
    servos[2].write(0);
    delay(250); // Adjust speed of step
  }

  // Return all legs safely back to the standing baseline (90°)
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

// Global variables for face sequence timing
unsigned long faceTimer = 0;
int currentFaceStep = 0;

// Non-blocking sequence for expression transitions
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
        currentFaceStep = 0; // Reset loop sequence
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

  // Initialize all 4 Servos for 180° operation
  for (int i = 0; i < NUM_SERVOS; i++) {
    servos[i].setPeriodHertz(50);
    // Standard 180° pulse width range: 500us to 2500us
    servos[i].attach(SERVO_PINS[i], 500, 2500);
    
    // Write standing angles (90° for all servos)
    servos[i].write(STAND_ANGLES[i]);
  }

  // Initialize Bluetooth
  SerialBT.begin("ESP32_RoboDog");
  Serial.println("Bluetooth Started! Ready to pair.");
}

unsigned long lastBlinkTime = 0;

void loop() {
  // Bluetooth processing runs continuously
  if (SerialBT.available()) {
    String command = SerialBT.readStringUntil('\n');
    command.trim();
    command.toLowerCase();

    if (command.indexOf("hello") != -1) {
      hello();
    } 
    else if (command.indexOf("test servo") != -1) {
      testServoAngle();
    }
    else if (command.indexOf("play dead") != -1) {
      playDead();
    }
    else if (command.indexOf("sit") != -1) {
      sit();
    }
    else if (command.indexOf("walk") != -1) {
      walk(4); // Walk 4 steps
    }
    else if (command.indexOf("long walk") != -1) {
      walk(8); // Walk 8 steps
    }
    else if (command.indexOf("stand") != -1){
      setAllServosToStand();
    }
    

  }

  // Update idle animation continuously without blocking
  updateFaceSequence();
}