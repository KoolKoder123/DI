#include <Arduino.h>
#include <IRremote.hpp>

#define IR_RECEIVER_PIN 11

// --- BUTTON CODES ---
// We know CH- is this value:
#define CODE_INTRO_PATTERN  0xBA45FF00  
#define CODE_FINALE   0xB847FF00
#define CODE_RESET  0xFA0588FF
#define CODE_R1  0xA85788FF
#define CODE_R2  0xA75888FF
#define CODE_R3  0xA65988FF
#define CODE_R4  0x9F6088FF
#define CODE_5  0xE31CFF00
#define CODE_QUEEN_FLICKERING  0xBD42FF00
#define CODE_FLICKER_SLOW  0xAD52FF00
#define CODE_FLICKER_FAST  0xB54AFF00
#define CODE_QUEEN_RAP_MORE  0xBB44FF00
#define CODE_DOCTOR_RAP_MORE  0xBF40FF00
#define CODE_PAUSE 0xBC43FF00
// CODE_VOL_MIN removed: bottom-left controlled by CODE_R2 during MODE_R2

void setup() {
  Serial.begin(9600);
  while (!Serial) delay(10); 

  Serial.println("\n--- REMOTE BUTTON DETECTOR ---");
  Serial.println("Press buttons on remote to test.");
  
  IrReceiver.begin(IR_RECEIVER_PIN, ENABLE_LED_FEEDBACK);
}

void loop() {
  if (IrReceiver.decode()) {
    
    // Ignore "Repeat" codes (when you hold the button down)
    if (!(IrReceiver.decodedIRData.flags & IRDATA_FLAGS_IS_REPEAT)) {
      
      uint32_t received_code = IrReceiver.decodedIRData.decodedRawData;

      switch (received_code) {
        case CODE_INTRO_PATTERN:
          Serial.println(">> ACTION: CH- Pressed!");
          break;
        case CODE_FINALE:
          Serial.println(">> ACTION: CH+ Pressed!");
          break;
        case CODE_RESET:
          Serial.println(">> ACTION: 0 Pressed!");
          break;
        case CODE_R1:
          Serial.println(">> ACTION: 1 Pressed!");
          break;
        case CODE_R2:
          Serial.println(">> ACTION: 2 Pressed!");
          break;
        case CODE_R3:
          Serial.println(">> ACTION: 3 Pressed!");
          break;
        case CODE_R4:
          Serial.println(">> ACTION: 4 Pressed!");
          break;
        case CODE_5:
          Serial.println(">> ACTION: 5 Pressed!");
          break;
        case CODE_QUEEN_FLICKERING:
          Serial.println(">> ACTION: 7 Pressed!");
          break;
        case CODE_FLICKER_SLOW:
          Serial.println(">> ACTION: 8 Pressed!");
          break;
        /* CODE_FLICKER_FAST behavior removed in main build; ignore in test */
        case CODE_QUEEN_RAP_MORE:
          Serial.println(">> ACTION: Prev Pressed!");
          break;
        case CODE_DOCTOR_RAP_MORE:
          Serial.println(">> ACTION: Next Pressed!");
          break;
        case CODE_PAUSE:
          Serial.println(">> ACTION: Pause Pressed!");
          break;
        
        default:
          Serial.print("Unknown Button Code: 0x");
          Serial.println(received_code, HEX);
          break;
      } 
    }
    
    IrReceiver.resume(); 
  }
}