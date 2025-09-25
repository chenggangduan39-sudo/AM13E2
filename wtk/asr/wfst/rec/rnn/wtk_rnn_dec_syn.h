#ifndef WTK_FST_REC_RNN_WTK_RNN_DEC_SYN
#define WTK_FST_REC_RNN_WTK_RNN_DEC_SYN
#include "wtk/core/wtk_type.h" 
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rnn_dec_syn wtk_rnn_dec_syn_t;

typedef struct
{
	wtk_matf_t *input_wrd;	//|voc*hid|  |1*voc|*|voc*hid|=|1*hid|
	wtk_matf_t *input_hid;	//|hid*hid|  |1*hid|*|hid*hid|=|1*hid|
	wtk_matf_t *output_wrd;	//|voc*hid|  |voc*hid|*|hid*1|=|voc*1|
	int hid_size;
	int voc_size;
}wtk_rnn_dec_fsyn_t;

typedef struct
{
	wtk_matb_t *input_wrd;
	wtk_matb_t *input_hid;
	wtk_matb_t *output_wrd;
	int hid_size;
	int voc_size;
}wtk_rnn_dec_fix_syn_t;


struct wtk_rnn_dec_syn
{
	wtk_rnn_dec_fsyn_t *fsyn;
	wtk_rnn_dec_fix_syn_t *fix;
};


wtk_rnn_dec_syn_t* wtk_rnn_dec_syn_new(int use_fix,char *fn,wtk_source_loader_t *sl);
void wtk_rnn_dec_syn_delete(wtk_rnn_dec_syn_t *syn);
void wtk_rnn_dec_syn_write_fix(wtk_rnn_dec_syn_t *syn,char *fn);
#ifdef __cplusplus
};
#endif
#endif
