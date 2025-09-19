#ifndef WTK_FST_RESCORE_WTK_RESCORE_H_
#define WTK_FST_RESCORE_WTK_RESCORE_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/wfst/wtk_wfstenv_cfg.h"
#include "wtk_rescore_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rescore wtk_rescore_t;

struct wtk_rescore
{
	wtk_rescore_cfg_t *cfg;
	wtk_lm_rescore_t *lm;
	wtk_strbuf_t *buf;
	wtk_fst_net2_t *output_net;
};

wtk_rescore_t* wtk_rescore_new(wtk_rescore_cfg_t *cfg,wtk_wfstenv_cfg_t *env);
void wtk_rescore_delete(wtk_rescore_t *r);
void wtk_rescore_reset(wtk_rescore_t *r);
int wtk_rescore_bytes(wtk_rescore_t *r);
void wtk_rescore_clean_hist(wtk_rescore_t *r);
int wtk_rescore_process(wtk_rescore_t *r,wtk_fst_net2_t *input_net);
void wtk_rescore_get_result(wtk_rescore_t *r,wtk_string_t *v,char *sep,int sep_bytes);
void wtk_rescore_get_result2(wtk_rescore_t *r,wtk_strbuf_t *buf,char *sep,int sep_bytes);
wtk_fst_rec_trans_t* wtk_rescore_get_nbest(wtk_rescore_t *r,wtk_heap_t *heap,int nbest,char *sep,int bytes);
#ifdef __cplusplus
};
#endif
#endif
