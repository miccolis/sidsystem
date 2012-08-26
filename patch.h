/*
 * A patch.
 *
 * TODO use more accurate type declarations.
 */
struct patch {
    // Oscillators
    int waveOscA;
    int attackOscA;
    int decayOscA;
    int sustainOscA;
    int releaseOscA;

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
    int cutoff;
    int resonance;
    int bypass;
    int mode;

    // General
    int volume;

    // System
    int id;
    char name[8];
};




