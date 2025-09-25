#ifndef WTK_TTS_PARSER_WTK_MAXSEG
#define WTK_TTS_PARSER_WTK_MAXSEG
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_rbtree.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_maxseg_wrd_path wtk_maxseg_wrd_path_t;
typedef struct wtk_maxseg wtk_maxseg_t;

typedef int (*wtk_maxseg_is_wrd_f)(void *ths,char *s,int bytes);

typedef struct
{
	wtk_rbnode_t rb_n;
	wtk_string_t v;
	wtk_queue2_t path_q;
	wtk_maxseg_wrd_path_t *min_input_pth;
	unsigned use:1;
}wtk_maxseg_wrd_item_t;

struct wtk_maxseg_wrd_path
{
	wtk_queue_node_t out_n;
	wtk_queue_node_t q_n;
	wtk_maxseg_wrd_item_t *from;
	wtk_maxseg_wrd_item_t *to;
	wtk_string_t *v;
};

struct wtk_maxseg
{
	wtk_heap_t *heap;
	wtk_strbuf_t *buf;
	void *wrd_ths;
	wtk_maxseg_is_wrd_f wrd_f;
};


void wtk_maxseg_init(wtk_maxseg_t *seg,wtk_heap_t *heap,wtk_strbuf_t *buf,void *wrd_ths,wtk_maxseg_is_wrd_f wrd_f);
void wtk_maxseg_clean(wtk_maxseg_t *seg);

/*
 *@return queue : wtk_maxseg_wrd_path_t queue;
 */
wtk_queue_t wtk_maxseg_seg(wtk_maxseg_t *seg,char *data,int bytes);


void wtk_maxseg_print_queue(wtk_queue_t *q);
void wtk_maxseg_print_queue_string(wtk_queue_t *q);
#ifdef __cplusplus
};
#endif
#endif
