#ifndef WTK_MER_FIX_H
#define WTK_MER_FIX_H
#ifdef __cplusplus
extern "C" {
#endif

#ifdef USE_NEON
    #include "wtk/asr/fextra/fnn/qlas/wtk_qlasasm.h"
#endif

#if defined USE_NEON || defined(USE_MAT_FIX)
    #define FIX_TYPE short
#endif
#define FIX (1<<10)
#define FTOI(x) ((FIX_TYPE)((x)*FIX))
#define ITOF(x) ((float)((x)*1.0/FIX))

#ifdef __cplusplus
}
#endif
#endif