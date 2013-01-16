/*
 * A patch.
 *
 * TODO use more accurate type declarations.
 */

#define PATCHNAME_LEN 8 // Max length of patch names.

struct patchSettings {
    // Oscillators
    // A: 0-6
    int waveOscA;    // 0 - 6
    int pulseWidthOscA;
    int attackOscA;  // nibble
    int decayOscA;   // nibble
    int sustainOscA; // nibble
    int releaseOscA; // nibble
    int filterOscA; // bool

    // B: 7-14
    int waveOscB;
    int pulseWidthOscB;
    int attackOscB;
    int decayOscB;
    int sustainOscB;
    int releaseOscB;
    int filterOscB;
    int detuneOscB;

    // C: 15-22
    int waveOscC;
    int pulseWidthOscC;
    int attackOscC;
    int decayOscC;
    int sustainOscC;
    int releaseOscC;
    int filterOscC;
    int detuneOscC;

    // Filter: 23-25
    int cutoff;     // 11-bit
    int resonance;  // nibble
    int mode;       // 0 - 4

    // General: 26
    int volume;     // nibble
    // portamento
    // retrigger

    // System
    int id;
    char name[8];
};

struct livePatch {
    uint8_t registers[25];
    patchSettings patch;
};

// Returns two bytes, one register value in each.
uint16_t patchParamRegister(int param) {
    switch (param) {
        // OSC A
        case 0: return 4;           // Waveform
        case 1: return (2 << 8 | 3);// Pulse width
        case 2: return 5;           // Attack
        case 3: return 5;           // Decay
        case 4: return 6;           // Sustain
        case 5: return 6;           // Releasea
        case 6: return 23;          // Filter enable

        // OSC B
        case 7: return 11;
        case 8: return (9 << 8 | 10);
        case 9: return 12;
        case 10: return 12;
        case 11: return 13;
        case 12: return 13;
        case 13: return 23;
        case 14: return (7 << 8 | 8); // Detune

        // OSC C
        case 15: return 18;
        case 16: return (16 << 8 | 17);
        case 17: return 19;
        case 18: return 19;
        case 19: return 20;
        case 20: return 20;
        case 21: return 23;
        case 22: return (14 << 8 | 15);

        // Filter
        case 23: return (21 << 8) | 22; // Filter cutoff
        case 24: return 23;             // Resonance
        case 25: return 24;             // Filter mode

        // General
        case 26: return 24;             // Volume
    }
}

int *loadPatchValuePtr(int param, livePatch *p) {
    switch (param) {
        case 0: return &(p->patch.waveOscA);
        case 1: return &(p->patch.pulseWidthOscA);
        case 2: return &(p->patch.attackOscA);
        case 3: return &(p->patch.decayOscA);
        case 4: return &(p->patch.sustainOscA);
        case 5: return &(p->patch.releaseOscA);
        case 6: return &(p->patch.filterOscA);

        case 7: return &(p->patch.waveOscB);
        case 8: return &(p->patch.pulseWidthOscB);
        case 9: return &(p->patch.attackOscB);
        case 10: return &(p->patch.decayOscB);
        case 11: return &(p->patch.sustainOscB);
        case 12: return &(p->patch.releaseOscB);
        case 13: return &(p->patch.filterOscB);
        case 14: return &(p->patch.detuneOscB);

        case 15: return &(p->patch.waveOscC);
        case 16: return &(p->patch.pulseWidthOscC);
        case 17: return &(p->patch.attackOscC);
        case 18: return &(p->patch.decayOscC);
        case 19: return &(p->patch.sustainOscC);
        case 20: return &(p->patch.releaseOscC);
        case 21: return &(p->patch.filterOscC);
        case 22: return &(p->patch.detuneOscC);

        case 23: return &(p->patch.cutoff);
        case 24: return &(p->patch.resonance);
        case 25: return &(p->patch.mode);

        case 26: return &(p->patch.volume);
    }
}

int loadPatchValue(int param, livePatch *p) {
    int *v = loadPatchValuePtr(param, p);
    return *v;
}

void patchUpdateRegister(livePatch *p, int param) {
    // TODO support pulseWidth, detune, bypass
    switch(param) {
        // 0/1: Osc A: frequency (midi)
    case 0:
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
        switch (p->patch.waveOscA) {
            case 0: p->registers[4] = 0x10; break;
            case 1: p->registers[4] = 0x20; break;
            case 2: p->registers[4] = 0x40; break;
            case 3: p->registers[4] = 0x4; break;
            case 4: p->registers[4] = 0x2; break;
            case 5: p->registers[4] = 0x80; break;
        }
        break;
    case 1:
        // Osc A: pulse width (setting to 2048 for now)
        p->registers[2] = 0;
        p->registers[4] = 8;
        break;
    case 2:
    case 3:
        // Osc A: Attack, Decay
        p->registers[5] = (p->patch.attackOscA << 4) + p->patch.decayOscA;
        break;
    case 4:
    case 5:
        // Osc A: Sustain, Release
        p->registers[6] = (p->patch.sustainOscA << 4) + p->patch.releaseOscA;
        break;
    case 6:
        // Osc A: Filter enable
        break;

        // 7/8: Osc B: frequency (midi)
    case 7:
        // Osc B: Control Register
        switch (p->patch.waveOscB) {
            case 0: p->registers[11] = 0x10; break;
            case 1: p->registers[11] = 0x20; break;
            case 2: p->registers[11] = 0x40; break;
            case 3: p->registers[11] = 0x4; break;
            case 4: p->registers[11] = 0x2; break;
            case 5: p->registers[11] = 0x80; break;
        }
        break;
    case 8:
        // Osc B: pulse width (setting to 2048 for now)
        p->registers[9] = 0;
        p->registers[10] = 8;
        break;
    case 9:
    case 10:
        // Osc B: Attack, Decay
        p->registers[12] = (p->patch.attackOscB << 4) + p->patch.decayOscB;
        break;
    case 11:
    case 12:
        // Osc B: Sustain, Release
        p->registers[13] = (p->patch.sustainOscB << 4) + p->patch.releaseOscB;
        break;
    case 13:
        // Osc B: Filter enable
        break;
    case 14:
        // Osc B: Detune
        break;

        // 14/15: Osc C: frequency (midi)
    case 15:
        // Osc C: Control Register
        switch (p->patch.waveOscC) {
            case 0: p->registers[18] = 0x10; break;
            case 1: p->registers[18] = 0x20; break;
            case 2: p->registers[18] = 0x40; break;
            case 3: p->registers[18] = 0x4; break;
            case 4: p->registers[18] = 0x2; break;
            case 5: p->registers[18] = 0x80; break;
        }
        break;
    case 16:
        // Osc C: pulse width (setting to 2048 for now)
        p->registers[16] = 0;
        p->registers[17] = 8;
        break;
    case 17:
    case 18:
        // Osc C: Attack, Decay
        p->registers[19] = (p->patch.attackOscC << 4) + p->patch.decayOscC;
        break;
    case 19:
    case 20:
        // Osc C: Sustain, Release
        p->registers[20] = (p->patch.sustainOscC << 4) + p->patch.releaseOscC;
        break;
    case 21:
        // Osc C: Filter enable
        break;
    case 22:
        // Osc C: Detune
        break;

    case 23:
        // Filter frequency (11 bits)
        p->registers[21] = p->patch.cutoff & 0x7; // 0000 0111
        p->registers[22] = p->patch.cutoff >> 3;
        break;
    case 24:
        // Resonance, Routing
        p->registers[23] = p->patch.resonance << 4;
        // 0100 - OSC C
        // 0010 - OSC B
        // 0001 - OSC A
        p->registers[23] |= 0x7; // 0111
        break;
    case 25:
    case 26:
        // Mode / Vol
        // 0001 0000 (0x10) - lowpass
        // 0010 0000 (0x40) - bandpass
        // 0100 0000 (0x20) - highpass
        // 0101 0000 (0x50) - notch
        switch (p->patch.mode) {
            case 0: p->registers[24] = 0x10; break;
            case 1: p->registers[24] = 0x40; break;
            case 2: p->registers[24] = 0x20; break;
            case 3: p->registers[24] = 0x50; break;
        }
        break;
    }
}

void patchToRegisters(livePatch *p) {
    for (int i = 0; i < 27; i++) {
        patchUpdateRegister(p, i);
    }
}

// Update the setting, and the register value.
void setPatchValue(int param, livePatch *p, int val) {
    int *v = loadPatchValuePtr(param, p);
    *v = val;
    patchUpdateRegister(p, param);
}

bool copyPatch(patchSettings *pSrc, livePatch *pDest) {
    pDest->patch.waveOscA       = pSrc->waveOscA;
    pDest->patch.pulseWidthOscA = pSrc->pulseWidthOscA;
    pDest->patch.attackOscA     = pSrc->attackOscA;
    pDest->patch.decayOscA      = pSrc->decayOscA;
    pDest->patch.sustainOscA    = pSrc->sustainOscA;
    pDest->patch.releaseOscA    = pSrc->releaseOscA;
    pDest->patch.filterOscA     = pSrc->filterOscA;

    pDest->patch.waveOscB       = pSrc->waveOscB;
    pDest->patch.pulseWidthOscB = pSrc->pulseWidthOscB;
    pDest->patch.attackOscB     = pSrc->attackOscB;
    pDest->patch.decayOscB      = pSrc->decayOscB;
    pDest->patch.sustainOscB    = pSrc->sustainOscB;
    pDest->patch.releaseOscB    = pSrc->releaseOscB;
    pDest->patch.filterOscB     = pSrc->filterOscB;
    pDest->patch.detuneOscB     = pSrc->detuneOscB;

    pDest->patch.waveOscC       = pSrc->waveOscC;
    pDest->patch.pulseWidthOscC = pSrc->pulseWidthOscC;
    pDest->patch.attackOscC     = pSrc->attackOscC;
    pDest->patch.decayOscC      = pSrc->decayOscC;
    pDest->patch.sustainOscC    = pSrc->sustainOscC;
    pDest->patch.releaseOscC    = pSrc->releaseOscC;
    pDest->patch.filterOscC     = pSrc->filterOscC;
    pDest->patch.detuneOscC     = pSrc->detuneOscC;
 
    pDest->patch.cutoff    = pSrc->cutoff;
    pDest->patch.resonance = pSrc->resonance;
    pDest->patch.mode      = pSrc->mode;

    pDest->patch.volume = pSrc->volume;

    pDest->patch.id = pSrc->id;
    setString(pSrc->name, pDest->patch.name, PATCHNAME_LEN);
    patchToRegisters(pDest);
    return true;
}

//void displayRegisters(patch *p) {
//    patchToRegisters(p);
//
//    int d = 2500;
//    lcd.clear();
//    for (int i = 0; i < 25; i++) {
//        if (i != 0 && i % 7 == 0) {
//            delay(d);
//            lcd.clear();
//        }
//        if (p->registers[i] < 16)
//            lcd.print(0);
//        lcd.print(p->registers[i], HEX);
//        lcd.print(' ');
//        if (i % 7 == 4)
//            lcd.setCursor(0,1);
//    }
//    delay(d);
//}
