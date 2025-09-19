#ifndef WTK_ASR_WDEC_WTK_WDEC_NET
#define WTK_ASR_WDEC_WTK_WDEC_NET
#include "wtk/core/wtk_type.h" 
#include "wtk/asr/model/wtk_dict.h"
#include "wtk/core/wtk_larray.h"
#include "wtk_wdec_net_cfg.h"
#include "wtk/core/wtk_queue3.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wdec_net wtk_wdec_net_t;

typedef struct wtk_wdec_arc wtk_wdec_arc_t;
typedef struct wtk_wdec_node wtk_wdec_node_t;

struct wtk_wdec_arc
{
	wtk_queue_node_t input_n;
	wtk_queue_node_t output_n;
	wtk_wdec_node_t *pre;
	wtk_wdec_node_t *nxt;
	union
	{
		wtk_dict_phone_t *phn;
		wtk_hmm_t *hmm;
	}v;
	wtk_queue3_t inst_q;
	int label;
};

struct wtk_wdec_node
{
	wtk_queue3_t input_q;
	wtk_queue3_t output_q;
	wtk_dict_word_t *word;
};


struct wtk_wdec_net
{
	wtk_wdec_net_cfg_t *cfg;
	wtk_heap_t *heap;
	wtk_wdec_node_t *phn_start;
	wtk_wdec_node_t *phn_end;
	wtk_wdec_node_t *sil_end;
	wtk_wdec_node_t *raw_start;
	int label_set[5];
};


wtk_wdec_net_t* wtk_wdec_net_new(wtk_wdec_net_cfg_t *cfg);
void wtk_wdec_net_delete(wtk_wdec_net_t *net);
void wtk_wdec_net_reset(wtk_wdec_net_t *net);

void wtk_wdec_node_print_pre(wtk_wdec_node_t *node);
void wtk_wdec_node_print_phn_pre(wtk_wdec_node_t *node);
void wtk_wdec_node_print_phn_nxt(wtk_wdec_node_t *node);
#ifdef __cplusplus
};
#endif
#endif
