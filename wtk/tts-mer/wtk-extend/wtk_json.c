#include "wtk_json.h"
#include "wtk_heap.h"
#include "wtk_type.h"

void wtk_json_parser_set_state(wtk_json_parser_t *p,wtk_json_parse_state_t state);

wtk_json_t* wtk_json_heap_new(wtk_heap_t *heap)
{
	wtk_json_t *json;

	json=(wtk_json_t*)wtk_heap_malloc(heap,sizeof(wtk_json_t));
	json->heap=heap;
	json->main=NULL;
	return json;
}

wtk_json_parser_t* wtk_json_parser_heap_new( wtk_heap_t *heap)
{
	wtk_json_parser_t *p;

	p=(wtk_json_parser_t*)wtk_heap_malloc( heap, sizeof(wtk_json_parser_t));
	p->json=wtk_json_heap_new( heap);
	p->heap=heap;
	p->buf=wtk_strbuf_heap_new( heap, 256,1);
	wtk_queue_init(&(p->stack_q));
	p->cur_json=NULL;
	wtk_str_parse_init(&(p->str_parse),p->buf);
	p->cur=0;
	wtk_queue_init(&(p->stack_q));
	wtk_json_parser_set_state(p,WTK_JSON_PARSE_MAIN_WAIT);
	return p;
}

int wtk_json_trie_start_with(wtk_json_item_t *js, wtk_string_t **key, int ksize)
{/* 
字典树查找
判断某 key路径是否存在
json[key][key][key]
 */
    int i;
    wtk_json_item_t *cur = js;
    for (i=0; i<ksize; ++i)
    {
        cur = wtk_json_item_get_path_item( cur, key[i]->data, key[i]->len, NULL);
        if (!cur)
        {
            return 0;
        }
    }
    return 1;
}

wtk_json_item_t* wtk_json_item_get_loop(wtk_json_item_t *item, int key_len, ...)
{/* 
json连续查找
相当于
json[key][key][key];
请保持 char *key 以'\0' 结尾';
 */
    int i;
    va_list arg;
    wtk_json_item_t *it=item;
    char *key;

    va_start(arg, key_len);

    for (i=0; i<key_len; ++i)
    {
        key = va_arg(arg, char*);
        it=wtk_json_item_get_path_item(it, key, strlen(key), NULL);
        if (!it) { wtk_exit_debug("json[%d][%s] key not find \n", i, key); }
    }
    va_end(arg);
    return it;
}