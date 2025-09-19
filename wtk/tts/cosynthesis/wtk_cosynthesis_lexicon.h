#ifndef __WTK_COSYN_LEX_H__
#define __WTK_COSYN_LEX_H__
#include "wtk_cosynthesis_lexicon_cfg.h"
#include "wtk/core/wtk_vpool2.h"
#include <ctype.h>

#include "wtk_cosynthesis_output.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    WTK_COSYN_OUT_DICT=0,
    WTK_COSYN_MATCH_SNT,
    WTK_COSYN_PROCESS_ED_IN,
    WTK_COSYN_PROCESS_ED_OUT,
    WTK_COSYN_ERR,
	WTK_COSYN_NOTHING
}wtk_cosyn_cmt_t;

typedef enum
{
    NOTHING=0,
    NORMAL,
    ADD,
    DEL
}wtk_process_t;

typedef enum
{
    WTK_ED_SKIP=0,
    WTK_ED_ADD,
    WTK_ED_DEL,
    WTK_ED_SUB,
    WTK_ED_INIT
}wtk_ed_cmt_t;

typedef struct
{
    uint8_t val;
    wtk_ed_cmt_t cmd;
}wtk_ed_node_t;

typedef struct
{
    // uint32_t snt_id;
    wtk_ed_node_t **node;
}wtk_ed_t;

typedef struct 
{
    wtk_queue_node_t q_n;
    uint32_t snt_id;
    uint8_t ed;
    uint8_t* path;
    uint8_t path_len;
}wtk_ed_path_t;

typedef struct 
{
    wtk_string_t *left_phone;
    wtk_string_t *right_phone;
    wtk_string_t *left_type;
    wtk_string_t *right_type;
    wtk_string_t **flabel;
    wtk_string_t *sil_flabel;
    wtk_string_t *left_phone_nosil;
    wtk_string_t *right_phone_nosil;
    int nphone;
}wtk_flabel_feats_t;

typedef struct 
{
    uint16_t word_id;
    uint16_t *unit_id;
    int nunit;
    wtk_queue_t wrd_q;
    wtk_flabel_feats_t *finfo;
    wtk_process_t need_process;
    int sil_tot_dur;   //number of frame
    float *select_cost;
    float **conca_cost;
    float **sil_conca_cost;
//    float **conca_cost_inset;
}wtk_cosynthesis_lexicon_input_t;

typedef struct 
{
    wtk_cosynthesis_lexicon_cfg_t *cfg;
    wtk_cosynthesis_lexicon_input_t *input;
    int ninput;
    int match_snt_idx;
    wtk_ed_t *ed;
    wtk_heap_t *heap;
    wtk_vpool2_t *ed_path_pool;
    uint8_t min_edit_distance;
    uint8_t* min_path;
    int* min_path_map;
    int min_path_len;
    wtk_queue_t *ed_pathq;
    char *word_seq[50];
    wtk_word_t *wrds;              //only valid in every input.
    wtk_cosynthesis_output_t *output;

    //for Concurrent.
    FILE *g_audio_res_fp;
    FILE *g_feat_res_fp;

}wtk_cosynthesis_lexicon_t;

wtk_cosynthesis_lexicon_t *wtk_cosynthesis_lexicon_new(wtk_cosynthesis_lexicon_cfg_t *cfg);
void wtk_cosynthesis_lexicon_delete(wtk_cosynthesis_lexicon_t *lex);
void wtk_cosynthesis_lexicon_reset(wtk_cosynthesis_lexicon_t *lex);
int wtk_cosynthesis_lexicon_process(wtk_cosynthesis_lexicon_t *lex, char* data,int len);
int get_word_index(wtk_cosynthesis_lexicon_t *lex,uint16_t word_id);
int wtk_cosynthesis_lexicon_txtparser(wtk_cosynthesis_lexicon_t *lex, char* data,int len);
short* wtk_cosynthesis_lexicon_getaudio(wtk_cosynthesis_lexicon_t* lex, wtk_unit_t *unit, int *len);

void wtk_cosynthesis_lexicon_loadinput(wtk_cosynthesis_lexicon_t *lex);
void wtk_cosynthesis_lexicon_cleaninput(wtk_cosynthesis_lexicon_t *lex);
#ifdef __cplusplus
};
#endif

#endif
