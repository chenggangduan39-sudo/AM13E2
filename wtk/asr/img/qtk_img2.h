#ifndef QTK_IMG2_REC_H_
#define QTK_IMG2_REC_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_vpool2.h"
#include "qtk_img2_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif
typedef struct qtk_img2_rec qtk_img2_rec_t;
typedef struct qtk_ifeat2 qtk_ifeat2_t;
typedef void (*qtk_img2_rec_notify_f)(void *ths, int res, float prob, int start, int end);

typedef enum
{
	QTK_IMG2_SIL,
	QTK_IMG2_SPEECH,
	QTK_IMG2_TBD,
	QTK_IMG2_WAIT
} qtk_img2_rec_state_e;

struct qtk_ifeat2
{
	float *log;
	float *prob;
	wtk_queue_node_t q_n;
	float speech_prob;
	int index;
};

struct qtk_img2_rec
{
    qtk_img2_cfg_t *cfg;
    wtk_kparm_t **kparm;
    qtk_nnet3_t *nnet3;
    wtk_queue_t q;
    wtk_queue_t sil_q;
    qtk_img2_rec_state_e state;
    wtk_vpool2_t *pool;
    qtk_img2_rec_notify_f notify;
    qtk_img2_thresh_cfg_t *thresh;
    qtk_img2_thresh_cfg_t *thresh_echo;

    void *notify_ths;
    int wait_count;
    int sil_num;
    int check_index;
    int index;
    int wake_id;
    float prob;
    int start;
    int end;
    int sp_sil;
    int feature_cols;

    unsigned int out_num;
    unsigned waked : 1;
    unsigned idle : 1;
};

qtk_img2_rec_t *qtk_img2_rec_new(qtk_img2_cfg_t *cfg);
void qtk_img2_rec_delete(qtk_img2_rec_t *ir);
void qtk_img2_rec_reset(qtk_img2_rec_t *ir);
void qtk_img2_rec_reset2(qtk_img2_rec_t *ir);
void qtk_img2_rec_start(qtk_img2_rec_t *ir);
int qtk_img2_rec_feed(qtk_img2_rec_t *ir, char *data, int bytes, int is_end);
int qtk_img2_rec_get_time(qtk_img2_rec_t *ir, float *fs, float *fe);
void qtk_img2_rec_set_notify(qtk_img2_rec_t *ir, qtk_img2_rec_notify_f notify, void *ths);
float qtk_img2_rec_get_conf(qtk_img2_rec_t *ir);
void qtk_img2_thresh_set_cfg(qtk_img2_rec_t *ir, qtk_img2_thresh_cfg_t *thresh, int echo);
void qtk_img2_rec_get_sp_sil(qtk_img2_rec_t *ir, int sp_sil);
int qtk_img2_set_prob(qtk_img2_rec_t *ir, float prob, int echo);

#ifdef __cplusplus
};
#endif
#endif
