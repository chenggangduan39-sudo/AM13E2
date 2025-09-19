#ifndef WTK_MER_STATE_ALIGN_H_
#define WTK_MER_STATE_ALIGN_H_
#include "tts-mer/wtk_mer_common.h"
#include "tts-mer/cfg/wtk_mer_cfg_syn.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    wtk_stridx_t *continous;
    wtk_larray_t *discrete_larr;
} disc_conti_t;

void wtk_mer_lab_split_call(wtk_strbuf_t **str_arr, char *item, int len, int index);
int wtk_mer_check_sil( char *lab);
wtk_matf_t* wtk_mer_state_align(wtk_mer_syn_qes_t *qes, char *subphn_feat, wtk_strbuf_t ***lab_arrs, int lab_len);
void wtk_mer_dur_to_lab(wtk_matf_t *dur_matf, wtk_strbuf_t ***lab_arrs, int lab_len);

#ifdef __cplusplus
}
#endif
#endif
