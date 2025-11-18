#ifndef WTK_DECODER_VITERBI_WTK_REC_CFG_H_
#define WTK_DECODER_VITERBI_WTK_REC_CFG_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/asr/wfst/rec/wtk_wfst_dnn_cfg.h"
#include "wtk/core/segmenter/wtk_prune.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rec_cfg wtk_rec_cfg_t;
struct wtk_rec_cfg
{
	//--------------- dnn section ---------
	wtk_wfst_dnn_cfg_t dnn;
	wtk_prune_cfg_t prune;
	//-------------- hlda section --------
	wtk_matrix_t *hlda_matrix;
	char *hlda_fn;
	//----------- rec section -------------
	short ntok;
	short nbest;
	float wordpen;		//word insertion penalty, like -p in hvite
	float pscale;		//Pronunciation probs scale factor.
	float lmscale;		//LM(Net probs) scale factor, like -s in hvite
	float word_beam;		//seperate word end beam width
	float gen_beam;		//global beam width,like -t in hvite
	float n_beam;		//beam width for non-best tokens.
	int path_coll_thresh;	//max path records created before collection
	int align_coll_thresh;
	int bit_heap_min;
	int bit_heap_max;
	float bit_heap_growf;
	unsigned char state:1;	//Keep track of state alignment, like -f in hvite;
	unsigned char model:1;	//Keep track of model alignment, like -m in hvite;
	unsigned char use_dnn:1;
	unsigned use_hlda_bin:1;
	unsigned use_prune:1;
};

int wtk_rec_cfg_init(wtk_rec_cfg_t* cfg);
int wtk_rec_cfg_clean(wtk_rec_cfg_t* cfg);
int wtk_rec_cfg_update_local(wtk_rec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_rec_cfg_update(wtk_rec_cfg_t *cfg);
int wtk_rec_cfg_update2(wtk_rec_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
