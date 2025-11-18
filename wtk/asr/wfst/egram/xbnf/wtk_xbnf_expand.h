#ifndef WTK_FST_EGRAM_XBNF_WTK_XBNF_EXPAND
#define WTK_FST_EGRAM_XBNF_WTK_XBNF_EXPAND
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_larray.h"
#include "wtk_xbnf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_xbnf_expand wtk_xbnf_expand_t;

typedef struct  wtk_xbnf_expand_pth wtk_xbnf_expand_pth_t;

struct wtk_xbnf_expand_pth
{
	wtk_queue_node_t q_n;
	//wtk_xbnf_expand_pth_t *prev;
	wtk_string_t *str;
};

typedef struct wtk_xbnf_expand_inst wtk_xbnf_expand_inst_t;

struct wtk_xbnf_expand_inst
{
	wtk_xbnf_expand_inst_t *prev;
	wtk_queue_t pth_q;
};

struct wtk_xbnf_expand
{
	wtk_xbnf_t *xbnf;
	wtk_heap_t *heap;
	wtk_queue_t inst_q;
};

wtk_xbnf_expand_t* wtk_xbnf_expand_new(wtk_xbnf_t *xbnf);
void wtk_xbnf_expand_delete(wtk_xbnf_expand_t *expand);

void wtk_xbnf_expand_process(wtk_xbnf_expand_t *e);

void wtk_xbnf_expand_process2(wtk_xbnf_t *x);
#ifdef __cplusplus
};
#endif
#endif
