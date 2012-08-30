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
#include "param.h"

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
    static patch activePatch;           // Active program (ie being played & edited)
    static param activeParam;           // Active parameter (ie being edited)
    static int menuValue = 0;           // Value of parameter.

    if (activePage == menu_start) {
        lcd.print("<< powered up >>");
        delay(2000);
        activePage = menu_patch;
        loadPatch(0, &activePatch);
        lcd.blink();
        lcd.cursor();
    }

    int update = pollButtons();
    if (updateState(&activePage, &activePatch, &activeParam, &menuValue, update))
        updateMenu(&activePage, &activePatch, &activeParam, &menuValue);

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
boolean updateState(int *pPage, patch *pPatch, param *pParam, int *pValue, int update) {
    static int curEncoderVal = -1;

    if (curEncoderVal != encoderVal) curEncoderVal = encoderVal;
    else if (update == 0) return false;

    if (*pPage == menu_patch) {
        // Input validation is rough now, but we only have 4 programs
        if (encoderVal < 0) { encoderVal = 0; return true; }
        if (encoderVal > 3) { encoderVal = 3; return true; }
        if (update & 1) {
            loadPatch(encoderVal, pPatch);
            loadParam(0, pParam);
            *pValue = loadPatchValue(pParam->id, pPatch);
            *pPage = menu_param;
        }
        else {
            *pValue = encoderVal;
        }
    }
    else if (*pPage == menu_param) {
        if (encoderVal < -1) { encoderVal = -1; return true; }
        if (encoderVal > 20) { encoderVal = 20; return true; }

        loadParam(encoderVal, pParam);
        *pValue = loadPatchValue(pParam->id, pPatch);

        if (update & 1) *pPage = menu_value;
        if (update & 2) *pPage = menu_patch;
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
    return true;
}

// Update the GUI based on system state change.
void updateMenu(int *pPage, patch *pPatch, param *pParam, int *pValue) {

    lcd.clear();
    lcd.print(pPatch->name);
    lcd.setCursor(0, 1);

    if (*pPage == menu_patch) {
        char patchString[PATCHNAME_LEN] = {' '};
        loadPatchName(*pValue, patchString);
        lcd.print(patchString);
        lcd.setCursor(0, 1);
    }
    else if (*pPage == menu_param || *pPage == menu_value) {
        lcd.print(pParam->name);
        lcd.setCursor(9, 1);
        switch (pParam->type) {
            case PARAM_LABEL:
                {
                    char optionString[PARAMNAME_LEN]= {' '};
                    loadParamOption(pParam, *pValue, optionString);
                    lcd.print(optionString);
                }
                break;
            case PARAM_4BIT:
                lcd.print(*pValue);
                break;
            case PARAM_UNAVAIL:
            default:
                // noop
                break;
        }

        if      (*pPage == menu_param) lcd.setCursor(0, 1);
        else if (*pPage == menu_value) lcd.setCursor(9, 1);
    }
}

// Parameter methods
boolean loadParamOption(param *pParam, int idx, char *pStr) {
    if (pParam->id == param_confirm) {
        if      (idx == 0) setString("Cancel", pStr, PARAMNAME_LEN);
        else if (idx == 1) setString("Yes", pStr, PARAMNAME_LEN);
        else return false;
        return true;
    }
    switch (pParam->id) {
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
            else return false;
            return true;
        case 18:
            // Filter mode
            if      (idx == 0) setString("Low Pass", pStr, PARAMNAME_LEN);
            else if (idx == 1) setString("Hi Pass", pStr, PARAMNAME_LEN);
            else if (idx == 2) setString("Band Pass", pStr, PARAMNAME_LEN);
            else if (idx == 3) setString("Notch", pStr, PARAMNAME_LEN);
            else return false;
            return true;
    }
    return false;
}

boolean loadParam(int id, param *pParam) {
    if (id == param_confirm) {
        param def = {PARAM_LABEL, id, "Save?"};
        return copyParam(&def, pParam);
    }

    char osc = 'A';
    if      (id > 4 && id < 10)  osc = 'B';
    else if (id > 9 && id < 15) osc = 'C';

    switch (id) {
        case 0:
        case 5:
        case 10:
            {
                param def = {PARAM_LABEL, id, "  Wave"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 1:
        case 6:
        case 11:
            {
                param def = {PARAM_4BIT, id, "  Att"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 2:
        case 7:
        case 12:
            {
                param def = {PARAM_4BIT, id, "  Dec"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 3:
        case 8:
        case 13:
            {
                param def = {PARAM_4BIT, id, "  Sus"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 4:
        case 9:
        case 14:
            {
                param def = {PARAM_4BIT, id, "  Rel"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 15:
            {
                // Filter Cutoff in HZ TODO.
                param def = {PARAM_UNAVAIL, id, "Cutoff"};
                return copyParam(&def, pParam);
            }
        case 16:
            {
                param def = {PARAM_4BIT, id, "Reso"};
                return copyParam(&def, pParam);
            }
        case 17:
            {
                 // Filter bypass, maybe later.
                param def = {PARAM_UNAVAIL, id, "Bypass"};
                return copyParam(&def, pParam);
            }
        case 18:
            {
                param def = {PARAM_LABEL, id, "Mode"};
                return copyParam(&def, pParam);
            }
        case 19:
            {
                param def = {PARAM_4BIT, id, "Volume"};
                return copyParam(&def, pParam);
            }
    }
    return false;
}

boolean copyParam(param *pSrc, param *pDest) {
    pDest->type = pSrc->type;
    pDest->id = pSrc->id;
    setString(pSrc->name, pDest->name, PARAMNAME_LEN);
    return true;
}

// Patch methods
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

int loadPatchValue(int param, patch *p) {
    switch (param) {
        case 0: return p->waveOscA;
        case 1: return p->attackOscA;
        case 2: return p->decayOscA;
        case 3: return p->sustainOscA;
        case 4: return p->releaseOscA;

        case 5: return p->waveOscB;
        case 6: return p->attackOscB;
        case 7: return p->decayOscB;
        case 8: return p->sustainOscB;
        case 9: return p->releaseOscB;

        case 10: return p->waveOscC;
        case 11: return p->attackOscC;
        case 12: return p->decayOscC;
        case 13: return p->sustainOscC;
        case 14: return p->releaseOscC;

        case 15: return p->cutoff;
        case 16: return p->resonance;
        case 17: return p->bypass;
        case 18: return p->mode;

        case 19: return p->volume;
    }
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

void displayError(char *pErr) {
    lcd.clear();
    lcd.print(pErr);
    delay(2000);
}

void setString(const char src[], char *dest, int len) {
    int i;
    for (i = 0; i < len && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < len; i++)
        dest[i] = '\0';
}
