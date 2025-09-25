#ifndef WTK_KSR_WAKEUP_WTK_KWAKE
#define WTK_KSR_WAKEUP_WTK_KWAKE
#include "wtk/core/wtk_type.h" 
#include "wtk_kwake_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kwake wtk_kwake_t;

typedef void(*wtk_kwake_notify_f)(void *ths,int cmd);

typedef enum
{
	WTK_KWAKE_QUIET,
	WTK_KWAKE_SPEECH,
	WTK_KWAKE_NOISE,
}wtk_kwake_state_t;

struct wtk_kwake
{
	wtk_kwake_cfg_t *cfg;
	wtk_kwake_post_cfg_t *post;
	wtk_kwake_post_cfg_t *post_normal;
	wtk_kwake_post_cfg_t *post_quiet;
	wtk_kwake_post_cfg_t *xpost;
	wtk_kxparm_t *parm;
	wtk_robin_t *rb_win;
	wtk_robin_t *rb_smooth;
	wtk_kwake_post_rf_inst_t *rf;
	float *smooth;
	float *feat;
	int smooth_idx;
	int feat_idx;
	int poped;
	int nvec;
	int *wake_slot;
	float prob;
	float quiet;
	int first_n;
	int second_n;
	int idle_pos;
	int hint_pos;
	int check_step;
	float max_phn_prob;
	unsigned int input;
	int nhist;
	float hist_prob;
	float wk_prob;
	wtk_kwake_state_t state;
	float *quick_prob;
	short *fix_quick_prob;
	short *fix_smooth;
	short *fix_feat;
	int fix_max_phn_prob;
	int fix_prob;
	short wsi;
	unsigned quicked:1;
	unsigned idled:1;
	unsigned waked:1;
	unsigned echoing:1;
	wtk_kwake_notify_f notify;
	void *ths;
	//unsigned use_fxipoint:1;
};

int wtk_kwake_bytes(wtk_kwake_t *wake);
wtk_kwake_t* wtk_kwake_new(wtk_kwake_cfg_t *cfg);
wtk_kwake_t* wtk_kwake_new2(wtk_kwake_cfg_t *cfg,int outdim);
void wtk_kwake_delete(wtk_kwake_t *w);
void wtk_kwake_start(wtk_kwake_t *w);
void wtk_kwake_start2(wtk_kwake_t *w,int echoing);
void wtk_kwake_set_echoing(wtk_kwake_t *w,int echoing);
void wtk_kwake_reset(wtk_kwake_t *w);
void wtk_kwake_reset2(wtk_kwake_t *w,int reset_cmn);
int wtk_kwake_feed(wtk_kwake_t *w,short *data,int len,int is_end);
int wtk_kwake_feed_float(wtk_kwake_t *w,float *data,int len,int is_end);
void wtk_kwake_on_featf(wtk_kwake_t *wake,float *feat, int index);
void wtk_kwake_get_wake_time2(wtk_kwake_t *w,float *fs,float *fe,float dur);
void wtk_kwake_get_wake_time(wtk_kwake_t *w,float *fs,float *fe);
void wtk_kwake_print(wtk_kwake_t *w);
float wtk_kwake_get_conf(wtk_kwake_t *w);
void wtk_kwake_set_notify(wtk_kwake_t *kwake,void *ths,wtk_kwake_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
