#include "lauxlib.h"

#include <string.h>

#define BIN_INFO "__Q_BIN_INFO"
#define BIN_PATH "__Q_BIN_PATH"
#define CODE_START "__Q_CODE_START"

LUALIB_API int luaQ_msgh(lua_State *L) {
  const char *msg = lua_tostring(L, 1);
  if (msg == NULL) {  /* is error object not a string? */
    if (luaL_callmeta(L, 1, "__tostring") &&  /* does it have a metamethod */
        lua_type(L, -1) == LUA_TSTRING)  /* that produces a string? */
      return 1;  /* that is the message */
    else
      msg = lua_pushfstring(L, "(error object is a %s value)",
                               luaL_typename(L, 1));
  }
  luaL_traceback(L, L, msg, 1);  /* append a standard traceback */
  return 1;  /* return the traceback */
}

static void xor_string(char *src, size_t len, const char *key, size_t klen, size_t cnt) {
    for (int i = 0; i < len; i++) {
        src[i] ^= key[(i + cnt)% klen];
    }
}

typedef struct {
    char buf[4096];
    FILE *f;
    uint32_t sz;
    uint32_t cnt;
} LoadB;

static const char *getB (lua_State *L, void *ud, size_t *size) {
  LoadB *lb = (LoadB *)ud;
  size_t readed, klen;
  char key[1024];

  if (lb->cnt == lb->sz) {
      return NULL;
  }

  readed = lb->sz - lb->cnt;
  if (readed > sizeof(lb->buf)) {
      readed = sizeof(lb->buf);
  }

  klen = snprintf(key, sizeof(key), "qdr+%d-key", lb->sz);

  *size = fread(lb->buf, 1, readed, lb->f);
  xor_string(lb->buf, *size, key, klen, lb->cnt);
  lb->cnt += *size;

  return lb->buf;
}

static int searcher_bin(lua_State *L) {
    const char *path;
    const char *name = luaL_gsub(L, luaL_checkstring(L, 1), ".", "_");
    uint32_t code_start, offset;
    LoadB lb;
    int status;

    lua_getfield(L, LUA_REGISTRYINDEX, CODE_START);
    code_start = lua_tointeger(L, -1);
    lua_getfield(L, LUA_REGISTRYINDEX, BIN_PATH);
    path = lua_tostring(L, -1);

    lua_getfield(L, LUA_REGISTRYINDEX, BIN_INFO);
    if (lua_getfield(L, -1, name) != LUA_TNUMBER) {
        lua_pushfstring(L, "%s not found in bin file", name);
        return 1;
    }
    offset = lua_tointeger(L, - 1);

    lb.f = fopen(path, "rb");
    if (lb.f == NULL) {
        lua_pushfstring(L, "%s open failed", path);
        return 1;
    }

    fseek(lb.f, code_start + offset, SEEK_SET);
    fread(&lb.sz, 1, sizeof(lb.sz), lb.f);
    lb.cnt = 0;

    status = lua_load(L, getB, &lb, lua_tostring(L, -1), "b");

    fclose(lb.f);
    if (status == LUA_OK) {
        lua_pushstring(L, name);
        return 2;
    }

    lua_pushstring(L, "load binfile syntax error") ;
    return 1;
}

static int l_setsearchers(lua_State *L) {
    int idx;
    if (lua_getglobal(L, "package") != LUA_TTABLE) {
        return -1;
    }
    if (lua_getfield(L, -1, "searchers") != LUA_TTABLE) {
        return -1;
    }
    lua_len(L, -1);
    idx = lua_tointeger(L, -1);
    lua_pop(L, 1);
    lua_pushvalue(L, -2);
    lua_pushcclosure(L, searcher_bin, 1);
    lua_rawseti(L, -2, idx + 1);
    return 0;
}

LUALIB_API int luaQ_bootstrap(lua_State *L, const char *fn) {
    char signature[] = "~qdr~";
    FILE *f;
    int ret;
    char buf[4096];
    char key[1024];
    uint32_t klen;

    f = fn == NULL ? stdin : fopen(fn, "rb");
    if (f == NULL) {
        goto err;
    }

    if (fread(buf, 1, sizeof(signature) - 1, f) == sizeof(signature) - 1 &&
        strncmp(buf, signature, sizeof(signature) - 1) == 0) {
        uint32_t sz;
        uint32_t bytes, bootstrap_pos;
        uint32_t offset = 0;
        uint32_t hdr_sz = 0;
        int bootstrap_found = 0;
        hdr_sz += sizeof(signature) - 1;

        lua_newtable(L);
        for (; 1 ;) {
            if (fread(&sz, sizeof(sz), 1, f) != 1 ||
                sz > sizeof(buf)) {
                goto err;
            }
            if (sz == 0) {
                hdr_sz += sizeof(sz);
                break;
            }
            if (fread(buf, 1, sz, f) != sz ||
                fread(&bytes, sizeof(bytes), 1, f) != 1) {
                goto err;
            }

            klen = snprintf(key, sizeof(key), "qdr+%d-key", sz);
            xor_string(buf, sz, key, klen, 0);

            if (strncmp("bootstrap", buf, sz) == 0) {
                bootstrap_pos = offset;
                offset += bytes;
                bootstrap_found = 1;
                hdr_sz += sz + sizeof(sz) + sizeof(bytes);
                continue;
            }

            lua_pushlstring(L, buf, sz);
            lua_pushinteger(L, offset);
            lua_settable(L, -3);
            offset += bytes;
            hdr_sz += sz + sizeof(sz) + sizeof(bytes);
        }

        if (bootstrap_found == 0) {
            goto err;
        }

        lua_setfield(L, LUA_REGISTRYINDEX, BIN_INFO);
        lua_pushinteger(L, hdr_sz);
        lua_setfield(L, LUA_REGISTRYINDEX, CODE_START);
        lua_pushstring(L, fn);
        lua_setfield(L, LUA_REGISTRYINDEX, BIN_PATH);

        if (l_setsearchers(L)) {
            goto err;
        }

        {
            LoadB lb;

            fseek(f, hdr_sz + bootstrap_pos, SEEK_SET);
            lb.f = f;
            lb.cnt = 0;
            fread(&lb.sz, 1, sizeof(lb.sz), lb.f);

            ret = lua_load(L, getB, &lb, lua_tostring(L, -1), "b");
        }
    } else {
        ret = luaL_loadfile(L, fn);
    }

    if (f && f != stdin) {
        fclose(f);
    }

    return ret;
err:
    if (f && f != stdin) {
        fclose(f);
    }
    return LUA_ERRFILE;
}
