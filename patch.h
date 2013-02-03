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
    int pulseWidthOscA; // 12-bit
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
    int note;
};

// Returns two bytes, one register value in each.
uint16_t patchParamRegister(int param);

void patchToRegisters(livePatch *p);

int loadPatchValue(int param, livePatch *p);

// Update the setting, and the register value.
void setPatchValue(livePatch *p, int param, int val);

bool copyPatch(patchSettings *pSrc, livePatch *pDest);
