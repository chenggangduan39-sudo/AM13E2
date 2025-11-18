#include "qtk_isemdlg.h"

static void qtk_isemdlg_init(qtk_isemdlg_t *i) {
    i->cfg = NULL;
    i->session = NULL;
    i->ins.spx = NULL;
    i->syn = 0;

    i->parser = NULL;
    i->input = NULL;
    i->result = NULL;

    i->semdlg_item = NULL;
}

qtk_isemdlg_t *qtk_isemdlg_new(qtk_isemdlg_cfg_t *cfg, qtk_session_t *session,
                               wtk_model_t *model) {
    qtk_isemdlg_t *i;
    int ret;

    i = (qtk_isemdlg_t *)wtk_malloc(sizeof(qtk_isemdlg_t));
    qtk_isemdlg_init(i);

    i->cfg = cfg;
    i->session = session;

    if (cfg->use_spx) {
        i->ins.spx = qtk_spx_new(&cfg->spx, session);
        if (!i->ins.spx) {
            wtk_log_log0(i->session->log, "isemdlg spx new failed.");
            ret = -1;
            goto end;
        }
    } else if (cfg->semdlg) {
        i->ins.semdlg = wtk_semdlg_new(cfg->semdlg);
    } else {
        ret = -1;
        goto end;
    }

    i->parser = wtk_json_parser_new();
    i->input = wtk_strbuf_new(256, 1);
    i->result = wtk_strbuf_new(256, 1);

    if (model) {
        i->semdlg_item = wtk_model_get_item_s(model, "semdlg");
    }

    wtk_log_log(i->session->log, "isemdlg %.*s new spx %d lc_custom %d",
                i->cfg->name.len, i->cfg->name.data, i->cfg->use_spx,
                i->cfg->lc_custom);
    ret = 0;
end:
    if (ret != 0) {
        qtk_isemdlg_delete(i);
        i = NULL;
    }
    return i;
}

void qtk_isemdlg_delete(qtk_isemdlg_t *i) {
    if (i->ins.semdlg) {
        i->cfg->use_spx ? qtk_spx_delete(i->ins.spx)
                        : wtk_semdlg_delete(i->ins.semdlg);
    }

    if (i->parser) {
        wtk_json_parser_delete(i->parser);
    }

    if (i->input) {
        wtk_strbuf_delete(i->input);
    }

    if (i->result) {
        wtk_strbuf_delete(i->result);
    }

    wtk_free(i);
}
#if 0
static int qtk_isemdlg_pre(qtk_isemdlg_t *i)
{
	wtk_json_parser_t *parser = i->parser;
	wtk_json_t *json = NULL;
	wtk_json_item_t *item;
	int ret;

	if(i->use_json) {
		wtk_json_parser_reset(parser);
		wtk_json_parser_parse(parser,i->input->data,i->input->pos);
		if(!parser->json->main) {
			ret = -1;
			goto end;
		}
		json = parser->json;
	} else {
		json = wtk_json_new();
		json->main = wtk_json_new_object(json);
		wtk_json_obj_add_str2_s(json,json->main,"rec",i->input->data,i->input->pos);
	}

	item = wtk_json_new_object(json);
	wtk_json_obj_add_str2_ss(json,item,"fld",".usr");
	wtk_json_obj_add_ref_number_s(json,item,"state",i->semdlg_item->v.v.i);
	wtk_json_obj_add_item2_s(json,json->main,"syn",item);

	ret = 0;
end:
	if(ret == 0) {
		wtk_json_print(json,i->input);
	}
	if(!i->use_json && json) {
		wtk_json_delete(json);
	}
	i->use_json = ret==0?1:i->use_json;
	return ret;
}
#endif
static int qtk_isemdlg_spx_process(qtk_isemdlg_t *i) {
    wtk_json_parser_t *parser = i->parser;
    wtk_json_item_t *item;
    qtk_spx_msg_t *msg = NULL;
    int ret;

    wtk_log_log(i->session->log, "%.*s : %.*s ", i->cfg->name.len,
                i->cfg->name.data, i->input->pos, i->input->data);
    ret = qtk_spx_start(i->ins.spx, i->input->data, i->input->pos, 0);
    if (ret != 0) {
        goto end;
    }

    msg = qtk_spx_get_result(i->ins.spx);
    if (!msg) {
        ret = -1;
        goto end;
    }

    wtk_log_log(i->session->log, "recved [%.*s]", msg->body->pos,
                msg->body->data);
    wtk_json_parser_reset(parser);
    wtk_json_parser_parse(parser, msg->body->data, msg->body->pos);
    if (!parser->json->main) {
        ret = -1;
        goto end;
    }

    switch (msg->type) {
    case QTK_SPX_MSG_END:
        break;
    case QTK_SPX_MSG_ERR:
        item = wtk_json_obj_get_s(parser->json->main, "errId");
        if (item && item->type == WTK_JSON_NUMBER) {
            qtk_session_feed_errcode(i->session, QTK_ERROR, _QTK_SERVER_ERR,
                                     msg->body->data, msg->body->pos);
        }
        ret = -1;
        goto end;
        break;
    default:
        wtk_log_warn(i->session->log, "unexpect msg type %d", msg->type);
        ret = -1;
        goto end;
        break;
    }

    ret = 0;
end:
    if (msg) {
        qtk_spx_push_msg(i->ins.spx, msg);
    }
    return ret;
}

static int qtk_isemdlg_dlg_process(qtk_isemdlg_t *i) {
    wtk_json_parser_t *parser = i->parser;
    wtk_string_t v;
    int ret;

    wtk_log_log(i->session->log, "%.*s : %.*s", i->cfg->name.len,
                i->cfg->name.data, i->input->pos, i->input->data);
    wtk_semdlg_process(i->ins.semdlg, i->input->data, i->input->pos);
    v = wtk_semdlg_get_result(i->ins.semdlg);
    wtk_semdlg_reset(i->ins.semdlg);
    if (v.len <= 0) {
        ret = -1;
        goto end;
    }

    wtk_json_parser_reset(parser);
    wtk_json_parser_parse(parser, v.data, v.len);
    if (!parser->json->main) {
        ret = -1;
        goto end;
    }

    ret = 0;
end:
    return ret;
}

static int qtk_isemdlg_check(qtk_isemdlg_t *i) {
    wtk_json_parser_t *parser = i->parser;
    wtk_json_item_t *item;

    item = wtk_json_obj_get_s(parser->json->main, "output");
    if (item && item->type == WTK_JSON_STRING && item->v.str->len > 0) {
        return 0;
    }

    item = wtk_json_obj_get_s(parser->json->main, "cmd");
    if (item &&
        (item->type == WTK_JSON_ARRAY || item->type == WTK_JSON_OBJECT)) {
        return 0;
    }

    item = wtk_json_obj_get_s(parser->json->main, "post");
    if (item &&
        (item->type == WTK_JSON_ARRAY || item->type == WTK_JSON_OBJECT)) {
        return 0;
    }

    item = wtk_json_obj_get_s(parser->json->main, "host");
    if (item &&
        (item->type == WTK_JSON_ARRAY || item->type == WTK_JSON_OBJECT)) {
        return 0;
    }

    return -1;
}

static void qtk_isemdlg_post(qtk_isemdlg_t *i) {
    wtk_json_parser_t *parser = i->parser;
    wtk_json_item_t *item;

    item = wtk_json_obj_get_s(parser->json->main, "host");
    if (!item || item->type != WTK_JSON_OBJECT) {
        return;
    }

    item = wtk_json_obj_get_s(item, "robot");
    if (!item || item->type != WTK_JSON_OBJECT) {
        return;
    }

    item = wtk_json_obj_get_s(item, "cmd");
    if (!item || item->type != WTK_JSON_STRING || item->v.str->len <= 0) {
        return;
    }

    if (wtk_string_cmp_s(item->v.str, "wakeup") == 0) {
        wtk_model_item_set_i(i->semdlg_item, 1);
    } else if (wtk_string_cmp_s(item->v.str, "sleep") == 0) {
        wtk_model_item_set_i(i->semdlg_item, 0);
    }
}

wtk_string_t qtk_isemdlg_process(qtk_isemdlg_t *i, char *data, int bytes,
                                 int use_json) {
    wtk_json_parser_t *parser = i->parser;
    wtk_json_item_t *rec;
    wtk_string_t v;

    int ret;

    if (bytes <= 0) {
        ret = -1;
        goto end;
    }
    wtk_strbuf_reset(i->input);
    if (use_json) {
        wtk_json_parser_reset(parser);
        wtk_json_parser_parse(parser, data, bytes);
        if (!parser->json->main) {
            wtk_log_warn0(i->session->log, "semdlg feed data is not json");
            ret = -1;
            goto end;
        }
        rec = wtk_json_obj_get_s(parser->json->main, "rec");
        if (!rec || rec->type != WTK_JSON_STRING) {
            wtk_log_warn0(i->session->log, "not found [rec] in feed data");
            ret = -1;
            goto end;
        }
        wtk_strbuf_push(i->input, rec->v.str->data, rec->v.str->len);
    } else {
        wtk_strbuf_push(i->input, data, bytes);
    }
    wtk_log_log(i->session->log, "%.*s :  syn %d  semdlg_item %p",
                i->cfg->name.len, i->cfg->name.data, i->syn, i->semdlg_item);

    //	if(i->syn && i->semdlg_item) {
    //		ret = qtk_isemdlg_pre(i);
    //		if(ret != 0) {
    //			goto end;
    //		}
    //	}

    if (i->cfg->use_spx) {
        ret = qtk_isemdlg_spx_process(i);
        if (ret != 0) {
            goto end;
        }
    } else {
        ret = qtk_isemdlg_dlg_process(i);
        if (ret != 0) {
            goto end;
        }
    }
    ret = i->cfg->lc_custom ? qtk_isemdlg_check(i) : 0;
    if (ret != 0) {
        goto end;
    }

    if (i->semdlg_item) {
        qtk_isemdlg_post(i);
    }

    wtk_json_item_print(parser->json->main, i->result);

    ret = 0;
end:
    if (ret != 0) {
        wtk_string_set(&v, 0, 0);
    } else {
        wtk_string_set(&v, i->result->data, i->result->pos);
    }
    return v;
}

void qtk_isemdlg_set_syn(qtk_isemdlg_t *i, int syn) {
    i->syn = syn == 1 ? 1 : 0;
}

void qtk_isemdlg_set_env(qtk_isemdlg_t *i, char *env, int bytes) {
    if (i->cfg->use_spx) {
        qtk_spx_set_env(i->ins.spx, env, bytes);
    } else {
        wtk_semdlg_set_env(i->ins.semdlg, env, bytes);
    }
}

void qtk_isemdlg_set_coreType(qtk_isemdlg_t *i, char *data, int len) {
    if (i->cfg->use_spx) {
        qtk_spx_set_coreType(i->ins.spx, data, len);
    }
}
void qtk_isemdlg_set_semRes(qtk_isemdlg_t *i, char *data, int len) {
    if (i->cfg->use_spx) {
        qtk_spx_set_semRes(i->ins.spx, data, len);
    }
}
