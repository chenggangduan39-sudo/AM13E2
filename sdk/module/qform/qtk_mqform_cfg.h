#ifndef QTK_MODULE_BFIO_QTK_MQFORM_CFG
#define QTK_MODULE_BFIO_QTK_MQFORM_CFG

#include "wtk/core/cfg/wtk_main_cfg.h"
#include "sdk/module/qtk_module_tool.h"
#include "sdk/audio/recorder/qtk_recorder_cfg.h"
#include "sdk/audio/player/qtk_player_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_mqform_cfg qtk_mqform_cfg_t;
struct qtk_mqform_cfg
{
	char *sqform;
	qtk_recorder_cfg_t recorder_cfg;
	qtk_player_cfg_t player_cfg;

	wtk_strbuf_t *swakeup_buf;
	float echo_shift;
	int resample_rate;
	int out_channel;
	int max_output_length;
	unsigned use_dsp:1;
	unsigned use_recorder:1;
	unsigned use_player:1;
	unsigned use_sample:1;
	unsigned use_log_wav:1;
	unsigned debug:1;
};

int qtk_mqform_cfg_init(qtk_mqform_cfg_t *cfg);
int qtk_mqform_cfg_clean(qtk_mqform_cfg_t *cfg);
int qtk_mqform_cfg_update_local(qtk_mqform_cfg_t *cfg,wtk_local_cfg_t *main);
int qtk_mqform_cfg_update(qtk_mqform_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
