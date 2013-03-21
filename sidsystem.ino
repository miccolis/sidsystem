/* 

SID System

A modern MOS 6581 powered Synthesizer.

The circuit:


SID

 * CLK from digital pin 9
 * CS from analogue pin 0
 * D0 - D7 from digital 10, 8 - 2
 * A0 - 4 from analogue 5 - 1
 
*/

#include <inttypes.h>
#include <math.h>
#include "utils.h"
#include "patch.h"
#include "param.h"

#define PROGRAMS_AVAILABLE 19 
#define NO_PARAM 0xFF

// Clock settings for pin D9 are in setup()

// CS
const int sid_cs = A0;

// Address
const int sid_a0 = A5;
const int sid_a1 = A4;
const int sid_a2 = A3;
const int sid_a3 = A2;
const int sid_a4 = A1;

// Data
const int sid_d0 = 10;
const int sid_d1 = 8;
const int sid_d2 = 7;
const int sid_d3 = 6;
const int sid_d4 = 5;
const int sid_d5 = 4;
const int sid_d6 = 3;
const int sid_d7 = 2;

String command = "";
boolean commandAvail = false;

void setup() {

    //Use Timer/Counter1 to generate a 1MHz square wave on Arduino pin 9.
    DDRB |= _BV(DDB1);                 //set OC1A/PB1 as output
    TCCR1A = _BV(COM1A0);              //toggle OC1A on compare match
    OCR1A = 7;                         //top value for counter
    TCCR1B = _BV(WGM12) | _BV(CS10);   //CTC mode, prescaler clock/1

    pinMode(sid_cs, OUTPUT);
    digitalWrite(sid_cs, HIGH);

    pinMode(sid_a0, OUTPUT);
    pinMode(sid_a1, OUTPUT);
    pinMode(sid_a2, OUTPUT);
    pinMode(sid_a3, OUTPUT);
    pinMode(sid_a4, OUTPUT);

    pinMode(sid_d0, OUTPUT);
    pinMode(sid_d1, OUTPUT);
    pinMode(sid_d2, OUTPUT);
    pinMode(sid_d3, OUTPUT);
    pinMode(sid_d4, OUTPUT);
    pinMode(sid_d5, OUTPUT);
    pinMode(sid_d6, OUTPUT);
    pinMode(sid_d7, OUTPUT);

    command.reserve(20);

    delay(500);
    Serial.begin(9600);
    Serial.println("<< powered up >>");

}

void loop() {
    static livePatch patch;       // Active program (ie being played & edited)

    while (Serial.available()) {
        char inChar = Serial.read(); 
        if (inChar == '\n') {
            commandAvail = true;
        } 
        else {
            command += inChar;
        }
    }

    if (commandAvail) {
        Serial.println(command);
        if (command.charAt(0) == 'p') {
            char s[2];
            command.substring(1).toCharArray(s, 2);
            int p = atoi(s);
            loadPatch(p, &patch);
            updateSynth(&patch);
            Serial.println(patch.patch.name);
        }
        else if (command.charAt(0) == 'n') {
            char s[2];
            command.substring(1).toCharArray(s, 2);
            int n = atoi(s);
            uint8_t noteOn[] = {0, n, 127};
            uint8_t noteOff[] = {0, n, 0};
            Serial.println("Note on");
            updatePerformance(&patch, true, 0, noteOn); // patch, note, control, note message
            delay(250);
            updatePerformance(&patch, true, 0, noteOff); // patch, note, control, note message
            Serial.println("Note off");
        }
        else {
            Serial.println("Invalid command");
        }
        command = "";
        commandAvail = false;
    }

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

    digitalWrite(sid_a0, loc & B00001);
    digitalWrite(sid_a1, loc & B00010);
    digitalWrite(sid_a2, loc & B00100);
    digitalWrite(sid_a3, loc & B01000);
    digitalWrite(sid_a4, loc & B10000);

    digitalWrite(sid_d0, val & B00000001);
    digitalWrite(sid_d1, val & B00000010);
    digitalWrite(sid_d2, val & B00000100);
    digitalWrite(sid_d3, val & B00001000);
    digitalWrite(sid_d4, val & B00010000);
    digitalWrite(sid_d5, val & B00100000);
    digitalWrite(sid_d6, val & B01000000);
    digitalWrite(sid_d7, val & B10000000);

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
uint8_t updatePerformance(livePatch *p, int midiNotePlayed, int midiControlPlayed, uint8_t midiOn[3]) {
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

    //if (midiControlPlayed) {
    //    midiControlPlayed = false;
    //    if (midiAssignments[midiCC[1]] != NO_PARAM) {
    //        param target;
    //        loadParam(midiAssignments[midiCC[1]], &target);
    //        int v = (float)midiCC[2] / 127 * (float)(paramLimit(&target));
    //        updatePerfParam(p, target.id, v);
    //        return target.id;
    //    }
    //}
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
