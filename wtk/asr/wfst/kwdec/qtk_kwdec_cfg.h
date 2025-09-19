#ifndef QTK_KWFST_DEC_H_
#define QTK_KWFST_DEC_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk_wakeup_trans_model_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/core/segmenter/wtk_prune_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net3_cfg.h"
#include "wtk/asr/wfst/rec/rescore/wtk_rescore_cfg.h"


#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_kwdec_cfg qtk_kwdec_cfg_t;
typedef struct qtk_kwdec_words_set qtk_kwdec_words_set_t;

struct qtk_kwdec_words_set
{
	char *word;
	int key_id;
	float pdf_conf;
	int min_kws;
};

struct qtk_kwdec_cfg
{
	//qtk_trans_model_cfg_t trans_model;
	wtk_fst_net_cfg_t net;
	//wtk_prune_cfg_t prune;

	int size;
	int use_prune;
	float beam;
	int max_active;
	int min_active;
	int prune_interval;
	float beam_delta;
	float lattice_beam;
	float prune_scale;
	float lm_scale;
	unsigned int add_softmax:1;
	unsigned int use_wdec:1;
    int wdec_post_win_size;
    float ac_scale;
    int min_kws;
    float pdf_conf;
    int sil_frame;
	int default_key_id;
	qtk_kwdec_words_set_t *set;
	wtk_array_t *words;
	int n_networds;
	unsigned use_words_set;
	wtk_heap_t *h;
	int reset_time;
};

int qtk_kwdec_cfg_init(qtk_kwdec_cfg_t *cfg);
int qtk_kwdec_cfg_clean(qtk_kwdec_cfg_t *cfg);
int qtk_kwdec_cfg_update_local(qtk_kwdec_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_kwdec_cfg_update(qtk_kwdec_cfg_t *cfg,wtk_source_loader_t *sl);
void qtk_kwdec_cfg_print(qtk_kwdec_cfg_t *cfg);
void qtk_kwdec_cfg_words_set(qtk_kwdec_cfg_t *cfg, char **words,int n);

#ifdef __cplusplus
};
#endif
#endif
