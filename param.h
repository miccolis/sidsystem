/*
 * A parameter
 */

#define PARAMNAME_LEN 8 // Max length of param names.

#define PARAM_UNAVAIL 0
#define PARAM_LABEL 1   // First bit is a flag for human readable labels
#define PARAM_4BIT 32    // 16 << 1
#define PARAM_11BIT 4096  // 2048 << 1

struct param {
    int type;
    int id;
    char name[8];
};

bool copyParam(param *pSrc, param *pDest) {
    pDest->type = pSrc->type;
    pDest->id = pSrc->id;
    setString(pSrc->name, pDest->name, PARAMNAME_LEN);
    return true;
}

