/* Menus! */

// include the library code:
#include <LiquidCrystal.h>

/*
 The circuit:

 * LCD RS pin to digital pin 12
 * LCD Enable pin to digital pin 11
 * LCD D4 pin to digital pin 7
 * LCD D5 pin to digital pin 6
 * LCD D6 pin to digital pin 5
 * LCD D7 pin to digital pin 4
 * LCD R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)
 
 * knob A to pin 2
 * knob B to pin 3
 * knob button pin 8 (w/ pulldown resistor)
 
 */

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);
const int ENC_A = 2;
const int ENC_B = 3;
const int BUTTON = 8;

// These three defines (ROTATION_SPEED, ENCODER_POSITION_MAX, and
// ENCODER_POSITION_MIN) control how fast the circular bar graph
// will fill up as you rotate the encoder.These depend on the 
// encoderVal variable being an unsigned 8-bit type.
#define ROTATION_SPEED 3  // MIN: 0, MAX: 5, 3 is a good value
#define ENCODER_POSITION_MAX  (256 >> (ROTATION_SPEED - 1)) - 1
#define ENCODER_POSITION_MIN  0  // Don't go below 0

#define PATCHNAME_LEN 8 // Max length of patch names.
#define PARAMNAME_LEN 8 // Max length of paramnames.

#define PARAM_UNAVAIL 0
#define PARAM_LABEL 1
#define PARAM_YESNO 2
#define PARAM_PERCENT 3
#define PARAM_4BIT 4

/*
 * Global state.
 */
signed int encoderVal;  // Store the encoder's rotation counts

// Should these be defines?
const int MENU_HOME = 0;
const int MENU_PATCH= 1;
const int MENU_PARAM= 2;
const int MENU_CONFIRM= 4;

// Combine these eventually?
int menuState = 0;
int menuPatch = 0;
int menuParam = 0;

void setup() {
    lcd.begin(16, 2);
    lcd.print("<< powered up >>");

    //Serial.begin(9600); // For debugging.
    
    pinMode(BUTTON, INPUT);
    
    pinMode(ENC_A, INPUT);
    digitalWrite(ENC_A, HIGH);
    pinMode(ENC_B, INPUT);
    digitalWrite(ENC_B, HIGH);
    
    noInterrupts();
    attachInterrupt(0, readEncoder, CHANGE);
    attachInterrupt(1, readEncoder, CHANGE);
    interrupts();
}

void loop() {
    static unsigned long pressed;
    static short int buttonState;

    if (buttonState != digitalRead(BUTTON)) {
        unsigned long t = millis();
        if (t > pressed + 100) {
            pressed = t;
            buttonState = digitalRead(BUTTON);
            if (buttonState == 0) {
                if (menuState == MENU_CONFIRM) menuState = MENU_PARAM;
                else menuState++;
            }
        }
    }

    updateMenu(menuState, menuPatch, menuParam);
}

void updateMenu(int type, int patch, int param) {
    static unsigned long refreshed;
    static int prevType;
    static int prevEncoderVal;

    char patchName[PATCHNAME_LEN];
    char paramName[PARAMNAME_LEN];

    // Limit LCD updates to every 10ms.
    unsigned long t = millis();
    if (t < refreshed + 10) return;
    refreshed = t;
    
    if (prevType != type) {
        prevType = type;

        switch (type) {
            case MENU_HOME:
                if (loadPatchName(patch, patchName)) {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print(patchName);
                }
                break;
            case MENU_PATCH:
                if (loadPatchName(patch, patchName) &&
                    loadParamName(patch, paramName)
                ) {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print(patchName);
                    lcd.setCursor(0, 1);
                    lcd.print(paramName);
                }
                break;
            case MENU_PARAM:
                if (loadPatchName(patch, patchName) &&
                    loadParamName(patch, paramName)
                ) {
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print(paramName);
                    lcd.setCursor(0, 1);
                    lcd.print(encoderVal);
                }
                break;
            case MENU_CONFIRM:
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print("Are you sure?");
                lcd.setCursor(0, 1);
                lcd.print("YES");
                break;
        }
    }

    if (prevEncoderVal != encoderVal) {
        prevEncoderVal = encoderVal;
        lcd.setCursor(0,1);
        lcd.print(encoderVal);
    }
}

// Load a parameter label. Return true on success.
boolean loadParamName(int param, char *pStr) {
    switch (param) {
        case 0: setString("Waveform", pStr, PARAMNAME_LEN); return true;
        case 1: setString("Attack", pStr, PARAMNAME_LEN); return true;
        case 2: setString("Decay", pStr, PARAMNAME_LEN); return true;
        case 3: setString("Sustain", pStr, PARAMNAME_LEN); return true;
        case 4: setString("Release", pStr, PARAMNAME_LEN); return true;
    }
    return false;
}

// Load a parameter name. See PARAM_X for return values.
int loadParamOption(int param, int idx, char *pStr) {
    switch (param) {
        case 0: // Waveform
            if      (idx == 0) setString("Tre", pStr, PARAMNAME_LEN);
            else if (idx == 1) setString("Saw", pStr, PARAMNAME_LEN);
            else if (idx == 2) setString("Pulse", pStr, PARAMNAME_LEN);
            else if (idx == 4) setString("Noise", pStr, PARAMNAME_LEN);
            else return PARAM_UNAVAIL;
            return PARAM_LABEL;
            break;
        case 1: return PARAM_4BIT; // Attach
        case 2: return PARAM_4BIT; // Decay
        case 3: return PARAM_4BIT; // Sustain
        case 4: return PARAM_4BIT; // Release
    }
    return PARAM_UNAVAIL;
}

// Load a patch label. Return true on success.
boolean loadPatchName(int patch, char *pStr) {
    switch (patch) {
        case 0: setString("Bleep", pStr, PATCHNAME_LEN); return true;
        case 1: setString("Spacey", pStr, PATCHNAME_LEN); return true;
        case 2: setString("Belong", pStr, PATCHNAME_LEN); return true;
    }
    return false;
}

void setString(const char src[], char *dest, int len) {
    int i;
    for (i = 0; i < len && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < len; i++)
        dest[i] = '\0';
}


// interrupt handler for the rotary encoder.
void readEncoder() {
  noInterrupts();
  delayMicroseconds(5000);  // 'debounce'
  
  // enc_states[] is a fancy way to keep track of which direction
  // the encoder is turning. 2-bits of oldEncoderState are paired
  // with 2-bits of newEncoderState to create 16 possible values.
  // Each of the 16 values will produce either a CW turn (1),
  // CCW turn (-1) or no movement (0).
  int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
  static uint8_t oldEncoderState = 0;
  static uint8_t newEncoderState = 0;

  // First, find the newEncoderState. This'll be a 2-bit value
  // the msb is the state of the B pin. The lsb is the state
  // of the A pin on the encoder.
  newEncoderState = (digitalRead(ENC_B)<<1) | (digitalRead(ENC_A));
  
  // Now we pair oldEncoderState with new encoder state
  // First we need to shift oldEncoder state left two bits.
  // This'll put the last state in bits 2 and 3.
  oldEncoderState <<= 2;
  // Mask out everything in oldEncoderState except for the previous state
  oldEncoderState &= 0xC0;
  // Now add the newEncoderState. oldEncoderState will now be of
  // the form: 0b0000(old B)(old A)(new B)(new A)
  oldEncoderState |= newEncoderState; // add filteredport value

  // Now we can update encoderVal with the updated position
  // movement. We'll either add 1, -1 or 0 here.
  encoderVal += enc_states[oldEncoderState];
  
  interrupts();
}
