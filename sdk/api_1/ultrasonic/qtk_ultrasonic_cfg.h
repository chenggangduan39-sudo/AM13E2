#ifndef __QTK_ULTRASONIC_CFG_H__
#define __QTK_ULTRASONIC_CFG_H__

#include "qtk/ult/evm2/qtk_ultevm2_cfg.h"
#include "qtk/ult/qtk_ult_track_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_ultrasonic_cfg{
	wtk_string_t cfg_fn;
	qtk_ultevm2_cfg_t *ultevm_cfg;
	qtk_ult_track_cfg_t *ult_track_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
	void *ult_cfg;
	int *skip_channels;
	float mic_shift;
	float echo_shift;
	int nskip;
	int mics;

	char *input_fn;
	char *out_fn;

	unsigned int use_log_wav:1;
	unsigned int use_bin:1;
	unsigned int use_manual:1;
	unsigned int use_ultevm:1;
	unsigned int use_ult_track:1;
}qtk_ultrasonic_cfg_t;

int qtk_ultrasonic_cfg_init(qtk_ultrasonic_cfg_t *cfg);
int qtk_ultrasonic_cfg_clean(qtk_ultrasonic_cfg_t *cfg);
int qtk_ultrasonic_cfg_update_local(qtk_ultrasonic_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_ultrasonic_cfg_update(qtk_ultrasonic_cfg_t *cfg);
int qtk_ultrasonic_cfg_update2(qtk_ultrasonic_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_ultrasonic_cfg_t *qtk_ultrasonic_cfg_new(char *fn);
void qtk_ultrasonic_cfg_delete(qtk_ultrasonic_cfg_t *cfg);
qtk_ultrasonic_cfg_t *qtk_ultrasonic_cfg_new_bin(char *bin_fn);
void qtk_ultrasonic_cfg_delete_bin(qtk_ultrasonic_cfg_t *cfg);
#ifdef __cplusplus
};
#endif



#endif