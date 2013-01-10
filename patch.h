/*
 * A patch.
 *
 * TODO use more accurate type declarations.
 */

#define PATCHNAME_LEN 8 // Max length of patch names.

struct patch {
    // Oscillators
    int waveOscA;    // 0 - 6
    int attackOscA;  // nibble
    int decayOscA;   // nibble
    int sustainOscA; // nibble
    int releaseOscA; // nibble

    int waveOscB;
    int attackOscB;
    int decayOscB;
    int sustainOscB;
    int releaseOscB;

    int waveOscC;
    int attackOscC;
    int decayOscC;
    int sustainOscC;
    int releaseOscC;

    // Filter
    int cutoff;     // 11-bit
    int resonance;  // nibble
    int bypass;     // unused
    int mode;       // 0 - 4

    // General
    int volume;     // nibble

    // System
    int id;
    char name[8];
};

// Identical to Patch, but for inclusion of registers.
struct livePatch {
    uint8_t registers[25];

    // Oscillators
    int waveOscA;    // 0 - 6
    int attackOscA;  // nibble
    int decayOscA;   // nibble
    int sustainOscA; // nibble
    int releaseOscA; // nibble

    int waveOscB;
    int attackOscB;
    int decayOscB;
    int sustainOscB;
    int releaseOscB;

    int waveOscC;
    int attackOscC;
    int decayOscC;
    int sustainOscC;
    int releaseOscC;

    // Filter
    int cutoff;     // 11-bit
    int resonance;  // nibble
    int bypass;     // unused
    int mode;       // 0 - 4

    // General
    int volume;     // nibble

    // System
    int id;
    char name[8];
};

int *loadPatchValuePtr(int param, livePatch *p) {
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

int loadPatchValue(int param, livePatch *p) {
    int *v = loadPatchValuePtr(param, p);
    return *v;
}

void setPatchValue(int param, livePatch *p, int val) {
    int *v = loadPatchValuePtr(param, p);
    *v = val;
}

void patchToRegisters(livePatch *p) {

     // 0/1: Osc A: frequency (midi)
     // Osc A: pulse width (setting to 2048 for now)
     p->registers[2] = 0;
     p->registers[3] = 8;
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
         case 0: p->registers[4] = 0x10; break;
         case 1: p->registers[4] = 0x20; break;
         case 2: p->registers[4] = 0x40; break;
         case 3: p->registers[4] = 0x4; break;
         case 4: p->registers[4] = 0x2; break;
         case 5: p->registers[4] = 0x80; break;
     }
     // Osc A: Attack, Decay
     p->registers[5] = (p->attackOscA << 4) + p->decayOscA;
     // Osc A: Sustain, Release
     p->registers[6] = (p->sustainOscA << 4) + p->releaseOscA;

     // 7/8: Osc B: frequency (midi)
     // Osc B: pulse width (setting to 2048 for now)
     p->registers[9] = 0;
     p->registers[10] = 8;

     // Osc B: Control Register
     switch (p->waveOscB) {
         case 0: p->registers[11] = 0x10; break;
         case 1: p->registers[11] = 0x20; break;
         case 2: p->registers[11] = 0x40; break;
         case 3: p->registers[11] = 0x4; break;
         case 4: p->registers[11] = 0x2; break;
         case 5: p->registers[11] = 0x80; break;
     }
     // Osc B: Attack, Decay
     p->registers[12] = (p->attackOscB << 4) + p->decayOscB;
     // Osc B: Sustain, Release
     p->registers[13] = (p->sustainOscB << 4) + p->releaseOscB;

     // 14/15: Osc C: frequency (midi)
     // Osc C: pulse width (setting to 2048 for now)
     p->registers[16] = 0;
     p->registers[17] = 8;

     // Osc C: Control Register
     switch (p->waveOscC) {
         case 0: p->registers[18] = 0x10; break;
         case 1: p->registers[18] = 0x20; break;
         case 2: p->registers[18] = 0x40; break;
         case 3: p->registers[18] = 0x4; break;
         case 4: p->registers[18] = 0x2; break;
         case 5: p->registers[18] = 0x80; break;
     }
     // Osc C: Attack, Decay
     p->registers[19] = (p->attackOscC << 4) + p->decayOscC;
     // Osc C: Sustain, Release
     p->registers[20] = (p->sustainOscC << 4) + p->releaseOscC;

     // Filter frequency (11 bits)
     int ffreq = p->cutoff;
     p->registers[21] = ffreq & 0x7; // 0000 0111
     p->registers[22] = ffreq >> 3;

     // Resonance, Routing
     p->registers[23] = p->resonance << 4;
     p->registers[23] |= 0x7; // 0111

     // Mode / Vol
     // 0001 0000 (0x10) - lowpass
     // 0010 0000 (0x40) - bandpass
     // 0100 0000 (0x20) - highpass
     // 0101 0000 (0x50) - notch
     switch (p->mode) {
         case 0: p->registers[24] = 0x10; break;
         case 1: p->registers[24] = 0x40; break;
         case 2: p->registers[24] = 0x20; break;
         case 3: p->registers[24] = 0x50; break;
     }
}

bool copyPatch(patch *pSrc, livePatch *pDest) {
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
