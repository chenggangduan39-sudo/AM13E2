#ifndef WTK_TTS_SYN_WTK_SYN_CFG
#define WTK_TTS_SYN_WTK_SYN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_syn_dtree.h"
#include "wtk_syn_hmm.h"
#include "wtk_syn_dwin.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_cfg wtk_syn_cfg_t;

typedef struct
{
	wtk_syn_dwin_cfg_t lf0_cfg;
	wtk_syn_dwin_cfg_t mcp_cfg;
	wtk_syn_dwin_cfg_t bap_cfg;
	wtk_syn_dwin_t *lf0;
	wtk_syn_dwin_t *mcp;
	wtk_syn_dwin_t *bap;
}wtk_syn_win_cfg_t;

int wtk_syn_win_cfg_init(wtk_syn_win_cfg_t *cfg);
int wtk_syn_win_cfg_clean(wtk_syn_win_cfg_t *cfg);
int wtk_syn_win_cfg_update_local(wtk_syn_win_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_syn_win_cfg_update(wtk_syn_win_cfg_t *cfg,wtk_strpool_t *pool);
int wtk_syn_win_cfg_update2(wtk_syn_win_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);


struct wtk_syn_cfg
{
	wtk_syn_dtree_cfg_t tree_cfg;
	wtk_syn_hmm_cfg_t hmm_cfg;
	wtk_syn_win_cfg_t win;
	wtk_syn_dtree_t *tree;
	wtk_syn_hmm_t *hmm;
	int frame;
	int stream_pad_sil_dur;
	int stream_min_dur;
	int steam_min_left_dur;
	int f0_norm_win;
	unsigned use_stream:1;
};

int wtk_syn_cfg_init(wtk_syn_cfg_t *cfg);
int wtk_syn_cfg_bytes(wtk_syn_cfg_t *cfg);
int wtk_syn_cfg_clean(wtk_syn_cfg_t *cfg);
int wtk_syn_cfg_update_local(wtk_syn_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_syn_cfg_update(wtk_syn_cfg_t *cfg);
int wtk_syn_cfg_update2(wtk_syn_cfg_t *cfg,wtk_strpool_t *pool);
int wtk_syn_cfg_update3(wtk_syn_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);
#ifdef __cplusplus
};
#endif
#endif
