#include <Arduino.h>

#define IR_PIN     2   // The White/Yellow wire from the Receiver

// Variables to track the beam status
int counter = 0;
int last_sensor_state = LOW; 

void setup() {
  // 1. Initialize Serial
  Serial.begin(9600);
  
  // Wait for Serial Monitor to connect (Crucial for Uno R4)
  while (!Serial) {
    delay(10); 
  }

  Serial.println("\n--- STARTING IR TEST ---");
  Serial.println("Alignment Check: Ensure sensors are pointing at each other.");

  // 2. Configure Pin
  // INPUT_PULLUP is best practice. 
  // Usually: LOW = Beam Connected (Seeing Light), HIGH = Beam Broken.
  pinMode(IR_PIN, INPUT_PULLUP);
  
  // Read the initial state to prevent a false count at startup
  last_sensor_state = digitalRead(IR_PIN);
}

void loop() {
  // 1. Read the current value from the sensor
  int current_state = digitalRead(IR_PIN);

  // 2. CHECK FOR CHANGE
  // Did the state change from LOW (Connected) to HIGH (Broken)?
  if (current_state == HIGH && last_sensor_state == LOW) {
    
    // Increment the counter
    counter++;
    
    // Print to Serial Monitor
    Serial.print(">> BEAM BROKEN! Total Count: ");
    Serial.println(counter);
    
  }

  // 3. Save the current state for the next loop comparison
  last_sensor_state = current_state;

  // 4. Short delay to prevent reading electrical noise (Debouncing)
  delay(20); 
}