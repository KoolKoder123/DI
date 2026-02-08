#include <Arduino.h>
#include <IRremote.hpp>

#define IR_RECEIVER_PIN 11

// --- BUTTON CODES ---
// We know CH- is this value:
#define CODE_CH_MINUS  0xBA45FF00  
#define CODE_CH_PLUS   0xB847FF00
#define CODE_RESET  0xFE0188FF
#define CODE_R1  0xFD0288FF
#define CODE_R2  0xE718FF00
#define CODE_R3  0xA15EFF00
#define CODE_R4  0xF708FF00
#define CODE_5  0xE31CFF00
#define CODE_7  0xBD42FF00
#define CODE_8  0xAD52FF00
#define CODE_9  0xB54AFF00
#define CODE_PREV  0xBB44FF00
#define CODE_NEXT  0xBF40FF00
#define CODE_PAUSE 0xBC43FF00
// CODE_VOL_MIN removed: bottom-left controlled by CODE_2 during MODE_R2

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
        case CODE_CH_MINUS:
          Serial.println(">> ACTION: CH- Pressed!");
          break;
        case CODE_CH_PLUS:
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
        case CODE_7:
          Serial.println(">> ACTION: 7 Pressed!");
          break;
        case CODE_8:
          Serial.println(">> ACTION: 8 Pressed!");
          break;
        case CODE_9:
          Serial.println(">> ACTION: 9 Pressed!");
          break;
        case CODE_PREV:
          Serial.println(">> ACTION: Prev Pressed!");
          break;
        case CODE_NEXT:
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