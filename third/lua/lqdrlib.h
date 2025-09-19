#ifndef __LQDRLIB_H__
#define __LQDRLIB_H__
#pragma once
#include "lua.h"
#ifdef __cplusplus
extern "C" {
#endif

LUALIB_API int luaQ_msgh(lua_State *L);
LUALIB_API int luaQ_bootstrap(lua_State *L, const char *fn);

#ifdef __cplusplus
};
#endif
#endif
