#ifndef QTK_WAKEUP_DECODER_H_
#define QTK_WAKEUP_DECODER_H_

#include "qtk_kwdec.h"
#include "qtk_wakeup_decoder_cfg.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/asr/wfst/wtk_wfstdec.h"
#include "wtk/asr/wfst/wtk_wfstenv_cfg.h"
#include "wtk/asr/vad/wtk_vad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_wakeup_decoder_route qtk_wakeup_decoder_route_t;
typedef struct qtk_wakeup_decoder qtk_wakeup_decoder_t;

typedef enum
{
	QTK_WAKEUP_DECODER_EVT_START,
	QTK_WAKEUP_DECODER_EVT_FEED,
	QTK_WAKEUP_DECODER_EVT_END
}qtk_wakeup_decoder_evt_type_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	qtk_wakeup_decoder_evt_type_t type;
	wtk_feat_t *f;
	float *out;
	qtk_blas_matrix_t *blas;
	int index;
}qtk_wakeup_decoder_evt_t;

struct qtk_wakeup_decoder_route
{
	qtk_kwdec_t *dec;
	wtk_thread_t thread;
	wtk_lockhoard_t evt_hoard;
	wtk_blockqueue_t rec_input_q;
	wtk_sem_t rec_wait_sem;
	wtk_sem_t rec_start_wait_sem;
        wtk_lock_t mut;
        int run;
};

struct qtk_wakeup_decoder
{
    wtk_queue_t vad_q;
	qtk_wakeup_decoder_cfg_t *cfg;
	wtk_cfg_file_t *env_parser;
	wtk_wfstenv_cfg_t env;
	wtk_queue_t parm_q;
	wtk_fextra_t *parm;
	qtk_kwdec_t *dec;
	qtk_wakeup_decoder_route_t **asr;
    wtk_vad_t *vad;
    wtk_vframe_state_t last_vframe_state;
	//--------------------- thread section -------------
	//wtk_lockhoard_t evt_hoard;
	//wtk_thread_t thread;
	//wtk_blockqueue_t rec_input_q;
	wtk_blockqueue_t feature_bak_q;
	//wtk_sem_t rec_wait_sem;
	//wtk_sem_t rec_start_wait_sem;
	double time_stop;
	int wav_bytes;
	int rec_wav_bytes;
	int index;
	int start;
	unsigned int run:1;
};

void qtk_wakeup_decoder_get_wake_time(qtk_wakeup_decoder_t *decoder,float *fs,float *fe);
qtk_wakeup_decoder_route_t* qtk_wakeup_decoder_route_new(qtk_kwdec_cfg_t* dec_cfg,char* name);
qtk_wakeup_decoder_t* qtk_wakeup_decoder_new(qtk_wakeup_decoder_cfg_t* cfg);
int qtk_wakeup_decoder_start(qtk_wakeup_decoder_t* decoder);
int qtk_wakeup_decoder_start2(qtk_wakeup_decoder_t* decoder,char *data,int bytes);
void qtk_wakeup_decoder_feed_nnet3_feature(qtk_wakeup_decoder_t *decoder,qtk_blas_matrix_t *f,int end);
int qtk_wakeup_decoder_feed(qtk_wakeup_decoder_t* decoder,char *data,int bytes,int is_end);
void qtk_wakeup_decoder_reset(qtk_wakeup_decoder_t* decoder);
void qtk_wakeup_decoder_delete(qtk_wakeup_decoder_t* decoder);

#ifdef __cplusplus
}
;
#endif
#endif

