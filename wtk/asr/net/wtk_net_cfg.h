#ifndef WTK_DECODER_NET_WTK_NET_CFG_H_
#define WTK_DECODER_NET_WTK_NET_CFG_H_
#include "wtk/asr/model/wtk_dict.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_net_cfg wtk_net_cfg_t;
struct wtk_net_cfg
{
	unsigned factor_lm:1;				//factor lm likelihoods throughout words
	unsigned allow_xwrd_exp:1;			//allow context exp across words.
	unsigned allow_ctx_exp:1;			//allow context expr to get model names.
	unsigned force_ctx_exp:1;			//FORCECXTEXP, force triphone context expr to get model names.
	unsigned force_right_biphones:1;	//force biphone context expr to get model name.
	unsigned force_left_biphones:1;
	unsigned sp_word_boundary:1;						//cfWordBoundary
};

int wtk_net_cfg_init(wtk_net_cfg_t *cfg);
int wtk_net_cfg_update_local(wtk_net_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_net_cfg_update(wtk_net_cfg_t *cfg);
int wtk_net_cfg_clean(wtk_net_cfg_t *cfg);
void wtk_net_cfg_print(wtk_net_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
