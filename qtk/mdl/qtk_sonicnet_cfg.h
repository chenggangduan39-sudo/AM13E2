#ifndef BCFD56E0_41C4_4D82_86E8_926BFDD89244
#define BCFD56E0_41C4_4D82_86E8_926BFDD89244

#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_sonicnet_cfg qtk_sonicnet_cfg_t;

struct qtk_sonicnet_cfg {
    qtk_nnrt_cfg_t rt;
    qtk_nnrt_cfg_t vad_rt;
    int left_context;
    int right_context;
    int nzc;
    int *chunk_sz;
    int nchunk_sz;
    float act_hint_thresh;
    float dis_start;
    float dis_end;
    int channel;
    int vad_chunk_sz;
    float vad_chunk_overlap;
    int K;
    int D;
    float dynamic_scaler;
    unsigned vad_use_thread : 1;
    unsigned use_ifft : 1;
};

int qtk_sonicnet_cfg_init(qtk_sonicnet_cfg_t *cfg);
int qtk_sonicnet_cfg_clean(qtk_sonicnet_cfg_t *cfg);
int qtk_sonicnet_cfg_update(qtk_sonicnet_cfg_t *cfg);
int qtk_sonicnet_cfg_update_local(qtk_sonicnet_cfg_t *cfg, wtk_local_cfg_t *lc);

#endif /* BCFD56E0_41C4_4D82_86E8_926BFDD89244 */
