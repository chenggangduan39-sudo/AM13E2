#ifndef WTK_ASR_WDEC_WTK_WDEC_CFG
#define WTK_ASR_WDEC_WTK_WDEC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk/asr/wfst/wtk_wfstdec.h"
#include "wtk/asr/model/wtk_dict.h"
#include "wtk_wdec_net_cfg.h"
#include "wtk/core/segmenter/wtk_prune.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wdec_cfg wtk_wdec_cfg_t;

typedef struct
{
	float *min_wrd_conf;
	float min_wrd_conf2;
	float min_conf;
	float min_step_conf;
	float min_hmm_conf;
}wtk_wdec_post_cfg_t;


struct wtk_wdec_cfg
{
	union
	{
		wtk_main_cfg_t *main_cfg;
		wtk_mbin_cfg_t *bin_cfg;
	}cfg;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	wtk_prune_cfg_t prune;
	wtk_fextra_cfg_t fextra;
	wtk_wfst_dnn_cfg_t dnn;
	wtk_wdec_net_cfg_t net;
	wtk_wdec_post_cfg_t post;
	int step;
	unsigned add_path:1;
};

int wtk_wdec_cfg_init(wtk_wdec_cfg_t *cfg);
int wtk_wdec_cfg_clean(wtk_wdec_cfg_t *cfg);
int wtk_wdec_cfg_update_local(wtk_wdec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wdec_cfg_update(wtk_wdec_cfg_t *cfg);
int wtk_wdec_cfg_update2(wtk_wdec_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_wdec_cfg_t* wtk_wdec_cfg_new(char *fn);
void wtk_wdec_cfg_delete(wtk_wdec_cfg_t *cfg);

wtk_wdec_cfg_t* wtk_wdec_cfg_new_bin(char *fn);
void wtk_wdec_cfg_delete_bin(wtk_wdec_cfg_t *cfg);
wtk_wdec_cfg_t* wtk_wdec_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
void wtk_wdec_cfg_delete_bin2(wtk_wdec_cfg_t *cfg);


void wtk_wdec_post_cfg_init(wtk_wdec_post_cfg_t *cfg);
void wtk_wdec_post_cfg_clean(wtk_wdec_post_cfg_t *cfg);
int wtk_wdec_post_cfg_update_local(wtk_wdec_post_cfg_t *cfg,wtk_local_cfg_t *lc);
void wtk_wdec_post_cfg_update(wtk_wdec_post_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
