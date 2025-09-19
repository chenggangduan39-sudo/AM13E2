#ifndef WTK_KSR_WAKEUP_WTK_KWAKE_CFG
#define WTK_KSR_WAKEUP_WTK_KWAKE_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/cfg/wtk_main_cfg.h" 
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/asr/wakeup/kwake_post_rf/wtk_kwake_post_rf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kwake_cfg wtk_kwake_cfg_t;

typedef struct
{
	int prob;
	int pth_prob;
	int wrd_prob;
	int wrd_min_phn_prob;
	int char_min_phn_prob;
	int char_prob;
	int max_prev_triger;
	int max_prev_triger_thresh;
	int max_prev_ratio_triger;
	int max_prev_ratio;
	int max_next_triger;
	int max_next_triger_thresh;
	int max_next_ratio_triger;
	int max_next_ratio;
	int min_mid_ratio_triger;
	int min_mid_ratio;
	int max_other_triger;
	int max_other_triger_thresh;
	int max_other_id_triger;
	int max_other_id_triger_thresh;
	int speech_thresh;
	int speech_ratio;
	int speech_dur_thresh;
	int speech_dur_min;
}wtk_kwake_node_fixprob_t;

typedef struct
{
	wtk_kwake_node_fixprob_t *fix;
	int id;
	int min_frame;
	int max_frame;
	int wrd_depth;
	float prob;
	float pth_prob;
	float wrd_prob;
	float wrd_min_phn_prob;
	int char_depth;
	float char_min_phn_prob;
	float char_prob;
	int max_prev_node;
	float max_prev_triger;
	float max_prev_triger_thresh;
	float max_prev_ratio_triger;
	float max_prev_ratio;
	int max_next_node;
	float max_next_triger;
	float max_next_triger_thresh;
	float max_next_ratio_triger;
	float max_next_ratio;
	float min_mid_ratio_triger;
	float min_mid_ratio;
	float max_other_triger;
	float max_other_triger_thresh;
	int max_other_id;
	float max_other_id_triger;
	float max_other_id_triger_thresh;
	int speech_win;
	int speech_min_win;
	float speech_thresh;
	float speech_ratio;
	float speech_dur_thresh;
	float speech_dur_min;
	int speech_dur_win;
	int speech_dur_win2;
	float speech_inter_thresh;
	int speech_inter_max_win;
	unsigned check_edge:1;
}wtk_kwake_node_t;

typedef struct
{
	int id;
	int max_count;
	float thresh_min;
	float thresh_max;
	float max_prob;
	float min_ratio;
	float low_thresh;
	int low_cnt;
	int *node;
	int nnode;
}wtk_kwake_peak_node_t;

typedef struct
{
	int wake_s_thresh;
	int prob;
	int max_phn_prob;
	int sil_thresh;
	int bkg_thresh;
	int min_bkg;
	int *id_prob;
	int half_scale1;
	int half_scale2;
}wtk_kwake_post_cfg_fixpob_t;

typedef struct
{
	wtk_kwake_post_cfg_fixpob_t *fix;
	wtk_kwake_node_t *quick_network;
	wtk_kwake_node_t *network;
	wtk_kwake_peak_node_t *peak;
	wtk_kwake_post_rf_cfg_t rf_cfg;
	int nnet;
	int npeak;
	float quick_min_prob;
	float quick_max_phn_prob;
	float wake_s_thresh;
	int wake_min_win;
	float prob;
	float max_phn_prob;
	float sil_thresh;
	int sil_win;
	float bkg_thresh;
	int bkg_win;
	float min_bkg;
	float *id_prob;
	int *id;
	int nid;
	float half_scale1;
	float half_scale2;
	unsigned use_rf:1;
}wtk_kwake_post_cfg_t;

typedef struct
{
	int *in;
	int nin;
	int out;
}wtk_kwake_feat_node_t;

typedef struct
{
	wtk_kwake_feat_node_t *nodes;
	int nnode;
}wtk_kwake_feature_cfg_t;

struct wtk_kwake_cfg
{
	wtk_kxparm_cfg_t parm;
	union
	{
		wtk_main_cfg_t *cfg;
		wtk_mbin_cfg_t *bin_cfg;
	}cfg;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	wtk_kwake_post_cfg_t post;
	wtk_kwake_post_cfg_t triger;
	wtk_kwake_post_cfg_t echo;
	wtk_kwake_post_cfg_t triger_quiet;
	wtk_kwake_post_cfg_t quick;
	wtk_kwake_post_cfg_t echo_quick;
	wtk_kwake_feature_cfg_t feature;
	float triger_time;
	int triger_len;
	int bkg_idx;
	int sil_idx;
	int smooth_left;
	int smooth_right;
	int max_win;
	int min_win;
	int step;
	int check_step;
	float quick_max_phn_prob;
	float quick_prob;
	int fix_quick_max_phn_prob;
	int fix_quick_prob;
	int shift;
	float quiet_time;
	float normal_time;
	float quiet_max_prob;
	float normal_min_prob;
	int quiet_hist;
	int normal_hist;
	unsigned use_feature:1;
	unsigned use_full_triger:1;
	unsigned use_triger:1;
	unsigned use_echo:1;
	unsigned use_full_echo:1;
	unsigned use_quick:1;
	unsigned use_echo_quick:1;
	unsigned use_full_quick:1;
	unsigned use_triger_quiet:1;
	unsigned debug:1;
	unsigned use_fixpoint:1;
};

int wtk_kwake_post_cfg_min_end(wtk_kwake_post_cfg_t *post,int depth);

int wtk_kwake_cfg_bytes(wtk_kwake_cfg_t *cfg);
int wtk_kwake_cfg_init(wtk_kwake_cfg_t *cfg);
int wtk_kwake_cfg_clean(wtk_kwake_cfg_t *cfg);
int wtk_kwake_cfg_update_local(wtk_kwake_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_kwake_cfg_update(wtk_kwake_cfg_t *cfg);
int wtk_kwake_cfg_update2(wtk_kwake_cfg_t *cfg,wtk_source_loader_t *sl);

void wtk_kwake_cfg_delete(wtk_kwake_cfg_t *cfg);
void wtk_kwake_cfg_delete_bin(wtk_kwake_cfg_t *cfg);
void wtk_kwake_cfg_delete_bin2(wtk_kwake_cfg_t *cfg);

void wtk_kwake_post_cfg_init(wtk_kwake_post_cfg_t * cfg);
void wtk_kwake_post_cfg_update_local(wtk_kwake_post_cfg_t * cfg,wtk_local_cfg_t *main);
wtk_kwake_cfg_t* wtk_kwake_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
wtk_kwake_cfg_t* wtk_kwake_cfg_new_bin(char *bin_fn,char *cfg_fn);
wtk_kwake_cfg_t* wtk_kwake_cfg_new_bin2(char *bin_fn);
wtk_kwake_cfg_t* wtk_kwake_cfg_new(char *fn);

#ifdef __cplusplus
};
#endif
#endif
