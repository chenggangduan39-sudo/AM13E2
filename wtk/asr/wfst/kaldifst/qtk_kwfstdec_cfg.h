#ifndef QTK_KWFST_DEC_H_
#define QTK_KWFST_DEC_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "qtk_trans_model_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/core/segmenter/wtk_prune_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net3_cfg.h"
#include "wtk/asr/wfst/rec/rescore/wtk_rescore_cfg.h"


#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_kwfstdec_cfg qtk_kwfstdec_cfg_t;

typedef struct
{
	int phn_id;
	int state_id;
}qtk_kwfstdec_trans2phn;

struct qtk_kwfstdec_cfg
{
	//qtk_trans_model_cfg_t trans_model;
	wtk_fst_net_cfg_t net;
	wtk_prune_cfg_t prune;
	wtk_fst_net3_cfg_t lat_net;
	wtk_rescore_cfg_t rescore;
	qtk_kwfstdec_trans2phn *trans2phn;
	int size;
	int use_prune;
	int remove_label;
	float beam;
	float laws_beam;
	int max_active;
	int min_active;
	int prune_interval;
	int frame_skip;
	int idle_filler_id;
	int norm_filler_id;
	int idle_hint;
	float idle_conf;
	float norm_conf;
	float beam_delta;
	float lattice_beam;
	float prune_scale;
	float lm_scale;
	float ac_scale;
	float hot_thresh;
	char *hot_words;
	char *trans_fn;
	float pool_scale;          //for pool mem control
	unsigned int use_rescore:1;
	unsigned int use_lat:1;
	unsigned int use_hot:1;//needs use_lat
	unsigned int add_softmax:1;
	unsigned int use_eval:1;
	unsigned int use_laws_beam : 1;
	unsigned int use_memctl:1;  //minimize memory using
	unsigned int use_context:1;  //context biasing
	unsigned int use_av_conf:1;  //context biasing
	unsigned int has_filler:1;
	unsigned int use_multi_filler:1;
};

int qtk_kwfstdec_cfg_init(qtk_kwfstdec_cfg_t *cfg);
int qtk_kwfstdec_cfg_clean(qtk_kwfstdec_cfg_t *cfg);
int qtk_kwfstdec_cfg_update_local(qtk_kwfstdec_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_kwfstdec_cfg_update(qtk_kwfstdec_cfg_t *cfg,wtk_source_loader_t *sl);
void qtk_kwfstdec_cfg_print(qtk_kwfstdec_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
