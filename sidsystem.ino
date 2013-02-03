/* 

SID System

A modern MOS 6581 powered Synthesizer.

The circuit:

LCD

 * RS pin to digital pin A5
 * Enable pin to digital pin A4
 * LCD4 pin to digital pin 7
 * LCD5 pin to digital pin 6
 * LCD6 pin to digital pin 5
 * LCD7 pin to digital pin 4
 * R/W pin to ground
 * 10K resistor:
 * ends to +5V and ground
 * (???)wiper to LCD VO pin (pin 3)
 
Rotary Encoder 

 * A to digital pin 2
 * B to ditigal pin 3
 * button digital pin 8

Esc button

 * momentary switch to digital pin 10

MIDI

 * IN to digital pin 0 (must be disconnected during usb transfers)

Shift register A

 * DS to analogue pin 0
 * ST_CP to analogue pin 1
 * SH_CP to analogue pin 2

Shift register B

 * ST_CP to analogue pin 1
 * SH_CP to analogue pin 0
 * DS to Shift register Q7'

SID

 * CLK from digital pin 9
 * CS from analogue pin 3
 
*/

#include <LiquidCrystal.h>
#include <math.h>
#include "utils.h"
#include "patch.h"
#include "param.h"
#include "MIDI.h"

#define PROGRAMS_AVAILABLE 19 
#define NO_PARAM 0xFF

LiquidCrystal lcd(A5, A4, 7, 6, 5, 4);
const int enc_a = 2;
const int enc_b = 3;
const int enc_button = 8;
const int button_esc = 10;

const int sr_ds = A0;
const int sr_st_cp= A1;
const int sr_sh_cp= A2;

const int sid_cs = A3;

// Clock settings are duplicated in setup()
const int sid_clk_reg = PORTB;
const int sid_clk_bit = DDB1;

const int menu_start = 0;
const int menu_patch = 1;
const int menu_param = 2;
const int menu_value = 3;
const int param_confirm = -1;

const int lcd_width = 16;
const int lcd_lines = 2;

// Global state.
signed int encoderVal;
uint8_t midiOn[3];
uint8_t midiCC[3];
bool midiNotePlayed = false;
bool midiControlPlayed = false;
uint8_t midiAssignments[120];

void setup() {

    //Use Timer/Counter1 to generate a 1MHz square wave on Arduino pin 9.
    DDRB |= _BV(DDB1);                 //set OC1A/PB1 as output
    TCCR1A = _BV(COM1A0);              //toggle OC1A on compare match
    OCR1A = 7;                         //top value for counter
    TCCR1B = _BV(WGM12) | _BV(CS10);   //CTC mode, prescaler clock/1

    pinMode(enc_button, INPUT);
    
    pinMode(enc_a, INPUT);
    digitalWrite(enc_a, HIGH);
    pinMode(enc_b, INPUT);
    digitalWrite(enc_b, HIGH);

    pinMode(button_esc, INPUT);
    digitalWrite(button_esc, HIGH);

    pinMode(sr_ds, OUTPUT);
    pinMode(sr_sh_cp, OUTPUT);
    pinMode(sr_st_cp, OUTPUT);

    pinMode(sid_cs, OUTPUT);
    digitalWrite(sid_cs, HIGH);

    for (int i; i < 120; i++) midiAssignments[i] = 0xFF;
    MIDI.begin();
    MIDI.setHandleNoteOn(HandleNoteOn);
    MIDI.setHandleNoteOff(HandleNoteOff);
    MIDI.setHandleControlChange(HandleControlChange);
    
    delay(500);
    lcd.begin(lcd_width, lcd_lines);
    lcd.print("<< powered up >>");

    noInterrupts();
    attachInterrupt(0, readEncoder, CHANGE);
    attachInterrupt(1, readEncoder, CHANGE);
    interrupts();

    delay(500);
    lcd.blink();
    lcd.cursor();
}

void loop() {
    static int page = menu_start; // "page" see "menu_x" constants.
    static livePatch patch;       // Active program (ie being played & edited)
    static param parameter;       // Active parameter (ie being edited)
    static int value = 0;         // Value of parameter.

    static unsigned long lastUpdate  = 0;
    static bool needsUpdate = false;

    if (page == menu_start) {
        page = menu_patch;
        loadPatch(0, &patch);
        updateSynth(&patch);
    }

    MIDI.read();

    needsUpdate = updateState(&page, &patch, &parameter, &value, pollButtons(),
                                updatePerformance(&patch)) || needsUpdate;

    // Limit frequency of UI updates.
    if (needsUpdate && lastUpdate < (millis() + 500)) {
        updateMenu(&page, &patch, &parameter, &value);
        lastUpdate = millis();
        needsUpdate = false;
    }
}

// MIDI Callbacks
void HandleNoteOn(byte channel, byte note, byte velocity) {
    midiNotePlayed = true;
    midiOn[0] = channel;
    midiOn[1] = note;
    midiOn[2] = velocity;
}

void HandleNoteOff(byte channel, byte note, byte velocity) {
    midiNotePlayed = true;
    midiOn[0] = channel;
    midiOn[1] = note;
    midiOn[2] = 0;
}

void HandleControlChange(byte channel, byte number, byte value) {
    midiControlPlayed = true;
    midiCC[0] = channel;
    midiCC[1] = number;
    midiCC[2] = value;
}

// Interrupt handler for the rotary encoder.
void readEncoder() {
    noInterrupts();
    static uint8_t prev;
    uint8_t cur = (digitalRead(enc_b) << 1) | (digitalRead(enc_a));
    if (prev != cur) {
        if (cur == 3) encoderVal += (prev == 1 ? 1 : -1);
        prev = cur;
    }
    interrupts();
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
            if (buttons[i] == enc_button && buttonState[i] == 1) update = 1;
            // Esc button
            if (buttons[i] == button_esc && buttonState[i] == 0) update += 2;
        }
    }
    return update;
}

// Update system state based on input. Responsible for validation.
// Return true if menu should be updated.
bool updateState(int *pPage, livePatch *pPatch, param *pParam, int *pValue, int update, uint8_t playedParam) {
    static int curEncoderVal = -1;

    // Early condition checks:
    // 1. If the encoder has changed, we proceed
    if (curEncoderVal != encoderVal) curEncoderVal = encoderVal;
    // 2. If there aren't input updates, nor CC messages, we bail.
    else if (update == 0 && playedParam == NO_PARAM) return false;

    if (*pPage == menu_patch) {
        // Input validation is rough now, but we only have 4 programs
        if (encoderVal < 0) { encoderVal = 0; return true; }
        if (encoderVal > PROGRAMS_AVAILABLE) { encoderVal = PROGRAMS_AVAILABLE; return true; }
        if (update & 1) {
            loadPatch(encoderVal, pPatch);
            updateSynth(pPatch);
            loadParam(0, pParam);
            *pValue = loadPatchValue(pParam->id, pPatch);
            *pPage = menu_param;
        }
        else {
            *pValue = encoderVal;
        }
        return true;
    }
    else if (*pPage == menu_param) {
        if (encoderVal < -1) { encoderVal = -1; return true; }
        if (encoderVal > 26) { encoderVal = 26; return true; }

        loadParam(encoderVal, pParam);
        *pValue = loadPatchValue(pParam->id, pPatch);

        if (update & 1) {
            *pPage = menu_value;
            encoderVal = *pValue;
        }
        else if (update & 2) {
            *pPage = menu_patch;
            encoderVal = pPatch->patch.id;
        }
        return true;
    }
    else if (*pPage == menu_value) {
        if (playedParam != NO_PARAM) {
            // If we're responding to a CC, update the UI if it's the visible
            // one, and back out of it.
            if (playedParam == pParam->id) {
                *pPage = menu_param;
                encoderVal = pParam->id;
                return true;
            }
            else {
                return false;
            }
        }

        // Respond to inputs.
        if (update & 1) {
            // Backout to parameter selection
            *pPage = menu_param;
            encoderVal = pParam->id;
            return true;
        }
        else if (update & 2) {
            // Update midi mapping...
            midiAssignments[midiCC[1]] = pParam->id;
            lcd.clear();
            lcd.print("Assigned CC");
            lcd.setCursor(0,1);
            lcd.print(midiCC[1]);
            delay(1000);
            // ...and backout.
            *pPage = menu_param;
            encoderVal = pParam->id;
            return true;
        }

        // Handle encoder.
        int limit = paramLimit(pParam);
        if (encoderVal < 0) encoderVal = 0;
        else if (encoderVal > limit ) encoderVal = limit;
        else {
            *pValue = encoderVal;
            updatePerfParam(pPatch, pParam->id, *pValue);
        }
        return true;
    }
}

// Update the GUI based on system state change.
void updateMenu(int *pPage, livePatch *pPatch, param *pParam, int *pValue) {

    lcd.clear();
    lcd.print(pPatch->patch.name);
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
        if (pParam->type & PARAM_LABEL) {
            char optionString[PARAMNAME_LEN]= {' '};
            loadParamOption(pParam, *pValue, optionString);
            lcd.print(optionString);
        }
        else if (pParam->type != PARAM_UNAVAIL) {
            lcd.print(*pValue);
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
        case 7:
        case 15:
            // Waveform
            if      (idx == 0) setString("Triangle", pStr, PARAMNAME_LEN);
            else if (idx == 1) setString("Saw", pStr, PARAMNAME_LEN);
            else if (idx == 2) setString("Pulse", pStr, PARAMNAME_LEN);
            else if (idx == 3) setString("Ring mod", pStr, PARAMNAME_LEN);
            else if (idx == 4) setString("Sync Tri", pStr, PARAMNAME_LEN);
            else if (idx == 5) setString("Sync Saw", pStr, PARAMNAME_LEN);
            else if (idx == 6) setString("Sync Pul", pStr, PARAMNAME_LEN);
            else if (idx == 7) setString("Noise", pStr, PARAMNAME_LEN);
            else return false;
            return true;
        case 6:
        case 13:
        case 21:
            if (idx == 0) setString("Off", pStr, PARAMNAME_LEN);
            else if (idx == 1) setString("On", pStr, PARAMNAME_LEN);
            else return false;
            return true;
        case 25:
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
    if      (id > 6 && id < 15)  osc = 'B';
    else if (id > 14 && id < 23) osc = 'C';

    switch (id) {
        case 0:
        case 7:
        case 15:
            {
                param def = {PARAM_LABEL | 8 , id, "  Wave"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 1:
        case 8:
        case 16:
            {
                param def = {PARAM_12BIT, id, "  PW"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 2:
        case 9:
        case 17:
            {
                param def = {PARAM_4BIT, id, "  Att"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 3:
        case 10:
        case 18:
            {
                param def = {PARAM_4BIT, id, "  Dec"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 4:
        case 11:
        case 19:
            {
                param def = {PARAM_4BIT, id, "  Sus"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 5:
        case 12:
        case 20:
            {
                param def = {PARAM_4BIT, id, "  Rel"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 6:
        case 13:
        case 21:
            {
                param def = {PARAM_LABEL | PARAM_1BIT, id, "  Filt"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 14:
        case 22:
            {
                param def = {PARAM_DETUNE, id, "  tune"};
                def.name[0] = osc;
                return copyParam(&def, pParam);
            }
        case 23:
            {
                // Filter Cutoff in HZ
                param def = {PARAM_11BIT, id, "Cutoff"};
                return copyParam(&def, pParam);
            }
        case 24:
            {
                param def = {PARAM_4BIT, id, "Reso"};
                return copyParam(&def, pParam);
            }
        case 25:
            {
                param def = {PARAM_LABEL | 4, id, "Mode"};
                return copyParam(&def, pParam);
            }
        case 26:
            {
                param def = {PARAM_4BIT, id, "Volume"};
                return copyParam(&def, pParam);
            }
    }
    return false;
}

// Patch methods
bool loadPatchName(int id, char *pStr) {
    livePatch p;
    loadPatch(id, &p);
    setString(p.patch.name, pStr, PATCHNAME_LEN);
    return true;
}

bool loadPatch(int id, livePatch *pProg) {
    // TODO user patches.
    return loadFactoryDefaultPatch(id, pProg);
}

bool loadFactoryDefaultPatch(int id, livePatch *pProg) {
    if (id == 0) {
        patchSettings factory = {
            0, 0, 8, 0, 8, 2, 1,
            0, 0, 8, 0, 8, 2, 1, 0,
            0, 0, 8, 0, 8, 2, 1, 0,
            200, 0, 1, 0,
            id, "Bleep",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 1) {
        patchSettings factory = {
            2, 2048, 12, 12, 15, 0, 1,
            2, 2048, 12, 12, 15, 0, 1, 0,
            2, 2048, 12, 12, 15, 0, 1, 0,
            2000, 8, 0, 0,
            id, "Spacey",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 2) {
        patchSettings factory = {
            1, 0, 1, 1, 4, 1, 1, 
            2, 2048, 2, 4, 4, 2, 1, 0,
            3, 0, 3, 4, 4, 3, 1, 0,
            2047, 0, 0, 4,
            id, "Belong",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 3) {
        patchSettings factory = {
            2, 2048, 0, 8, 0, 0, 1,
            2, 2048, 0, 8, 0, 0, 1, 0,
            2, 2048, 0, 8, 0, 0, 1, 0,
            1024, 0, 0, 0,
            id, "Disaste",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 4) {
        patchSettings factory = {
            1, 0, 0, 15, 14, 5, 1,
            1, 0, 0, 15, 14, 5, 1, 4,
            1, 0, 0, 15, 14, 5, 1, 8,
            1024, 4, 0, 0,
            id, "Sawbass",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 5) {
        patchSettings factory = {
            4, 0, 8, 0, 14, 2, 1,
            0, 0, 8, 0, 14, 2, 1, 0,
            0, 0, 0, 0, 0, 0, 1, 50,
            200, 4, 1, 0,
            id, "Bowser",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 6) {
        patchSettings factory = {
            2, 500,  12, 7, 0, 12, 1,
            6, 1000, 12, 8, 0, 12, 1, 10,
            6, 2000, 12, 9, 0, 12, 1, 20,
            2000, 1, 0, 0,
            id, "syncpad",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 7) {
        patchSettings factory = {
            7, 0, 1, 2, 10, 5, 1,
            0, 0, 1, 4, 12, 5, 1, 0,
            0, 0, 5, 4, 15, 8, 0, 50,
            1320, 2, 3, 0,
            id, "digi",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 8) {
        patchSettings factory = {
            0, 0, 3, 10, 5, 5, 1,
            3, 0, 3, 10, 5, 5, 1, 1,
            0, 0, 0, 0, 0, 0, 1, 50,
            2000, 2, 0, 0,
            id, "modmod",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 9) {
        patchSettings factory = {
            0, 0, 3, 3, 9, 6, 1,
            4, 0, 3, 3, 9, 6, 1, 10,
            0, 0, 0, 5, 3, 6, 1, 2,
            2000, 2, 0, 0,
            id, "sings",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 10) {
        patchSettings factory = {
            1, 0, 3, 3, 9, 6, 1,
            5, 0, 3, 3, 9, 6, 1, 10,
            5, 0, 0, 5, 3, 6, 1, 40,
            700, 3, 1, 0,
            id, "pluky",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 11) {
        patchSettings factory = {
            7, 0, 0, 3, 0, 0, 1,
            3, 0, 1, 2, 13, 4, 1, 10,
            3, 0, 1, 2, 13, 4, 1, 20,
            1400, 2, 14, 0,
            id, "boomer",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 12) {
        patchSettings factory = {
            7, 0, 0, 3, 0, 0, 1,
            7, 0, 1, 2, 13, 4, 1, 5,
            3, 0, 1, 2, 13, 4, 1, 1,
            1400, 2, 1, 0,
            id, "metalsc",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 13) {
        patchSettings factory = {
            0, 0, 1, 4, 13, 4, 1,
            4, 0, 1, 4, 13, 6, 1, 60,
            0, 0, 5, 2, 13, 4, 1, 2,
            2000, 2, 0, 0,
            id, "slider",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 14) {
        patchSettings factory = {
            0, 0, 4, 0, 15, 3, 1,
            5, 0, 0, 3, 10, 0, 1, 10,
            5, 0, 0, 3, 10, 0, 1, 8,
            2000, 4, 0, 0,
            id, "lowrm",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 15) {
        patchSettings factory = {
            0, 0, 4, 4, 12, 3, 0,
            7, 0, 0, 0, 0, 0, 1, 0,
            3, 0, 2, 4, 11, 0, 1, 2,
            2000, 4, 0, 0,
            id, "nring",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 16) {
        patchSettings factory = {
            0, 0, 1, 4, 12, 3, 0,
            4, 0, 1, 4, 12, 0, 0, 5,
            7, 0, 0, 4, 0, 0, 1, 0,
            2000, 4, 2, 0,
            id, "tin",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 17) {
        patchSettings factory = {
            2, 1400, 1, 4, 12, 3, 1,
            2, 300,  1, 4, 12, 3, 1, 2,
            3, 0,    0, 4, 10, 8, 0, 0,
            2000, 0, 0, 0,
            id, "pcomplx",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 18) {
        patchSettings factory = {
            3, 0,    1, 4, 8, 3, 1,
            2, 1000, 1, 4, 8, 3, 1, 0,
            3, 0,    1, 4, 8, 3, 0, 10,
            2000, 0, 0, 0,
            id, "rounds",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 19) {
        patchSettings factory = {
            0, 0, 4, 4, 8, 6, 0,
            4, 0, 4, 4, 8, 6, 0, 2,
            4, 0, 3, 4, 8, 3, 0, 64,
            2000, 0, 1, 0,
            id, "fff",
        };
        return copyPatch(&factory, pProg);
    }
    return false;
}

// SID management
void writeSidRegister(byte loc, byte val) {
    digitalWrite(sr_st_cp, LOW);
    shiftOut(sr_ds , sr_sh_cp, MSBFIRST, loc);
    shiftOut(sr_ds , sr_sh_cp, MSBFIRST, val);
    digitalWrite(sr_st_cp, HIGH);

    // Data is written as clock goes from high to low.
    digitalWrite(sid_cs, LOW);
    // As written the time between the `sid_cs` line going to low and back up to
    // high is always at least three full cycle on the `mhz click` so more
    // accurate tracking doesn't appear to be required.
    digitalWrite(sid_cs, HIGH);
}

// Wrapper around writeSidRegister, forces changes to be in livepatch.registers
void writeSR(livePatch *p, uint8_t i) {
    writeSidRegister(i, p->registers[i]);
}

void updateSynth(livePatch *p) {
    patchToRegisters(p);
    for (int i = 0; i < 25; i++) {
        if (i == 0 || i == 1 || i == 7 || i == 8  || i == 14 || i == 15) {
            // Don't overwrite frequency registers.
        }
        else {
            writeSR(p, i);
        }
    }
}

void noteToRegisters(livePatch *p, char osc) {
    // Values for C7 through B7.
    // B7, G7 & G#7 intentionally differ from the hex values in the data sheet
    const uint32_t octave[12] = {0x892B, 0x9153, 0x99F7, 0xA31F, 0xACD2, 0xB719, 0xC1FC,
        0xCD85, 0xD9BD, 0xE6B0, 0xF467, 0x102F0};

    uint32_t note = octave[p->note % 12];
    int shift = (7 - (p->note / 12));
    note = note >> shift;

    // 'u' is for unison! ...and updates all registers.
    if (osc == 'u' || osc == 'a') {
        // Currently no detune for osc a
        p->registers[0] = note & 0xFF;
        p->registers[1] = note >> 8;
    }
    if (osc == 'u' || osc == 'b') {
        uint16_t n = note;
        if (p->patch.detuneOscB){
            n = n * pow(2, (float)p->patch.detuneOscB / 120);
        }
        p->registers[7] = n & 0xFF;
        p->registers[8] = n >> 8;
    }
    if (osc == 'u' || osc == 'c') {
        uint16_t n = note;
        if (p->patch.detuneOscC) {
            n = n * pow(2, (float)p->patch.detuneOscC / 120);
        }
        p->registers[14] = n & 0xFF;
        p->registers[15] = n >> 8;
    }
}

// Return NO_PARAM or the parameter id played.
uint8_t updatePerformance(livePatch *p) {
    // Control registers.
    const uint8_t controlReg[3] = {4, 11, 18};
    // Frequency registers
    const uint8_t freqReg[3][2] = {{0, 1}, {7, 8}, {14, 15}};
    static uint8_t lastNote = 0; // first bit is on/off, 7-bits are note.

    // Ignore MIDI channels for now.
    if (midiNotePlayed) {
        midiNotePlayed = false;
        
        // Control registers
        if (midiOn[2] > 0) {
            p->note = midiOn[1];
            noteToRegisters(p, 'u');
            for (int i = 0; i < 3; i++) {
                writeSR(p, freqReg[i][0]);
                writeSR(p, freqReg[i][1]);
            }

            // Volume
            p->registers[24] |= ((midiOn[2] >> 3) + p->patch.volume) & 0xF;
            writeSR(p, 24);

            for (int i = 0; i < 3; i++) { // Open gates
                if (lastNote) {
                    p->registers[controlReg[i]] &= 0xFE;
                    writeSR(p, controlReg[i]);
                }
                p->registers[controlReg[i]] |= 0x1;
                writeSR(p, controlReg[i]);
            }
            lastNote = midiOn[1] | 0x80;
        }
        else if (lastNote == (midiOn[1] | 0x80)) {
            for (int i = 0; i < 3; i++) { // Close gates
                p->registers[controlReg[i]] &= 0xFE;
                writeSR(p, controlReg[i]);
            }
            lastNote = 0;
        }
    }

    if (midiControlPlayed) {
        midiControlPlayed = false;
        if (midiAssignments[midiCC[1]] != NO_PARAM) {
            param target;
            loadParam(midiAssignments[midiCC[1]], &target);
            int v = (float)midiCC[2] / 127 * (float)(paramLimit(&target));
            updatePerfParam(p, target.id, v);
            return target.id;
        }
    }
    return NO_PARAM;
}

void updatePerfParam(livePatch *pPatch, int param, int val) {
    setPatchValue(pPatch, param, val);
    if (param == 14 || param == 22) {
        // Detune is a special case for now, should be able to move noteToRegisters
        // and this logic over to patch.h 
        noteToRegisters(pPatch, param == 14 ? 'b' : 'c');
    }
    int loc = patchParamRegister(param);
    writeSR(pPatch, loc & 0xFF);
    if (loc & 0xFF00) {
        loc = loc >> 8;
        writeSR(pPatch, loc);
    }
}
