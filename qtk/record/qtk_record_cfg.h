#ifndef __QTK_RECORD_CFG_H__
#define __QTK_RECORD_CFG_H__
#ifdef __cplusplus
extern "C"{
#endif
#include "wtk/core/cfg/wtk_local_cfg.h"
typedef struct qtk_record_cfg{
    char *snd_name;
    int channel;
    int sample_rate;
	int bytes_per_sample;
    int buf_time;
    int *skip_channels;
	int nskip;
    int mic_gain;
    int cb_gain;
    unsigned int use_gain_set:1;
    unsigned int use_log_ori_audio:1;
    unsigned int use_get_card:1;
}qtk_record_cfg_t;


qtk_record_cfg_t* qtk_record_cfg_new();
void qtk_record_cfg_delete(qtk_record_cfg_t *cfg);
int qtk_record_cfg_init(qtk_record_cfg_t *cfg);
int qtk_record_cfg_clean(qtk_record_cfg_t *cfg);
int qtk_record_cfg_update_local(qtk_record_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_record_cfg_update(qtk_record_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
