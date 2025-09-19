#include "qtk_ecloud.h"

wtk_string_t qtk_ecloud_get_result(qtk_ecloud_t *e);

void qtk_ecloud_init(qtk_ecloud_t *e) {
    qtk_engine_param_init(&e->param);
    e->session = NULL;
    e->spx = NULL;
    e->spx_cfg = NULL;
    e->evaltxt = NULL;
    e->parser = NULL;
}
qtk_ecloud_t *qtk_ecloud_new(qtk_session_t *s, wtk_local_cfg_t *params) {
    qtk_ecloud_t *e;

    e = (qtk_ecloud_t *)wtk_malloc(sizeof(qtk_ecloud_t));
    qtk_ecloud_init(e);
    e->session = s;

    qtk_engine_param_set_session(&e->param, e->session);
    qtk_engine_param_feed(&e->param, params);
    e->spx_cfg = qtk_spx_cfg_new(e->param.cfg);
    e->spx = qtk_spx_new(e->spx_cfg, e->session);
    if (!e->spx) {
        wtk_debug("================>spx new failed\n");
    }
    e->evaltxt = wtk_strbuf_new(3200, 1);
    e->parser = wtk_json_parser_new();
    e->rlt_buf = wtk_strbuf_new(512, 1);
    return e;
}
int qtk_ecloud_start(qtk_ecloud_t *e) {
    int ret;

    ret = qtk_spx_start(e->spx, e->evaltxt->data, e->evaltxt->pos, 0);
    if (ret != 0) {
        wtk_log_log(e->session->log, "spx start failed ret = %d\n", ret);
        return ret;
    } else {
        return 0;
    }
}
void qtk_ecloud_delete(qtk_ecloud_t *e) {
    if (e->spx) {
        qtk_spx_delete(e->spx);
    }
    if (e->rlt_buf) {
        wtk_strbuf_delete(e->rlt_buf);
    }
    if (e->spx_cfg) {
        qtk_spx_cfg_clean(e->spx_cfg);
        qtk_spx_cfg_delete(e->spx_cfg);
    }
    if (e->parser) {
        wtk_json_parser_delete(e->parser);
    }
    if (e->evaltxt) {
        wtk_strbuf_delete(e->evaltxt);
    }
    wtk_free(e);
}
void qtk_ecloud_reset(qtk_ecloud_t *e) {
    if (e->spx) {
        qtk_spx_reset(e->spx);
    }
}
int qtk_ecloud_feed(qtk_ecloud_t *e, char *data, int bytes, int is_end) {
    wtk_string_t v;
    qtk_var_t var;

    if (bytes > 0) {
        qtk_spx_feed(e->spx, data, bytes, 0);
    }
    if (is_end) {
        qtk_spx_feed(e->spx, NULL, 0, 1);
        v = qtk_ecloud_get_result(e);
        var.type = QTK_EVAL_TEXT;
        var.v.str.data = v.data;
        var.v.str.len = v.len;
        e->notify_f(e->notify_ths, &var);
    }
    return 0;
}
void qtk_ecloud_set(qtk_ecloud_t *e, char *data, int bytes) {
    wtk_cfg_file_t *cfile = NULL;
    wtk_cfg_item_t *item;
    wtk_queue_node_t *qn;
    int ret;

    cfile = wtk_cfg_file_new();
    if (!cfile) {
        return;
    }
    ret = wtk_cfg_file_feed(cfile, data, bytes);
    if (ret != 0) {
        goto end;
    }
    for (qn = cfile->main->cfg->queue.pop; qn; qn = qn->next) {
        item = data_offset2(qn, wtk_cfg_item_t, n);
        if (wtk_string_cmp2(item->key, &qtk_engine_set_str[2]) == 0) {
            wtk_strbuf_reset(e->evaltxt);
            wtk_strbuf_push(e->evaltxt, item->value.str->data,
                            item->value.str->len);
        }
    }
end:
    if (cfile) {
        wtk_cfg_file_delete(cfile);
    }
}
void qtk_ecloud_set_notify(qtk_ecloud_t *e, void *ths,
                           qtk_engine_notify_f notify) {
    e->notify_ths = ths;
    e->notify_f = notify;
}

wtk_string_t qtk_ecloud_get_middle_phone(char *data, int bytes) {
    typedef enum {
        QTK_EVAL_MP_INIT,
        QTK_EVAL_MP_LEFT,
    } qtk_eval_mp_state_t;
    qtk_eval_mp_state_t state;
    wtk_string_t v;
    char *s, *e;
    int left;
    int pos;

    s = data;
    e = data + bytes;
    pos = 0;
    left = 0;
    state = QTK_EVAL_MP_INIT;
    while (s < e) {
        switch (state) {
        case QTK_EVAL_MP_INIT:
            if (*s == '-') {
                state = QTK_EVAL_MP_LEFT;
            }
            ++left;
            break;
        case QTK_EVAL_MP_LEFT:
            if (*s == '+') {
                goto end;
            } else {
                ++pos;
            }
            break;
        }
        ++s;
    }

end:
    wtk_string_set(&v, data + left, pos);
    return v;
}
int qtk_ecloud_wrd_phone_middle(wtk_json_t *json, wtk_json_item_t *item) {
    wtk_json_item_t *i, *i1, *ichar, *iscore;
    wtk_json_item_t *dst, *idst;
    wtk_queue_node_t *qn, *qn1;
    wtk_json_array_item_t *ai, *ai1;
    wtk_string_t v;
    int ret;

    i = wtk_json_obj_get_s(item, "scores");
    if (!i || i->type != WTK_JSON_ARRAY) {
        ret = -1;
        goto end;
    }

    for (qn = i->v.array->pop; qn; qn = qn->next) {
        ai = data_offset2(qn, wtk_json_array_item_t, q_n);
        i1 = wtk_json_obj_get_s(ai->item, "phone");
        if (i1 && i1->type == WTK_JSON_ARRAY) {
            dst = wtk_json_new_array(json);
            for (qn1 = i1->v.array->pop; qn1; qn1 = qn1->next) {
                ai1 = data_offset2(qn1, wtk_json_array_item_t, q_n);
                ichar = wtk_json_obj_get_s(ai1->item, "char");
                if (!ichar || ichar->type != WTK_JSON_STRING ||
                    ichar->v.str->len <= 0) {
                    continue;
                }
                iscore = wtk_json_obj_get_s(ai1->item, "score");
                if (!iscore || iscore->type != WTK_JSON_NUMBER) {
                    continue;
                }
                v = qtk_ecloud_get_middle_phone(ichar->v.str->data,
                                                ichar->v.str->len);
                idst = wtk_json_new_object(json);
                wtk_json_obj_add_str2_s(json, idst, "ichar", v.data, v.len);
                wtk_json_obj_add_ref_number_s(json, idst, "score",
                                              iscore->v.number);
                wtk_json_array_add_item(json, dst, idst);
            }
            wtk_json_obj_remove_s(ai->item, "phone");
            wtk_json_obj_add_item2_s(json, ai->item, "phone", dst);
        }
    }
    ret = 0;
end:
    return ret;
}

wtk_string_t qtk_ecloud_get_result(qtk_ecloud_t *e) {
    wtk_json_parser_t *parser = e->parser;
    wtk_json_item_t *item;
    qtk_spx_msg_t *msg;
    wtk_string_t v;
    int ret;

    msg = qtk_spx_get_result(e->spx);
    if (!msg) {
        wtk_debug("msg is null\n");
    }
    wtk_log_log(e->session->log, "spx result:%.*s\n", msg->body->pos,
                msg->body->data);
    wtk_json_parser_reset(parser);
    wtk_json_parser_parse(parser, msg->body->data, msg->body->pos);
    if (!parser->json->main) {
        wtk_log_warn0(e->session->log, "json parser failed");
    }
    switch (msg->type) {
    case QTK_SPX_MSG_END:
        break;
    case QTK_SPX_MSG_ERR:
        item = wtk_json_obj_get_s(parser->json->main, "errId");
        if (item && item->type == WTK_JSON_NUMBER) {
            _qtk_warning(e->session, (int)item->v.number);
        }
        ret = -1;
        goto end;
        break;
    default:
        wtk_log_warn(e->session->log, "unexpect msg type %d", msg->type);
        ret = -1;
        goto end;
    }
    item = parser->json->main;
    if (e->use_phone_ml) {
        ret = qtk_ecloud_wrd_phone_middle(parser->json, item);
        if (ret != 0) {
            wtk_log_warn(e->session->log, "ret = %d", ret);
            goto end;
        }
    }
    wtk_json_item_print(item, e->rlt_buf);
    wtk_string_set(&v, e->rlt_buf->data, e->rlt_buf->pos);
    ret = 0;

end:
    if (msg) {
        qtk_spx_push_msg(e->spx, msg);
    }
    if (ret != 0) {
        wtk_string_set(&v, 0, 0);
    }

    //	wtk_string_set(&v,msg->body->data,msg->body->pos);
    return v;
}
