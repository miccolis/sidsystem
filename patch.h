/*
 * A patch.
 *
 * TODO use more accurate type declarations.
 */
struct patch {
    // Oscillators
    int waveOscA;
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
    int resonance;
    int bypass;
    int mode;

    // General
    int volume;     // nibble

    // System
    int id;
    char name[8];
};
