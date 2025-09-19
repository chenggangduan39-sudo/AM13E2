#ifndef WTK_FST_EGRAM_XBNF_WTK_XBNF_H_
#define WTK_FST_EGRAM_XBNF_WTK_XBNF_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/core/wtk_strpool.h"
#include "wtk/core/wtk_os.h"
#include "wtk_xbnf_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_xbnf wtk_xbnf_t;

typedef struct wtk_xbnf_expr wtk_xbnf_expr_t;

typedef enum
{
	WTK_XBNF_COMMENT,
	WTK_XBNF_EXPR_INIT,
	WTK_XBNF_EXPR_NAME,
	WTK_XBNF_EXPR_WAIT_EQ,
	WTK_XBNF_EXPR_WAIT_V,
	WTK_XBNF_EXPR_VAR,
	WTK_XBNF_EXPR_ATTR,
	WTK_XBNF_INCLUDE,
}wtk_xbnf_state_t;

typedef enum
{
	WTK_XBNF_ITEM_STR,
	WTK_XBNF_ITEM_VAR,
	//WTK_XBNF_ITEM_OR,
	WTK_XBNF_ITEM_PARENTHESIS,//(
	WTK_XBNF_ITEM_BRACKET,	//[  {0,1}
	WTK_XBNF_ITEM_BRACE,	//{	 {0,}
	WTK_XBNF_ITEM_ANGLE,	//<	 {1,}
	//WTK_XBNF_ITEM_LIST,
	WTK_XBNF_ITEM_BRANCH,	//USE for merge
}wtk_xbnf_item_type_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_queue_t list_q;	//wtk_xbnf_item_t queue;
}wtk_xbnf_item_list_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_queue_t set_q;	//wtk_xbnf_item_list_t queue
	unsigned eof:1;
}wtk_xbnf_item_set_t;

typedef struct wtk_xbnf_item wtk_xbnf_item_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_string_t *k;
	wtk_string_t *v;
}wtk_xbnf_item_attr_item_t;

typedef struct
{
	wtk_string_t *k;
	wtk_string_t *v;
	wtk_queue_t attr_q;
	int min;
	int max;
}wtk_xbnf_item_attr_t;

struct wtk_xbnf_item
{
	wtk_queue_node_t q_n;
	wtk_queue_node_t stack_n;
	wtk_xbnf_item_type_t type;
	wtk_xbnf_item_attr_t attr;
	union
	{
		wtk_string_t *str;
		wtk_xbnf_expr_t *expr;
		wtk_xbnf_item_set_t *set; //
	}v;
	void *hook;
	unsigned eof:1;
	unsigned start:1;
};


struct wtk_xbnf_expr
{
	wtk_queue_node_t q_n;
	wtk_string_t *name;
	//wtk_xbnf_item_list_t *list;
	//wtk_queue_t set_q; //wtk_xbnf_item_list_t queue
	wtk_xbnf_item_set_t *set;
};

struct wtk_xbnf
{
	wtk_xbnf_cfg_t *cfg;
	wtk_strpool_t *pool;
	wtk_xbnf_state_t state;
	int sub_state;
	wtk_strbuf_t *buf;
	wtk_strbuf_t *tmp_buf;
	wtk_strbuf_t *attr_buf;
	wtk_xbnf_expr_t *main_expr;
	wtk_xbnf_expr_t *cur_expr;
	wtk_xbnf_item_t *cur_item;
	wtk_queue_t stack_q;
	//wtk_queue_t stack_q; //[,(,|,{,<
	wtk_heap_t *heap;
	wtk_str_hash_t *expr_hash;
	wtk_queue_t expr_q;
	wtk_string_t *pwd;
	wtk_segmenter_t *seg;
        int wdec_cnt;
        int wrds_cnt[5];
};

wtk_xbnf_t* wtk_xbnf_new(wtk_xbnf_cfg_t *cfg);
void wtk_xbnf_delete(wtk_xbnf_t *b);
void wtk_xbnf_reset(wtk_xbnf_t *b);

int wtk_xbnf_compile(wtk_xbnf_t *b,char *data,int bytes);
int wtk_xbnf_compile_file(wtk_xbnf_t *b,char *fn);
void wtk_xbnf_print(wtk_xbnf_t *b);
void wtk_xbnf_item_print(wtk_xbnf_item_t *item);
int wtk_xbnf_dump_lex(wtk_xbnf_t *b,char *fn);

char *wtk_xbnf_cfg_next_wrd(wtk_xbnf_cfg_t *cfg, char *s, char *e,
                            wtk_strbuf_t *buf, int *chn_wrd);
int wtk_xbnf_rec(wtk_xbnf_t *b,char *data,int bytes);
int wtk_xbnf_item_has_sub(wtk_xbnf_item_t *item);
char wtk_xbnf_item_type_get_char(wtk_xbnf_item_type_t t);

void wtk_xbnf_print_expand(wtk_xbnf_t *b);
#ifdef __cplusplus
};
#endif
#endif
