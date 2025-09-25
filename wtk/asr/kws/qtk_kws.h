#ifndef QTK_KWS_H_
#define QTK_KWS_H_
#include "qtk_kws_cfg.h"
#include "wtk/asr/fextra/torchnn/qtk_torchnn.h"
#include "wtk/asr/vad/wtk_vad2.h"

//#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_kws qtk_kws_t;
typedef void (*qtk_kws_res_notify_f)(void *ths,int res,char *name,int name_len);
typedef void (*qtk_kws_res_notify2_f)(void *ths,int cnt,wtk_string_t **res,float* scores);
//when wake+svprint res:
//1 level1 wakeup success
//2 level2 wakeup success
//3 svprint success
//name/name_len for svprint name


struct qtk_kws
{
	qtk_kws_cfg_t *cfg;
	qtk_kws_res_notify_f notify;
	void * notify_ths;
	qtk_kws_res_notify2_f notify2;
	void * notify_ths2;
	wtk_kxparm_t *parm;
	wtk_kwake_t *wake;
	wtk_kvad_t *vad;
	wtk_vad2_t *vad2;
	qtk_img_rec_t *img;
	wtk_svprint_t *svprint;
	qtk_torchnn_t *wakenn;
	qtk_torchnn_t *wakee2e_nn;
	qtk_torchnn_t *svprint_nn;
	wtk_strbuf_t *user;
	wtk_strbuf_t *wav_buf;
	wtk_string_t *name;
	// wtk_wavfile_t *log_wav;

	wtk_strbuf_t *sliding_buf;
	int window_size;
	int shift_size;
	int process_size;
	int acc_size;
	int acc_size2;

	float sv_prob;
	int index;
	int enroll;
	int eval;

	float speech_db;
	float bkg_db;

	unsigned vad_state:1;
	unsigned has_res:1;
};

qtk_kws_t* qtk_kws_new(qtk_kws_cfg_t *cfg);
void qtk_kws_delete(qtk_kws_t *kws);
int qtk_kws_start(qtk_kws_t *kws);
int qtk_kws_reset(qtk_kws_t *kws);
void qtk_kws_clean(qtk_kws_t *kws);
void qtk_kws_reload(qtk_kws_t *kws);
char* qtk_kws_get_fn(qtk_kws_t *kws);
int qtk_kws_feed(qtk_kws_t *kws,char *data,int bytes,int is_end);
void qtk_kws_feed2(qtk_kws_t *kws,char *data,int bytes,int is_end);
int qtk_kws_enroll(qtk_kws_t *kws,char *name,int len);
int qtk_kws_enroll_end(qtk_kws_t *kws);
int qtk_kws_set_enroll_fn(qtk_kws_t *kws,char *fn, int len);
void qtk_kws_set_notify(qtk_kws_t * kws,qtk_kws_res_notify_f notify, void *ths);
void qtk_kws_set_notify2(qtk_kws_t * kws,qtk_kws_res_notify2_f notify, void *ths);

float qtk_kws_get_prob(qtk_kws_t *kws);
void qtk_kws_set_sv_thresh(qtk_kws_t *kws, float thresh);
void qtk_kws_set_max_spk(qtk_kws_t *kws, int max_spk);
void qtk_kws_set_result_dur(qtk_kws_t *kws, float val);

#ifdef __cplusplus
};
#endif
#endif
