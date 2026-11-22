// Servo Motor Test -> Half degree motor 
// Goes 0-180 and comes back, first increases speed at every 5 second - goes till max and comes back the same way 

#include <Servo.h>

Servo sg90;

// Hardware config
const int SERVO_PIN = 9;
const int MIN_US    = 550;   // 0° pulse width (safe margin for SG90 end-stops)
const int MAX_US    = 2400;  // 180° pulse width

// Speed levels in degrees/sec: Slowest (~5s per sweep) -> Max (~0.5s per sweep)
const float SPEED_LEVELS[] = {36.0, 72.0, 135.0, 225.0, 360.0};
const int   NUM_LEVELS     = sizeof(SPEED_LEVELS) / sizeof(SPEED_LEVELS[0]);

// Timing
const unsigned long STEP_INTERVAL_MS = 5000; // Change speed every 5 seconds
const unsigned long SERVO_REFRESH_MS = 20;   // 50 Hz native SG90 refresh rate

// Motion state
float currentAngle = 0.0;
int   sweepDir     = 1;      // +1 = 0°->180°, -1 = 180°->0°
int   speedIdx     = 0;
int   rampDir      = 1;      // +1 = speeding up, -1 = slowing down

unsigned long lastServoUpdate = 0;
unsigned long lastSpeedChange = 0;

void writeSmoothAngle(float angle) {
  int pulseUs = MIN_US + (int)((angle / 180.0f) * (MAX_US - MIN_US));
  sg90.writeMicroseconds(pulseUs);
}

void printStatus() {
  if (speedIdx == NUM_LEVELS - 1)      Serial.print("[MAX SPEED]    ");
  else if (speedIdx == 0)              Serial.print("[MIN SPEED]    ");
  else if (rampDir > 0)                Serial.print("[SPEEDING UP]  ");
  else                                 Serial.print("[SLOWING DOWN] ");

  Serial.print("Level ");
  Serial.print(speedIdx + 1);
  Serial.print("/");
  Serial.print(NUM_LEVELS);
  Serial.print(" | ");
  Serial.print(SPEED_LEVELS[speedIdx], 0);
  Serial.print(" deg/s (~");
  Serial.print(180.0f / SPEED_LEVELS[speedIdx], 1);
  Serial.println("s per sweep)");
}

void setup() {
  Serial.begin(115200);
  sg90.attach(SERVO_PIN, MIN_US, MAX_US);
  writeSmoothAngle(currentAngle);

  lastServoUpdate = millis();
  lastSpeedChange = millis();

  Serial.println("UNO R4 | SG90 Smooth Continuous Speed Ramp");
  printStatus();
}

void loop() {
  unsigned long now = millis();

  // 1. Continuous 50 Hz motion
  if (now - lastServoUpdate >= SERVO_REFRESH_MS) {
    float dtSec = (now - lastServoUpdate) / 1000.0f;
    lastServoUpdate = now;

    currentAngle += sweepDir * SPEED_LEVELS[speedIdx] * dtSec;

    if (currentAngle >= 180.0f) {
      currentAngle = 180.0f;
      sweepDir = -1;
    } else if (currentAngle <= 0.0f) {
      currentAngle = 0.0f;
      sweepDir = 1;
    }

    writeSmoothAngle(currentAngle);
  }

  // 2. Ping-pong speed ramp every 5 seconds (1->2->3->4->5->4->3->2->1...)
  if (now - lastSpeedChange >= STEP_INTERVAL_MS) {
    lastSpeedChange = now;
    speedIdx += rampDir;

    if (speedIdx >= NUM_LEVELS - 1) {
      speedIdx = NUM_LEVELS - 1;
      rampDir = -1;
    } else if (speedIdx <= 0) {
      speedIdx = 0;
      rampDir = 1;
    }

    printStatus();
  }
}