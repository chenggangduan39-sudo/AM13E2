#ifndef WTK_BFIO_QFORM_WTK_MIX_SPEECH_CFG
#define WTK_BFIO_QFORM_WTK_MIX_SPEECH_CFG
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum
{
    WTK_MIX_SPEECH_NORM_MIX,
}wtk_mix_speech_type_t;

typedef struct wtk_mix_speech_cfg wtk_mix_speech_cfg_t;
struct wtk_mix_speech_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;

    int max_1;
    int min_1;
    int max_2;
    int min_2;

    int f_win;
    
	int *mic_channel;
	int nmicchannel;
	int channel;
    float *mic_scale;
    float nmic_scale;
    int scale_cnt;
    float *scale_step;
    float micenr_thresh;
    int micenr_cnt;

    wtk_mix_speech_type_t mix_type;
};

int wtk_mix_speech_cfg_init(wtk_mix_speech_cfg_t *cfg);
int wtk_mix_speech_cfg_clean(wtk_mix_speech_cfg_t *cfg);
int wtk_mix_speech_cfg_update_local(wtk_mix_speech_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_mix_speech_cfg_update(wtk_mix_speech_cfg_t *cfg);
int wtk_mix_speech_cfg_update2(wtk_mix_speech_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_mix_speech_cfg_t* wtk_mix_speech_cfg_new(char *fn);
void wtk_mix_speech_cfg_delete(wtk_mix_speech_cfg_t *cfg);
wtk_mix_speech_cfg_t* wtk_mix_speech_cfg_new_bin(char *fn);
void wtk_mix_speech_cfg_delete_bin(wtk_mix_speech_cfg_t *cfg);

void wtk_mix_speech_cfg_set_channel(wtk_mix_speech_cfg_t *cfg, int channel, float main_scale);
#ifdef __cplusplus
};
#endif
#endif
