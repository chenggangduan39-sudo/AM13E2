#ifndef WTK_FST_WAKEUP_WTK_WAKEUP_CFG
#define WTK_FST_WAKEUP_WTK_WAKEUP_CFG
#include "wtk/core/cfg/wtk_main_cfg.h" 
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/fextra/wtk_fextra_cfg.h"
#include "wtk/core/segmenter/wtk_prune.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/asr/svm/wtk_svm_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wakeup_cfg wtk_wakeup_cfg_t;

typedef struct wtk_wakeup_node wtk_wakeup_node_t;

struct wtk_wakeup_node
{
	int depth;
	int idx;
	int max_frame;
	int max_frame2;
	int min_frame;
	int min_frame2;
	float cur_thresh;
	float pth_thresh;
	float pth_thresh2;
	float pth_pow_thresh;
	float max_df;
	int end_detect_win;
};

typedef struct
{
	wtk_wakeup_node_t *network;
	int min_frame;
	int min_raise;
	int n_network;
	int smooth_left;
	int smooth_right;
	int left_win;
	int right_win;
	int min_tri_frame;
	int tri_win;
	int min_win;
	float speech_sum_scale;
	float avg_thresh;
	float min_hint_thresh;
	int min_hint_cnt;
	float pass_thresh;
	float bg_thresh;
	float sil_thresh;
	float scale_s;
	float scale_e;
	float min_s;
	float min_e;
	float max_end_win;
	unsigned use_bg_check:1;
	unsigned use_half_check:1;
	unsigned debug:1;
}wtk_wakeup_post_cfg_t;


struct wtk_wakeup_cfg
{
	wtk_fextra_cfg_t parm;
	wtk_wakeup_post_cfg_t post;
	wtk_string_t word;
	union
	{
		wtk_main_cfg_t *cfg;
		wtk_mbin_cfg_t *bin_cfg;
	}cfg;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	int inst_cache;
	int pth_cache;
	int max_win;
	unsigned use_win:1;
	unsigned debug:1;
	unsigned print_v:1;
	unsigned log_wav:1;
};

int wtk_wakeup_cfg_init(wtk_wakeup_cfg_t *cfg);
int wtk_wakeup_cfg_clean(wtk_wakeup_cfg_t *cfg);
int wtk_wakeup_cfg_delete_bin(wtk_wakeup_cfg_t *cfg);
int wtk_wakeup_cfg_delete_bin2(wtk_wakeup_cfg_t *cfg);
int wtk_wakeup_cfg_update_local(wtk_wakeup_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wakeup_cfg_update(wtk_wakeup_cfg_t *cfg);
int wtk_wakeup_cfg_update2(wtk_wakeup_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_wakeup_cfg_t* wtk_wakeup_cfg_new_bin2(char *bin_fn);
wtk_wakeup_cfg_t* wtk_wakeup_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
wtk_wakeup_cfg_t* wtk_wakeup_cfg_new(char *fn);
void wtk_wakeup_cfg_delete(wtk_wakeup_cfg_t *cfg);

int wtk_wakeup_post_cfg_init(wtk_wakeup_post_cfg_t *cfg);
int wtk_wakeup_post_cfg_clean(wtk_wakeup_post_cfg_t *cfg);
int wtk_wakeup_post_cfg_update_local(wtk_wakeup_post_cfg_t *cfg,wtk_local_cfg_t *main);
int wtk_wakeup_post_cfg_update(wtk_wakeup_post_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
