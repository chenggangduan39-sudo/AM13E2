#ifndef WTK_TAC_LPCNET_H
#define WTK_TAC_LPCNET_H
#include "tts-tac/wtk_tac_common.h"
#include "tts-tac/model/wtk_tac_wavrnn.h"
#include "tts-tac/cfg/wtk_tac_cfg_syn_lpcnet.h"
// #include "tts-tac/wtk_tac_tts.h"
#ifdef __cplusplus
extern "C" {
#endif

void wtk_tac_lpcnet_process(wtk_tac_cfg_syn_lpcnet_t *cfg, wtk_matf_t *mel, wtk_mer_wav_stream_t *wav,int is_end);
void wtk_tac_lpcnet_set_notify(wtk_tac_cfg_syn_lpcnet_t *cfg, wtk_lpcnet_notify_f notify, void *user_data);
void wtk_tac_lpcnet_process_stream(wtk_tac_cfg_syn_lpcnet_t *cfg, wtk_matf_t *mel,int is_end);

#ifdef __cplusplus
}
#endif
#endif
