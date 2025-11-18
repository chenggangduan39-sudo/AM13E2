#ifndef WTK_BFIO_PREEMPH_WTK_PREEMPH
#define WTK_BFIO_PREEMPH_WTK_PREEMPH
#ifdef __cplusplus
extern "C" {
#endif

void wtk_preemph_dc(float *mic,float *mem,int len);
float wtk_preemph_asis(float *mic,int len,float memD);
float wtk_preemph_asis2(float *mic,int len,float memX);

#ifdef __cplusplus
};
#endif
#endif