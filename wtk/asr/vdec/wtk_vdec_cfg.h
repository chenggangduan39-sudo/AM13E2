#ifndef WTK_VITE_DEC_WTK_VDEC_CFG_H_
#define WTK_VITE_DEC_WTK_VDEC_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/fextra/wtk_fextra_cfg.h"
#include "wtk/asr/model/wtk_hmmset_cfg.h"
#include "wtk/asr/vdec/rec/wtk_rec_cfg.h"
#include "wtk/asr/net/wtk_latset.h"
#include "wtk/asr/net/wtk_ebnf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vdec_cfg wtk_vdec_cfg_t;
struct wtk_vdec_cfg
{
	wtk_fextra_cfg_t parm;
	wtk_hmmset_cfg_t hmmset;
	wtk_net_cfg_t net;
	wtk_rec_cfg_t rec;
	wtk_dict_t *dict;
	wtk_latset_t *lat_set;
	wtk_label_t *label;
	//----------- ebnf --------------
	wtk_ebnf_t *ebnf;
	//------------------binary ------------------
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	//--------------------------------
	char *dict_fn;
	char *net_fn;
	char *ebnf_fn;
	unsigned use_ebnf:1;
};

int wtk_vdec_cfg_init(wtk_vdec_cfg_t *cfg);
int wtk_vdec_cfg_clean(wtk_vdec_cfg_t *cfg);
int wtk_vdec_cfg_update_local(wtk_vdec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vdec_cfg_update(wtk_vdec_cfg_t *cfg);
int wtk_vdec_cfg_update2(wtk_vdec_cfg_t *cfg,wtk_source_loader_t *sl);
void wtk_vdec_cfg_delete_bin(wtk_vdec_cfg_t *cfg);
wtk_vdec_cfg_t* wtk_vdec_cfg_new_bin(char *bin_fn);
#ifdef __cplusplus
};
#endif
#endif
