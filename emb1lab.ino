// Define the GPIO pins where LEDs are connected
const int LED1 = 3;  // LED 1 connected to digital pin 3
const int LED2 = 4;  // LED 2 connected to digital pin 4
const int LED3 = 5;  // LED 3 connected to digital pin 5
void setup() {
  // Set the LED pins as OUTPUT so we can control them
  pinMode(LED1, OUTPUT);  
  pinMode(LED2, OUTPUT);  
  pinMode(LED3, OUTPUT);  
}
void loop() {
  // Turn on LED1, turn off LED2 and LED3
  digitalWrite(LED1, HIGH);  
  digitalWrite(LED2, LOW);  
  digitalWrite(LED3, LOW);
  delay(350);  // Wait 350 milliseconds
  // Turn on LED2, turn off LED1 and LED3
  digitalWrite(LED1, LOW);  
  digitalWrite(LED2, HIGH);  
  digitalWrite(LED3, LOW);
  delay(350);  // Wait 350 milliseconds
  // Turn on LED3, turn off LED1 and LED2
  digitalWrite(LED1, LOW);  
  digitalWrite(LED2, LOW);  
  digitalWrite(LED3, HIGH);
  delay(350);  // Wait 350 milliseconds
  // Loop repeats indefinitely
}
