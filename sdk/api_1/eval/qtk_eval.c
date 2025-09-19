#include "qtk_eval.h"
static void qtk_eval_init(qtk_eval_t *eval) {
    eval->cfg = NULL;
    eval->session = NULL;

    eval->ins.spx = NULL;

    eval->parser = NULL;
    eval->rlt_buf = NULL;
}

qtk_eval_t *qtk_eval_new(qtk_eval_cfg_t *cfg, qtk_session_t *session) {
    qtk_eval_t *eval;
    int ret;

    eval = (qtk_eval_t *)wtk_malloc(sizeof(qtk_eval_t));
    qtk_eval_init(eval);

    eval->cfg = cfg;
    eval->session = session;

    wtk_log_log(eval->session->log, "use cldeval = %d\n", cfg->use_cldeval);
    if (cfg->use_cldeval) {
        eval->ins.spx = qtk_spx_new(&cfg->spx, session);
        if (!eval->ins.spx) {
            wtk_log_log0(eval->session->log, "eval spx new failed.");
            ret = -1;
            goto end;
        }
    } else if (cfg->engsnt) {
        eval->ins.engsnt = wtk_engsnt_new(cfg->engsnt);
    } else {
        ret = -1;
        goto end;
    }

    eval->rlt_buf = wtk_strbuf_new(512, 1);
    eval->parser = wtk_json_parser_new();

    ret = 0;
end:
    if (ret != 0) {
        qtk_eval_delete(eval);
        eval = NULL;
    }
    return eval;
}

void qtk_eval_delete(qtk_eval_t *eval) {
    if (eval->cfg->use_cldeval) {
        if (eval->ins.spx) {
            qtk_spx_delete(eval->ins.spx);
        }
    } else {
        wtk_engsnt_delete(eval->ins.engsnt);
    }

    if (eval->parser) {
        wtk_json_parser_delete(eval->parser);
    }
    if (eval->rlt_buf) {
        wtk_strbuf_delete(eval->rlt_buf);
    }

    wtk_free(eval);
}

int qtk_eval_start(qtk_eval_t *eval, char *evaltxt, int len) {
    int ret;

    if (len <= 0) {
        _qtk_warning(eval->session, _QTK_EVAL_EVALTEXT_NOTSET);
        wtk_log_warn0(eval->session->log, "no eval test");
        return -1;
    }

    if (eval->cfg->use_cldeval) {
        ret = qtk_spx_start(eval->ins.spx, evaltxt, len, 0);
        if (ret != 0) {
            wtk_log_warn(eval->session->log, "ret = %d", ret);
            goto end;
        }
    } else {
        ret =
            wtk_engsnt_start(eval->ins.engsnt, WTK_REFTXT_NORMAL, evaltxt, len);
        if (ret != 0) {
            wtk_log_warn(eval->session->log, "ret = %d", ret);
            goto end;
        }
    }

    ret = 0;
end:
    return ret;
}

int qtk_eval_feed(qtk_eval_t *eval, char *data, int bytes, int is_end) {
    int ret = 0;

    if (eval->cfg->use_cldeval) {
        ret = qtk_spx_feed(eval->ins.spx, data, bytes, is_end);
    } else {
        ret = wtk_engsnt_feed(eval->ins.engsnt,
                              is_end ? WTK_PARM_END : WTK_PARM_APPEND, data,
                              bytes);
    }

    return ret;
}

int qtk_eval_reset(qtk_eval_t *eval) {
    int ret = 0;

    if (eval->cfg->use_cldeval) {
        qtk_spx_reset(eval->ins.spx);
    } else {
        ret = wtk_engsnt_reset(eval->ins.engsnt);
    }

    return ret;
}

wtk_string_t qtk_eval_get_middle_phone(char *data, int bytes) {
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

int qtk_eval_wrd_phone_middle(wtk_json_t *json, wtk_json_item_t *item) {
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
                v = qtk_eval_get_middle_phone(ichar->v.str->data,
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

static wtk_string_t qtk_eval_process_spx_result(qtk_eval_t *eval) {
    wtk_json_parser_t *parser = eval->parser;
    wtk_json_item_t *item;
    qtk_spx_msg_t *msg;
    wtk_string_t v;
    int ret;

    msg = qtk_spx_get_result(eval->ins.spx);
    if (!msg) {
        ret = -1;
        wtk_log_warn0(eval->session->log, "msg null");
        goto end;
    }
    wtk_log_log(eval->session->log, "eval recv [%.*s]", msg->body->pos,
                msg->body->data);
    wtk_json_parser_reset(parser);
    wtk_json_parser_parse(parser, msg->body->data, msg->body->pos);
    if (!parser->json->main) {
        wtk_log_warn0(eval->session->log, "json parser failed");
        ret = -1;
        goto end;
    }

    switch (msg->type) {
    case QTK_SPX_MSG_END:
        break;
    case QTK_SPX_MSG_ERR:
        item = wtk_json_obj_get_s(parser->json->main, "errId");
        if (item && item->type == WTK_JSON_NUMBER) {
            _qtk_warning(eval->session, (int)item->v.number);
        }
        ret = -1;
        goto end;
        break;
    default:
        wtk_log_warn(eval->session->log, "unexpect msg type %d", msg->type);
        ret = -1;
        goto end;
    }

    item = parser->json->main;
    if (eval->cfg->use_phone_ml) {
        ret = qtk_eval_wrd_phone_middle(parser->json, item);
        if (ret != 0) {
            wtk_log_warn(eval->session->log, "ret = %d", ret);
            goto end;
        }
    }
    wtk_json_item_print(item, eval->rlt_buf);
    wtk_string_set(&v, eval->rlt_buf->data, eval->rlt_buf->pos);

    ret = 0;
end:
    if (msg) {
        qtk_spx_push_msg(eval->ins.spx, msg);
    }
    if (ret != 0) {
        wtk_string_set(&v, 0, 0);
    }
    return v;
}

static wtk_string_t qtk_eval_process_engsnt_result(qtk_eval_t *eval) {
    wtk_json_parser_t *parser = eval->parser;
    wtk_string_t v;
    char *result = NULL;
    int result_len;
    int ret;

    ret =
        wtk_engsnt_get_jsonresult(eval->ins.engsnt, 100, &result, &result_len);
    if (ret != 0) {
        goto end;
    }

    if (eval->cfg->use_phone_ml) {
        wtk_json_parser_reset(parser);
        wtk_json_parser_parse(parser, result, result_len);

        if (!parser->json->main) {
            wtk_log_warn0(eval->session->log, "json parser failed");
            ret = -1;
            goto end;
        }

        ret = qtk_eval_wrd_phone_middle(parser->json, parser->json->main);
        if (ret != 0) {
            wtk_log_warn(eval->session->log, "ret = %d", ret);
            goto end;
        }

        wtk_json_item_print(parser->json->main, eval->rlt_buf);
    } else {
        wtk_strbuf_reset(eval->rlt_buf);
        wtk_strbuf_push(eval->rlt_buf, result, result_len);
    }

    wtk_string_set(&v, eval->rlt_buf->data, eval->rlt_buf->pos);

    ret = 0;
end:
    if (result) {
        wtk_free(result);
    }
    if (ret != 0) {
        wtk_string_set(&v, 0, 0);
    }
    return v;
}

wtk_string_t qtk_eval_get_result(qtk_eval_t *eval) {
    wtk_string_t v;

    if (eval->cfg->use_cldeval) {
        v = qtk_eval_process_spx_result(eval);
    } else {
        v = qtk_eval_process_engsnt_result(eval);
    }

    return v;
}

void qtk_eval_cancel(qtk_eval_t *eval) {
    if (eval->cfg->use_cldeval) {
        qtk_spx_cancel(eval->ins.spx);
    }
}

void qtk_eval_set_coreType(qtk_eval_t *eval, char *data, int len) {
    if (eval->cfg->use_cldeval) {
        qtk_spx_set_coreType(eval->ins.spx, data, len);
    }
}

void qtk_eval_set_res(qtk_eval_t *eval, char *data, int len) {
    if (eval->cfg->use_cldeval) {
        qtk_spx_set_res(eval->ins.spx, data, len);
    }
}
