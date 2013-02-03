#include "param.h"
#include "utils.h"

bool copyParam(param *pSrc, param *pDest) {
    pDest->type = pSrc->type;
    pDest->id = pSrc->id;
    setString(pSrc->name, pDest->name, PARAMNAME_LEN);
    return true;
}

int paramLimit(param *p) {
    return (p->type & PARAM_LMASK) - 1;
}
