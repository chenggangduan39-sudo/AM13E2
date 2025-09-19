#ifndef WTK_VITE_VPRINT_DETECT_WTK_VDETECT_CFG
#define WTK_VITE_VPRINT_DETECT_WTK_VDETECT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/model/wtk_hmmset.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/rbin/wtk_ubin.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vdetect_cfg wtk_vdetect_cfg_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_string_t name;
	char *hmm_fn;
	wtk_hmm_t *hmm;
	double prob;
	double llr;
	char attr;
}wtk_vdetect_usr_t;

struct wtk_vdetect_cfg
{
	wtk_ubin_t *ubin;
	wtk_hmmset_cfg_t *hmmset;
	wtk_heap_t *heap;
	wtk_queue_t usr_q;
	wtk_string_t usr_dn;
	float thresh;
	int skip_frame;
	unsigned load:1;
	unsigned use_ubin:1;
};

int wtk_vdetect_cfg_init(wtk_vdetect_cfg_t *cfg);
int wtk_vdetect_cfg_clean(wtk_vdetect_cfg_t *cfg);
int wtk_vdetect_cfg_update_local(wtk_vdetect_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vdetect_cfg_update(wtk_vdetect_cfg_t *cfg);
void wtk_vdetect_cfg_reset_usr(wtk_vdetect_cfg_t* cfg);
int wtk_vdetect_cfg_load_usr_dn_fn(wtk_vdetect_cfg_t *cfg,wtk_string_t *fn);
int wtk_vdetect_cfg_del_usr_dn_fn(wtk_vdetect_cfg_t *cfg,wtk_string_t *fn);
int wtk_vdetect_cfg_update2(wtk_vdetect_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
