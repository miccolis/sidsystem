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
#define PARAM_PERCENT 3 // unused...
#define PARAM_4BIT 4

// Should these be defines?
const int MENU_START = -1;
const int MENU_HOME = 0;
const int MENU_PATCH = 1;
const int MENU_PARAM = 2;
const int MENU_CONFIRM = 4;

/*
 * Global state.
 */

signed int encoderVal;  // Store the encoder's rotation counts

// Combine these eventually?
int menuState = MENU_START;
int menuPatch = 0;
int menuParam = 0;

void setup() {
    lcd.begin(16, 2);

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
    delay(20);
}

void updateMenu(int page, int patch, int param) {
    static int curPage;
    static int curEncoderVal;

    char patchString[PATCHNAME_LEN];
    char paramString[PARAMNAME_LEN];
    char optionString[PARAMNAME_LEN];

    if (curPage != page) {
        curPage = page;

        encoderVal = 0;
        curEncoderVal = -1;

        switch (page) {
            case MENU_START:
                lcd.print("<< powered up >>");
                menuState = MENU_HOME;
                menuPatch = 0;
                menuParam = 0;
                delay(2000);
                return;
            case MENU_HOME:
                if (loadPatchName(patch, patchString)) {
                    lcd.clear();
                    lcd.print(patchString);
                }
                break;
            case MENU_PATCH:
                if (loadPatchName(patch, patchString)) {
                    lcd.clear();
                    lcd.print(patchString);
                }
                break;
            case MENU_PARAM:
                if (loadPatchName(patch, patchString) &&
                    loadParamName(patch, paramString)
                ) {
                    lcd.clear();
                    lcd.print(patchString);
                    lcd.setCursor(0, 1);
                    lcd.print(paramString);
                }
                break;
            case MENU_CONFIRM:
                lcd.clear();
                lcd.print("Are you sure?");
                break;
        }
    }

    if (curEncoderVal != encoderVal) {
        curEncoderVal = encoderVal;

        lcd.setCursor(0,1); // encoder always operates on second line.
        // TODO clear out this line

        int optType;

        switch(page) {
            case MENU_HOME:
                if (loadPatchName(curEncoderVal, patchString))
                    lcd.print(patchString);
                break;
            case MENU_PATCH:
                if (loadParamName(curEncoderVal, paramString))
                    lcd.print(paramString);
                break;
            case MENU_PARAM:
                optType = loadParamOption(param, curEncoderVal, optionString);
                if (optType == PARAM_LABEL) {
                    lcd.print(optionString);
                }
                else if (optType == PARAM_YESNO) {
                    lcd.print(curEncoderVal & 1 ? "YES" : "NO");
                }
                else if (optType == PARAM_4BIT) {
                    lcd.print(curEncoderVal);
                }
                break;
            case MENU_CONFIRM:
                lcd.print(curEncoderVal & 1 ? "YES" : "NO");
                break;
        }
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
