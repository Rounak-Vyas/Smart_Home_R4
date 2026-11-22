// Hardware config
const int LED_PIN = 6; // PWM pin (keeps Pin 9 free for the SG90)

// 5 perceptually balanced brightness levels (0-255): Dimmest -> Max
const int BRIGHTNESS_LEVELS[] = {5, 20, 60, 140, 255};
const int NUM_LEVELS          = sizeof(BRIGHTNESS_LEVELS) / sizeof(BRIGHTNESS_LEVELS[0]);

// Timing
const unsigned long STEP_INTERVAL_MS = 5000; // Change brightness every 5 seconds

// State
int levelIdx = 0;
int rampDir  = 1; // +1 = brightening, -1 = dimming
unsigned long lastStepTime = 0;

void printStatus() {
  if (levelIdx == NUM_LEVELS - 1) Serial.print("[MAX BRIGHTNESS] ");
  else if (levelIdx == 0)         Serial.print("[MIN BRIGHTNESS] ");
  else if (rampDir > 0)           Serial.print("[BRIGHTENING]    ");
  else                            Serial.print("[DIMMING]        ");

  Serial.print("Level ");
  Serial.print(levelIdx + 1);
  Serial.print("/");
  Serial.print(NUM_LEVELS);
  Serial.print(" | PWM: ");
  Serial.print(BRIGHTNESS_LEVELS[levelIdx]);
  Serial.print(" (");
  Serial.print((BRIGHTNESS_LEVELS[levelIdx] * 100) / 255);
  Serial.println("%)");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  analogWrite(LED_PIN, BRIGHTNESS_LEVELS[levelIdx]);
  lastStepTime = millis();

  Serial.println("UNO R4 | LED Brightness Ramp Test");
  printStatus();
}

void loop() {
  unsigned long now = millis();

  // Ping-pong brightness every 5 seconds (1->2->3->4->5->4->3->2->1...)
  if (now - lastStepTime >= STEP_INTERVAL_MS) {
    lastStepTime = now;
    levelIdx += rampDir;

    if (levelIdx >= NUM_LEVELS - 1) {
      levelIdx = NUM_LEVELS - 1;
      rampDir = -1;
    } else if (levelIdx <= 0) {
      levelIdx = 0;
      rampDir = 1;
    }

    analogWrite(LED_PIN, BRIGHTNESS_LEVELS[levelIdx]);
    printStatus();
  }
}