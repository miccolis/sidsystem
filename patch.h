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

int loadPatchValue(int param, patch *p) {
    int *v = loadPatchValuePtr(param, p);
    return *v;
}

void setPatchValue(int param, patch *p, int val) {
    int *v = loadPatchValuePtr(param, p);
    *v = val;
}

bool copyPatch(patch *pSrc, patch *pDest) {
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

