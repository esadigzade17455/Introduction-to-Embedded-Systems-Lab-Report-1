// --- 1. Define Pins ---
// Joystick connections
const int pinSW = 2;   // Push button (SW) connected to digital pin 2
const int pinX = A0;   // X-axis (VRx) connected to analog pin A0
const int pinY = A1;   // Y-axis (VRy) connected to analog pin A1

// LED outputs for each direction
const int ledUp = 6;    
const int ledDown = 9;  
const int ledLeft = 10; 
const int ledRight = 11;

// --- 2. Define Thresholds & Dead-zone ---
// Joystick center ≈ 512; dead-zone prevents noise/jitter near center
const int thresholdLow = 400;   // Lower bound for direction detection
const int thresholdHigh = 600;  // Upper bound for direction detection

void setup() {
  // Initialize serial communication at high speed for faster data updates
  Serial.begin(115200); 

  // Configure joystick button as input with internal pull-up resistor
  // (LOW when pressed, HIGH when released)
  pinMode(pinSW, INPUT_PULLUP);

  // Configure LED pins as outputs
  pinMode(ledUp, OUTPUT);
  pinMode(ledDown, OUTPUT);
  pinMode(ledLeft, OUTPUT);
  pinMode(ledRight, OUTPUT);
}

void loop() {
  // Start timing measurement to evaluate sampling speed
  unsigned long startTime = micros();

  // --- READ INPUTS ---
  // Read analog values from joystick axes (0–1023)
  int valX = analogRead(pinX);
  int valY = analogRead(pinY);

  // Read button state (0 = pressed, 1 = not pressed)
  int valSW = digitalRead(pinSW);

  // --- RESET LEDS ---
  // Turn off all LEDs before determining new direction
  digitalWrite(ledUp, LOW);
  digitalWrite(ledDown, LOW);
  digitalWrite(ledLeft, LOW);
  digitalWrite(ledRight, LOW);

  // Default direction when joystick is in dead-zone
  String direction = "CENTER";

  // --- DIRECTION LOGIC ---
  // Check Y-axis for UP/DOWN movement
  if (valY < thresholdLow) {
    digitalWrite(ledUp, HIGH);   // Activate UP LED
    direction = "UP";
  } 
  else if (valY > thresholdHigh) {
    digitalWrite(ledDown, HIGH); // Activate DOWN LED
    direction = "DOWN";
  }

  // Check X-axis for LEFT/RIGHT movement
  if (valX < thresholdLow) {
    digitalWrite(ledLeft, HIGH);  // Activate LEFT LED
    direction = "LEFT";
  } 
  else if (valX > thresholdHigh) {
    digitalWrite(ledRight, HIGH); // Activate RIGHT LED
    direction = "RIGHT";
  }

  // --- SWITCH LOGIC ---
  // If button is pressed, indicate click and turn on all LEDs
  if (valSW == LOW) {
    direction = direction + " + CLICK";
    digitalWrite(ledUp, HIGH);
    digitalWrite(ledDown, HIGH);
    digitalWrite(ledLeft, HIGH);
    digitalWrite(ledRight, HIGH);
  }

  // --- SERIAL OUTPUT ---
  // Print detected direction
  Serial.print("Dir: ");
  Serial.print(direction);
  
  // Measure and print loop execution time (sampling speed evidence)
  unsigned long endTime = micros();
  Serial.print(" | Loop Time (us): ");
  Serial.println(endTime - startTime);
  
  // Small delay to keep serial output readable (does not significantly affect performance)
  delay(10); 
}