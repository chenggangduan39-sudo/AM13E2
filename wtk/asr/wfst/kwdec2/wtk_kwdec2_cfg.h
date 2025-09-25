#ifndef WTK_KWDEC2_CFG_H_
#define WTK_KWDEC2_CFG_H_
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/asr/wfst/egram/wtk_egram.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/asr/wfst/net/wtk_fst_net3_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk_kwdec2_mdl_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kwdec2_cfg wtk_kwdec2_cfg_t;
typedef struct wtk_kwdec2_words_set wtk_kwdec2_words_set_t;

struct wtk_kwdec2_words_set
{
	char *word;
	int key_id;
	float pdf_conf;//idle conf
	float pdf_conf2;
	int min_kws;
};

struct wtk_kwdec2_cfg
{
    wtk_kxparm_cfg_t parm;
	wtk_kwdec2_trans_model_cfg_t mdl;
	wtk_fst_net_cfg_t net;
	wtk_egram_cfg_t egram;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	int size;
	int use_prune;
	float beam;
	int max_active;
	int min_active;
	float beam_delta;
	unsigned int add_softmax:1;
    int wdec_post_win_size;
    float ac_scale;
    int min_kws;
    float pdf_conf;
	float plus_conf;
    int sil_frame;
	int default_key_id;
    char* mdl_fn;
	wtk_kwdec2_words_set_t *set;
	wtk_array_t *words;
	int n_networds;
	unsigned use_words_set;
	unsigned use_fixpoint;
	unsigned use_egram;
	wtk_heap_t *h;
	int reset_time;
	int shift;
	int ebnf_dump;
	union
	{
		wtk_main_cfg_t *main_cfg;
		wtk_mbin_cfg_t *bin_cfg;
	}cfg;
};


int wtk_kwdec2_cfg_init(wtk_kwdec2_cfg_t *cfg);
void wtk_kwdec2_cfg_clean_words_set(wtk_kwdec2_cfg_t *cfg);
int wtk_kwdec2_cfg_clean(wtk_kwdec2_cfg_t *cfg);
int wtk_kwdec2_cfg_update_local(wtk_kwdec2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_kwdec2_cfg_update(wtk_kwdec2_cfg_t *cfg);
int wtk_kwdec2_cfg_update2(wtk_kwdec2_cfg_t *cfg,wtk_source_loader_t *sl);
void wtk_kwdec2_cfg_words_set(wtk_kwdec2_cfg_t *cfg, char **words,int n);
int wtk_kwdec2_cfg_set_words_set(wtk_kwdec2_cfg_t *cfg, char *data, int len);
void wtk_kwdec2_cfg_delete(wtk_main_cfg_t *main_cfg);
void wtk_kwdec2_cfg_delete_bin(wtk_kwdec2_cfg_t *cfg);
void wtk_kwdec2_cfg_delete_bin2(wtk_kwdec2_cfg_t *cfg);
wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new_bin(char *bin_fn,char *cfg_fn);
wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new_bin2(char *bin_fn);
wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
wtk_kwdec2_cfg_t* wtk_kwdec2_cfg_new(char *fn);

#ifdef __cplusplus
};
#endif
#endif