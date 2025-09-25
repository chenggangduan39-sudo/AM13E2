#include "qtk_spx.h"
#ifndef _WIN32
#include "uuid.h"
#endif

static int lc_failed = 0;
static int lc_try_fail = 0;

static qtk_spx_msg_t *qtk_spx_msg_new(qtk_spx_t *spx) {
    qtk_spx_msg_t *msg;

    msg = (qtk_spx_msg_t *)wtk_malloc(sizeof(qtk_spx_msg_t));
    if (spx->cfg->use_luabuf) {
        msg->lua = wtk_strbuf_new(spx->cfg->lua_bufsize, 1);
    } else {
        msg->lua = NULL;
    }
    msg->body = wtk_strbuf_new(spx->cfg->bufsize, 1);

    return msg;
}

static int qtk_spx_msg_delete(qtk_spx_msg_t *msg) {
    if (msg->lua) {
        wtk_strbuf_delete(msg->lua);
    }
    wtk_strbuf_delete(msg->body);
    wtk_free(msg);
    return 0;
}

qtk_spx_msg_t* qtk_spx_get_result(qtk_spx_t* spx){
    return NULL;
}

qtk_spx_msg_t *qtk_spx_pop_msg(qtk_spx_t *spx) {
    qtk_spx_msg_t *msg;

    msg = (qtk_spx_msg_t *)wtk_lockhoard_pop(&spx->msg_hoard);
    if (msg->lua) {
        wtk_strbuf_reset(msg->lua);
    }
    wtk_strbuf_reset(msg->body);
    return msg;
}

void qtk_spx_push_msg(qtk_spx_t *spx, qtk_spx_msg_t *msg) {
    wtk_lockhoard_push(&spx->msg_hoard, msg);
}

static void qtk_spx_on_httpc(qtk_spx_t *spx, qtk_http_response_t *rep) {
    wtk_lock_lock(&spx->lock);
    qtk_spx_msg_t *msg;
    wtk_string_t *v = NULL;
    wtk_json_parser_t *parser = spx->parser;
    wtk_json_item_t *hint = NULL;
    wtk_json_item_t *rec = NULL;
    wtk_json_item_t *item = NULL;
    unsigned long counter;

    msg = qtk_spx_pop_msg(spx);
    // qtk_http_response_print(rep);
    wtk_strbuf_push(msg->body, rep->body->data, rep->body->pos);
    wtk_json_parser_reset(parser);
    wtk_json_parser_parse(parser, msg->body->data, msg->body->pos);

    switch (rep->status) {
    case 100:
        msg->type = QTK_SPX_MSG_APPEND;
        break;
    case 200:
        msg->type = QTK_SPX_MSG_END;
        break;
    default:
        msg->type = QTK_SPX_MSG_ERR;
        wtk_log_warn(spx->session->log, "status %d", rep->status);
        wtk_log_warn(spx->session->log, "result type %d msg [%.*s]",
                        msg->type, msg->body->pos, msg->body->data);
        if (!parser->json->main) {
            goto end;
        }
        item = wtk_json_obj_get_s(parser->json->main, "errId");
        if (item && item->type == WTK_JSON_NUMBER) {
            qtk_session_feed_errcode(spx->session, QTK_ERROR,_QTK_SERVER_ERR, 
                                    msg->body->data,msg->body->pos);
        }
        goto end;
        break;
    }

    v = (wtk_string_t *)wtk_str_hash_find_s(rep->hdr_hash, "lua");
    if (v && msg->lua) {
        wtk_strbuf_push(msg->lua, v->data, v->len);
    }
    
    v = (wtk_string_t *)wtk_str_hash_find_s(rep->hdr_hash, "hook");
    if (!v) {
        wtk_log_warn0(spx->session->log, "no hook");
        goto end;
    }
    counter =wtk_str_atoi(v->data + spx->id->len, v->len - spx->id->len);

    if (parser->json->main){
        wtk_json_obj_add_ref_number_s(parser->json,parser->json->main,"counter",counter);
        wtk_json_obj_add_ref_number_s(parser->json,parser->json->main,"spx_counter",spx->counter);
        wtk_strbuf_reset(msg->body);
        wtk_json_item_print(parser->json->main,msg->body);
        if (spx->cfg->use_hint) {
            hint = wtk_json_obj_get_s(parser->json->main, "hint");
            if (hint && hint->v.str->len > 0) {
                spx->notify_func(spx->ths, msg);
            }else{
                rec = wtk_json_obj_get_s(parser->json->main, "rec");
                if(rec && rec->v.str->len > 0){
                    spx->notify_func(spx->ths, msg);
                }
            }
        } else {
            rec = wtk_json_obj_get_s(parser->json->main, "rec");
            if (rec && rec->v.str->len > 0) {
                spx->notify_func(spx->ths, msg);
            }
        }
    }

end:
    qtk_spx_push_msg(spx, msg);
    wtk_lock_unlock(&spx->lock);
}

static void qtk_spx_init(qtk_spx_t *spx) {
    spx->cfg = NULL;
    spx->session = NULL;

    spx->cldhub = NULL;
    spx->oggenc = NULL;

    spx->env = NULL;
    spx->buf = NULL;
    spx->hook = NULL;
    spx->id = NULL;
    spx->counter = 0;

    spx->res = NULL;
    spx->coreType = NULL;
    spx->semRes = NULL;

    spx->add_hdr_ths = NULL;
    spx->add_hdr_func = NULL;

    spx->parser=NULL;
    spx->ths=NULL;
    spx->notify_func = NULL;
}

int qtk_spx_set_cldhub(qtk_spx_t *spx) {
    if (spx->session->cldhub) {
        spx->cldhub = spx->session->cldhub;
        qtk_cldhub_set_spx(spx->cldhub, spx->id->data, spx,
                           (qtk_cldhub_spx_notify_f)qtk_spx_on_httpc);
        return 0;
    }
    return -1;
}

qtk_spx_t *qtk_spx_new(qtk_spx_cfg_t *cfg, qtk_session_t *session) {
    qtk_spx_t *spx;
    uuid_t uuid;
    char buf[128];

    spx = (qtk_spx_t *)wtk_malloc(sizeof(qtk_spx_t));
    qtk_spx_init(spx);
    wtk_lock_init(&spx->lock);

    spx->cfg = cfg;
    spx->session = session;

    wtk_log_log(spx->session->log,
                "spx coretype = %s use_ogg = %d  useStream = %d",
                spx->cfg->coreType.data, cfg->use_ogg, cfg->useStream);

    spx->hook = wtk_strbuf_new(64, 1);
 #ifdef _WIN32
    GUID guid;
    CoCreateGuid(&guid);
    snprintf(buf, sizeof(buf),
             "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", guid.Data1,
             guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
             guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
             guid.Data4[6], guid.Data4[7]);
 #else
    uuid_generate(uuid);
    uuid_unparse(uuid, buf);
#endif
    wtk_strbuf_push_c(spx->hook, (unsigned char)strlen(buf));
    wtk_strbuf_push(spx->hook, buf, strlen(buf));
    spx->id = wtk_string_dup_data(spx->hook->data, spx->hook->pos);
    wtk_log_log(spx->session->log, "spx id = %s", buf);

    qtk_spx_set_cldhub(spx);

    wtk_lockhoard_init(&spx->msg_hoard, offsetof(qtk_spx_msg_t, hoard_n),
                       cfg->cache, (wtk_new_handler_t)qtk_spx_msg_new,
                       (wtk_delete_handler_t)qtk_spx_msg_delete, spx);

    spx->buf = wtk_strbuf_new(512, 1);
    spx->env = wtk_strbuf_new(128, 1);
    qtk_spx_clean_env(spx);
    spx->parser = wtk_json_parser_new();

    if (cfg->use_ogg) {
        spx->oggenc = qtk_oggenc_new(&cfg->oggenc);
    }
    ++lc_try_fail;

    return spx;
}

void qtk_spx_set_add_hdr(qtk_spx_t *spx, void *add_hdr_ths,
                         qtk_spx_add_hdr_func add_hdr_func) {
    spx->add_hdr_ths = add_hdr_ths;
    spx->add_hdr_func = add_hdr_func;
}

void qtk_spx_set_notify(qtk_spx_t *spx, void *ths,qtk_spx_notify_func notify_func) {
    spx->ths = ths;
    spx->notify_func = notify_func;
}

void qtk_spx_delete(qtk_spx_t *spx) {
    --lc_try_fail;


    if (spx->cldhub) {
        qtk_cldhub_del_spx(spx->cldhub, spx->id->data);
    }

    wtk_lock_lock(&spx->lock);
    wtk_lockhoard_clean(&spx->msg_hoard);

    if (spx->env) {
        wtk_strbuf_delete(spx->env);
    }
    if (spx->buf) {
        wtk_strbuf_delete(spx->buf);
    }
    if (spx->id) {
        wtk_string_delete(spx->id);
    }
    if (spx->hook) {
        wtk_strbuf_delete(spx->hook);
    }
    if (spx->oggenc) {
        qtk_oggenc_delete(spx->oggenc);
    }
    if (spx->res) {
        wtk_string_delete(spx->res);
    }
    if (spx->coreType) {
        wtk_string_delete(spx->coreType);
    }
    if (spx->semRes) {
        wtk_string_delete(spx->semRes);
    }
    if (spx->parser) {
        wtk_json_parser_delete(spx->parser);
    }
    wtk_lock_unlock(&spx->lock);
    wtk_lock_clean(&spx->lock);
    wtk_free(spx);
}

static void qtk_spx_mk_start(qtk_spx_t *spx, char *txt, int bytes,
                             int use_json) {
	int ret=0; char tmp[64]={0};
	ret = snprintf(tmp, 64, "use_hint=%d;use_vad=%d;max_hint=%d;use_timestamp=%d;", 
					spx->iasr_cfg->use_hint, spx->iasr_cfg->use_vad,spx->iasr_cfg->max_hint,spx->iasr_cfg->use_timestamp);
	qtk_spx_set_env(spx,tmp, ret);

    wtk_strbuf_t *buf = spx->buf;
    wtk_json_t *json;
    wtk_json_item_t *man, *item, *param;

    json = wtk_json_new();
    man = wtk_json_new_object(json);
    wtk_json_obj_add_str2_ss(json, man, "cmd", "start");

    param = wtk_json_new_object(json);
    wtk_json_obj_add_item2_s(json, man, "param", param);

    item = wtk_json_new_object(json);
    if (spx->cfg->use_ogg) {
        wtk_json_obj_add_str2_ss(json, item, "audioType", "ogg");
    } else {
        wtk_json_obj_add_str2_ss(json, item, "audioType", "wav");
    }
    wtk_json_obj_add_ref_number_s(json, item, "sampleBytes",
                                  spx->cfg->bytes_per_sample);
    wtk_json_obj_add_ref_number_s(json, item, "sampleRate", spx->cfg->rate);
    wtk_json_obj_add_ref_number_s(json, item, "channel", spx->cfg->channel);
    wtk_json_obj_add_item2_s(json, param, "audio", item);

    item = wtk_json_new_object(json);
    wtk_json_obj_add_str2_s(json, item, "coreType", spx->cfg->coreType.data,
                            spx->cfg->coreType.len);
    if (spx->cfg->synRes.len > 0) {
        wtk_json_obj_add_str2_s(json, item, "synRes", spx->cfg->synRes.data,
                                spx->cfg->synRes.len);
    }
    if (spx->cfg->semRes.len > 0) {
        wtk_json_obj_add_str2_s(json, item, "semRes", spx->cfg->semRes.data,
                                spx->cfg->semRes.len);
    }
    if (spx->cfg->res.len > 0) {
        wtk_json_obj_add_str2_s(json, item, "res", spx->cfg->res.data,
                                spx->cfg->res.len);
    }
    wtk_json_obj_add_ref_number_s(json, item, "usehotword",
                                  spx->cfg->use_hotword);
    if (spx->env->pos > 0) {
        wtk_json_obj_add_str2_s(json, item, "env", spx->env->data,
                                spx->env->pos);
    }
    if (spx->cfg->iType > 0) {
        wtk_json_obj_add_ref_number_s(json, item, "iType", spx->cfg->iType);
    }
    wtk_json_obj_add_ref_number_s(json, item, "pitch", spx->cfg->pitch);
    wtk_json_obj_add_ref_number_s(json, item, "volume", spx->cfg->volume);
    wtk_json_obj_add_ref_number_s(json, item, "speed", spx->cfg->speed);
    wtk_json_obj_add_ref_number_s(json, item, "useStream", spx->cfg->useStream);
    wtk_json_obj_add_ref_number_s(json, item, "stripSpace",
                                  spx->cfg->skip_space);
    if (bytes > 0) {
        wtk_json_obj_add_str2_s(json, item, "text", txt, bytes);
    }
    if (use_json) {
        wtk_json_obj_add_ref_number_s(json, item, "useJson", use_json);
    }
    if (spx->add_hdr_func) {
        spx->add_hdr_func(spx->add_hdr_ths, json, item);
    }
    wtk_json_obj_add_item2_s(json, param, "request", item);

    wtk_json_item_print(man, buf);
    wtk_json_delete(json);
}

static void qtk_spx_mk_end(qtk_spx_t *spx) {
    wtk_strbuf_t *buf = spx->buf;

    wtk_strbuf_reset(buf);
    wtk_strbuf_push_s(buf, "{\"cmd\":\"stop\"}");
    // wtk_debug("%.*s\n", buf->pos, buf->data);
}

static void qtk_spx_start_update_hook(qtk_spx_t *spx) {
    wtk_strbuf_reset(spx->hook);
    wtk_strbuf_push(spx->hook, spx->id->data, spx->id->len);
    wtk_strbuf_push_f(spx->hook, "%ld", spx->counter);
}

static int qtk_spx_add_cmd_hdr(qtk_spx_t *spx, wtk_strbuf_t *buf) {
    wtk_strbuf_push_s(buf, "Content-Type: text/plain\r\n");
    if (spx->hook) {
        wtk_strbuf_push_s(buf, "Hook: ");
        wtk_strbuf_push(buf, spx->hook->data, spx->hook->pos);
        wtk_strbuf_push_s(buf, "\r\n");
    }
    return 0;
}

int qtk_spx_add_data_hdr(qtk_spx_t *spx, wtk_strbuf_t *buf) {
    if (spx->oggenc) {
        wtk_strbuf_push_s(buf, "Content-Type: audio/ogg\r\n");
    } else {
        wtk_strbuf_push_s(buf, "Content-Type: audio/x-wav\r\n");
    }

    wtk_strbuf_push_s(buf, "Hook: ");
    wtk_strbuf_push(buf, spx->hook->data, spx->hook->pos);
    wtk_strbuf_push_s(buf, "\r\n");
    return 0;
}

static void qtk_spx_on_oggenc(qtk_spx_t *spx, char *data, int bytes) {
    wtk_strbuf_push(spx->buf, data, bytes);
}

int qtk_spx_start(qtk_spx_t *spx, char *txt, int bytes, int use_json) {
    wtk_strbuf_t *buf = spx->buf;
    int ret;

    if (!spx->cldhub) {
        ret = qtk_spx_set_cldhub(spx);
        if (ret != 0) {
            goto end;
        }
    }
    ++spx->counter;
    qtk_spx_mk_start(spx, txt, bytes, use_json);
    wtk_log_log(spx->session->log, "start [%.*s]", buf->pos, buf->data);
    // wtk_debug("start [%.*s] use_json %d\n",buf->pos,buf->data,use_json);

    qtk_spx_start_update_hook(spx);
    // wtk_debug("****************** spx counter : %ld spx hook : %.*s\n",
    // spx->counter, spx->hook->pos, spx->hook->data);
    ret = qtk_cldhub_feed(spx->cldhub, buf->data, buf->pos, spx,
                          (qtk_httpc_add_hdr_f)qtk_spx_add_cmd_hdr);
    if (ret != 0) {
        ret = qtk_cldhub_feed(spx->cldhub, buf->data, buf->pos, spx,
                              (qtk_httpc_add_hdr_f)qtk_spx_add_cmd_hdr);
        if (ret != 0) {
            goto end;
        }
    }

    if (spx->oggenc) {
        wtk_strbuf_reset(spx->buf);
        qtk_oggenc_set_write(spx->oggenc, spx,
                             (qtk_oggenc_write_f)qtk_spx_on_oggenc);
        qtk_oggenc_start(spx->oggenc, spx->cfg->rate, spx->cfg->channel,
                         spx->cfg->bytes_per_sample << 3);
    }

    ret = 0;
end:
    return ret;
}

int qtk_spx_feed(qtk_spx_t *spx, char *data, int bytes, int is_end) {
    wtk_strbuf_t *buf = spx->buf;
    int ret;

    if (spx->oggenc) {
        qtk_oggenc_encode(spx->oggenc, data, bytes, is_end);
        data = buf->data;
        bytes = buf->pos;
    }

    if (bytes > 0) {
        ret = qtk_cldhub_feed(spx->cldhub, data, bytes, spx,
                              (qtk_httpc_add_hdr_f)qtk_spx_add_data_hdr);
        if (ret != 0) {
            wtk_log_log0(spx->session->log, "qtk cldhub feed failed");
            goto end;
        }
        wtk_strbuf_reset(buf);
    }
    if (is_end) {
        wtk_strbuf_reset(buf);
        qtk_spx_mk_end(spx);
        //wtk_log_log(spx->session->log, "end [%.*s]", buf->pos, buf->data);
        ret = qtk_cldhub_feed(spx->cldhub, buf->data, buf->pos, spx,
                              (qtk_httpc_add_hdr_f)qtk_spx_add_cmd_hdr);
        if (ret != 0) {
            goto end;
        }
    }

    ret = 0;
end:
    return ret;
}

void qtk_spx_reset(qtk_spx_t *spx) {
    qtk_spx_msg_t *msg;
    wtk_queue_node_t *qn;

    //	++spx->counter;
    if (spx->oggenc) {
        qtk_oggenc_reset(spx->oggenc);
    }
    wtk_strbuf_reset(spx->buf);
}

void qtk_spx_cancel(qtk_spx_t *spx) { }

void qtk_spx_set_env(qtk_spx_t *spx, char *env, int bytes) {
    wtk_strbuf_reset(spx->env);
    wtk_strbuf_push(spx->env, env, bytes);
}

void qtk_spx_clean_env(qtk_spx_t *spx) {
    wtk_strbuf_reset(spx->env);
    wtk_strbuf_push(spx->env, spx->cfg->env.data, spx->cfg->env.len);
}

void qtk_spx_set_speed(qtk_spx_t *spx, float speed) { spx->cfg->speed = speed; }

void qtk_spx_set_volume(qtk_spx_t *spx, float volume) {
    spx->cfg->volume = volume;
}

void qtk_spx_set_pitch(qtk_spx_t *spx, float pitch) { spx->cfg->pitch = pitch; }

void qtk_spx_set_coreType(qtk_spx_t *spx, char *data, int len) {
    if (spx->coreType) {
        wtk_string_delete(spx->coreType);
    }
    spx->coreType = wtk_string_dup_data(data, len);
    wtk_string_set(&spx->cfg->coreType, spx->coreType->data,
                   spx->coreType->len);
}
void qtk_spx_set_res(qtk_spx_t *spx, char *data, int len) {
    if (spx->res) {
        wtk_string_delete(spx->res);
    }
    spx->res = wtk_string_dup_data(data, len);
    wtk_string_set(&spx->cfg->res, spx->res->data, spx->res->len);
}
void qtk_spx_set_semRes(qtk_spx_t *spx, char *data, int len) {
    if (spx->semRes) {
        wtk_string_delete(spx->semRes);
    }
    spx->semRes = wtk_string_dup_data(data, len);
    wtk_string_set(&spx->cfg->semRes, spx->semRes->data, spx->semRes->len);
}
void qtk_spx_set_useStream(qtk_spx_t *spx, int useStream) {
    spx->cfg->useStream = useStream;
}
void qtk_spx_set_skip_space(qtk_spx_t *spx, int skip_space) {
    spx->cfg->skip_space = skip_space;
}
