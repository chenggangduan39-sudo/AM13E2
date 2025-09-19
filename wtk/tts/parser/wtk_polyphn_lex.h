#ifndef WTK_TTS_PARSER_WTK_POLYPHN_LEX
#define WTK_TTS_PARSER_WTK_POLYPHN_LEX
#include "wtk/core/wtk_type.h" 
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/core/wtk_larray.h"
#include "wtk_tts_def.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_polyphn_lex wtk_polyphn_lex_t;

typedef enum
{
	WTK_POLYPHN_EXPR_ITEM_NONE,
	WTK_POLYPHN_EXPR_ITEM_C,
	WTK_POLYPHN_EXPR_ITEM_Z,
	WTK_POLYPHN_EXPR_ITEM_P,
	WTK_POLYPHN_EXPR_ITEM_I,
}wtk_polyphn_expr_item_if_type_t;

typedef struct
{
	wtk_queue_node_t q_n;
	short start;
	short end;
	wtk_string_t **strs;
	short nstr;
	wtk_polyphn_expr_item_if_type_t type;
	unsigned not:1;
}wtk_polyphn_expr_item_if_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_queue_t if_q;
	float prob;
	wtk_string_t *syl;
	wtk_string_t *pos;
	wtk_tts_wrd_pron_t *pron;
}wtk_polyphn_expr_item_t;

typedef struct
{
	wtk_string_t *wrd;
	wtk_queue_t item_q;	//wtk_polyphn_expr_item_t
}wtk_polyphn_wrd_t;


struct wtk_polyphn_lex
{
	wtk_str_hash_t *hash;
};

wtk_polyphn_lex_t* wtk_polyphn_lex_new();
void wtk_polyphn_lex_delete(wtk_polyphn_lex_t *l);
int wtk_polyphn_lex_load(wtk_polyphn_lex_t *l,char *fn);
int wtk_polyphn_lex_load2(wtk_polyphn_lex_t *l,wtk_source_t *src);
wtk_polyphn_wrd_t* wtk_polyphn_lex_find(wtk_polyphn_lex_t *l,char *data,int bytes);
void wtk_polyphn_wrd_print(wtk_polyphn_wrd_t *w);
void wtk_polyphn_expr_item_if_print(wtk_polyphn_expr_item_if_t *item);
int wtk_polyphn_expr_item_if_match(wtk_polyphn_expr_item_if_t *xif,char *wrd,int wrd_bytes,char *pos,int pos_bytes);
int wtk_polyphn_expr_item_if_match2(wtk_polyphn_expr_item_if_t *xif,char *wrd,int wrd_bytes,char *pos,int pos_bytes,char *idx,int idx_bytes);
void wtk_polyphn_expr_item_print(wtk_polyphn_expr_item_t *item);
#ifdef __cplusplus
};
#endif
#endif
