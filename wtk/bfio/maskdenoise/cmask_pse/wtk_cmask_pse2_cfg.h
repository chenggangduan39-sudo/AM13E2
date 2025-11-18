#ifndef WTK_BFIO_MASKDENOISE_CMASK_PSE2_WTK_CMASK_PSE2_CFG_H
#define WTK_BFIO_MASKDENOISE_CMASK_PSE2_WTK_CMASK_PSE2_CFG_H
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_fbank.h"
#include "wtk/bfio/qform/wtk_mask_bf.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
// #define ONNX_DEC
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_pse2_cfg wtk_cmask_pse2_cfg_t;

struct wtk_cmask_pse2_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int channel;
    int *mic_channel;
    int nmicchannel;
    int *sp_channel;
    int nspchannel;
    int nbfchannel;

    int wins;
    int rate;

    int emb_len;
    int gb_len;
    int *emb_feat_len;
    int nemb_feat;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t emb;
#endif
    int num_frame;
    wtk_fbank_cfg_t fbank;
    qtk_nnrt_cfg_t pse_rt;

    wtk_mask_bf_cfg_t mask_bf;

    int clip_s;
    int clip_e;

    float sym;
    int cnon_clip_s;
    int cnon_clip_e;

    float spenr_thresh;
    int spenr_cnt;
    float micenr_thresh;
    int micenr_cnt;

    float max_bs_out;

    unsigned use_onnx : 1;
    unsigned use_cnon : 1;
    unsigned use_bf : 1;
};

int wtk_cmask_pse2_cfg_init(wtk_cmask_pse2_cfg_t *cfg);
int wtk_cmask_pse2_cfg_clean(wtk_cmask_pse2_cfg_t *cfg);
int wtk_cmask_pse2_cfg_update(wtk_cmask_pse2_cfg_t *cfg);
int wtk_cmask_pse2_cfg_update2(wtk_cmask_pse2_cfg_t *cfg,
                               wtk_source_loader_t *sl);
int wtk_cmask_pse2_cfg_update_local(wtk_cmask_pse2_cfg_t *cfg,
                                    wtk_local_cfg_t *lc);

wtk_cmask_pse2_cfg_t *wtk_cmask_pse2_cfg_new(char *fn);
void wtk_cmask_pse2_cfg_delete(wtk_cmask_pse2_cfg_t *cfg);
wtk_cmask_pse2_cfg_t *wtk_cmask_pse2_cfg_new_bin(char *fn);
void wtk_cmask_pse2_cfg_delete_bin(wtk_cmask_pse2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
