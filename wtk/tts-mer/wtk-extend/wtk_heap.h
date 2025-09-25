#ifndef WTK_MER_HEAP_H_
#define WTK_MER_HEAP_H_
#include "wtk/tts-mer/wtk-extend/wtk_tts_common.h"
#ifdef __cplusplus
extern "C" {
#endif

wtk_strbuf_t* wtk_strbuf_heap_new( wtk_heap_t *heap, int init_len,float rate);
wtk_mati_t* wtk_mati_heap_new(wtk_heap_t *heap, int row,int col);
wtk_matf_t* wtk_matf_heap_new(wtk_heap_t *heap, int row,int col);
wtk_matdf_t* wtk_matdf_heap_new(wtk_heap_t *heap, int row,int col);
wtk_vecf_t* wtk_mer_vecf_heap_new( wtk_heap_t *heap, int len);
wtk_veci_t* wtk_veci_heap_new( wtk_heap_t *heap, int len);

#ifdef __cplusplus
}
#endif
#endif