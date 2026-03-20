#include <Wire.h>      // Library for I2C communication with the DS1307 RTC
#include <SevSeg.h>    // Library for controlling the 4-digit 7-segment display

// Create display control object
SevSeg sevseg;

// --- Pin and constant definitions ---
const byte SQW_PIN    = 2;      // DS1307 square-wave output connected to interrupt pin 2
const byte BUTTON_PIN = 3;      // Push-button connected to pin 3
const byte segPins[]   = {4, 5, 6, 7, 8, 9, 10, 11}; // Segment pins a,b,c,d,e,f,g,dp
const byte digitPins[] = {A3, A2, A1, A0};           // Digit control pins for 4-digit display
const byte DS1307_ADDR = 0x68; // I2C address of DS1307 RTC
const byte TARGET = 10;        // Target number player must stop on
const unsigned long WIN_MS = 100;       // Allowed reaction error window: ±100 ms
const unsigned long DEBOUNCE_MS = 30;   // Button debounce time in milliseconds

// Enable display self-test at startup
#define RUN_START_SELFTEST 1

// --- Variables shared with interrupt routine ---
volatile unsigned long lastTickMs = 0;  // Stores time of the latest RTC tick
volatile bool tickFlag = false;         // Flag set by interrupt when a new tick occurs

// --- Game state machine ---
enum State { IDLE, RUNNING, RESULT };
State state = IDLE;               // Initial state: waiting to start
byte counterVal = 0;              // Current displayed counter value
unsigned long targetTickMs = 0;   // Exact time when target value was reached
bool resultSuccess = false;       // Stores whether the attempt was successful

// --- Button debounce variables ---
bool buttonState = HIGH;          // Stable debounced button state
bool lastButtonState = HIGH;      // Previous raw button state
unsigned long lastDebounceTime = 0; // Last time button state changed

// Interrupt Service Routine: called on every 1 Hz pulse from DS1307 SQW output
void onTick() {
  lastTickMs = millis();  // Record current system time
  tickFlag = true;        // Notify main loop that a tick occurred
}

// Ensures the DS1307 clock is running by clearing the CH (Clock Halt) bit if needed
void ds1307EnsureRunning() {
  Wire.beginTransmission(DS1307_ADDR);
  Wire.write((byte)0x00);   // Point to seconds register
  Wire.endTransmission();

  Wire.requestFrom(DS1307_ADDR, (byte)1); // Read seconds register
  if (Wire.available()) {
    byte sec = Wire.read();
    if (sec & 0x80) {       // Check if CH bit is set
      sec &= 0x7F;          // Clear CH bit to start the clock
      Wire.beginTransmission(DS1307_ADDR);
      Wire.write((byte)0x00);
      Wire.write(sec);
      Wire.endTransmission();
    }
  }
}

// Configure DS1307 square-wave output to 1 Hz
void ds1307SetSQW_1Hz() {
  Wire.beginTransmission(DS1307_ADDR);
  Wire.write((byte)0x07);   // Control register address
  Wire.write((byte)0x10);   // Enable SQW output at 1 Hz
  Wire.endTransmission();
}

// Display current counter value on 7-segment display
void showCount(byte v) {
  char buf[5] = "    ";     // Temporary display buffer

  if (v < 10) {
    buf[0] = ' ';
    buf[1] = ' ';
    buf[2] = '0' + v;       // Show single digit
    buf[3] = ' ';
  } else if (v == 10) {
    buf[0] = '1';
    buf[1] = '0';           // Show "10"
    buf[2] = ' ';
    buf[3] = ' ';
  } else {
    buf[0] = buf[1] = buf[2] = buf[3] = '-'; // Error/overflow display
  }

  // Reverse character order to match physical display wiring
  char flipped[5];
  flipped[0] = buf[3];
  flipped[1] = buf[2];
  flipped[2] = buf[1];
  flipped[3] = buf[0];
  flipped[4] = '\0';

  sevseg.setChars(flipped);
}

// Display result message: GOOD for success, FAIL for failure
void showResult(bool ok) {
  char buf[5];
  if (ok) { 
    buf[0]='G'; buf[1]='O'; buf[2]='O'; buf[3]='D'; 
  } else {    
    buf[0]='F'; buf[1]='A'; buf[2]='I'; buf[3]='L'; 
  }

  // Reverse characters to match display orientation
  char flipped[5];
  flipped[0] = buf[3];
  flipped[1] = buf[2];
  flipped[2] = buf[1];
  flipped[3] = buf[0];
  flipped[4] = '\0';

  sevseg.setChars(flipped);
}

// Reset game back to idle state
void resetToIdle() {
  state = IDLE;
  counterVal = 0;
  resultSuccess = false;
  showCount(0);   // Display 0 while idle
}

// Startup self-test to verify all digits/segments work correctly
void displaySelfTest() {
  const char* patterns[] = {"8   ", " 8  ", "  8 ", "   8"};
  for (int r=0; r<2; r++) {
    for (int i=0; i<4; i++) {
      sevseg.setChars(patterns[i]);
      unsigned long t0 = millis();
      while (millis()-t0 < 400) sevseg.refreshDisplay();
    }
  }

  sevseg.setChars("----");
  unsigned long t1 = millis();
  while (millis()-t1 < 300) sevseg.refreshDisplay();
}

// Detect a valid button press event using debounce and direct register reading
bool buttonPressedEvent() {
  bool reading = !(PIND & (1<<3)); // Read pin 3 directly; active LOW button
  bool pressed = false;

  // If reading changed, restart debounce timer
  if (reading != lastButtonState) lastDebounceTime = millis();

  // Accept new state only after debounce delay
  if ((millis()-lastDebounceTime) > DEBOUNCE_MS) {
    if (reading != buttonState) {
      buttonState = reading;
      if (buttonState) pressed = true; // Register press event only on press
    }
  }

  lastButtonState = reading;
  return pressed;
}

void setup() {
  // Configure button pin (PD3) as input
  DDRD &= ~(1<<3);
  PORTD |= (1<<3);  // Enable internal pull-up resistor on button pin

  // Configure SQW pin (PD2) as input
  DDRD &= ~(1<<2);
  PORTD |= (1<<2);  // Enable pull-up resistor on SQW input

  // Initialize I2C communication
  Wire.begin();

  // Start RTC if halted and configure 1 Hz square wave
  ds1307EnsureRunning();
  ds1307SetSQW_1Hz();

  // Initialize 7-segment display
  bool resistorsOnSegments = true;
  byte hardwareConfig = COMMON_CATHODE;
  sevseg.begin(hardwareConfig, 4, (byte*)digitPins, (byte*)segPins, resistorsOnSegments);
  sevseg.setBrightness(70);

  // Run display self-test if enabled
#if RUN_START_SELFTEST
  displaySelfTest();
#endif

  // Attach interrupt to RTC square-wave signal
  attachInterrupt(digitalPinToInterrupt(SQW_PIN), onTick, FALLING);

  // Start in idle state
  resetToIdle();
}

void loop() {
  // Refresh multiplexed display continuously
  sevseg.refreshDisplay();

  // If a 1 Hz tick occurred, process it
  if (tickFlag) {
    noInterrupts();                 // Protect shared variables
    tickFlag = false;
    unsigned long tickMsCopy = lastTickMs;
    interrupts();

    if (state == RUNNING) {
      if (counterVal < TARGET) {
        counterVal++;               // Increment counter once per RTC tick
        showCount(counterVal);

        // Store exact time when target is reached
        if (counterVal == TARGET) targetTickMs = tickMsCopy;
      } else {
        // If counter goes beyond target without press, mark as failure
        resultSuccess = false;
        state = RESULT;
        showResult(false);
      }
    }
  }

  // Check if button was pressed
  if (buttonPressedEvent()) {
    unsigned long nowMs = millis();

    if (state == IDLE) {
      // Start the game
      counterVal = 0;
      showCount(0);
      state = RUNNING;
    }
    else if (state == RUNNING) {
      unsigned long diff = 9999;

      // If button pressed exactly when target shown
      if (counterVal == TARGET) {
        diff = nowMs - targetTickMs;
      }
      // If button pressed one count early, estimate distance to next tick
      else if (counterVal == TARGET-1) {
        unsigned long expectedTarget = lastTickMs + 1000;
        if (expectedTarget > nowMs) diff = expectedTarget - nowMs;
        else diff = nowMs - expectedTarget;
      }

      // Success if within ±100 ms
      resultSuccess = (diff <= WIN_MS);
      state = RESULT;
      showResult(resultSuccess);
    }
    else if (state == RESULT) {
      // Press again to reset game
      resetToIdle();
    }
  }
}