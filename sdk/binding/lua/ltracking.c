#include "lua/lauxlib.h"
#include "lua/lua.h"

#include "qtk/cv/tracking/qtk_mot_sort.h"
#include "wtk/core/wtk_str.h"

#define SORT_METHOD "QBL_MOT_SORT_WRAPPER"
#define GET_SORT(L, idx)                                                       \
    cast(qtk_mot_sort_t *, luaL_checkudata(L, idx, SORT_METHOD))

static int mot_release_(lua_State *L) {
    qtk_mot_sort_t *s = GET_SORT(L, 1);
    qtk_mot_sort_clean(s);
    return 0;
}

static int tbl2bbox_(lua_State *L, qtk_cv_bbox_t *bbox) {
    int is_num;
    lua_pushstring(L, "x1");
    lua_gettable(L, -2);
    bbox->x1 = lua_tonumberx(L, -1, &is_num);
    if (!is_num) {
        goto err;
    }
    lua_pop(L, 1);

    lua_pushstring(L, "x2");
    lua_gettable(L, -2);
    bbox->x2 = lua_tonumberx(L, -1, &is_num);
    if (!is_num) {
        goto err;
    }
    lua_pop(L, 1);

    lua_pushstring(L, "y1");
    lua_gettable(L, -2);
    bbox->y1 = lua_tonumberx(L, -1, &is_num);
    if (!is_num) {
        goto err;
    }
    lua_pop(L, 1);

    lua_pushstring(L, "y2");
    lua_gettable(L, -2);
    bbox->y2 = lua_tonumberx(L, -1, &is_num);
    if (!is_num) {
        goto err;
    }
    lua_pop(L, 1);
    return 0;
err:
    return -1;
}

static int bbox2tbl_(lua_State *L, qtk_cv_bbox_t *bbox) {
    lua_newtable(L);

    lua_pushstring(L, "x1");
    lua_pushnumber(L, bbox->x1);
    lua_settable(L, -3);

    lua_pushstring(L, "x2");
    lua_pushnumber(L, bbox->x2);
    lua_settable(L, -3);

    lua_pushstring(L, "y1");
    lua_pushnumber(L, bbox->y1);
    lua_settable(L, -3);

    lua_pushstring(L, "y2");
    lua_pushnumber(L, bbox->y2);
    lua_settable(L, -3);

    return 0;
}

static int set_bbox_(lua_State *L, qtk_cv_bbox_t *bbox, int ndet) {
    int cur_stack = lua_gettop(L);
    for (int i = 0; i < ndet; i++) {
        lua_pushinteger(L, i + 1);
        lua_gettable(L, -2);

        if (!lua_istable(L, -1) || tbl2bbox_(L, bbox + i)) {
            goto err;
        }

        lua_pop(L, 1);
    }
    return 0;
err:
    lua_settop(L, cur_stack);
    return -1;
}

static int mot_set_result_(lua_State *L, qtk_mot_sort_t *s, int ndet) {
    // set result
    lua_createtable(L, ndet, 0);
    for (int i = 0; i < ndet; i++) {
        lua_pushinteger(L, s->result[i]);
        lua_seti(L, -2, i + 1);
    }

    // set tracklet
    lua_newtable(L);
    {
        wtk_queue_node_t *node;
        qtk_mot_sort_tracklet_t *kt;

        for (node = s->trackers.pop; node; node = node->next) {
            kt = data_offset2(node, qtk_mot_sort_tracklet_t, q_n);
            lua_newtable(L);

            if (kt->age > 0) {
                lua_pushstring(L, "predict");
                bbox2tbl_(L, &kt->predict);
                lua_settable(L, -3);
            }

            lua_pushstring(L, "det_id");
            lua_pushinteger(L, kt->det_id);
            lua_settable(L, -3);

            lua_pushstring(L, "hit_streak");
            lua_pushinteger(L, kt->hit_streak);
            lua_settable(L, -3);

            lua_pushstring(L, "time_since_update");
            lua_pushinteger(L, kt->time_since_update);
            lua_settable(L, -3);

            lua_pushstring(L, "age");
            lua_pushinteger(L, kt->age);
            lua_settable(L, -3);

            lua_seti(L, -2, kt->id);
        }
    }
    return 2;
}

static int mot_update_(lua_State *L) {
    void *ud = NULL;
    lua_Alloc alloc = lua_getallocf(L, &ud);
    int ndet;
    qtk_cv_bbox_t *dets;
    qtk_mot_sort_t *s = GET_SORT(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_len(L, 2);
    ndet = lua_tointeger(L, -1);
    lua_pop(L, 1);
    if (ndet > 0) {
        dets = alloc(ud, NULL, 0, ndet * sizeof(qtk_cv_bbox_t));
        if (set_bbox_(L, dets, ndet)) {
            alloc(ud, dets, ndet * sizeof(qtk_cv_bbox_t), 0);
            luaL_error(L, "wrong fmt of dets");
        }
        qtk_mot_sort_update(s, ndet, dets);
        alloc(ud, dets, ndet * sizeof(qtk_cv_bbox_t), 0);
    } else {
        qtk_mot_sort_update(s, 0, NULL);
    }
    return mot_set_result_(L, s, ndet);
}

static void sort_cfg_update_(lua_State *L, qtk_mot_sort_cfg_t *cfg) {
    lua_pushstring(L, "max_age");
    if (lua_gettable(L, -2) == LUA_TNUMBER) {
        cfg->max_age = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_pushstring(L, "min_hits");
    if (lua_gettable(L, -2) == LUA_TNUMBER) {
        cfg->min_hits = lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_pushstring(L, "use_distance");
    if (lua_gettable(L, -2) == LUA_TBOOLEAN) {
        cfg->use_distance = lua_toboolean(L, -1);
    }
    lua_pop(L, 1);

    if (cfg->use_distance) {
        lua_pushstring(L, "distance_threshold");
        if (lua_gettable(L, -2) == LUA_TNUMBER) {
            cfg->distance_threshold = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
    } else {
        lua_pushstring(L, "iou_threshold");
        if (lua_gettable(L, -2) == LUA_TNUMBER) {
            cfg->iou_threshold = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
    }
}

static int lnew_tracking_(lua_State *L) {
    const char *kind;
    size_t kind_len;
    qtk_mot_sort_t *sort;

    kind = luaL_checklstring(L, 1, &kind_len);
    if (wtk_str_equal_s(kind, kind_len, "sort")) {
        qtk_mot_sort_cfg_t sort_cfg;
        qtk_mot_sort_cfg_init(&sort_cfg);

        if (lua_gettop(L) >= 2) {
            lua_settop(L, 2);
            sort_cfg_update_(L, &sort_cfg);
        }

        sort = lua_newuserdatauv(L, sizeof(qtk_mot_sort_t), 0);
        qtk_mot_sort_init(sort, &sort_cfg);
        luaL_setmetatable(L, SORT_METHOD);
    } else {
        luaL_error(L, "%s unsupport", kind);
    }
    return 1;
}

static void create_sort_meta_(lua_State *L) {
    const luaL_Reg handler[] = {
        {"__gc", mot_release_}, {"update", mot_update_}, {NULL, NULL}};
    luaL_newmetatable(L, SORT_METHOD);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, handler, 0);
    lua_pop(L, 1);
}

LUAMOD_API int luaopen_tracking(lua_State *L) {
    luaL_checkversion(L);
    luaL_Reg lib[] = {{"new", lnew_tracking_}, {NULL, NULL}};
    luaL_newlib(L, lib);
    create_sort_meta_(L);
    return 1;
}
