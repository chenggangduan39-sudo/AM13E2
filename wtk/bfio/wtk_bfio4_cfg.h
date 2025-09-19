#ifndef WTK_BFIO_WTK_BFIO4_CFG
#define WTK_BFIO_WTK_BFIO4_CFG
#include "wtk/core/cfg/wtk_main_cfg.h" 
#include "wtk/core/cfg/wtk_mbin_cfg.h" 
// #include "wtk/bfio/wtk_kvadwake.h" 
#include "wtk/bfio/maskdenoise/wtk_gainnet_denoise.h"
#include "wtk/asr/img/qtk_img_cfg.h"
#include "wtk/asr/img/qtk_img2_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct bfio4_img_thresh_cfg
{
    float av_prob0;
    float av_prob1;
    float max_prob0;
    float max_prob1;
    float avx0;
    float avx1;
    float maxx0;
    float maxx1;
}bfio4_img_thresh_cfg_t;

typedef struct wtk_bfio4_cfg wtk_bfio4_cfg_t;
struct wtk_bfio4_cfg
{
    wtk_gainnet_denoise_cfg_t denoise;

    char *img_fn;
    qtk_img_cfg_t *img;
    qtk_img_thresh_cfg_t img_denoise;


    char *img2_fn;
    qtk_img2_cfg_t *img2;
    bfio4_img_thresh_cfg_t img2_denoise;

    void *hook;
	int min_pvlen;

    unsigned use_rbin_res:1;
    unsigned debug:1;
    unsigned use_wake:1;
    unsigned use_asr:1;
    unsigned use_denoise_wake:1;
    unsigned use_denoise_asr:1;
    unsigned use_thread:1;
};

int wtk_bfio4_cfg_init(wtk_bfio4_cfg_t *cfg);
int wtk_bfio4_cfg_clean(wtk_bfio4_cfg_t *cfg);
int wtk_bfio4_cfg_update_local(wtk_bfio4_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_bfio4_cfg_update(wtk_bfio4_cfg_t *cfg);
int wtk_bfio4_cfg_update2(wtk_bfio4_cfg_t *cfg, wtk_source_loader_t *sl);

// void wtk_bfio4_cfg_set_wakeword(wtk_bfio4_cfg_t *bfio4, char *wrd);

wtk_bfio4_cfg_t* wtk_bfio4_cfg_new(char *cfg_fn);
void wtk_bfio4_cfg_delete(wtk_bfio4_cfg_t *cfg);
wtk_bfio4_cfg_t* wtk_bfio4_cfg_new_bin(char *bin_fn);
void wtk_bfio4_cfg_delete_bin(wtk_bfio4_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
