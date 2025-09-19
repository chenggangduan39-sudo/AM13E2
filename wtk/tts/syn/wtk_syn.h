#ifndef WTK_TTS_SYN_WTK_SYN
#define WTK_TTS_SYN_WTK_SYN
#include "wtk/core/wtk_type.h" 
#include "wtk_syn_pstream.h"
#include "wtk/tts/parser/wtk_tts_def.h"
#include "wtk_syn_cfg.h"
#include "wtk_syn_sigp.h"
#include "wtk_syn_lc.h"
#include "wtk/core/wtk_os.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn wtk_syn_t;




struct wtk_syn
{
	wtk_syn_cfg_t *cfg;
	wtk_heap_t *heap;
	wtk_heap_t *glb_heap;
	wtk_syn_lc_t *lc;
	wtk_syn_qs_item_t *fixf0_item;
	wtk_syn_sigp_t* sigp;
	int snt_idx;
	wtk_queue_node_t *syn_cur;
	wtk_queue_node_t *syn_nxt;
	wtk_strbuf_t *buf;
	unsigned is_snt_end:1;
};


wtk_syn_t* wtk_syn_new(wtk_syn_cfg_t *cfg);
void wtk_syn_delete(wtk_syn_t *s);
void wtk_syn_reset(wtk_syn_t *s);
void wtk_syn_set_volume_scale(wtk_syn_t *s,float scale);
int wtk_syn_process(wtk_syn_t *s,wtk_tts_lab_t *lab);
int wtk_syn_get_cur_snt_index(wtk_syn_t *s);
int wtk_syn_process_snt(wtk_syn_t *s,wtk_tts_snt_t *snt,float rho);
void wtk_syn_test(wtk_syn_t *s);
wtk_string_t wtk_syn_get_cur_timeinfo(wtk_syn_t *s);
int wtk_syn_bytes(wtk_syn_t* s);
#ifdef __cplusplus
};
#endif
#endif
