#ifndef WTK_BFIO_AGC_WTK_AGC_CFG
#define WTK_BFIO_AGC_WTK_AGC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_agc_cfg wtk_agc_cfg_t;
struct wtk_agc_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    
    wtk_qmmse_cfg_t qmmse;

    float micenr_thresh;
    int micenr_cnt;

	int channel;
	int *mic_channel;
	int nmicchannel;

    int clip_s;
    int clip_e;

	wtk_equalizer_cfg_t eq;

    float max_out;

    unsigned use_eq:1;
    unsigned use_qmmse:1;
    unsigned use_bs_win:1;
};

int wtk_agc_cfg_init(wtk_agc_cfg_t *cfg);
int wtk_agc_cfg_clean(wtk_agc_cfg_t *cfg);
int wtk_agc_cfg_update_local(wtk_agc_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_agc_cfg_update(wtk_agc_cfg_t *cfg);
int wtk_agc_cfg_update2(wtk_agc_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_agc_cfg_t* wtk_agc_cfg_new(char *fn);
void wtk_agc_cfg_delete(wtk_agc_cfg_t *cfg);
wtk_agc_cfg_t* wtk_agc_cfg_new_bin(char *fn);
void wtk_agc_cfg_delete_bin(wtk_agc_cfg_t *cfg);

wtk_agc_cfg_t* wtk_agc_cfg_new2(char *fn, char *fn2);
void wtk_agc_cfg_delete2(wtk_agc_cfg_t *cfg);
wtk_agc_cfg_t* wtk_agc_cfg_new_bin2(char *fn, char *fn2);
void wtk_agc_cfg_delete_bin2(wtk_agc_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
