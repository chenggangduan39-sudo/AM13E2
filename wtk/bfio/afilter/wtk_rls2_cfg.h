#ifndef WTK_BFIO_AFILTER_WTK_RLS2_CFG
#define WTK_BFIO_AFILTER_WTK_RLS2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rls2_cfg wtk_rls2_cfg_t;
struct wtk_rls2_cfg
{
    int L;
    int N;
    int nl;

    int channel;

    float lemma;
    float sigma;
    float p;
    float w_alpha;
    
	int Td;
    int T60;
	int nd;
    float delta;
    int rate;
    int step;
    int iters;

    float px;
    float Q_eye;

    float Q_eye_alpha;
    float min_lemma;
    float max_lemma;

    unsigned use_wx:1;
    unsigned use_admm:1;
    unsigned use_zvar:1;
};

int wtk_rls2_cfg_init(wtk_rls2_cfg_t *cfg);
int wtk_rls2_cfg_clean(wtk_rls2_cfg_t *cfg);
int wtk_rls2_cfg_update_local(wtk_rls2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_rls2_cfg_update(wtk_rls2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif