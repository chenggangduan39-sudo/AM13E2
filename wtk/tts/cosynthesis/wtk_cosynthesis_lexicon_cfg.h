#ifndef __WTK_COSYN_LEX_CFG_H__
#define __WTK_COSYN_LEX_CFG_H__
#include <stdio.h>
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_larray.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    uint16_t unit_id;
    char *data;
    int audio_pos;     //audio position in corpus.
    int data_len;      //audio length in corpus.
    int raw_audio_len; //raw audio length
//    int cur_data_len;  //current audio length.
    int is_compress;
    int shifit;
    int nphone;
    float *lf0;
    float *dur;
    float *spec;
    int *lf0_idx;
    int *dur_idx;
    int *spec_idx;
    float *kld_lf0;
    float *spec_l;
    float *spec_r;
    float tcost;
    float sil_prev_l;      //last word unit id is sil?
}wtk_unit_t;

typedef struct
{
    char *word;
    int word_len;
    uint16_t word_id;
    uint16_t nunit;
    wtk_unit_t *unit;
    int unit_pos;      //unit start-position in unit file.
    int feat_pos;      //unit start-position in feat file.
}wtk_word_t;

typedef struct
{
    uint16_t *raw_word_ids;
    uint16_t *unit_ids;
    uint16_t *nosil_word_ids;
    uint8_t raw_word_cnt;
    uint8_t nosil_word_cnt;
}wtk_snt_t;

typedef struct 
{
    int lf0_len;
    int dur_len;
    int spec_len;
    int hmm_lf0_len;
    int hmm_dur_len;
    int hmm_mcep_len;
    int spec_llen;
}
wtk_cosynthesis_feat_cfg_t;

typedef struct 
{
	wtk_rbin2_t *rbin;
    char *word_fn;
    char *snt_fn;
    char *unit_fn;
    char *feat_fn;
    int nwrds;
    int nsnts;
    wtk_cosynthesis_feat_cfg_t feat_cfg;
    uint16_t *snt_wrdlen_split_idx;
    int nsnt_wrdlen;
    wtk_word_t *wrds;
    wtk_snt_t *snts;
    int max_edit_distance;
    unsigned use_snt_wrdlen_split_idx :1;
    uint16_t sil_id;
    int samples;

    //snt and word info control
    int maxn_wrds;

    unsigned char *g_snt_res;
    unsigned char *g_wrd_res;
    unsigned char *g_audio_res;
    unsigned char *g_feat_res;

    FILE *g_snt_res_fp;
    FILE *g_wrd_res_fp;
    FILE *g_audio_res_fp;
    FILE *g_feat_res_fp;
    unsigned int use_fp:1;
}wtk_cosynthesis_lexicon_cfg_t;

int wtk_cosynthesis_lexicon_cfg_init(wtk_cosynthesis_lexicon_cfg_t *cfg);
int wtk_cosynthesis_lexicon_cfg_update_local(wtk_cosynthesis_lexicon_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_cosynthesis_lexicon_cfg_clean(wtk_cosynthesis_lexicon_cfg_t *cfg);
int wtk_cosynthesis_lexicon_cfg_update(wtk_cosynthesis_lexicon_cfg_t *cfg);
int wtk_cosynthesis_lexicon_cfg_update2(wtk_cosynthesis_lexicon_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_cosynthesis_lexicon_cfg_update_file(wtk_cosynthesis_lexicon_cfg_t *cfg);
#ifdef __cplusplus
};
#endif

#endif
