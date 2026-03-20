#include <LiquidCrystal.h>   // Library for controlling the 16x2 LCD
#include <Math.h>            // Library for mathematical functions like log10()
// Create the LCD object and define which Arduino pins are connected to it
LiquidCrystal lcd(12, 11, 5, 4, 3, 6);
// --- Pin definitions ---
const byte LED_PIN = 8;   // LED turns on when loud sound is detected
const byte MIC_AO  = A0;  // Analog output pin of microphone sensor
const byte MIC_DO  = 2;   // Digital output pin of microphone sensor (not used in main logic)
// --- Sound detection variables ---
int soundLevel = 0;       // Measured sound amplitude in the current sampling window
int THRESHOLD  = 10;      // Minimum sound level required to detect a loud event
bool aboveThreshold = false; // True if current sound level is above threshold
bool wasAbove = false;       // Stores previous threshold state to detect new events only
unsigned long eventCount = 0; // Counts how many loud sound events happened
// Calculated sound value in approximate decibels
float dBLevel = 0.0;
// --- Variables for sampling sound over a fixed time window ---
unsigned long lastWindowStart = 0;         // Start time of the current sampling window
const unsigned long WINDOW_MS = 50;        // Window duration in milliseconds
int signalMin = 1023;                      // Minimum ADC value found in the window
int signalMax = 0;                         // Maximum ADC value found in the window
// --- LED timing control ---
unsigned long ledStartTime = 0;            // Time when LED was turned on
bool ledOn = false;                        // Tracks whether LED is currently on
const unsigned long LED_ON_TIME = 300;     // Keep LED on for 300 ms after loud event
// --- LCD update timing ---
unsigned long lastLcdUpdate = 0;           // Last time LCD was refreshed
const unsigned long LCD_INTERVAL = 100;    // Refresh LCD every 100 ms
// --- Serial monitor update timing ---
unsigned long lastSerialUpdate = 0;           // Last time serial data was sent
const unsigned long SERIAL_INTERVAL = 100;    // Send serial data every 100 ms
void setup() {
  // Set LED pin as output and make sure LED starts OFF
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // Set microphone digital output pin as input
  pinMode(MIC_DO, INPUT);
  // Initialize the LCD with 16 columns and 2 rows
  lcd.begin(16, 2);
  lcd.clear();
  // Display startup message
  lcd.setCursor(0, 0);
  lcd.print("Sound Monitor");
  lcd.setCursor(0, 1);
  lcd.print("Init...");
  // Start serial communication for PC/GUI monitoring
  Serial.begin(115200);
  // Small startup delay so user can read LCD message
  delay(800);
  lcd.clear();
  // Save the start time of the first sampling window
  lastWindowStart = millis();
}
void loop() {
  // Get current time once at the start of loop for timing checks
  unsigned long now = millis();
  // Read current analog microphone signal
  int raw = analogRead(MIC_AO);
  // Track minimum and maximum microphone values during the sampling window
  if (raw < signalMin) signalMin = raw;
  if (raw > signalMax) signalMax = raw;
  // When the sampling window is complete, process the sound level
  if (now - lastWindowStart >= WINDOW_MS) {
    // Sound level is measured as the peak-to-peak amplitude
    soundLevel = signalMax - signalMin;
    // Convert amplitude to an approximate decibel-like value
    if (soundLevel < 1) {
      dBLevel = 0.0;
    } else {
      dBLevel = 20.0 * log10((float)soundLevel);
    }
    // Check whether sound level is above the threshold
    aboveThreshold = (soundLevel >= THRESHOLD);
    // Count a new sound event only when signal crosses threshold from below to above
    if (aboveThreshold && !wasAbove) {
      eventCount++;                    // Increase loud event counter
      digitalWrite(LED_PIN, HIGH);     // Turn LED on
      ledOn = true;                    // Mark LED as active
      ledStartTime = now;              // Save LED start time
    }
    // Store current threshold state for next window comparison
    wasAbove = aboveThreshold;
    // Reset min and max values for the next sampling window
    signalMin = 1023;
    signalMax = 0;
    lastWindowStart = now;
  }
  // Turn LED off after it has stayed on long enough
  if (ledOn && (now - ledStartTime >= LED_ON_TIME)) {
    digitalWrite(LED_PIN, LOW);
    ledOn = false;
  }
  // Update LCD periodically without blocking the program
  if (now - lastLcdUpdate >= LCD_INTERVAL) {
    lastLcdUpdate = now;
    // Clear and rewrite first line
    lcd.setCursor(0, 0);
    lcd.print("                ");
    lcd.setCursor(0, 0);
    lcd.print("L:");              // Display sound level
    lcd.print(soundLevel);
    lcd.print(" ");
    lcd.print((int)dBLevel);     // Display approximate dB value
    lcd.print("dB");
    // Clear and rewrite second line
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print("T:");             // Display threshold value
    lcd.print(THRESHOLD);
    lcd.print(" ");
    // Display sound condition
    if (aboveThreshold) {
      lcd.print("LOUD");
    } else {
      lcd.print("QUIET");
    }
  }
  // Send monitoring data to Serial periodically
  if (now - lastSerialUpdate >= SERIAL_INTERVAL) {
    lastSerialUpdate = now;
    Serial.print("Level=");
    Serial.print(soundLevel);
    Serial.print(", dB=");
    Serial.print(dBLevel);
    Serial.print(", Threshold=");
    Serial.print(THRESHOLD);
    Serial.print(", Above=");
    Serial.print(aboveThreshold ? 1 : 0);
    Serial.print(", Events=");
    Serial.println(eventCount);
  }
}
