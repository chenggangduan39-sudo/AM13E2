#ifndef WTK_FST_EGRAM_XBNF_WTK_XBNF_REC_H_
#define WTK_FST_EGRAM_XBNF_WTK_XBNF_REC_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk/core/json/wtk_json.h"
#include "wtk_xbnf_rec_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_xbnf_rec wtk_xbnf_rec_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_xbnf_item_t *item;
	unsigned eof:1;
}wtk_xbnf_rec_item_path_t;


typedef struct wtk_xbnf_rec_path wtk_xbnf_rec_path_t;

typedef struct wtk_xbnf_rec_path_v wtk_xbnf_rec_path_v_t;

struct wtk_xbnf_rec_path_v
{
	wtk_xbnf_rec_path_v_t *nxt;
	//wtk_xbnf_rec_path_t *sub_path_l;
	wtk_string_t *v;
};

struct wtk_xbnf_rec_path
{
	wtk_xbnf_rec_path_t *prev;
	wtk_xbnf_item_set_t *set;
	wtk_xbnf_item_t *item;
	wtk_xbnf_rec_path_v_t *v;
	int nv;
	int nwrd;
	int pos_s;
};

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_xbnf_rec_path_t *path;
	wtk_xbnf_item_t *item;
	wtk_string_t *last;
	int repeat;
}wtk_xbnf_rec_inst_t;

typedef struct
{
	int nwrd;
	wtk_string_t *v;
}wtk_xbnf_rec_output_item_t;

struct wtk_xbnf_rec
{
	wtk_xbnf_rec_cfg_t *cfg;
	wtk_strbuf_t *buf;
	wtk_strbuf_t *tmp_buf;
	wtk_strbuf_t *pth_buf;
	wtk_heap_t *heap;
	wtk_vpool_t *inst_pool;
	wtk_queue_t inst_q;
	wtk_xbnf_rec_output_item_t output;
	int pos;
	unsigned eof:1;
};

wtk_xbnf_rec_t* wtk_xbnf_rec_new(wtk_xbnf_rec_cfg_t *cfg);
void wtk_xbnf_rec_delete(wtk_xbnf_rec_t *r);
void wtk_xbnf_rec_reset(wtk_xbnf_rec_t *r);
int wtk_xbnf_rec_process(wtk_xbnf_rec_t *r,char *data,int bytes);
void wtk_xbnf_rec_print(wtk_xbnf_rec_t *r);
wtk_json_item_t* wtk_xbnf_rec_get_json(wtk_xbnf_rec_t *r,wtk_json_t *json);
#ifdef __cplusplus
};
#endif
#endif
