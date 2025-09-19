#ifndef QTK__WAKEUP_DECODER_CFG_H_
#define QTK__WAKEUP_DECODER_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/wfst/kwdec/qtk_kwdec_cfg.h"
#include "wtk/asr/fextra/wtk_fextra_cfg.h"
#include "wtk/core/cfg/wtk_version_cfg.h"
#include "wtk/asr/vad/wtk_vad.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_wakeup_decoder_cfg qtk_wakeup_decoder_cfg_t;


struct qtk_wakeup_decoder_cfg
{
	union
	{
		wtk_main_cfg_t *main_cfg;
		wtk_mbin_cfg_t *bin_cfg;
	}cfg;

	qtk_kwdec_cfg_t kwdec;
	qtk_kwdec_cfg_t kwdec2;
    wtk_vad_cfg_t vad;
	wtk_fextra_cfg_t extra;
	wtk_version_cfg_t version;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	char *feat_fn;
	int flush_parm;
	int asr_route;
	int f_cnt;
	wtk_matrix_t *m;
	wtk_string_t res;
	unsigned use_mt;
    unsigned int use_vad:1;
};

int qtk_wakeup_decoder_cfg_init(qtk_wakeup_decoder_cfg_t *cfg);
int qtk_wakeup_decoder_cfg_clean(qtk_wakeup_decoder_cfg_t *cfg);
int qtk_wakeup_decoder_cfg_update_local(qtk_wakeup_decoder_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_wakeup_decoder_cfg_update(qtk_wakeup_decoder_cfg_t *cfg);
int qtk_wakeup_decoder_cfg_update2(qtk_wakeup_decoder_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_wakeup_decoder_cfg_bytes(qtk_wakeup_decoder_cfg_t *cfg);

wtk_main_cfg_t* qtk_wakeup_decoder_cfg_new(char *cfg_fn);
qtk_wakeup_decoder_cfg_t* qtk_wakeup_decoder_cfg_new2(char *cfg_fn);
void qtk_wakeup_decoder_cfg_delete(wtk_main_cfg_t *main_cfg);
void qtk_wakeup_decoder_cfg_delete2(qtk_wakeup_decoder_cfg_t *cfg);
qtk_wakeup_decoder_cfg_t* qtk_wakeup_decoder_cfg_new_bin(char *bin_fn);
qtk_wakeup_decoder_cfg_t* qtk_wakeup_decoder_cfg_new_bin2(char *bin_fn,unsigned int seek_pos);
void qtk_wakeup_decoder_cfg_delete_bin(qtk_wakeup_decoder_cfg_t *cfg);
void qtk_wakeup_decoder_cfg_delete_bin2(qtk_wakeup_decoder_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
