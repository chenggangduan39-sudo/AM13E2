#ifndef WTK_BFIO_VBOX_WTK_CONSIST_CFG
#define WTK_BFIO_VBOX_WTK_CONSIST_CFG
#include "wtk/core/fft/wtk_stft.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/fft/wtk_stft.h"
// #include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_consist_cfg wtk_consist_cfg_t;
struct wtk_consist_cfg
{
    // wtk_ssl2_cfg_t ssl;

    void *hook;
    char *playfn;

    float eq_offset;
    float nil_er;
    float mic_corr_er;
    float mic_corr_aver;
    float mic_energy_er;
    float mic_energy_aver;
    float mic_energy_min;

    float spk_corr_er;
    float spk_corr_aver;
    float spk_energy_er;
    float spk_energy_aver;

    int channel;
    int spchannel;
    unsigned int use_xcorr:1;
    unsigned int use_equal:1;
};

int wtk_consist_cfg_init(wtk_consist_cfg_t *cfg);
int wtk_consist_cfg_clean(wtk_consist_cfg_t *cfg);
int wtk_consist_cfg_update_local(wtk_consist_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_consist_cfg_update(wtk_consist_cfg_t *cfg);
int wtk_consist_cfg_update2(wtk_consist_cfg_t *cfg, wtk_source_loader_t *lc);

wtk_consist_cfg_t* wtk_consist_cfg_new(char *cfg_fn);
void wtk_consist_cfg_delete(wtk_consist_cfg_t *cfg);
wtk_consist_cfg_t* wtk_consist_cfg_new_bin(char *bin_fn);
void wtk_consist_cfg_delete_bin(wtk_consist_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif

