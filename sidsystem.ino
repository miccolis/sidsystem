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
#include "patch.h"
#include "param.h"
#include "MIDI.h"

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
#define PARAM_11BIT 8

const int menu_start = 0;
const int menu_patch = 1;
const int menu_param = 2;
const int menu_value = 3;
const int param_confirm = -1;

const int lcd_width = 16;
const int lcd_lines = 2;

// Global state.
signed int encoderVal;  // Store the encoder's rotation counts
byte midiOn[3];
byte midiCC[3];
bool midiNotePlayed = false;
bool midiControlPlayed = false;

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

    MIDI.begin();
    MIDI.setHandleNoteOn(HandleNoteOn);
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
    static int activePage = menu_start; // "page" see "menu_x" constants.
    static patch activePatch;           // Active program (ie being played & edited)
    static param activeParam;           // Active parameter (ie being edited)
    static int menuValue = 0;           // Value of parameter.

    if (activePage == menu_start) {
        activePage = menu_patch;
        loadPatch(0, &activePatch);
        updateSynth(&activePatch);
    }

    MIDI.read();
    updatePerformance(&activePatch);

    int update = pollButtons();
    if (updateState(&activePage, &activePatch, &activeParam, &menuValue, update))
        updateMenu(&activePage, &activePatch, &activeParam, &menuValue);
}

// MIDI Callbacks
void HandleNoteOn(byte channel, byte note, byte velocity) {
    midiNotePlayed = true;
    midiOn[0] = channel;
    midiOn[1] = note;
    midiOn[2] = velocity;
}
void HandleControlChange(byte channel, byte number, byte value) {
    midiControlPlayed = true;
    midiCC[0] = channel;
    midiCC[1] = number;
    midiCC[2] = value;
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
            updateSynth(pPatch);
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

        if (update & 1) {
            *pPage = menu_value;
            encoderVal = *pValue;
        }
        else if (update & 2) {
            *pPage = menu_patch;
            encoderVal = pPatch->id;
        }
    }
    else if (*pPage == menu_value) {
        // TODO validate value against param def
        if (update) {
            *pPage = menu_param;
            encoderVal = pParam->id;
        }
        else {
            *pValue = encoderVal;
            setPatchValue(pParam->id, pPatch, *pValue);
            // TODO push update to synth.
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
            case PARAM_11BIT:
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
                // Filter Cutoff in HZ
                param def = {PARAM_11BIT, id, "Cutoff"};
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
    // TODO user patches.
    return loadFactoryDefaultPatch(id, pProg);
}

int loadPatchValue(int param, patch *p) {
    int *v = loadPatchValuePtr(param, p);
    return *v;
}

void setPatchValue(int param, patch *p, int val) {
    int *v = loadPatchValuePtr(param, p);
    *v = val;
}

int *loadPatchValuePtr(int param, patch *p) {
    switch (param) {
        case 0: return &(p->waveOscA);
        case 1: return &(p->attackOscA);
        case 2: return &(p->decayOscA);
        case 3: return &(p->sustainOscA);
        case 4: return &(p->releaseOscA);

        case 5: return &(p->waveOscB);
        case 6: return &(p->attackOscB);
        case 7: return &(p->decayOscB);
        case 8: return &(p->sustainOscB);
        case 9: return &(p->releaseOscB);

        case 10: return &(p->waveOscC);
        case 11: return &(p->attackOscC);
        case 12: return &(p->decayOscC);
        case 13: return &(p->sustainOscC);
        case 14: return &(p->releaseOscC);

        case 15: return &(p->cutoff);
        case 16: return &(p->resonance);
        case 17: return &(p->bypass);
        case 18: return &(p->mode);

        case 19: return &(p->volume);
    }
}

boolean loadFactoryDefaultPatch(int id, patch *pProg) {
    if (id == 0) {
        patch factory = {
            0, 2, 0, 8, 2,
            0, 2, 0, 8, 2,
            0, 2, 0, 8, 2,
            1024, 0, 0, 0, 0,
            id, "Bleep",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 1) {
        patch factory = {
            2, 12, 12, 15, 0,
            2, 12, 12, 15, 0,
            2, 12, 12, 15, 0,
            2000, 8, 0, 0, 0,
            id, "Spacey",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 2) {
        patch factory = {
            1, 1, 1, 4, 1,
            2, 2, 4, 4, 2,
            3, 3, 4, 4, 3,
            2047, 0, 0, 0, 4,
            id, "Belong",
        };
        return copyPatch(&factory, pProg);
    }
    else if (id == 3) {
        patch factory = {
            2, 0, 8, 0, 0,
            2, 0, 8, 0, 0,
            2, 0, 8, 0, 0,
            1024, 0, 0, 0, 0,
            id, "Disaste",
        };
        return copyPatch(&factory, pProg);
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

// SID management
void patchToRegisters(patch *p, byte *registers) {
    for (int i=0; i<25; i++) {
        registers[i] = 0;
    }

     // 0/1: Osc A: frequency (midi)
     // Osc A: pulse width (setting to 2048 for now)
     registers[2] = 0;
     registers[3] = 8;
     //  Osc A: Control Register
     //
     // 0000 0001 (1) - Gate (midi)
     // 0000 0010 (2) - Sync
     // 0000 0100 (4) - Ring mod
     // 0000 1000 (8) - Test (not used)
     // 0001 0000 (16) - Triangle
     // 0010 0000 (32) - Saw
     // 0100 0000 (64) - Square
     // 1000 0000 (128) - Noise
     switch (p->waveOscA) {
         case 0: registers[4] = 0x10; break;
         case 1: registers[4] = 0x20; break;
         case 2: registers[4] = 0x40; break;
         case 3: registers[4] = 0x4; break;
         case 4: registers[4] = 0x2; break;
         case 5: registers[4] = 0x80; break;
     }
     // Osc A: Attack, Decay
     registers[5] = (p->attackOscA << 4) + p->decayOscA;
     // Osc A: Sustain, Release
     registers[6] = (p->sustainOscA << 4) + p->releaseOscA;

     // 7/8: Osc B: frequency (midi)
     // Osc B: pulse width (setting to 2048 for now)
     registers[9] = 0;
     registers[10] = 8;

     // Osc B: Control Register
     switch (p->waveOscB) {
         case 0: registers[11] = 0x10; break;
         case 1: registers[11] = 0x20; break;
         case 2: registers[11] = 0x40; break;
         case 3: registers[11] = 0x4; break;
         case 4: registers[11] = 0x2; break;
         case 5: registers[11] = 0x80; break;
     }
     // Osc B: Attack, Decay
     registers[12] = (p->attackOscB << 4) + p->decayOscB;
     // Osc B: Sustain, Release
     registers[13] = (p->sustainOscB << 4) + p->releaseOscB;

     // 14/15: Osc C: frequency (midi)
     // Osc C: pulse width (setting to 2048 for now)
     registers[16] = 0;
     registers[17] = 8;

     // Osc C: Control Register
     switch (p->waveOscC) {
         case 0: registers[18] = 0x10; break;
         case 1: registers[18] = 0x20; break;
         case 2: registers[18] = 0x40; break;
         case 3: registers[18] = 0x4; break;
         case 4: registers[18] = 0x2; break;
         case 5: registers[18] = 0x80; break;
     }
     // Osc C: Attack, Decay
     registers[19] = (p->attackOscC << 4) + p->decayOscC;
     // Osc C: Sustain, Release
     registers[20] = (p->sustainOscC << 4) + p->releaseOscC;

     // Filter frequency (11 bits)
     int ffreq = p->cutoff;
     registers[21] = ffreq & 0x7; // 0000 0111
     registers[22] = ffreq >> 3;

     // Resonance, Routing
     registers[23] = p->resonance << 4;
     registers[23] |= 0x7; // 0111

     // Mode / Vol
     // 0001 0000 (0x10) - lowpass
     // 0010 0000 (0x40) - bandpass
     // 0100 0000 (0x20) - highpass
     // 0101 0000 (0x50) - notch
     switch (p->mode) {
         case 0: registers[24] = 0x10; break;
         case 1: registers[24] = 0x40; break;
         case 2: registers[24] = 0x20; break;
         case 3: registers[24] = 0x50; break;
     }
}

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

void updateSynth(patch *p) {
    static byte registers[25];
    patchToRegisters(p, registers);
    for (int i = 0; i < 25; i++) {
        if (i == 0 || i == 1 || i == 7 || i == 8  || i == 14 || i == 15) {
            // Don't overwrite frequency registers.
        }
        else {
            writeSidRegister(i, registers[i]);
        }
    }
}

void updatePerformance(patch *p) {
    // Values for C7 through B7.
    // B7, G7 & G#7 intentionally differ from the hex values in the data sheet
    static uint32_t octave[12] = {0x892B, 0x9153, 0x99F7, 0xA31F, 0xACD2, 0xB719, 0xC1FC,
        0xCD85, 0xD9BD, 0xE6B0, 0xF467, 0x102F0};

    // Control registers.
    static uint8_t controlReg[3] = {4, 11, 18};
    // Frequency registers
    static uint8_t freqReg[3][2] = {{0, 1}, {7, 8}, {14, 15}};
    static uint8_t lastNote = 0; // first bit is on/off, 7-bits are note.

    // Ignore MIDI channels for now.
    if (midiNotePlayed) {
        midiNotePlayed = false;
        
        // Control registers
        byte registers[25];
        patchToRegisters(p, registers); // TODO don't encode entire patch just
                                        // to toggle four bits.
        if (midiOn[2] > 0) {
            uint32_t note = octave[midiOn[1] % 12];
            int shift = (7 - (midiOn[1] / 12));
            note = note >> shift;
            uint8_t freqLo = note & 0xFF;
            uint8_t freqHi = note >> 8;

            for (int i = 0; i < 3; i++) {
                writeSidRegister(freqReg[i][0], freqLo);
                writeSidRegister(freqReg[i][1], freqHi);
            }

            // Volume
            uint8_t volume = ((midiOn[2] >> 3) + p->volume) & 0xF;
            writeSidRegister(24, registers[24] | volume);

            for (int i = 0; i < 3; i++) { // Open gates
                if (lastNote) {
                    writeSidRegister(controlReg[i], registers[controlReg[i]] & 0xFE);
                }
                writeSidRegister(controlReg[i], registers[controlReg[i]] | 0x1);
            }
            lastNote = midiOn[1] | 0x80;
        }
        else if (lastNote == (midiOn[1] | 0x80)) {
            for (int i = 0; i < 3; i++) { // Close gates
                writeSidRegister(controlReg[i], registers[controlReg[i]] & 0xFE);
            }
            lastNote = 0;
        }
    }

    if (midiControlPlayed) {
        midiControlPlayed = false;
        // TODO
    }
}

// interrupt handler for the rotary encoder.
void readEncoder() {
    static bool running = 0;
    if (running) return;
    running = 1;

    int8_t enc_states[] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};
    static uint8_t oldEncoderState = 0;
    static uint8_t newEncoderState = 0;

    newEncoderState = (digitalRead(enc_b)<<1) | (digitalRead(enc_a));
    // the form: 0b0000(old B)(old A)(new B)(new A)
    oldEncoderState <<= 2;
    oldEncoderState &= 0xC0;
    oldEncoderState |= newEncoderState;

    encoderVal += enc_states[oldEncoderState];
    running = 0;
}

void displayError(char *pErr) {
    lcd.clear();
    lcd.print(pErr);
    delay(2000);
}

void displayRegisters(patch *p) {
    byte registers[25];
    patchToRegisters(p, registers);

    int d = 2500;
    lcd.clear();
    for (int i = 0; i < 25; i++) {
        if (i != 0 && i % 7 == 0) {
            delay(d);
            lcd.clear();
        }
        if (registers[i] < 16)
            lcd.print(0);
        lcd.print(registers[i], HEX);
        lcd.print(' ');
        if (i % 7 == 4)
            lcd.setCursor(0,1);
    }
    delay(d);
}

void setString(const char src[], char *dest, int len) {
    int i;
    for (i = 0; i < len && src[i] != '\0'; i++)
        dest[i] = src[i];
    for ( ; i < len; i++)
        dest[i] = '\0';
}
