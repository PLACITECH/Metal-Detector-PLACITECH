/*  PROJECT: DIY Metal Detector Using LC Resonance
    ORIGINAL AUTHOR: Geo Programmer -- https://www.youtube.com/@GeoProgrammer
    MODIFIED BY: PLACITECH
    LICENSE: MIT

    DESCRIPTION:
    This sketch powers a simple DIY metal detector using an LC resonant circuit. It detects the presence of
    nearby metal objects by measuring subtle shifts in the coil's inductance. The system charges and discharges
    a capacitor while pulsing the coil, then uses analog readings to calculate variation.

    When a metal object alters the inductance, the Arduino triggers a buzzer and LED to notify the user — 
    with pitch and beep speed representing the level of variation. This is a great introductory project
    for learning about resonance, analog signal sampling, and embedded systems.

    SOCIALS:
    📺 YouTube:  https://youtube.com/@PLACITECH
    📸 Instagram: https://instagram.com/placitech_
    🎵 TikTok:   https://tiktok.com/@placitech

    DATE: June 2025
*/

#define SIGNAL_PIN A2     // Analog pin to read capacitor voltage
#define BUZZER_PIN 5      // Buzzer pin
#define COIL_PIN A4       // Coil pulse output pin
#define INDICATOR_LED 3   // LED pin for visual feedback

// Internal state tracking
long expectedTotal = 0;
long skippedCount = 0;
long variation = 0;
long lastBeepTime = 0;
long beepInterval = 0;
byte initialized = 0;

// Sends a short pulse to excite the LC circuit
void triggerCoil() {
  for (int pulse = 0; pulse < 3; pulse++) {
    digitalWrite(COIL_PIN, HIGH);
    delayMicroseconds(3);
    digitalWrite(COIL_PIN, LOW);
    delayMicroseconds(3);
  }
}


void setup() {
  Serial.begin(9600);

  pinMode(COIL_PIN, OUTPUT);
  digitalWrite(COIL_PIN, LOW);

  pinMode(SIGNAL_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  pinMode(INDICATOR_LED, OUTPUT);
  digitalWrite(INDICATOR_LED, LOW);
}



void loop() {
  int minimumValue = 1023;
  int maximumValue = 0;
  unsigned long signalSum = 0;

  // Read signal 256 times to get a batch sample
  for (int i = 0; i < 256; i++) {
    // Discharge capacitor
    pinMode(SIGNAL_PIN, OUTPUT);
    digitalWrite(SIGNAL_PIN, LOW);
    delayMicroseconds(20);
    pinMode(SIGNAL_PIN, INPUT);

    triggerCoil(); // Apply pulses

    int sample = analogRead(SIGNAL_PIN);
    minimumValue = min(sample, minimumValue);
    maximumValue = max(sample, maximumValue);
    signalSum += sample;

    unsigned long currentTime = millis();
    byte alertState = 0;

    // Quick update mode: check if output should change
    if (currentTime < lastBeepTime + 10) {
      if (variation > 0) alertState = 1;    // Metal increases inductance
      else if (variation < 0) alertState = 2; // Metal decreases inductance
    }

    // Regular update: handle buzzer interval
    if (currentTime > lastBeepTime + beepInterval) {
      if (variation > 0) alertState = 1;
      else if (variation < 0) alertState = 2;
      lastBeepTime = currentTime;
    }

    // If interval is too long, turn everything off
    if (beepInterval > 300) {
      alertState = 0;
    }

    // Handle buzzer and LED
    if (alertState == 0) {
      digitalWrite(INDICATOR_LED, LOW);
      noTone(BUZZER_PIN);
    } else if (alertState == 1) {
      tone(BUZZER_PIN, 2500);
      digitalWrite(INDICATOR_LED, HIGH);
    } else if (alertState == 2) {
      tone(BUZZER_PIN, 1500);
      digitalWrite(INDICATOR_LED, HIGH);
    }
  }

  // Remove outliers
  signalSum -= minimumValue;
  signalSum -= maximumValue;

  // First run: initialize expected value
  if (expectedTotal == 0) {
    expectedTotal = signalSum << 6;
  }

  long averageSum = (expectedTotal + 32) >> 6;
  variation = signalSum - averageSum;

  // Update expected value if change is small
  if (abs(variation) < (averageSum >> 10)) {
    expectedTotal += signalSum - averageSum;
    skippedCount = 0;
  } else {
    skippedCount++;
  }

  // If too many skips at startup, reinitialize average
  if ((skippedCount > 1) && (initialized == 0)) {
    expectedTotal = signalSum << 6;
    skippedCount = 0;
    initialized = 1;
  }

  // Convert variation into beep rate
  if (variation == 0) {
    beepInterval = 1000000; // No beep
  } else {
    beepInterval = averageSum / (2 * abs(variation));
  }
}