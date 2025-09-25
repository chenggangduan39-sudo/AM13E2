#ifndef G_JESEU3LX_H4JJ_NYYN_706O_CR4FNUK9WJ64
#define G_JESEU3LX_H4JJ_NYYN_706O_CR4FNUK9WJ64
#pragma once
#include "lua/lua.h"
#include "wtk/core/wtk_str.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

int qtk_api_init_fn(lua_State *L, const char *fn);
int qtk_api_get_func(lua_State *L, const char *func_name);
int qtk_api_call_func(lua_State *L, const char *func_name, wtk_string_t *arg,
                      wtk_strbuf_t *res);

#ifdef __cplusplus
};
#endif
#endif /* G_JESEU3LX_H4JJ_NYYN_706O_CR4FNUK9WJ64 */
