#ifndef WTK_MODEL_WTK_FKIND_H_
#define WTK_MODEL_WTK_FKIND_H_
#include "wtk/core/wtk_str.h"
#ifdef __cplusplus
extern "C" {
#endif
//typedef struct wtk_parmkind wtk_parmkind_t;
typedef unsigned short wtk_fkind_t;
typedef enum {
      WAVEFORM,            /* Raw speech waveform (handled by HWave) */
      LPC,LPREFC,LPCEPSTRA,LPDELCEP,   /* LP-based Coefficients */
      IREFC,                           /* Ref Coef in 16 bit form */
      MFCC,                            /* Mel-Freq Cepstra */
      FBANK,                           /* Log Filter Bank */
      MELSPEC,                         /* Mel-Freq Spectrum (Linear) */
      USER,                            /* Arbitrary user specified data */
      DISCRETE,                        /* Discrete VQ symbols (shorts) */
      PLP,                             /* Standard PLP coefficients */
      ANON
}wtk_bfkind_t;
#define HASENERGY  0100       /* _E log energy included */
#define HASNULLE   0200       /* _N absolute energy suppressed */
#define HASDELTA   0400       /* _D delta coef appended */
#define HASACCS   01000       /* _A acceleration coefs appended */
#define HASCOMPX  02000       /* _C is compressed */
#define HASZEROM  04000       /* _Z zero meaned */
#define HASCRCC  010000       /* _K has CRC check */
#define HASZEROC 020000       /* _0 0'th Cepstra included */
#define HASVQ    040000       /* _V has VQ index attached */
#define HASTHIRD 0100000       /* _T has Delta-Delta-Delta index attached */

#define BASEMASK  077         /* Mask to remove qualifiers */

int wtk_fkind_from_string(wtk_fkind_t* p,char* d,int dl);
char *wtk_fkind_to_str(wtk_fkind_t kind, char *buf);
#ifdef __cplusplus
};
#endif
#endif
