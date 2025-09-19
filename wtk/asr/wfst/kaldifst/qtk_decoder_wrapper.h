#ifndef QTK_DECODER_WRAPPER_H_
#define QTK_DECODER_WRAPPER_H_

#include "qtk_kwfstdec.h"
#include "qtk_kwfstdec_lite.h"
#include "qtk_decoder_wrapper_cfg.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk/core/json/wtk_json.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/asr/wfst/ebnfdec/wtk_ebnfdec2.h"
#include "wtk/asr/wfst/egram/wtk_egram.h"
#include "wtk/asr/wfst/rec/wtk_wfstr.h"
#include "wtk/asr/wfst/wtk_wfstdec.h"
#include "wtk/asr/wfst/wtk_wfstenv_cfg.h"
#include "wtk/core/strlike/wtk_chnlike.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/asr/xvprint/wtk_xvprint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_decoder_route qtk_decoder_route_t;
typedef struct qtk_decoder_wrapper qtk_decoder_wrapper_t;

typedef enum
{
	QTK_DECODER_EVT_START,
	QTK_DECODER_EVT_FEED,
	QTK_DECODER_EVT_END
}qtk_decoder_evt_type_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	qtk_decoder_evt_type_t type;
	wtk_feat_t *f;
	float *out;
	qtk_blas_matrix_t *blas;
	int index;
}qtk_decoder_evt_t;

struct qtk_decoder_route
{
	qtk_kwfstdec_t *dec;
	qtk_kwfstdec_lite_t *dec_lite;
	wtk_thread_t thread;
	wtk_lockhoard_t evt_hoard;
	wtk_blockqueue_t rec_input_q;
	wtk_sem_t rec_wait_sem;
	wtk_sem_t rec_start_wait_sem;
	wtk_strbuf_t *res_buf;
	wtk_strbuf_t *hint_buf;
	int run;
	double time_rec;
	double time_start;
};

struct qtk_decoder_wrapper
{
    wtk_queue_t vad_q;
	wtk_ebnfdec2_t *ebnfdec2;
	wtk_chnlike_t *chnlike;
	wtk_xbnf_rec_t *xbnf;
	qtk_decoder_wrapper_cfg_t *cfg;
	wtk_cfg_file_t *env_parser;
	wtk_wfstenv_cfg_t env;
	wtk_queue_t parm_q;
	wtk_fextra_t *parm;
	wtk_kxparm_t *kxparm;
	qtk_kwfstdec_t *dec;
	qtk_kwfstdec_lite_t *dec_lite;
	wtk_strbuf_t *res_buf;
	wtk_strbuf_t *json_buf;
	wtk_strbuf_t *hint_buf;
	wtk_strbuf_t *vad_buf;
	wtk_strbuf_t *whole_buf;
	wtk_strbuf_t *person_buf;
	wtk_json_t *json;
	qtk_decoder_route_t **asr;
    wtk_vad_t *vad;
	wtk_xvprint_t *xvprint;
	wtk_lex_t *lex;
    wtk_vframe_state_t last_vframe_state;
	//--------------------- thread section -------------
	//wtk_lockhoard_t evt_hoard;
	//wtk_thread_t thread;
	//wtk_blockqueue_t rec_input_q;
	wtk_blockqueue_t feature_bak_q;
	wtk_vecf_t* vf;
	//wtk_sem_t rec_wait_sem;
	//wtk_sem_t rec_start_wait_sem;
	double time_stop;
	int wav_bytes;
	int rec_wav_bytes;
	int index;
	unsigned int data_left:1;
	unsigned int run:1;
};

qtk_decoder_route_t* qtk_decoder_wrapper_route_new(qtk_kwfstdec_cfg_t* dec_cfg,char* name,int use_lite);
qtk_decoder_route_t* qtk_decoder_wrapper_route_new2(qtk_kwfstdec_cfg_t* dec_cfg,char* name, int use_outnet,int use_lite);
qtk_decoder_wrapper_t* qtk_decoder_wrapper_new(qtk_decoder_wrapper_cfg_t* cfg);
int qtk_decoder_wrapper_start(qtk_decoder_wrapper_t* wrapper);
int qtk_decoder_wrapper_start2(qtk_decoder_wrapper_t* wrapper,char *data,int bytes);
void qtk_decoder_wrapper_feed_nnet3_feature(qtk_decoder_wrapper_t *wrapper,qtk_blas_matrix_t *f,int end, int plus);
int qtk_decoder_wrapper_feed(qtk_decoder_wrapper_t* wrapper,char *data,int bytes,int is_end);
void qtk_decoder_wrapper_reset(qtk_decoder_wrapper_t* wrapper);
void qtk_decoder_wrapper_delete(qtk_decoder_wrapper_t* wrapper);
void qtk_decoder_wrapper_get_result(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v);
int qtk_decoder_wrapper_get_result2(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v);
void qtk_decoder_wrapper_feed_test_feat2(qtk_decoder_wrapper_t* wrapper,wtk_matrix_t *m,int cnt);
void qtk_decoder_wrapper_feed_test_feat3(qtk_decoder_wrapper_t* wrapper,wtk_matrix_t *m,int cnt);
void qtk_decoder_wrapper_feed_test_feat(qtk_decoder_wrapper_t* wrapper,int is_end);
void qtk_decoder_wrapper_get_hint_result(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v);
void qtk_decoder_wrapper_get_vad_result(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v);
void qtk_decoder_wrapper_get_result_with_name(qtk_decoder_wrapper_t *wrapper,wtk_string_t *v);
int qtk_decoder_wrapper_feedk(qtk_decoder_wrapper_t* wrapper,short *data,int bytes,int is_end);
int qtk_decoder_wrapper_set_xvector(qtk_decoder_wrapper_t *wrapper, char *fn);
void qtk_decoder_wrapper_set_xbnf(qtk_decoder_wrapper_t *wrapper,char *buf, int len);
float qtk_decoder_wrapper_get_conf(qtk_decoder_wrapper_t *wrapper);
int  qtk_decoder_wrapper_get_time(qtk_decoder_wrapper_t * wrapper,float *fs,float *fe);
//asr egram tirck
float qtk_decoder_wrapper_set_vadindex(qtk_decoder_wrapper_t * wrapper, int index);
#ifdef __cplusplus
}
;
#endif
#endif

