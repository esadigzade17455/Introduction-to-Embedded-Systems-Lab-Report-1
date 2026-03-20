#include <Arduino.h>

// --- 1. Define Pins ---
// Joystick connections
const int pinSW = 2;   // Push-button (digital input)
const int pinX  = A0;  // X-axis (analog input)
const int pinY  = A1;  // Y-axis (analog input)

// LED pins used to indicate direction
const int ledUp    = 6;
const int ledDown  = 9;
const int ledLeft  = 10;
const int ledRight = 11;

// --- 2. Thresholds and dead-zone ---
// Values around center (~512) are ignored to prevent noise/jitter
const int thresholdLow  = 400;   // Lower limit for movement detection
const int thresholdHigh = 600;   // Upper limit for movement detection

void setup() {
  // Initialize serial communication (high speed for GUI data transfer)
  Serial.begin(115200);

  // Configure joystick button as input with internal pull-up resistor
  // Default = HIGH, pressed = LOW
  pinMode(pinSW, INPUT_PULLUP);

  // Configure LED pins as outputs
  pinMode(ledUp, OUTPUT);
  pinMode(ledDown, OUTPUT);
  pinMode(ledLeft, OUTPUT);
  pinMode(ledRight, OUTPUT);
}

void loop() {
  // Record start time to measure loop execution time (sampling speed)
  unsigned long startTime = micros();

  // --- Read joystick inputs ---
  int valX = analogRead(pinX);   // Read X-axis (0–1023)
  int valY = analogRead(pinY);   // Read Y-axis (0–1023)
  int valSW = digitalRead(pinSW); // Read button state (0 or 1)

  // --- Reset all LEDs ---
  // Ensures only the correct direction LED is active
  digitalWrite(ledUp, LOW);
  digitalWrite(ledDown, LOW);
  digitalWrite(ledLeft, LOW);
  digitalWrite(ledRight, LOW);

  // --- Direction detection logic ---
  // Y-axis controls UP/DOWN
  if (valY < thresholdLow) {
    digitalWrite(ledUp, HIGH);      // Joystick pushed up
  } 
  else if (valY > thresholdHigh) {
    digitalWrite(ledDown, HIGH);    // Joystick pushed down
  }

  // X-axis controls LEFT/RIGHT
  if (valX < thresholdLow) {
    digitalWrite(ledLeft, HIGH);    // Joystick pushed left
  } 
  else if (valX > thresholdHigh) {
    digitalWrite(ledRight, HIGH);   // Joystick pushed right
  }

  // --- Button logic ---
  // If button is pressed (LOW due to pull-up), turn on all LEDs
  if (valSW == LOW) {
    digitalWrite(ledUp, HIGH);
    digitalWrite(ledDown, HIGH);
    digitalWrite(ledLeft, HIGH);
    digitalWrite(ledRight, HIGH);
  }

  // --- Serial output ---
  // Format: X,Y,SW,LoopTime
  // Used by GUI to display joystick data and calculate sample rate
  unsigned long endTime = micros();
  unsigned long loopTime = endTime - startTime; // Time per loop in microseconds

  Serial.print(valX);      // X-axis value
  Serial.print(",");
  Serial.print(valY);      // Y-axis value
  Serial.print(",");
  Serial.print(valSW);     // Button state
  Serial.print(",");
  Serial.println(loopTime); // Loop execution time (for sample rate)

  // Small delay to stabilize serial communication and GUI updates
  delay(10);
}
