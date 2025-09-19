#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_cfg_queue.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_larray.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_str.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_type.h"

struct rbin_path_pair_ {
    wtk_string_t *realpath;
    wtk_string_t *path;
};

static void print_usage_(const char *name) {
    printf("%s\n", name);
    printf("\t-x extract\n");
    printf("\t-s main cfg to compress or bin file to extract\n");
    printf("\t-d bin file for compress output or directory for extract\n");
    printf("\t-e entry cfg name\n");
}

static int path_in_dep_(wtk_string_t *path, wtk_larray_t *deps) {
    for (int i = 0; i < deps->nslot; i++) {
        wtk_string_t dep_path =
            **cast(wtk_string_t **, wtk_larray_get(deps, i));
        if (wtk_string_equal(path, &dep_path)) {
            return 1;
        }
    }
    return 0;
}

static int lc_replace_fn_path_(wtk_local_cfg_t *lc, wtk_larray_t *deps,
                               wtk_heap_t *heap) {
    wtk_cfg_item_t *item;
    wtk_queue_node_t *node;
    char path_buf[4096];
    char fn_suffix[] = "_fn";

    for (node = lc->cfg->queue.pop; node; node = node->next) {
        item = data_offset2(node, wtk_cfg_item_t, n);
        if (item->type == WTK_CFG_STRING &&
            wtk_str_end_with_s(item->key->data, item->key->len, fn_suffix) &&
            path_in_dep_(item->value.str, deps)) {
            int len = snprintf(path_buf, sizeof(path_buf), "${pwd}/%.*s",
                               item->value.str->len, item->value.str->data);
            item->value.str = wtk_heap_dup_string2(heap, path_buf, len);
            wtk_debug("%.*s\n", item->value.str->len, item->value.str->data);
        } else if (item->type == WTK_CFG_LC) {
            lc_replace_fn_path_(item->value.cfg, deps, heap);
        }
    }
    return 0;
}

static int extract_(const char *sfn, const char *dfn, char *entry_cfn) {
    wtk_rbin2_t *rb;
    wtk_rbin2_item_t *item, *main_cfg_item = NULL;
    wtk_queue_node_t *node;
    int ret = -1;
    wtk_strbuf_t *buf = wtk_strbuf_new(4096, 1);
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_larray_t *deps = wtk_larray_new(32, sizeof(wtk_string_t *));
    wtk_cfg_file_t *cfile = NULL;

    rb = wtk_rbin2_new();
    ret = wtk_rbin2_read(rb, cast(char *, sfn));
    if (ret != 0) {
        goto end;
    }

    if (entry_cfn == NULL) {
        ret = wtk_rbin2_extract(rb, cast(char *, dfn));
        goto end;
    }

    wtk_string_t main_cfg = {entry_cfn, strlen(entry_cfn)};
    for (node = rb->list.pop; node; node = node->next) {
        item = data_offset(node, wtk_rbin2_item_t, q_n);
        if (wtk_string_equal(&main_cfg, item->fn)) {
            main_cfg_item = item;
            if (!item->data) {
                wtk_rbin2_load_item(rb, item, 1);
            }
        } else {
            wtk_larray_push2(deps, &item->fn);
        }
    }

    cfile = wtk_cfg_file_new();
    wtk_cfg_file_feed(cfile, main_cfg_item->data->data,
                      main_cfg_item->data->len);
    lc_replace_fn_path_(cfile->main, deps, heap);
    wtk_local_cfg_value_to_pretty_string(cfile->main, buf, 0);
    main_cfg_item->data->data = buf->data;
    main_cfg_item->data->len = buf->pos;

    wtk_rbin2_extract(rb, cast(char *, dfn));

    ret = 0;
end:
    wtk_rbin2_delete(rb);
    wtk_heap_delete(heap);
    wtk_strbuf_delete(buf);
    wtk_larray_delete(deps);
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
    return 0;
}

static int dep_analysis_(wtk_heap_t *heap, wtk_local_cfg_t *lc,
                         wtk_larray_t *dep, wtk_string_t *pwd) {
    wtk_queue_node_t *node;
    wtk_cfg_item_t *item;
    char fn_suffix[] = "_fn";
    struct rbin_path_pair_ pair;
    wtk_strbuf_t *path_buf = wtk_strbuf_new(1024, 1);
    const char *s, *p;

    for (node = lc->cfg->queue.pop; node; node = node->next) {
        item = data_offset2(node, wtk_cfg_item_t, n);
        if (item->type == WTK_CFG_STRING &&
            wtk_str_end_with_s(item->key->data, item->key->len, fn_suffix)) {
            wtk_strbuf_reset(path_buf);
            wtk_strbuf_push_s(path_buf, "./");
            wtk_string_t *fn = wtk_heap_dup_string2(heap, item->value.str->data,
                                                    item->value.str->len);
            s = fn->data;
            if (pwd && wtk_string_cmp_withstart(item->value.str, pwd->data,
                                                pwd->len) == 0) {
                s += pwd->len + 1;
            }
            while (1) {
                int cnt = 0;
                p = strstr(s, "../");
                if (p == NULL) {
                    wtk_strbuf_push(path_buf, s, strlen(s));
                    break;
                }
                wtk_strbuf_push(path_buf, s, p - s);
                s = p + 3;
                cnt++;
                while (strstr(s, "../") == s) {
                    cnt++;
                    s += 3;
                }
                wtk_strbuf_push_f(path_buf, "%d/", cnt);
            }
            pair.realpath = fn;
            pair.path =
                wtk_heap_dup_string2(heap, path_buf->data, path_buf->pos);
            item->value.str = pair.path;
            wtk_debug("dep %.*s\n", fn->len, fn->data);
            wtk_larray_push2(dep, &pair);
        } else if (item->type == WTK_CFG_LC) {
            dep_analysis_(heap, item->value.cfg, dep, pwd);
        }
    }

    wtk_strbuf_delete(path_buf);
    return 0;
}

static int lc_prune_(wtk_local_cfg_t *lc) {
    wtk_queue_node_t *node;
    wtk_cfg_item_t *item;
    wtk_string_t v;
    char use_prefix[] = "use_";

    for (node = lc->cfg->queue.pop; node; node = node->next) {
        item = data_offset2(node, wtk_cfg_item_t, n);
        if (item->type == WTK_CFG_STRING &&
            wtk_string_cmp_s_withstart(item->key, use_prefix) == 0) {
            if (item->value.str->len == 1 && item->value.str->data[0] == '0') {
                v.data = item->key->data + (sizeof(use_prefix) - 1);
                v.len = item->key->len - (sizeof(use_prefix) - 1);
                if (0 == wtk_local_cfg_remove(lc, v.data, v.len, 0)) {
                    wtk_debug("prune %.*s\n", v.len, v.data);
                }
            }
        } else if (item->type == WTK_CFG_LC) {
            lc_prune_(item->value.cfg);
        }
    }
    return 0;
}

static int rbin_pack_(wtk_local_cfg_t *lc, wtk_larray_t *deps,
                      const char *dfn, const char *entry_cfn) {
    wtk_strbuf_t *buf = wtk_strbuf_new(1024, 1);
    wtk_rbin2_t *rbin = wtk_rbin2_new();
    wtk_string_t main_cfg = {cast(char *, entry_cfn), strlen(entry_cfn)};
    void **content_hub;
    int content_pos = 0;

    wtk_local_cfg_value_to_string(lc, buf);
    wtk_rbin2_add2(rbin, &main_cfg, buf->data, buf->pos);
    content_hub = wtk_malloc(sizeof(void *) * deps->nslot);

    for (int i = 0; i < deps->nslot; i++) {
        int file_length;
        char *content;
        struct rbin_path_pair_ pair =
            *cast(struct rbin_path_pair_ *, wtk_larray_get(deps, i));
        content = file_read_buf(pair.realpath->data, &file_length);
        if (content) {
            wtk_debug("pack %.*s %d\n", pair.path->len, pair.path->data,
                      file_length);
            pair.path->data += 2;
            pair.path->len -= 2;
            wtk_rbin2_add2(rbin, pair.path, content, file_length);
            content_hub[content_pos++] = content;
        }
    }

    wtk_rbin2_write(rbin, cast(char *, dfn));
    wtk_strbuf_delete(buf);
    wtk_rbin2_delete(rbin);

    for (int i = 0; i < content_pos; i++) {
        wtk_free(content_hub[i]);
    }
    wtk_free(content_hub);

    return 0;
}

static int compress_(const char *sfn, const char *dfn, const char *entry_cfn) {
    int ret = -1;
    wtk_cfg_file_t *cfile = NULL;
    wtk_larray_t *dep_files = NULL;
    wtk_heap_t *heap = NULL;
    wtk_string_t *pwd = NULL;
    wtk_cfg_item_t *item;

    cfile = wtk_cfg_file_new_fn(cast(char *, sfn));
    if (cfile == NULL) {
        goto end;
    }

    item = wtk_cfg_queue_find_s(cfile->main->cfg, "pwd");
    if (item && item->type == WTK_CFG_STRING) {
        pwd = item->value.str;
        wtk_local_cfg_remove(cfile->main, "pwd", sizeof("pwd") - 1, 1);
    }

    heap = wtk_heap_new(4096);
    dep_files = wtk_larray_new(100, sizeof(struct rbin_path_pair_));
    lc_prune_(cfile->main);

    dep_analysis_(heap, cfile->main, dep_files, pwd);
    rbin_pack_(cfile->main, dep_files, dfn, entry_cfn);

    ret = 0;
end:
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
    if (heap) {
        wtk_heap_delete(heap);
    }
    if (dep_files) {
        wtk_larray_delete(dep_files);
    }
    return ret;
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg;
    char *sfn = NULL, *dfn = NULL;
    char *entry_cfn = NULL;

    arg = wtk_arg_new(argc, argv);
    if (!arg) {
        goto end;
    }

    wtk_arg_get_str_s(arg, "s", &sfn);
    wtk_arg_get_str_s(arg, "d", &dfn);
    wtk_arg_get_str_s(arg, "e", &entry_cfn);

    if (!sfn && !dfn) {
        print_usage_(argv[0]);
        goto end;
    }

    if (wtk_arg_exist_s(arg, "x")) {
        extract_(sfn, dfn, entry_cfn);
    } else {
        compress_(sfn, dfn, entry_cfn);
    }

end:
    if (arg) {
        wtk_arg_delete(arg);
    }
    return 0;
}
