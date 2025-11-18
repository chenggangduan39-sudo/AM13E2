#ifndef WTK_ASR_WDEC_WTK_WDEC_NET_CFG
#define WTK_ASR_WDEC_WTK_WDEC_NET_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/wtk_wfstdec.h"
#include "wtk/asr/model/wtk_dict.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wdec_net_cfg wtk_wdec_net_cfg_t;

typedef struct wtk_wdec_words_set wtk_wdec_words_set_t;

struct wtk_wdec_words_set
{
	char* word;
	int label;
	float *min_wrd_conf;
	float min_conf;
};

struct wtk_wdec_net_cfg
{
	wtk_hmmset_cfg_t hmmset;
	wtk_label_t *label;
	wtk_dict_t *dict;
	wtk_array_t *phn;
	char *dict_fn;
	char *word;
	wtk_array_t *words;
	wtk_heap_t *h;
	unsigned use_words_set:1;
	unsigned use_cross:1;
	unsigned use_sil:1;
	wtk_wdec_words_set_t *set;
	int n_words;
};

int wtk_wdec_net_cfg_init(wtk_wdec_net_cfg_t *cfg);
int wtk_wdec_net_cfg_clean(wtk_wdec_net_cfg_t *cfg);
int wtk_wdec_net_cfg_update_local(wtk_wdec_net_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wdec_net_cfg_update2(wtk_wdec_net_cfg_t *cfg,wtk_source_loader_t *sl);
void wtk_wdec_net_cfg_set_words(wtk_wdec_net_cfg_t *cfg, char *words,int n);
#ifdef __cplusplus
};
#endif
#endif
