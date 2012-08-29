/* Menus! */

/*
 The circuit:

LCD

 * RS pin to digital pin 12
 * Enable pin to digital pin 11
 * D4 pin to digital pin 7
 * D5 pin to digital pin 6
 * D6 pin to digital pin 5
 * D7 pin to digital pin 4
 * R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * (???)wiper to LCD VO pin (pin 3)
 
Rotary Encoder 

 * A to digital pin 2
 * B to ditigal pin 3
 * button digital pin 8

Esc button

 * momentary switch to digital pin 9

Shift register A

 * SH_CP to analogue pin 1
 * ST_CP to analogue pin 2
 * DS to analogue pin 3

Shift register B

 * SH_CP to analogue pin 1
 * ST_CP to analogue pin 2
 * DS to Shift register Q7'
 
 */

#include <LiquidCrystal.h>
#include "patch.h"

LiquidCrystal lcd(12, 11, 7, 6, 5, 4);
const int enc_a = 2;
const int enc_b = 3;
const int enc_button = 8;
const int button_esc = 9;

// These three defines (ROTATION_SPEED, ENCODER_POSITION_MAX, and
// ENCODER_POSITION_MIN) control how fast the circular bar graph
// will fill up as you rotate the encoder.These depend on the 
// encoderVal variable being an unsigned 8-bit type.
#define ROTATION_SPEED 3  // MIN: 0, MAX: 5, 3 is a good value
#define ENCODER_POSITION_MAX  (256 >> (ROTATION_SPEED - 1)) - 1
#define ENCODER_POSITION_MIN  0  // Don't go below 0

#define PATCHNAME_LEN 8 // Max length of patch names.
#define PARAMNAME_LEN 8 // Max length of param names.

#define PARAM_UNAVAIL 0
#define PARAM_LABEL 1
#define PARAM_4BIT 4

const int menu_start = 0;
const int menu_patch = 1;
const int menu_param = 2;
const int menu_value = 3;
const int param_confirm = -1;

const int lcd_width = 16;
const int lcd_lines = 2;

// Global state.
signed int encoderVal;  // Store the encoder's rotation counts

void setup() {
    lcd.begin(lcd_width, lcd_lines);
    //Serial.begin(9600); // For debugging.
    
    pinMode(enc_button, INPUT);
    digitalWrite(enc_button, HIGH);
    
    pinMode(enc_a, INPUT);
    digitalWrite(enc_a, HIGH);
    pinMode(enc_b, INPUT);
    digitalWrite(enc_b, HIGH);

    pinMode(button_esc, INPUT);
    digitalWrite(button_esc, HIGH);
    
    noInterrupts();
    attachInterrupt(0, readEncoder, CHANGE);
    attachInterrupt(1, readEncoder, CHANGE);
    interrupts();
}

void loop() {
    static int activePage = menu_start; // "page" see "menu_x" constants.
    static int menuParam = 0;           // parameter being edited.
    static int menuValue = 0;           // value of parameter.
    static patch activePatch;           // Active program

    if (activePage == menu_start) {
        lcd.print("<< powered up >>");
        delay(2000);
        activePage = menu_patch;
        loadPatch(0, &activePatch);
        lcd.blink();
        lcd.cursor();
    }

    int update = pollButtons();
    updateState(&activePage, &activePatch, &menuParam, &menuValue, update);
    updateMenu(&activePage, &activePatch, &menuParam, &menuValue);
    delay(20);
}

// Responsible for detecting button presses.
int pollButtons() {
    static int buttons[2] = {enc_button, button_esc};
    static unsigned long pressed[2];
    static short int buttonState[2];
    unsigned long t = millis();
    int update = 0;

    for (int i = 0; i < 2; i++) {
        if (buttonState[i] != digitalRead(buttons[i]) && (t > pressed[i] + 100)) {
            pressed[i] = t;
            buttonState[i] = digitalRead(buttons[i]);
            // Knob push
            if (buttons[i] == enc_button && buttonState[i] == 0) update = 1;
            // Esc button
            if (buttons[i] == button_esc && buttonState[i] == 0) update += 2;
        }
    }
    return update;
}

// Update system state based on input. Responsible for validation.
void updateState(int *pPage, patch *pPatch, int *pParam, int *pValue, int update) {
    if (*pPage == menu_patch) {
        // Input validation is rough now, but we only have 4 programs
        if (encoderVal < 0) { encoderVal = 0; return; }
        if (encoderVal > 3) { encoderVal = 3; return; }
        if (update & 1) {
            loadPatch(encoderVal, pPatch);
            *pPage = menu_param;
            *pParam = 0;
            *pValue = 0; // todo update value from patch.
        }
        else {
            *pValue = encoderVal;
        }
    }
    else if (*pPage == menu_param) {
        if (encoderVal < -1) { encoderVal = -1; return; }
        if (encoderVal > 20) { encoderVal = 20; return; }
        if (update & 1) {
            *pPage = menu_value;
            *pParam = encoderVal;
            *pValue = 0;
        }
        else if (update & 2) {
            *pPage = menu_patch;
        }
        else {
            *pParam = encoderVal;
        }
    }
    else if (*pPage == menu_value) {
        // TODO validate value against param def
        if (update & 2) {
            *pPage = menu_param;
        }
        else {
            *pValue = encoderVal;
        }
    }
}

// Update the GUI based on system state change.
void updateMenu(int *pPage, patch *pPatch, int *pParam, int *pValue) {
    static char patchString[PATCHNAME_LEN] = {' '};
    static char paramString[PARAMNAME_LEN]= {' '};
    static char optionString[PARAMNAME_LEN]= {' '};
    static int curPage = -1;
    static int curEncoderVal = 0;

    if (curPage != *pPage) { 
        curPage = *pPage;
        encoderVal = 0;
    }
    else if(curEncoderVal != encoderVal) {
        curEncoderVal = encoderVal;
    }
    else {
        return;
    }

    lcd.clear();
    lcd.print(pPatch->name);
    lcd.setCursor(0, 1);

    if (*pPage == menu_patch) {
        loadPatchName(*pValue, patchString);
        lcd.print(patchString);
        lcd.setCursor(0, 1);
    }
    else if (*pPage == menu_param || *pPage == menu_value) {
        loadParamName(*pParam, paramString);
        lcd.print(paramString);
        lcd.setCursor(8, 1);
        switch (loadParamOption(*pParam, *pValue, optionString)) {
            case PARAM_LABEL:
                lcd.print(optionString);
                break;
            case PARAM_4BIT:
                lcd.print(*pValue);
                break;
        }

        if      (*pPage == menu_param) lcd.setCursor(0, 1);
        else if (*pPage == menu_value) lcd.setCursor(9, 1);
    }
}

// Load a parameter label. Return true on success.
boolean loadParamName(int param, char *pStr) {
    if (param == param_confirm ) {
        setString("Save?", pStr, PARAMNAME_LEN);
        return true;
    }
    if (param < 15) {
        // Per OSC settings.
        // TODO pass in OSC id A,B,C
    }
    switch (param) {
        case 0:
        case 5:
        case 10:
            setString("Waveform", pStr, PARAMNAME_LEN); return true;
        case 1:
        case 6:
        case 11:
            setString("Attack", pStr, PARAMNAME_LEN); return true;
        case 2:
        case 7:
        case 12:
            setString("Decay", pStr, PARAMNAME_LEN); return true;
        case 3:
        case 8:
        case 13:
            setString("Sustain", pStr, PARAMNAME_LEN); return true;
        case 4:
        case 9:
        case 14:
            setString("Release", pStr, PARAMNAME_LEN); return true;
        case 15: setString("Cutoff", pStr, PARAMNAME_LEN); return true;
        case 16: setString("Reso", pStr, PARAMNAME_LEN); return true;
        case 17: setString("Bypass", pStr, PARAMNAME_LEN); return true;
        case 18: setString("Mode", pStr, PARAMNAME_LEN); return true;
        case 19: setString("Volume", pStr, PARAMNAME_LEN); return true;
    }
    return false;
}

// Load a parameter name. See PARAM_X for return values.
int loadParamOption(int param, int idx, char *pStr) {
    if (param == param_confirm) {
        // Saving
        if      (idx == 0) setString("Cancel", pStr, PARAMNAME_LEN);
        else if (idx == 0) setString("Yes", pStr, PARAMNAME_LEN);
        return PARAM_LABEL;
    }
    switch (param) {
        case 0:
        case 5:
        case 10:
            // Waveform
            if      (idx == 0) setString("Triangle", pStr, PARAMNAME_LEN);
            else if (idx == 1) setString("Saw", pStr, PARAMNAME_LEN);
            else if (idx == 2) setString("Pulse", pStr, PARAMNAME_LEN);
            else if (idx == 3) setString("Ring mod", pStr, PARAMNAME_LEN);
            else if (idx == 4) setString("Sync", pStr, PARAMNAME_LEN);
            else if (idx == 5) setString("Noise", pStr, PARAMNAME_LEN);
            else return PARAM_UNAVAIL;
            return PARAM_LABEL;
            break;
        case 1:
        case 6:
        case 11:
            return PARAM_4BIT; // Attack
        case 2:
        case 7:
        case 12:
            return PARAM_4BIT; // Decay
        case 3:
        case 8:
        case 13:
            return PARAM_4BIT; // Sustain
        case 4:
        case 9:
        case 14:
            return PARAM_4BIT; // Release
        case 15: return PARAM_UNAVAIL; // Filter Cutoff in HZ TODO.
        case 16: return PARAM_4BIT; // Filter Reso
        case 17: return PARAM_UNAVAIL; // Filter bypass, maybe later.
        case 18:
            // Filter mode
            if      (idx == 0) setString("Low Pass", pStr, PARAMNAME_LEN);
            else if (idx == 0) setString("Hi Pass", pStr, PARAMNAME_LEN);
            else if (idx == 0) setString("Band Pass", pStr, PARAMNAME_LEN);
            else if (idx == 0) setString("Notch", pStr, PARAMNAME_LEN);
            return PARAM_LABEL;
        case 19: return PARAM_4BIT; // Volume
    }
    return PARAM_UNAVAIL;
}

// Load a patch label. Return true on success.
boolean loadPatchName(int id, char *pStr) {
    patch p;
    loadPatch(id, &p);
    setString(p.name, pStr, PATCHNAME_LEN);
    return true;
}

boolean loadPatch(int id, patch *pProg) {
    // Currently we have four hard coded patches
    if (id < 0) id = 0;
    if (id > 3) id = 3;

    return loadFactoryDefaultPatch(id, pProg);
}

void setString(const char src[], char *dest, int len) {
    int i;
    for (i = 0; i < len && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < len; i++)
        dest[i] = '\0';
}

void lcdClearLn(int l) {
    for (int i = 0; i < lcd_width; i++) {
        lcd.setCursor(i, l);
        lcd.write(' ');
    }
    lcd.setCursor(0, l);
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
  newEncoderState = (digitalRead(enc_b)<<1) | (digitalRead(enc_a));
  
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

boolean copyPatch(patch *pSrc, patch *pDest) {
    pDest->waveOscA = pSrc->waveOscA;
    pDest->attackOscA = pSrc->attackOscA;
    pDest->decayOscA = pSrc->decayOscA;
    pDest->sustainOscA = pSrc->sustainOscA;
    pDest->releaseOscA = pSrc->releaseOscA;

    pDest->waveOscB = pSrc->waveOscB;
    pDest->attackOscB = pSrc->attackOscB;
    pDest->decayOscB = pSrc->decayOscB;
    pDest->sustainOscB = pSrc->sustainOscB;
    pDest->releaseOscB = pSrc->releaseOscB;

    pDest->waveOscC = pSrc->waveOscC;
    pDest->attackOscC = pSrc->attackOscC;
    pDest->decayOscC = pSrc->decayOscC;
    pDest->sustainOscC = pSrc->sustainOscC;
    pDest->releaseOscC = pSrc->releaseOscC;
 
    pDest->cutoff = pSrc->cutoff;
    pDest->resonance = pSrc->resonance;
    pDest->bypass = pSrc->bypass;
    pDest->mode = pSrc->mode;

    pDest->volume = pSrc->volume;

    pDest->id = pSrc->id;
    setString(pSrc->name, pDest->name, PATCHNAME_LEN);
    return true;
}

boolean loadFactoryDefaultPatch(int id, patch *pProg) {
    patch *pfactory;
    if (id == 0) {
        patch factory = {
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            id, "Bleep",
        };
        pfactory = &factory;
        return copyPatch(pfactory, pProg);
    }
    else if (id == 1) {
        patch factory = {
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            id, "Spacey",
        };
        pfactory = &factory;
        return copyPatch(pfactory, pProg);
    }
    else if (id == 2) {
        patch factory = {
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            id, "Belong",
        };
        pfactory = &factory;
        return copyPatch(pfactory, pProg);
    }
    else if (id == 3) {
        patch factory = {
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            1, 2, 3, 4, 5,
            id, "Disaste",
        };
        pfactory = &factory;
        return copyPatch(pfactory, pProg);
    }
    return false;
}

void displayError(char *pErr) {
    lcd.clear();
    lcd.print(pErr);
    delay(2000);
}

