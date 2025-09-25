#include "wtk_cfg.h"

void wtk_cfg_macro_check()
{
#if defined(USE_MKL) || defined(USE_GO2BLAS)
    wtk_cfg_macro_check_printf(USE_BLAS, 1);
    #ifdef USE_MAT_FIX
        wtk_exit_debug("\nUSE_BLAS==1 && USE_MAT_FIX==1 不能同时开启\n");
    #endif

    #ifdef USE_MKL
        wtk_cfg_macro_check_printf(USE_MKL, 1);

        #ifdef USE_MKL_PACK
            wtk_cfg_macro_check_printf(USE_MKL_PACK, 1);
        #else
            wtk_cfg_macro_check_printf(USE_MKL_PACK, 0);
        #endif

    #elif defined USE_GO2BLAS
        wtk_cfg_macro_check_printf(USE_GO2BLAS, 1);
    #endif
#else
    wtk_cfg_macro_check_printf(USE_BLAS, 0);
#endif

#ifdef __AVX__
    wtk_cfg_macro_check_printf(USE_AVX, 1);
#elif defined(__SSE__)
    wtk_cfg_macro_check_printf(USE_SSE, 1);
#endif

#ifdef _OPENMP
    wtk_cfg_macro_check_printf(USE_OPENMP, 1);
#else
    wtk_cfg_macro_check_printf(USE_OPENMP, 0);
#endif

#if defined(USE_MAT_FIX) && defined(USE_NEON)
    wtk_exit_debug("\nUSE_NEON==1 && USE_MAT_FIX==1 不能同时开启\n");
#endif

wtk_cfg_macro_check_printf(USE_MAT_FIX, 
    #ifdef USE_MAT_FIX
    1
    #else
    0
    #endif
    );

wtk_cfg_macro_check_printf(IS_ANDROID, 
    #ifdef __ANDROID__
    1
    #else
    0
    #endif
    );

#ifdef USE_NEON
    wtk_cfg_macro_check_printf(USE_NEON, 1);
#endif

#ifndef USE_DEV
    wtk_cfg_macro_check_printf(USE_DEV, 0);
#else
    wtk_cfg_macro_check_printf(USE_DEV, 1);
#endif

#ifndef USE_DEBUG
    wtk_cfg_macro_check_printf(USE_DEBUG, 0);
#else
    wtk_cfg_macro_check_printf(USE_DEBUG, 1);
#endif
}