/*
 * A parameter
 */

#define PARAMNAME_LEN 8 // Max length of param names.

#define PARAM_UNAVAIL 0
#define PARAM_LABEL 0x8000 // MSB is a flag for human readable labels
#define PARAM_LMASK 0x7FFF // Label mask.
#define PARAM_1BIT 2
#define PARAM_4BIT 16
#define PARAM_11BIT 2048
#define PARAM_12BIT 4096
#define PARAM_DETUNE 240 // two octaves in 10 cent steps.

struct param {
    uint16_t type;
    int id;
    char name[8];
};

bool copyParam(param *pSrc, param *pDest) {
    pDest->type = pSrc->type;
    pDest->id = pSrc->id;
    setString(pSrc->name, pDest->name, PARAMNAME_LEN);
    return true;
}

int paramLimit(param *p) {
    return (p->type & PARAM_LMASK) - 1;
}
