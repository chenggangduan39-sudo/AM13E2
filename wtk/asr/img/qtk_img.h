#ifndef QTK_IMG_REC_H_
#define QTK_IMG_REC_H_
#include "qtk/nnrt/qtk_nnrt.h"
#include "qtk_img_cfg.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_vpool2.h"

#ifdef __cplusplus
extern "C"
{
#endif
typedef struct qtk_img_rec qtk_img_rec_t;
typedef struct qtk_ifeat qtk_ifeat_t;
typedef void (*qtk_img_rec_notify_f)(void *ths, int res, int start, int end);

typedef enum
{
	QTK_IMG_SIL,
	QTK_IMG_SPEECH,
	QTK_IMG_TBD,
	QTK_IMG_WAIT
} qtk_img_rec_state_e;

struct qtk_ifeat
{
	float *log;
	float *prob;
	wtk_queue_node_t q_n;
	float speech_prob;
	int index;
};

struct qtk_img_rec
{
	qtk_img_cfg_t *cfg;
        qtk_nnrt_t *nnrt;
        qtk_nnrt_value_t *cache;
        int64_t **shape;
        wtk_strbuf_t *feat;
        int pad_len;
        int chunk_len;
        int ret_len;
        int tail_len;
        int64_t num_feats;
        int64_t frames;

        wtk_kxparm_t *parm;
	wtk_queue_t q;
	wtk_queue_t sil_q;
	qtk_img_rec_state_e state;
        qtk_img_rec_state_e wait_state;
        wtk_vpool2_t *pool;
        qtk_img_rec_notify_f notify;
	qtk_img_thresh_cfg_t thresh;
	qtk_img_thresh_cfg_t thresh_echo;
	void *notify_ths;
	int wait_count;
        int wait_flag;
        int sil_num;
        int check_index;
	int index;
	int wake_id;
	float prob;
	int start;
        int start_off;
        int end;
        int end_off;
        int skip;
        int sp_sil;
        float before_av_prob;

        float last_av_prob;
        float last_max_prob;
	float last_avx;
	float last_maxx;

	unsigned waked : 1;
	unsigned idle : 1;
	unsigned print : 1;
};

qtk_img_rec_t *qtk_img_rec_new(qtk_img_cfg_t *cfg);
void qtk_img_rec_delete(qtk_img_rec_t *ir);
void qtk_img_rec_reset(qtk_img_rec_t *ir);
void qtk_img_rec_reset2(qtk_img_rec_t *ir);
void qtk_img_rec_start(qtk_img_rec_t *ir);
int qtk_img_rec_feed(qtk_img_rec_t *ir, char *data, int bytes, int is_end);
int qtk_img_rec_get_time(qtk_img_rec_t *ir, float *fs, float *fe);
void qtk_img_rec_set_notify(qtk_img_rec_t *ir, qtk_img_rec_notify_f notify, void *ths);
float qtk_img_rec_get_conf(qtk_img_rec_t *ir);
void qtk_img_rec_show(qtk_img_rec_t *img, int id);
void qtk_img_thresh_set_cfg(qtk_img_rec_t *ir, qtk_img_thresh_cfg_t thresh, int echo);
void qtk_img_rec_get_sp_sil(qtk_img_rec_t *ir, int sp_sil);
int qtk_img_set_prob(qtk_img_rec_t *ir, float prob, int echo);
#ifdef __cplusplus
};
#endif
#endif
