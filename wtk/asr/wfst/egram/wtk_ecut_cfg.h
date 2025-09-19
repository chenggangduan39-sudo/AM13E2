#ifndef WTK_FST_EGRAM_WTK_EGRAM_CUT_CFG_H_
#define WTK_FST_EGRAM_WTK_EGRAM_CUT_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/net/wtk_ebnf.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnfnet.h"
#include "wtk_e2fst_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ecut_cfg wtk_ecut_cfg_t;

struct wtk_ecut_cfg
{
	short *map1;
	short *map2;
	short *map3;
	short *lmin;
	unsigned char *lshift;
	unsigned char *l;
	unsigned char *b;
	char* mdl_info;
	int sil_id1;
	int sil_id2;
	int out;//start out lable
	char shift;//bias shift for last layer
	short min;//bias min for last layer
	int num_in;
	int num_dnn;
	int num_cutdnn;
	int num_col;
};

int wtk_ecut_cfg_init(wtk_ecut_cfg_t *cfg);
int wtk_ecut_cfg_clean(wtk_ecut_cfg_t *cfg);
int wtk_ecut_cfg_update_local(wtk_ecut_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_ecut_cfg_update(wtk_ecut_cfg_t *cfg);
int wtk_ecut_cfg_update2(wtk_ecut_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif
