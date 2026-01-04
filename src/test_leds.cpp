#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN     6   
#define NUM_LEDS    300    

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // 1. Initialize Serial Communication
  Serial.begin(9600);

  // WAIT for the Serial Monitor to open (Specific to Uno R4/Leonardo boards)
  // The code will pause here until you connect via VS Code
  while (!Serial) {
    delay(10); 
  }

  Serial.println("\n--- STARTING LED TEST ---");
  Serial.print("Total LEDs to light up: ");
  Serial.println(NUM_LEDS);

  // 2. Initialize Strip
  strip.begin();
  strip.show();            
  strip.setBrightness(50); 
  Serial.println("Strip Initialized. Starting loop...");
}

void loop() { 
  // Loop through every LED
  for(int i = 0; i < NUM_LEDS; i++) {    
    strip.setPixelColor(i, strip.Color(0, 0, 255)); // Blue
    strip.show(); 
    
    // Print a status update every 10 LEDs so the screen isn't flooded
    Serial.print("Lit LED #");
    Serial.println(i);
    
    delay(50);
  }

  Serial.println("--- ALL LEDS ON. TEST COMPLETE. ---");
  delay(2000); // Wait for 2 seconds with all LEDs ON

  Serial.println("Turning OFF...");
  strip.clear();  // Sets all pixels to '0' (Black) in memory
  strip.show();   // Pushes that 'Black' to the actual strip

  // Infinite pause
  while(true) {
    delay(100000);
  }
}