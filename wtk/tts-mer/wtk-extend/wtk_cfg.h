#ifndef WTK_MER_CFG_H
#define WTK_MER_CFG_H
#include "wtk/tts-mer/wtk-extend/wtk_tts_common.h"
#include "wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#define wtk_cfg_macro_check_printf(n, i) printf("-------- %-15s = %d \n", #n, !i?i:1);
void wtk_cfg_macro_check();

#define wtk_cfg_def_sprintf_init(lc, fn, v, item_name, ...) {v=wtk_local_cfg_find_string_s(lc, item_name);if(!v){wtk_exit_debug("-------->  not find cfg param:   %s\n", STR(item_name));};sprintf(fn, v->data ,##__VA_ARGS__, "");}
#define wtk_cfg_def_matf_load(lc, fn, v, item, item_name, ...) {wtk_cfg_def_sprintf_init(lc, fn, v, item_name, ##__VA_ARGS__);wtk_mer_matf_read_file2(item, fn, 1);}
#define wtk_cfg_def_vecf_load(lc, fn, v, item, item_name, ...) {wtk_cfg_def_sprintf_init(lc, fn, v, item_name, ##__VA_ARGS__);wtk_mer_vecf_read_file2(item, fn, 1);}


#define wtk_cfg_def_param_require(func,lc,cfg,item,v) {func(lc,cfg,item,v);if(!v){wtk_exit_debug("-----------> not find require cfg param:  %s \n", STR(item));}};
#define wtk_cfg_def_param_require_str(heap,lc,fn,v,item,item_name, ...) {wtk_cfg_def_sprintf_init(lc, fn, v, item_name, ##__VA_ARGS__);item=wtk_heap_dup_str(heap,fn);} // 支持额外参数format字符串

#ifdef __cplusplus
}
#endif
#endif