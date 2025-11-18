#ifndef WTK_FST_WFSTDEC_H_
#define WTK_FST_WFSTDEC_H_
#include "wtk/core/wtk_type.h"
#include "wtk_wfstdec_cfg.h"
#include "wtk/asr/wfst/rec/wtk_wfstr.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk_wfstenv_cfg.h"
#include "wtk/core/json/wtk_json.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/core/wtk_queue_slot.h"
#include "wtk/os/wtk_log.h"
#include "wtk/asr/wfst/ebnfdec/wtk_ebnfdec2.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnf_rec.h"
#include "wtk_wfstdec_output.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfstdec wtk_wfstdec_t;

typedef void (*wtk_wfstdec_notify_f)(void *ths,char *rec,int rec_bytes);

typedef enum
{
	WTK_WFSTEVT_START,
	WTK_WFSTEVT_FEAT,
	WTK_WFSTEVT_RESTART,
	WTK_WFSTEVT_END
}wtk_wfstevt_type_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_wfstevt_type_t type;
	wtk_feat_t *f;
}wtk_wfstevt_t;


struct wtk_wfstdec
{
	wtk_wfstdec_cfg_t *cfg;
	wtk_queue_t vad_q;	//wtk_vframe_output q;
	wtk_queue_t parm_q;	//wtk_feat_t queue;
	wtk_ebnfdec2_t *ebnfdec2;
	wtk_xbnf_rec_t *xbnf;
	wtk_fextra_t *parm;
#ifdef USE_DNNC
	wtk_dnnc_t *dnnc;
#endif
	wtk_fst_net_t *custom_net;
	wtk_fst_net_t *res_net;
	wtk_wfstr_t *rec;
	wtk_usrec_t *usrec;
	wtk_rescore_t *rescore;
	wtk_wfstdec_output_t *output;
	wtk_lockhoard_t evt_hoard; //wtk_fst_evt_t
	wtk_vad_t *vad;
	wtk_vframe_state_t last_vframe_state;
	wtk_cfg_file_t *env_parser;
	wtk_wfstenv_cfg_t env;
	wtk_fst_net2_t *input_net;
	//--------------------- thread section -------------
	wtk_thread_t thread;
	wtk_blockqueue_t rec_input_q;
	wtk_blockqueue_t feature_bak_q;
	wtk_sem_t rec_wait_sem;
	wtk_sem_t rec_start_wait_sem;

	wtk_strbuf_t *hint_buf;
	void *notify_ths;
	wtk_wfstdec_notify_f notify;
	double time_start;
	double time_stop;
	double time_rec;
	int delay_frame;

	unsigned int wav_bytes;
	unsigned int rec_wav_bytes;
	unsigned int is_end:1;
	unsigned int start_sil:1;
	unsigned int start:1;
	unsigned int rec_notify_end:1;
	unsigned int run:1;
};

wtk_wfstdec_t* wtk_wfstdec_new(wtk_wfstdec_cfg_t *cfg);
void wtk_wfstdec_delete(wtk_wfstdec_t *d);
void wtk_wfstdec_reset(wtk_wfstdec_t *d);
int wtk_wfstdec_start(wtk_wfstdec_t *d);
int wtk_wfstdec_start2(wtk_wfstdec_t *d,char *env,int env_bytes);
int wtk_wfstdec_set_usr_rescore(wtk_wfstdec_t *d,char *net,int net_bytes);
int wtk_wfstdec_feed(wtk_wfstdec_t *d,char *data,int bytes,int is_end);
int wtk_wfstdec_feed2(wtk_wfstdec_t *d,char *data,int bytes,int wait_eof,int is_end);
int wtk_wfstdec_wait_end(wtk_wfstdec_t *d,int timeout);
void wtk_wfstdec_get_result(wtk_wfstdec_t *d,wtk_string_t *v);
void wtk_wfstdec_get_str_result(wtk_wfstdec_t *d,wtk_string_t *v);
void wtk_wfstdec_get_hint_result(wtk_wfstdec_t *d,wtk_string_t *v);
void wtk_wfstdec_set_sil_result(wtk_wfstdec_t *d,char *data,int bytes);
void wtk_wfstdec_print(wtk_wfstdec_t *d);
void wtk_wfstdec_print_mlf(wtk_wfstdec_t *d,FILE *log);
void wtk_wfstdec_write_lat(wtk_wfstdec_t *d,char *fn);
int wtk_wfstdec_bytes(wtk_wfstdec_t *d);
void wtk_wfstdec_update_vad_restart_result(wtk_wfstdec_t *d);
wtk_string_t* wtk_wfstdec_flush_result(wtk_wfstdec_t *d);
int wtk_wfstdec_get_cur_result(wtk_wfstdec_t *d,wtk_string_t *v);
void wtk_wfstdec_set_notify(wtk_wfstdec_t *d,void *ths,wtk_wfstdec_notify_f notify);

void wtk_wfstdec_set_custom_net(wtk_wfstdec_t *d,wtk_fst_net_t *net);
float wtk_wfstdec_get_conf(wtk_wfstdec_t *d);
int wtk_wfstdec_is_forceout(wtk_wfstdec_t *d);
void wtk_wfstdec_print_path(wtk_wfstdec_t *d);
int wtk_wfstdec_can_be_end(wtk_wfstdec_t *d,wtk_string_t* v);

#ifdef __cplusplus
};
#endif
#endif
