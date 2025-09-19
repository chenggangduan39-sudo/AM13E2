#ifndef WTK_FST_WAKEUP_WTK_WAKEUP
#define WTK_FST_WAKEUP_WTK_WAKEUP
#include "wtk/core/wtk_type.h" 
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk_wakeup_cfg.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk/core/wtk_riff.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wakeup wtk_wakeup_t;

typedef void(*wtk_wakeup_notify_feat_f)(void *ths,int idx,float *pv);

/**
 * return 1 for valid 0 for invalid;
 */
typedef int(*wtk_wakeup_check_valid_f)(void *ths,float fs,float fe);

typedef struct
{
	unsigned int index;
	float *v;
}wtk_wakeup_feat_t;

typedef struct wtk_wakeup_path wtk_wakeup_path_t;

struct wtk_wakeup_path
{
	wtk_wakeup_path_t *prev;
	float prob;
	float max_prob;
	int max_prob_frame;
	unsigned short frame_init;
	unsigned short frame_s;
	unsigned short frame_e;
};

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_wakeup_node_t *node;
	wtk_wakeup_path_t *pth;
	unsigned short frame_s;
	unsigned short frame_e;
	int max_prob_frame;
	int rise_count;
	float pre_prob;
	float prob;
	float max_prob;
}wtk_wakeup_inst_t;


struct wtk_wakeup
{
	wtk_wakeup_cfg_t *cfg;
	wtk_wakeup_post_cfg_t *post_cfg;
	wtk_fextra_t *parm;
	wtk_queue_t param_q;
	wtk_queue_t inst_q;
	wtk_robin_t *win_robin;
	wtk_robin_t *feat_robin;
	wtk_vecf_t *vec_smooth;
	wtk_vecf_t *input_vec;
	wtk_wakeup_inst_t *final_inst;
	wtk_vpool_t *inst_pool;
	wtk_vpool_t *pth_pool;
	wtk_strbuf_t *buf;
	wtk_strbuf_t *wav_buf;
	int cache[8];
	int tmp_cache[8];
	int poped;
	float thresh;
	float prob;
	wtk_wakeup_check_valid_f check_valid;
	void *check_valid_ths;
	void *notify_ths;
	wtk_wakeup_notify_feat_f notify;
	unsigned int count;
	unsigned int wav_cnt;
	unsigned int idx;
	unsigned int deep;
	unsigned int frame;
	unsigned int get:1;
	unsigned int skip_post:1;
};

wtk_wakeup_t* wtk_wakeup_new(wtk_wakeup_cfg_t *cfg);
void wtk_wakeup_delete(wtk_wakeup_t *wakeup);
void wtk_wakeup_set_feature_notify(wtk_wakeup_t *wakeup,void *ths,wtk_wakeup_notify_feat_f notify);
void wtk_wakeup_set_check_valid(wtk_wakeup_t *wakeup,void *ths,wtk_wakeup_check_valid_f check);
int wtk_wakeup_start(wtk_wakeup_t *wakeup);
void wtk_wakeup_set_post(wtk_wakeup_t *wakeup,wtk_wakeup_post_cfg_t *post);
int wtk_wakeup_start2(wtk_wakeup_t *wakeup,wtk_wakeup_post_cfg_t *post);
int wtk_wakeup_reset(wtk_wakeup_t *wakeup);
/**
 * return 1 for found wakeup word;
 */
int wtk_wakeup_feed(wtk_wakeup_t *wakeup,char *data,int bytes,int is_end);
float wtk_wakeup_get_cur_valid_start_time(wtk_wakeup_t *wakeup);
int wtk_wakeup_get_final_time(wtk_wakeup_t *wakeup,float *s,float *e);
void wtk_wakeup_update_config(wtk_wakeup_t *wakeup,int smooth,int  min_raise,int max_frame,float cur_thresh,float pth_thresh);
void wtk_wakeup_set_smooth(wtk_wakeup_t *wakeup,int smooth);
void wtk_wakeup_set_min_raise(wtk_wakeup_t *wakeup,int min_raise);
void wtk_wakeup_set_max_frame(wtk_wakeup_t *wakeup,int max_frame);
void wtk_wakeup_set_cur_thresh(wtk_wakeup_t *wakeup,float cur_thresh);
void wtk_wakeup_set_pth_thresh(wtk_wakeup_t *wakeup,float pth_thresh);
void wtk_wakeup_print(wtk_wakeup_t *wakeup);
float wtk_wakeup_get_result(wtk_wakeup_t *wakeup,wtk_string_t *v);
float wtk_wakeup_get_result2(wtk_wakeup_t *wakeup,wtk_string_t *v,int *res,int detail);
void wtk_wakeup_path_print(wtk_wakeup_path_t *pth);
//in s
int wtk_wakeup_get_final_time(wtk_wakeup_t *wakeup,float *fs,float *fe);
int wtk_wakeup_get_ssl_time(wtk_wakeup_t *wakeup,float *fs,float *fe);

int wtk_wakeup_get_unsee_sample(wtk_wakeup_t *wakeup);

int wtk_wakeup_test_file(char *cfg,char *wav,float *fs,float *fe);
void wtk_wakeup_print_raw_win(wtk_wakeup_t *wakeup);
float wtk_wakeup_get_delay(wtk_wakeup_t *wakeup);
int wtk_wakeup_get_raw_final_time(wtk_wakeup_t *wakeup,float *fs,float *fe);
void wtk_wakeup_print_robin(wtk_wakeup_t *wakeup);
int wtk_wakeup_get_final_hint_time(wtk_wakeup_t *wakeup,float *fs,float *fe);
#ifdef __cplusplus
};
#endif
#endif
