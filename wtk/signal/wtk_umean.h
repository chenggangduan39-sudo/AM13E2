#ifndef WTK_SIGNAL_WTK_UMEAN
#define WTK_SIGNAL_WTK_UMEAN
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_umean wtk_umean_t;
struct wtk_umean
{
	wtk_robin_t *bak;
	wtk_robin_t *rb;
	wtk_complex_t *sum;
	wtk_complex_t *mean;
	int len;
};


wtk_umean_t* wtk_umean_new(int bak,int n,int len);
void wtk_umean_delete(wtk_umean_t *m);
void wtk_umean_reset(wtk_umean_t *m);
void wtk_umean_feed(wtk_umean_t *m,wtk_complex_t *v);
wtk_complex_t* wtk_umean_get_mean(wtk_umean_t *m);

void wtk_umean_feedf(wtk_umean_t *m,float *v,int nx);
float* wtk_umean_get_meanf(wtk_umean_t *m);
#ifdef __cplusplus
};
#endif
#endif
