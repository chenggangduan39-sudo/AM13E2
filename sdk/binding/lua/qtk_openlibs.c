#include "lua/lauxlib.h"
#include "lua/lua.h"
#include "lua/lualib.h"

extern int luaopen_tracking(lua_State *L);

static const luaL_Reg modules[] = {
    {"tracking", luaopen_tracking},
};

static int luaopen_qbl(lua_State *L) {
    lua_newtable(L);
    for (int i = 0; i < sizeof(modules) / sizeof(modules[0]); i++) {
        if (modules[i].func(L) == 1) {
            lua_setfield(L, -2, modules[i].name);
        }
    }
    return 1;
}

void qbl_openlibs(lua_State *L) {
    const luaL_Reg libs[] = {
        {"qbl", luaopen_qbl},
    };
    for (int i = 0; i < sizeof(libs) / sizeof(libs[0]); i++) {
        luaL_requiref(L, libs[i].name, libs[i].func, 0);
        lua_pop(L, 1);
    }
}
