#include "binding/lua/qtk_api.h"
#include "lua/lqdrlib.h"

int qtk_api_init_fn(lua_State *L, const char *fn) {
    int msgh;
    lua_pushcfunction(L, luaQ_msgh);
    msgh = lua_gettop(L);
    luaQ_bootstrap(L, fn);
    if (LUA_OK != lua_pcall(L, 0, 0, msgh)) {
        qtk_debug("api init failed: %s\n", lua_tostring(L, -1));
        lua_remove(L, msgh);
        lua_pop(L, 1);
        return -1;
    }
    lua_remove(L, msgh);
    return 0;
}

int qtk_api_get_func(lua_State *L, const char *func_name) {
    lua_getglobal(L, func_name);
    return 0;
}

int qtk_api_call_func(lua_State *L, const char *func_name, wtk_string_t *arg,
                      wtk_strbuf_t *res) {
    size_t sz;
    const char *res_data;
    int msgh = 0;
    lua_pushcfunction(L, luaQ_msgh);
    msgh = lua_gettop(L);
    qtk_api_get_func(L, func_name);
    lua_pushlstring(L, arg->data, arg->len);
    if (lua_pcall(L, 1, 1, msgh) != LUA_OK) {
        qtk_debug("call %s failed: %s\n", func_name, lua_tostring(L, -1));
        lua_remove(L, msgh);
        lua_pop(L, 1);
        return -1;
    }
    lua_remove(L, msgh);
    res_data = lua_tolstring(L, -1, &sz);
    wtk_strbuf_reset(res);
    wtk_strbuf_push(res, res_data, sz);
    lua_pop(L, 1);
    return 0;
}
