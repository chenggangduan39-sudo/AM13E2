#ifndef WTK_BFIO_QFORM_WTK_RTJOIN_CFG
#define WTK_BFIO_QFORM_WTK_RTJOIN_CFG
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rtjoin_cfg wtk_rtjoin_cfg_t;
struct wtk_rtjoin_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    
    int M;
    int M2;
    float power_alpha;
    float mufb;

    float power_alpha2;
    float power_alpha3;
    
	int channel;
    int clip_s;
    int clip_e;

    float power_thresh;

    int chioce_thresh;
    int change_frame_len;
    int change_frame_num;
    int change_frame_num2;
    int change_frame_delay;
    int init_change_frame;
    float change_thresh;
    float change_thresh2;
    float mix_scale;

    float csd_thresh;
    float csd_in_cnt;
    float csd_out_cnt;

    float mean_nchenr_thresh;
    int mean_nchenr_cnt;
    float sil_power_alpha2;
    float sound_power_alpha2;
    int nchenr_cnt;
    int change_delay;
    int nlms_change_init;
    float nlms_align_thresh;
    float nlms_align_thresh2;
    int nlms_change_cnt;
    float nlms_mix_thresh;

	wtk_equalizer_cfg_t eq;

    unsigned use_eq:1;
    unsigned use_choicech:1;
    unsigned use_control_bs:1;
    unsigned use_csd:1;
};

int wtk_rtjoin_cfg_init(wtk_rtjoin_cfg_t *cfg);
int wtk_rtjoin_cfg_clean(wtk_rtjoin_cfg_t *cfg);
int wtk_rtjoin_cfg_update_local(wtk_rtjoin_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_rtjoin_cfg_update(wtk_rtjoin_cfg_t *cfg);
int wtk_rtjoin_cfg_update2(wtk_rtjoin_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_rtjoin_cfg_t* wtk_rtjoin_cfg_new(char *fn);
void wtk_rtjoin_cfg_delete(wtk_rtjoin_cfg_t *cfg);
wtk_rtjoin_cfg_t* wtk_rtjoin_cfg_new_bin(char *fn);
void wtk_rtjoin_cfg_delete_bin(wtk_rtjoin_cfg_t *cfg);

void wtk_rtjoin_cfg_set_channel(wtk_rtjoin_cfg_t *cfg, int channel);
#ifdef __cplusplus
};
#endif
#endif
