#ifndef WTK_MER_JSON_H
#define WTK_MER_JSON_H
#include "wtk_tts_common.h"
#include "wtk/core/json/wtk_json_parse.h"
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

wtk_json_parser_t* wtk_json_parser_heap_new( wtk_heap_t *heap);
int wtk_json_trie_start_with(wtk_json_item_t *js, wtk_string_t **key, int ksize);
wtk_json_item_t* wtk_json_item_get_loop(wtk_json_item_t *item, int key_len, ...);

#ifdef __cplusplus
}
#endif
#endif