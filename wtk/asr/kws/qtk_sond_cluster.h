#ifndef QTK_SOND_CLUSTER_H_
#define QTK_SOND_CLUSTER_H_
#include "qtk_sond_cluster_cfg.h"
#include "wtk/core/wtk_median_filter.h"
//#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_sond_cluster qtk_sond_cluster_t;
typedef void (*qtk_sond_cluster_res_notify_f)(void *ths,int res,char *name,int name_len);


struct qtk_sond_cluster
{
	qtk_sond_cluster_cfg_t *cfg;
	qtk_sond_cluster_res_notify_f notify;
	void * notify_ths;
    wtk_kxparm_t *parm;
    wtk_kvad_t *vad;
    wtk_svprint_t *svprint;
	wtk_clu_t *clu;
	wtk_strbuf_t *user;
	wtk_strbuf_t *wav_buf;
	wtk_strbuf_t *feat;
	wtk_strbuf_t **spk_feat;
	wtk_strbuf_t *result_buf;
	wtk_strbuf_t **filter_feat2;
	wtk_string_t* wav;
	wtk_string_t* name[12];//max 10 speakers

	wtk_strbuf_t *vad_buf;
	wtk_strbuf_t *border_buf;
	wtk_strbuf_t *wav_cache;

#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx;
	qtk_onnx_item_t *fin_items;
	qtk_onnx_item_t *in_items;
#endif
	// wtk_wavfile_t *log_wav;
	float timestamp;
	int dframes;//done frames
	int enroll;
	int eval;
	int nframes;
	int frame;
	int first_chunk_len;
	int chunk_len;
	int wav_bytes;
	int clu_len;
	int spk_cnt;
	int frames;
	int emb_cnts;
	float frame_dur;
	float vad_plus;
    unsigned vad_state:1;
	unsigned end:1;
	unsigned has_tsvad:1;
	unsigned has_clu:1;
};

qtk_sond_cluster_t* qtk_sond_cluster_new(qtk_sond_cluster_cfg_t *cfg);
void qtk_sond_cluster_delete(qtk_sond_cluster_t *sond_cluster);
int qtk_sond_cluster_start(qtk_sond_cluster_t *sond_cluster);
int qtk_sond_cluster_reset(qtk_sond_cluster_t *sond_cluster);
int qtk_sond_cluster_feed(qtk_sond_cluster_t *sond_cluster,char *data,int bytes,int is_end);
void qtk_sond_cluster_get_result(qtk_sond_cluster_t* sond_cluster, wtk_string_t *v);
int qtk_sond_cluster_set_vad_time(qtk_sond_cluster_t *sond_cluster, float start, float end);

void qtk_sond_cluster_clean(qtk_sond_cluster_t *sond_cluster);
void qtk_sond_cluster_reload(qtk_sond_cluster_t *sond_cluster);
char* qtk_sond_cluster_get_fn(qtk_sond_cluster_t *sond_cluster);
int qtk_sond_cluster_set_enroll_fn(qtk_sond_cluster_t *sond_cluster,char *fn, int len);
int qtk_sond_cluster_prepare(qtk_sond_cluster_t* sond_cluster);
int qtk_sond_cluster_set_spk_nums(qtk_sond_cluster_t* sond_cluster, int cnt);
void  qtk_sond_cluster_logits_pre_compute(qtk_sond_cluster_t *sond_cluster,int shape, float *src,int cnt);

int qtk_sond_cluster_enroll(qtk_sond_cluster_t *sond_cluster,char *name,int len);
int qtk_sond_cluster_enroll_end(qtk_sond_cluster_t *sond_cluster);

#ifdef __cplusplus
};
#endif
#endif
