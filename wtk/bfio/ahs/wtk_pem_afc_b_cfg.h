#ifndef WTK_BFIO_AHS_WTK_PEM_AFC_B_CFG
#define WTK_BFIO_AHS_WTK_PEM_AFC_B_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "estimate/qtk_rir_estimate.h"
// #define ONNX_DEC
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif
#include "qtk/nnrt/qtk_nnrt_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_pem_afc_b_cfg wtk_pem_afc_b_cfg_t;

struct wtk_pem_afc_b_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    qtk_rir_estimate_cfg_t r_est;

	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;

    int rate;
    int wins;
    int hop_size;
    float DAC_delay;
    int Nframe_DAC_delay;
    int N_afc;
    int N_ar;
    int N_block;

    float mu1;
    float mu2;
    float delta;

    float power_alpha;

    float in_scale;
    float out_scale;

    int clip_s;
    int clip_e;
	wtk_equalizer_cfg_t eq;

    unsigned int use_filter:1;
    unsigned int use_onnx:1;
    unsigned int use_ncnn:1;
    unsigned use_eq:1;

    unsigned use_refcompensation:1;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t onnx;
#endif

    float compress_factor;

    qtk_nnrt_cfg_t rt;
    int num_in;
    int num_out;
};

int wtk_pem_afc_b_cfg_init(wtk_pem_afc_b_cfg_t *cfg);
int wtk_pem_afc_b_cfg_clean(wtk_pem_afc_b_cfg_t *cfg);
int wtk_pem_afc_b_cfg_update_local(wtk_pem_afc_b_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_pem_afc_b_cfg_update(wtk_pem_afc_b_cfg_t *cfg);
int wtk_pem_afc_b_cfg_update2(wtk_pem_afc_b_cfg_t *cfg, wtk_source_loader_t *sl);

wtk_pem_afc_b_cfg_t* wtk_pem_afc_b_cfg_new(char *fn);
void wtk_pem_afc_b_cfg_delete(wtk_pem_afc_b_cfg_t *cfg);
wtk_pem_afc_b_cfg_t* wtk_pem_afc_b_cfg_new_bin(char *fn);
void wtk_pem_afc_b_cfg_delete_bin(wtk_pem_afc_b_cfg_t *cfg);
void wtk_pem_afc_b_cfg_set(wtk_pem_afc_b_cfg_t *cfg, float out_scale,float delay);
#ifdef __cplusplus
};
#endif
#endif