  
/*   
An attempt to control an IR broadcaster, reciever and an HC-05 Bluetooth module with an Arduino Uno.
Combining source code found here:
  IR - Arduino Uno IR library example, copy found on my github - https://github.com/llcauquil/Arduino-IR-and-Bluetooth-Command-Relay/blob/main/IRrecord.c
  Bluetooth - https://create.arduino.cc/projecthub/electropeak/getting-started-with-hc-05-bluetooth-module-arduino-e0ca81

LiamLCauquil
STARTED: 3/5/2021
LAST MODDIFIED: 3/5/2021
*/
 
#include <SoftwareSerial.h>
#include <IRremote.h>

//VARIABLES
//variables for serial print. Kept all the serial prints so to check how to combine two seperate serials
int ir_serial_printer = 0;
int blue_serial_printer = 0;
unsigned int ir_serial = 115200;
unsigned int blue_serial = 9600;

//setting up IR values
int IR_RECEIVE_PIN = 11;
int SEND_BUTTON_PIN = 12;

int STATUS_PIN = LED_BUILTIN;
int DELAY_BETWEEN_REPEAT = 50;

IRrecv IrReceiver(IR_RECEIVE_PIN);
IRsend IrSender;

// On the Zero and others we switch explicitly to SerialUSB
#if defined(ARDUINO_ARCH_SAMD)
#define Serial SerialUSB
#endif

//setting up Bluetooth values
SoftwareSerial MyBlue(2, 3); // RX | TX  bluetooth name ??? figure out if it's better to hope to RX TX pins
int flag = 0; 
int LED = 8;

//SETUP
void setup() {
  IR_setup();
  Bluetooth_setup();
}

void IR_setup(){
  Serial.begin(ir_serial);// serial monitor for IR tech
  #if defined(__AVR_ATmega32U4__) || defined(SERIAL_USB) || defined(SERIAL_PORT_USBVIRTUAL)
    delay(2000); // To be able to connect Serial monitor after reset and before first printout
  #endif
    
  #if ir_serial_priner
    Serial.println(F("START " __FILE__ " from " __DATE__)); // Just to know which program is running on my Arduino
  #endif
  
  IrReceiver.enableIRIn();  // Start the receiver
  IrReceiver.blink13(true); // Enable feedback LED

  pinMode(SEND_BUTTON_PIN, INPUT_PULLUP);
  pinMode(STATUS_PIN, OUTPUT);
    
  #if ir_serial_printer
    Serial.print(F("Ready to receive IR signals at pin "));
    Serial.println(IR_RECEIVE_PIN);
  #endif

  #if ir_serial_printer
    #if defined(SENDING_SUPPORTED)
      Serial.print(F("Ready to send IR signals at pin "));
      Serial.print(IR_SEND_PIN);
      Serial.print(F(" on press of button at pin "));
      Serial.println(SEND_BUTTON_PIN);

      #else
        Serial.println(F("Sending not supported for this board!"));
      #endif
  #endif
}

void Bluetooth_setup(){
  Serial.begin(blue_serial);// serial monitor for bluetooth tech
  MyBlue.begin(9600);

  pinMode(LED, OUTPUT); 
  Serial.println("Ready to connect\nDefualt password is 1234 or 000"); 
}


// SUPPORTING IR FUNCTIONS
// Storage for the recorded code
int codeType = -1; // The type of code
uint32_t codeValue; // The code value if not raw
uint16_t address; // The address value if not raw
uint16_t rawCodes[RAW_BUFFER_LENGTH]; // The durations if raw
uint8_t codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

void code_switcher(int i){
  switch (i){
      case 0: //off
      codeValue = 0xFFE21D;
      break;

      case 1: //on
      codeValue =0xFFA25D;
      break;

      case 13: //white
      codeValue = 0xFF52AD;
      break;

      case 2: //red
      codeValue = 0xFF6897;
      break;

      case 3: //green
      codeValue = 0xFF9867;
      break;

      case 4: //blue
      codeValue = 0xFFB04F;
      break;

      case 5: //teel
      codeValue = 0xFF18E7;
      break;

      case 6: //aquamarine
      codeValue = 0xFF5AA5;
      break;

      case 7: //violet
      codeValue = 0xFF10EF;
      break;

      case 8: //pink
      codeValue = 0xFF42BD;
      break;

      case 9: //c
      codeValue = 0xFF22DD;
      break;

      case 10: //s
      codeValue = 0xFFE01F;
      break;

      case 11: //up
      codeValue = 0xFFA857;
      break;

      case 12: //down
      codeValue = 0xFF906F;
      break;

    }
}

// Stores the code for later playback. Most of this code is just logging and placing it into resuable values
void storeCode() {
    if (IrReceiver.results.isRepeat) {
      #if (ir_serial_printer)
        Serial.println("Ignore repeat");
      #endif
        return;
    }
    codeType = IrReceiver.results.decode_type;
    address = IrReceiver.results.address;

    if (codeType == UNKNOWN) {
        Serial.println("Received unknown code, saving as raw");
        codeLen = IrReceiver.results.rawlen - 1;
        // To store raw codes:
        // Drop first value (gap)
        // Convert from ticks to microseconds
        // Tweak marks shorter, and spaces longer to cancel out IR receiver distortion
        for (uint16_t i = 1; i <= codeLen; i++) {
            if (i % 2) {
                // Mark
                rawCodes[i - 1] = IrReceiver.results.rawbuf[i] * MICROS_PER_TICK - MARK_EXCESS_MICROS;
                Serial.print(" m");
            } else {
                // Space
                rawCodes[i - 1] = IrReceiver.results.rawbuf[i] * MICROS_PER_TICK + MARK_EXCESS_MICROS;
                Serial.print(" s");
            }
            Serial.print(rawCodes[i - 1], DEC);
        }
        Serial.println();
    } else {
        IrReceiver.printResultShort(&Serial);
        Serial.println();

        codeValue = IrReceiver.results.value;
        codeLen = IrReceiver.results.bits;
    }
}

void sendCode(bool aSendRepeat) {
    if (codeType == NEC) {
        if (aSendRepeat) {
            IrSender.sendNEC(REPEAT, codeLen);
            Serial.println("Sent NEC repeat");
        } else {
            IrSender.sendNEC(codeValue, codeLen);
            Serial.print("Sent NEC ");
            Serial.println(codeValue, HEX);
        }
    } else if (codeType == NEC_STANDARD) {
        if (aSendRepeat) {
            IrSender.sendNECRepeat();
            Serial.println("Sent NEC repeat");
        } else {
            IrSender.sendNECStandard(address, codeValue);
            Serial.print("Sent NEC_STANDARD address=0x");
            Serial.print(address, HEX);
            Serial.print(", command=0x");
            Serial.println(codeValue, HEX);
        }
    } else if (codeType == SONY) {
        IrSender.sendSony(codeValue, codeLen);
        Serial.print("Sent Sony ");
        Serial.println(codeValue, HEX);
    } else if (codeType == PANASONIC) {
        IrSender.sendPanasonic(codeValue, codeLen);
        Serial.print("Sent Panasonic");
        Serial.println(codeValue, HEX);
    } else if (codeType == JVC) {
        IrSender.sendJVC(codeValue, codeLen, false);
        Serial.print("Sent JVC");
        Serial.println(codeValue, HEX);
    } else if (codeType == RC5 || codeType == RC6) {
        if (!aSendRepeat) {
            // Flip the toggle bit for a new button press
            toggle = 1 - toggle;
        }
        // Put the toggle bit into the code to send
        codeValue = codeValue & ~(1 << (codeLen - 1));
        codeValue = codeValue | (toggle << (codeLen - 1));
        if (codeType == RC5) {
            Serial.print("Sent RC5 ");
            Serial.println(codeValue, HEX);
            IrSender.sendRC5(codeValue, codeLen);
        } else {
            IrSender.sendRC6(codeValue, codeLen);
            Serial.print("Sent RC6 ");
            Serial.println(codeValue, HEX);
        }
    } else if (codeType == UNKNOWN /* i.e. raw */) {
        // Assume 38 KHz
        IrSender.sendRaw(rawCodes, codeLen, 38);
        Serial.println("Sent raw");
    }
}

int lastButtonState;

// END OF SUPPORTING IR FUNCTIONS
 
void loop() 
{ 
  //ir loop

      // If button pressed, send the code.
    int buttonState = digitalRead(SEND_BUTTON_PIN); // Button pin is active LOW
    if (lastButtonState == LOW && buttonState == HIGH) {
        Serial.println("Button released");
        IrReceiver.enableIRIn(); // Re-enable receiver
    }

    if (buttonState == LOW) {
        Serial.println("Button pressed, now sending");
        digitalWrite(STATUS_PIN, HIGH);
        sendCode(lastButtonState == buttonState);
        digitalWrite(STATUS_PIN, LOW);
        delay(DELAY_BETWEEN_REPEAT); // Wait a bit between retransmissions
    } else if (IrReceiver.decode()) {
        storeCode();
        IrReceiver.resume(); // resume receiver
    }
    lastButtonState = buttonState;

  //bluetooth loop
 if (MyBlue.available()) 
   flag = MyBlue.read(); 
 if (flag == 1) 
 { 
   digitalWrite(LED, HIGH); 
   Serial.println("LED On"); 
 } 
 else if (flag == 0) 
 { 
   digitalWrite(LED, HIGH); 
   Serial.println("LED Off"); 
 } 
}  
