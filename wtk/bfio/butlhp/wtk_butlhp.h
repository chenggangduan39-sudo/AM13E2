#ifndef WTK_BFIO_BUTLHP_WTK_BUTLHP
#define WTK_BFIO_BUTLHP_WTK_BUTLHP
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef struct  wtk_butlp{
    int rate;
    float lkf;
    float a[6];
    float x[2];
    float pidsr;
}wtk_butlhp_t;

wtk_butlhp_t *wtk_butlhp_new(int fc, int rate, int is_hp);
int wtk_butlhp_feed(wtk_butlhp_t *butlhp,short *in, int len);
int wtk_butlhp_feed_float(wtk_butlhp_t *butlhp,float *in, int len);
int wtk_butlhp_delete(wtk_butlhp_t *butlhp);

#ifdef __cplusplus
};
#endif

#endif