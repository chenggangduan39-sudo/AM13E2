#ifndef WTK_BFIO_QFORM_BEAMNET_WTK_BEAMNET_CFG_H
#define WTK_BFIO_QFORM_BEAMNET_WTK_BEAMNET_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_beamnet_cfg wtk_beamnet_cfg_t;

struct wtk_beamnet_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    int nmic;
    float **mic_pos;
    int feature_len;
    float sv;
    int feature_type;

	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;
    int out_channels;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t seperator;
    qtk_onnxruntime_cfg_t beamformer;
#endif

    unsigned int use_onnx:1;
};

int wtk_beamnet_cfg_init(wtk_beamnet_cfg_t *cfg);
int wtk_beamnet_cfg_clean(wtk_beamnet_cfg_t *cfg);
int wtk_beamnet_cfg_update(wtk_beamnet_cfg_t *cfg);
int wtk_beamnet_cfg_update2(wtk_beamnet_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_beamnet_cfg_update_local(wtk_beamnet_cfg_t *cfg, wtk_local_cfg_t *lc);

wtk_beamnet_cfg_t* wtk_beamnet_cfg_new(char *fn);
void wtk_beamnet_cfg_delete(wtk_beamnet_cfg_t *cfg);
wtk_beamnet_cfg_t* wtk_beamnet_cfg_new_bin(char *fn);
void wtk_beamnet_cfg_delete_bin(wtk_beamnet_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
