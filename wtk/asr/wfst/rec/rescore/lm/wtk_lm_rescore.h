#ifndef WTK_FST_LM_WTK_LM_RESCORE_H_
#define WTK_FST_LM_WTK_LM_RESCORE_H_
#include "wtk/core/wtk_type.h"
#include "wtk_lm_rescore_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/asr/wfst/wtk_wfstenv_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lm_rescore wtk_lm_rescore_t;

struct wtk_lm_rescore
{
	wtk_lm_rescore_cfg_t *cfg;
	wtk_lmlat_t *main_lm;
	wtk_relm_t **custom_lm;
	wtk_relm_t *cur_custom_lm;
	wtk_lmlat_t *post_lm;
	wtk_fst_net2_t *output_net;
	wtk_wfstenv_cfg_t *env;
	unsigned use_custom:1;
};

wtk_lm_rescore_t*  wtk_lm_rescore_new(wtk_lm_rescore_cfg_t *cfg,
		wtk_wfstenv_cfg_t *env);//,wtk_fst_net_cfg_t *input_net_cfg);
void wtk_lm_rescore_delete(wtk_lm_rescore_t *r);
void wtk_lm_rescore_reset(wtk_lm_rescore_t *r);
int wtk_lm_rescore_bytes(wtk_lm_rescore_t *r);
int wtk_lm_rescore_update_history(wtk_lm_rescore_t *r,int *idx,int nid);
void wtk_lm_rescore_clean_hist(wtk_lm_rescore_t *r);
/**
 *	@brief
 *		* 如果use_dict=1;input中state->hook被设置，使用wtk_fst_net2_clean_hook2(input);
 *		* 如果use_dict=0;input中state->hook,trans->hook2被设置wtk_fst_net2_clean_hook(input);
 */
int wtk_lm_rescore_process(wtk_lm_rescore_t *r,wtk_fst_net2_t *input);
void wtk_lm_rescore_print(wtk_lm_rescore_t *r);
#ifdef __cplusplus
};
#endif
#endif
