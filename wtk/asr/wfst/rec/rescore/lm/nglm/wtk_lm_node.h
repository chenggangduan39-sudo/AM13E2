#ifndef WTK_FST_LM_WTK_LM_NODE_H_
#define WTK_FST_LM_WTK_LM_NODE_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_sort.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/asr/wfst/net/sym/wtk_fst_insym.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lm_node wtk_lm_node_t;

typedef enum
{
	WTK_LM2BIN_A,
	WTK_LM2BIN_B,
	WTK_LM2BIN_C,
	WTK_LM2BIN_a,
}wtk_lm2bin_type_t;

struct wtk_lm_node
{
	wtk_lm_node_t *parent;
	wtk_lm_node_t **childs;
	uint64_t child_offset;
	uint64_t self_offset;
	unsigned int nchild;
	unsigned int id;
	float prob;
	float bow;
	unsigned short ngram;
};

int wtk_lm2bin_type_bytes(wtk_lm2bin_type_t type);
void wtk_lm2bin_from_bin(wtk_lm2bin_type_t type,wtk_lm_node_t *n,char *data,float ps,float bs);
void wtk_lm2bin_from_bin_end(wtk_lm2bin_type_t type,wtk_lm_node_t *n,char *data,float ps);

void wtk_lm_node_init(wtk_lm_node_t *node);

wtk_lm_node_t* wtk_lm_node_find_child(wtk_lm_node_t *node,unsigned int idx);

//17+28 18+18  81
/*
          6位有效数字       6位有效数字            5位有效数字     下一位偏移(137438953472) 128G
         id(1048576)      prob(1+20:131072)     bow(1+17:131072)     offset
    96bit = 20bit     +   21bit      +         18bit        +  37bit

    20+1+20+1+17+37
*/
void wtk_lm_node_to_bin(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps,float bs);

/**
 *	low-level order: 12bytes
 */
void wtk_lm_node_from_bin(wtk_lm_node_t *n,char *data,int len,float ps,float bs);


//10 byte
void wtk_lm_node_to_bin_small(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps,float bs);

//9 byte
void wtk_lm_node_to_bin_small2(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps,float bs);

void wtk_lm_node_from_bin_small2(wtk_lm_node_t *n,char *data,int len,float ps,float bs);

void wtk_lm_node_from_bin_small(wtk_lm_node_t *n,char *data,int len,float ps,float bs);

void wtk_lm_node_to_bin_samll_end(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps);

void wtk_lm_node_from_bin_samll_end(wtk_lm_node_t *n,char *data,int len,float ps);

void wtk_lm_node_to_bin_samll_end2(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps);

void wtk_lm_node_from_bin_samll_end2(wtk_lm_node_t *n,char *data,int len,float ps);

/*
 * max order:
 * id+prob(20bit+20bit)
          6位有效数字       5位有效数字
         id(1048576)      prob(1+19:524288)
    96bit = 20bit     +   20bit

    20+1+20+1+17+37
*/
void wtk_lm_node_to_bin2(wtk_lm_node_t *n,wtk_strbuf_t *buf,float ps);


/**
 *	high-level order:5 bytes;
 */
void wtk_lm_node_from_bin2(wtk_lm_node_t *n,char *data,int len,float ps);

int wtk_lm_node_trace_id(wtk_lm_node_t *n,int *ids);

wtk_lm_node_t* wtk_lm_node_root(wtk_lm_node_t *n);

void wtk_lm_node_tostring(wtk_fst_insym_t *sym,wtk_lm_node_t *n,wtk_strbuf_t *buf);
void wtk_lm_node_print_parent(wtk_lm_node_t *n);
void wtk_lm_node_print_parent2(wtk_fst_insym_t *sym,wtk_lm_node_t *n);
void wtk_lm_node_print_child(wtk_fst_insym_t *sym,wtk_lm_node_t *n);
void wtk_lm_node_print(wtk_lm_node_t *n);
void wtk_lm_node_print2(wtk_fst_insym_t *sym,wtk_lm_node_t *n);
#ifdef __cplusplus
};
#endif
#endif
