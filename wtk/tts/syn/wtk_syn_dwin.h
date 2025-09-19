#ifndef WTK_TTS_SYN_WTK_SYN_DWIN
#define WTK_TTS_SYN_WTK_SYN_DWIN
#include "wtk/core/wtk_type.h" 
#include "wtk_syn_dwin_cfg.h"
#include "wtk/core/wtk_strpool.h"
#include "wtk_syn_def.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_dwin wtk_syn_dwin_t;

struct wtk_syn_dwin
{
	wtk_syn_dwin_cfg_t *cfg;
	wtk_heap_t *heap;
	int **width;
	wtk_syn_float_t **coef;
	int maxl;
	int num;
};

wtk_syn_dwin_t* wtk_syn_dwin_new(wtk_syn_dwin_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);
void wtk_syn_dwin_delete(wtk_syn_dwin_t *w);
#ifdef __cplusplus
};
#endif
#endif
