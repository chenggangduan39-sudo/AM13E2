#ifndef WTK_CLU_H
#define WTK_CLU_H
#include "wtk/asr/kws/clu/wtk_clu_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_str.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_clu wtk_clu_t;
typedef struct wtk_clu_vprint_data wtk_clu_vprint_data_t;
typedef void (*wtk_clu_notify_f)(void *ths, int num, int len, float *embed);

struct wtk_clu_vprint_data {
    int num;
    int size;
    int *mean;
    float *data;
};

struct wtk_clu {
    wtk_clu_cfg_t *cfg;
    wtk_strbuf_t *wav;
    wtk_strbuf_t *two_s_buffer;
    wtk_strbuf_t **vprint_idx;
    wtk_strbuf_t **result;
    wtk_clu_vprint_data_t *vprint_data;
    wtk_nnet3_xvector_compute_t *x;
    qtk_clustering_spectral_t *spec;
    void *notify_ths;
    wtk_clu_notify_f notify;
    int win_l;
    int vprint_idx_num;
};

wtk_clu_t *wtk_clu_new(wtk_clu_cfg_t *cfg);
void wtk_clu_delete(wtk_clu_t *clu);
void wtk_clu_reset(wtk_clu_t *clu);
void wtk_clu_start(wtk_clu_t *clu);
void wtk_clu_feed(wtk_clu_t *clu, char *data, int len, int is_end);
void wtk_clu_set_notify(wtk_clu_t *clu, wtk_clu_notify_f notify, void *ths);
void wtk_clu_get_result(wtk_clu_t *clu);
/*
 * if call feed end, oracle_num is set to zero
 */
void wtk_clu_set_oracle_num(wtk_clu_t *clu, int oracle_num);
#ifdef __cplusplus
};
#endif
#endif
