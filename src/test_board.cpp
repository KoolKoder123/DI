#include <Arduino.h>

void setup() {
  // Initialize serial communication at 9600 bits per second:
  Serial.begin(9600);

  // Initialize the digital pin LED_BUILTIN as an output.
  // On the Uno R4 WiFi, this is the small amber 'L' LED near the reset button.
  pinMode(LED_BUILTIN, OUTPUT);

  // Wait for the Serial port to connect. 
  // This is highly recommended for the R4 because it uses native USB.
  // Without this, the first message might print before you open the monitor.
  while (!Serial) {
    ; 
  }

  Serial.println("Arduino Uno R4 WiFi is connected and ready!");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  Serial.println("LED is ON");      // send message to computer
  delay(4000);                      // wait for some time

  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  Serial.println("LED is OFF");     // send message to computer
  delay(4000);                      // wait for some time
}