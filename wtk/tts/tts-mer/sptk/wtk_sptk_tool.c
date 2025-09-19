#include "tts-mer/sptk/SPTK.h"
#include "wtk_sptk_tool.h"

double* wtk_sptk_getsintbl(const int l)
{
    int j;
    int tblsize = l - l / 4 + 1;
    double arg = PI / l * 2;
    double *sintbl;
    sintbl = dgetmem(tblsize);
    sintbl[0] = 0;
    for (j = 1; j < tblsize; j++)
        sintbl[j] = sin(arg * (double) j);
    sintbl[l / 2] = 0;
    return sintbl;
}
