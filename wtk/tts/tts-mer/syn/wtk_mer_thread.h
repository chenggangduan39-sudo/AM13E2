#ifndef WTK_MER_THREAD_H_
#define WTK_MER_THREAD_H_
#include "tts-mer/wtk_mer_common.h"
#include "tts-mer/syn/wtk_mer_wav.h"
#include <unistd.h>
#ifdef __cplusplus
extern "C" {
#endif

int wtk_mer_get_cpu_core_num();
int wtk_mer_thread_wav_process(pthread_t *tid, int *m/* 当前线程号 */, wtk_matf_t **f0_mf2, wtk_matf_t **bap_mf2, wtk_matf_t **mgc_mf2);
int wtk_mer_thread_act_process(pthread_t *tid, int *m/* 当前线程号 */, wtk_mer_wav_t **hash2);

#ifdef __cplusplus
}
#endif
#endif
