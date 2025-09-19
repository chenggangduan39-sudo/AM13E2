#ifndef WTK_FST_REC_WTK_WFSTR_CFG_H_
#define WTK_FST_REC_WTK_WFSTR_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk_wfst_dnn_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net3_cfg.h"
#include "wtk/core/math/wtk_fixmath.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfstrec_cfg wtk_wfstrec_cfg_t;


struct wtk_wfstrec_cfg
{
	wtk_wfst_dnn_cfg_t dnn;
	wtk_prune_cfg_t prune;
	wtk_fst_net3_cfg_t lat_net;

	wtk_prune_cfg_t wrd_prune;
	int wrd_prune_thresh;

	wtk_prune_cfg_t phn_prune;
	int phn_prune_thresh;

	wtk_prune_cfg_t expand_prune;
	int expand_prune_thresh;

	wtk_matrix_t *hlda_matrix;
	char *hlda_fn;
	double min_log_exp;

	int tok_cache;
	int tok_reset_cache;
	int inst_cache;
	int pth_cache;
	int align_cache;
	int max_emit_hyps;
	float emit_beam;
	float phn_start_beam;
	float lat_beam;
	float phn_end_beam;
	float word_beam;
	float ac_lookahead_alpha;
	float frame_dur;
	float max_final_tok_pad_like;
	float sil_trans_scale;
	float lmscale2;
	int snt_end_frame;
	int max_sil_wrd;
	int min_sil_end_frame;
	int min_sil_frame;
	float min_sil_thresh;
	float ac_scale;
	float trans_scale;
	float custom_ac_scale;

	unsigned use_phn_prune:1;
	unsigned use_wrd_prune:1;
	unsigned use_expand_prune:1;
	unsigned use_single_best:1;
	unsigned use_dnn:1;
	unsigned use_prune:1;
	unsigned use_transpose_hmm:1;
	unsigned use_forceout:1;
	unsigned use_lat:1;
	unsigned inst_use_heap:1;
	unsigned use_eps_pth:1;
	unsigned dump_lat:1;
	unsigned state:1;
	unsigned model:1;
	unsigned use_hlda_bin:1;
	unsigned use_in_wrd_beam:1;
	unsigned use_end_hint:1;
	unsigned use_max_like_path:1;
	unsigned add_wrd_sep:1;	//used for calculate word conf;
	unsigned use_spre:1;
};

int wtk_wfstrec_cfg_init(wtk_wfstrec_cfg_t *cfg);
int wtk_wfstrec_cfg_clean(wtk_wfstrec_cfg_t *cfg);
int wtk_wfstrec_cfg_update_local(wtk_wfstrec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wfstrec_cfg_update(wtk_wfstrec_cfg_t *cfg);
int wtk_wfstrec_cfg_update2(wtk_wfstrec_cfg_t *cfg, wtk_source_loader_t *sl);
void wtk_wfstrec_cfg_update_hmmset(wtk_wfstrec_cfg_t *cfg,wtk_hmmset_t *hmmset);
int wtk_wfstrec_cfg_bytes(wtk_wfstrec_cfg_t *cfg);
int wtk_wfstrec_cfg_get_thresh(wtk_wfstrec_cfg_t *cfg,int hyps);
#ifdef __cplusplus
};
#endif
#endif
