
#define BLYNK_TEMPLATE_ID   "TMPL3hdV_9e4_"
#define BLYNK_TEMPLATE_NAME "Smart Home"
#define BLYNK_AUTH_TOKEN    "5wvVE9tqH-x-v6cFb3s__RDPMxQ_BDRQ"

#define BLYNK_PRINT Serial

#include <WiFiS3.h>
#include <BlynkSimpleWifi.h>
#include <Servo.h>

// 2. Wi-Fi Credentials (2.4 GHz network)
char ssid[] = "OnePlus 11 5G";
char pass[] = "b8res5ub";

// Hardware Pins
const int SERVO_PIN = 9;
const int LED_PIN   = 6;

// Servo Calibration
const int   MIN_US           = 550;
const int   MAX_US           = 2400;
const float MIN_SPEED_DEG_S  = 36.0f;
const float MAX_SPEED_DEG_S  = 360.0f;
const unsigned long SERVO_REFRESH_MS = 20; // 50 Hz

Servo sg90;
BlynkTimer timer;

// Cloud-controlled states
bool  servoOn          = false;
int   servoSpeedPct    = 50;
bool  ledOn            = false;
int   ledBrightnessPct = 50;

// Servo motion tracking
float currentAngle    = 0.0f;
int   sweepDir        = 1;
unsigned long lastServoUpdate = 0;

// Staggered telemetry state
int telemetryStep = 0;

// --- Helper Functions ---

void writeSmoothAngle(float angle) {
  int pulseUs = MIN_US + (int)((angle / 180.0f) * (MAX_US - MIN_US));
  sg90.writeMicroseconds(pulseUs);
}

void applyLedState() {
  if (!ledOn || ledBrightnessPct <= 0) {
    analogWrite(LED_PIN, 0);
    return;
  }
  float norm = ledBrightnessPct / 100.0f;
  int pwm = (int)(norm * norm * 255.0f);
  if (pwm < 1) pwm = 1;
  analogWrite(LED_PIN, pwm);
}

void updateServo() {
  unsigned long now = millis();
  if (now - lastServoUpdate < SERVO_REFRESH_MS) return;

  float dtSec = (now - lastServoUpdate) / 1000.0f;
  lastServoUpdate = now;

  // Cap dtSec so any brief Wi-Fi delay never causes a sudden angle jump
  if (dtSec > 0.03f) dtSec = 0.03f;

  if (!servoOn || servoSpeedPct <= 0) return;

  float speedDegPerSec = MIN_SPEED_DEG_S +
                         ((servoSpeedPct / 100.0f) * (MAX_SPEED_DEG_S - MIN_SPEED_DEG_S));

  currentAngle += sweepDir * speedDegPerSec * dtSec;

  if (currentAngle >= 180.0f) {
    currentAngle = 180.0f;
    sweepDir = -1;
  } else if (currentAngle <= 0.0f) {
    currentAngle = 0.0f;
    sweepDir = 1;
  }

  writeSmoothAngle(currentAngle);
}

// --- Staggered Telemetry (1 write per second, no burst stalls) ---

void sendNextTelemetry() {
  int activeServo = servoOn ? servoSpeedPct : 0;
  int activeLed   = ledOn   ? ledBrightnessPct : 0;

  switch (telemetryStep) {
    case 0:
      Blynk.virtualWrite(V4, activeServo);
      break;
    case 1:
      Blynk.virtualWrite(V5, activeLed);
      break;
    case 2:
      Blynk.virtualWrite(V6, (int)(activeServo * 0.7f + activeLed * 0.3f));
      break;
  }
  telemetryStep = (telemetryStep + 1) % 3;
}

// --- Blynk Cloud Handlers ---

BLYNK_CONNECTED() {
  Blynk.syncAll();
}

BLYNK_WRITE(V0) {
  servoOn = (param.asInt() == 1);
  Serial.print("Servo Power: ");
  Serial.println(servoOn ? "ON" : "OFF");
}

BLYNK_WRITE(V1) {
  servoSpeedPct = constrain(param.asInt(), 0, 100);
}

BLYNK_WRITE(V2) {
  ledOn = (param.asInt() == 1);
  applyLedState();
}

BLYNK_WRITE(V3) {
  ledBrightnessPct = constrain(param.asInt(), 0, 100);
  applyLedState();
}

// --- Main Setup & Loop ---

void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  analogWrite(LED_PIN, 0);

  sg90.attach(SERVO_PIN, MIN_US, MAX_US);
  writeSmoothAngle(currentAngle);

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // Send 1 parameter every 1000 ms (full V4/V5/V6 cycle every 3 seconds)
  timer.setInterval(1000L, sendNextTelemetry);
  lastServoUpdate = millis();
}

void loop() {
  Blynk.run();
  timer.run();
  updateServo();
}