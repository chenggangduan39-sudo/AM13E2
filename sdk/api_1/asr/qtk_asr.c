#include "qtk_asr.h"

static int qtk_asr_utf8_skip_space(char *data, int bytes) {
    char *s, *e, c;
    int pos, n, i;

    s = data;
    e = data + bytes;
    pos = 0;

    while (s < e) {
        c = *s;
        if (c == ' ') {
            ++s;
            continue;
        }
        n = wtk_utf8_bytes(c);
        for (i = 0; i < n; ++i) {
            data[pos++] = s[i];
        }
        s += n;
    }

    return pos;
}

static int qtk_asr_process_route(qtk_asr_t *asr, wtk_json_item_t *man,
                                 int priority, int usrec) {
    wtk_json_item_t *item;

    // wtk_debug("usrec = %d\n", usrec);
    if (!usrec && asr->usrec) {
        wtk_log_log(asr->session->log, "usrec  =%d | asr->usrec = %d", usrec,
                    asr->usrec);
        return -1;
    }

    if (usrec ? priority <= asr->usrec_priority
              : priority <= asr->rec_priority) {
        wtk_log_log0(asr->session->log,
                     "current priority is lower than before.");
        return -1;
    }

    item = wtk_json_obj_get_s(man, "conf");
    if (!item || item->type != WTK_JSON_NUMBER || usrec
            ? item->v.number < asr->cfg->gr_min_conf
            : item->v.number < asr->cfg->rec_min_conf) {
        asr->err = QTK_ASR_NOT_CREDIBLE;
        wtk_log_log0(asr->session->log, "out of here.");
        return -1;
    }

    item = wtk_json_obj_get_s(man, "rec");
    if (!item || item->type != WTK_JSON_STRING || item->v.str->len <= 0) {
        asr->err = QTK_ASR_INVALID_AUDIO;
        wtk_log_log0(asr->session->log, "out of here.");
        return -1;
    }

    asr->usrec = usrec;
    if (asr->usrec) {
        asr->usrec_priority = priority;
    } else {
        asr->rec_priority = priority;
    }
    return 0;
}

void qtk_asr_lex_process(qtk_asr_t* asr,char* key){
    wtk_string_t result;
    wtk_json_item_t *item=NULL;
    wtk_json_parser_t *parser = asr->parser;
    wtk_json_parser_reset(parser);
    wtk_json_parser_parse(parser, asr->lex_buf->data,asr->lex_buf->pos);
    
    item = wtk_json_obj_get(parser->json->main, key,strlen(key));
    //在spx那里已经进行解析判断了，不需要再次判断
    //if(!item)return;
    result=wtk_lex_process(asr->lex,item->v.str->data,item->v.str->len);
    wtk_json_obj_remove(parser->json->main,key,strlen(key));
    wtk_json_obj_add_str2(parser->json,parser->json->main,key,strlen(key),result.data,result.len);
    wtk_strbuf_reset(asr->lex_buf);
    wtk_json_item_print(parser->json->main,asr->lex_buf);
    
}

static void qtk_asr_on_route(qtk_asr_t *asr, qtk_ir_state_t state,
                             qtk_ir_notify_t *notify) {
    wtk_json_parser_t *parser = asr->parser;
    wtk_json_item_t *man, *item, *item1, *item2, *rec;
    int ret = -1, ret1 = -1;

    wtk_lock_lock(&asr->lock);

    if (!asr->valid) {
        asr->waited += notify->wait;
        ++asr->notifyed;
        wtk_lock_unlock(&asr->lock);
        return;
    }

    switch (state) {
    case QTK_IR_DATA:
        wtk_json_parser_reset(parser);
        wtk_json_parser_parse(parser, notify->rec.data, notify->rec.len);
        if (!parser->json->main) {
            wtk_log_warn(asr->session->log,
                         "route %d rec [%.*s] not json format", notify->serial,
                         notify->rec.len, notify->rec.data);
            asr->waited += notify->wait;
            ++asr->notifyed;
            wtk_lock_unlock(&asr->lock);
            return;
        }

        man = wtk_json_obj_get_s(parser->json->main, "result");
        if (!man) {
            man = parser->json->main;
        }

        switch (asr->iasrs[notify->serial]->cfg->type) {
        case QTK_IASR_GR:
        case QTK_IASR_GR_NEW:
            ret = qtk_asr_process_route(asr, man, notify->priority, 1);
            wtk_log_log(asr->session->log, "ret = %d and [%.*s]", ret,
                        notify->rec.len, notify->rec.data);
            if (ret == 0) {
                wtk_json_item_print(man, asr->rec);
                if (asr->skip_space) {
                    qtk_asr_utf8_skip_space(asr->rec->data, asr->rec->pos);
                }
            }
            break;
        case QTK_IASR_LC:
            ret = qtk_asr_process_route(asr, man, notify->priority, 0);
            wtk_log_log(asr->session->log, "ret = %d and [%.*s]", ret,
                        notify->rec.len, notify->rec.data);
            if (ret == 0) {
                wtk_json_item_print(man, asr->rec);
                if (asr->skip_space) {
                    qtk_asr_utf8_skip_space(asr->rec->data, asr->rec->pos);
                }
            }
            break;
        default:
#if 0
            if (notify->iasr->cfg->skip_comm) {
                wtk_log_log(asr->session->log, "skip iasr comm result [%.*s]",
                            notify->rec.len, notify->rec.data);
                ret = -1;
            } else {
                ret = qtk_asr_process_route(asr, man, notify->priority, 0);
                wtk_log_log(asr->session->log, "ret = %d and [%.*s]", ret,
                            notify->rec.len, notify->rec.data);
            }
            item = wtk_json_obj_get_s(man, "usrec");
            if (item) {
                ret1 = qtk_asr_process_route(asr, item, notify->priority, 1);
                wtk_log_log(asr->session->log, "ret1 = %d and [%.*s]", ret1,
                            notify->rec.len, notify->rec.data);
                if (ret1 == 0) {
                    item1 = wtk_json_obj_get_s(item, "rec");
                    item2 = wtk_json_obj_get_s(item, "conf"); // 置信度
                    wtk_json_obj_remove_s(man, "conf");
                    wtk_json_obj_remove_s(man, "rec");
                    wtk_json_obj_add_item2_s(parser->json, man, "conf", item2);
                    wtk_json_obj_add_item2_s(parser->json, man, "rec", item1);
                    wtk_json_obj_remove_s(man, "usrec");
                } else {
                    wtk_json_obj_remove_s(man, "usrec");
                }
            }
            if (ret == 0 || ret1 == 0) {
                rec = wtk_json_obj_get_s(man, "rec");
                //				if(asr->cfg->skip_space) {
                //					rec->v.str->len =
                //qtk_asr_utf8_skip_space(rec->v.str->data,rec->v.str->len);
                //				}
                if (asr->cfg->use_json) {
                    wtk_json_item_print(man, asr->rec);
                } else {
                    wtk_strbuf_reset(asr->rec);
                    wtk_strbuf_push(asr->rec, rec->v.str->data,
                                    rec->v.str->len);
                }
            }
#endif
            break;
        }
        break;

    case QTK_IR_ERR:
        wtk_log_warn0(asr->session->log, "route return err");
        break;
    case QTK_IR_NOTHING:
        break;
    }

    asr->waited += notify->wait;
    ++asr->notifyed;
    wtk_lock_unlock(&asr->lock);
}

static void qtk_asr_init(qtk_asr_t *asr) {
    asr->lex=NULL;
    asr->lex_buf=NULL;
    asr->cfg = NULL;
    asr->session = NULL;
    asr->iasrs = NULL;
    asr->routes = NULL;
    asr->wait = 0;
    asr->waited = 0;
    asr->notifyed = 0;
    asr->rec_priority = 0;
    asr->usrec_priority = 0;
    asr->valid = 0;
    asr->usrec = 0;

    asr->rec = NULL;
    asr->parser = NULL;
    asr->hint_ths=NULL;
    asr->hint_notify=NULL;
    asr->spxfinal_ths=NULL;
    asr->spxfinal_notify=NULL;
    asr->restult_ths=NULL;
    asr->result_notify=NULL;

    asr->err = -1;
    asr->skip_space = 0;
}

qtk_asr_t *qtk_asr_new(qtk_asr_cfg_t *cfg, qtk_session_t *session) {
    qtk_asr_t *asr;
    int i, j;
    int ret;

    asr = (qtk_asr_t *)wtk_malloc(sizeof(qtk_asr_t));
    qtk_asr_init(asr);
    asr->cfg = cfg;
    asr->session = session;

    asr->n_iasrs = cfg->n_iasrs - cfg->skip_iasrs;

    if (asr->n_iasrs > 0) {
        asr->iasrs =
            (qtk_iasr_t **)wtk_malloc(sizeof(qtk_iasr_t *) * asr->n_iasrs);
        memset(asr->iasrs, 0, sizeof(qtk_iasr_t *) * asr->n_iasrs);
        asr->routes = (qtk_iasr_route_t **)wtk_malloc(
            sizeof(qtk_iasr_route_t *) * asr->n_iasrs);
        memset(asr->routes, 0, sizeof(qtk_iasr_route_t *) * asr->n_iasrs);
        for (i = 0, j = 0; i < cfg->n_iasrs; ++i) {
            if (!cfg->iasrs_valid[i]) {
                continue;
            }

            asr->iasrs[j] = qtk_iasr_new(cfg->iasrs + i, session);
            if (!asr->iasrs[j]) {
                ret = -1;
                goto end;
            }
            asr->wait += cfg->iasrs[j].wait;
            asr->routes[j] = qtk_iasr_route_new(asr->iasrs[j], asr->session, j);
            qtk_iasr_route_set_notify(
                asr->routes[j], asr, (qtk_iasr_route_notify_f)qtk_asr_on_route);
            ++j;
        }
        wtk_lock_init(&asr->lock);
    }

    asr->rec = wtk_strbuf_new(256, 1);
    asr->parser = wtk_json_parser_new();
    if(cfg->use_lex){
        asr->lex_buf=wtk_strbuf_new(512,1);
        asr->lex=wtk_lex_new(&cfg->lex_cfg);
        wtk_lex_compile(asr->lex, cfg->lex_fn.data);
    }
    wtk_lock_init(&asr->lex_lock);

    ret = 0;
end:
    if (ret != 0) {
        qtk_asr_delete(asr);
        asr = NULL;
    }
    return asr;
}

void qtk_asr_delete(qtk_asr_t *asr) {

    int i;

    for (i = 0; i < asr->n_iasrs; ++i) {
        if (asr->routes[i]) {
            qtk_iasr_route_delete(asr->routes[i]);
        }
        if (asr->iasrs[i]) {
            qtk_iasr_delete(asr->iasrs[i]);
        }
    }

    if (asr->iasrs) {
        wtk_free(asr->iasrs);
    }
    if (asr->routes) {
        wtk_free(asr->routes);
    }

    if (asr->rec) {
        wtk_strbuf_delete(asr->rec);
    }

    if (asr->parser) {
        wtk_json_parser_delete(asr->parser);
    }

    //spx_delete后，不会再有云端http包回调过来
    //必须在spx_delete之后释放lex，如果在spx_delete之前释放，在[释放lex结束，spx_delete]区间内，会有概率接收http包并回调给你
    if(asr->cfg->use_lex){
        wtk_lock_lock(&asr->lex_lock);
		wtk_lex_delete(asr->lex);
        wtk_strbuf_delete(asr->lex_buf);
		asr->lex=NULL;
        asr->lex_buf=NULL;
        wtk_lock_unlock(&asr->lex_lock);
    }

    wtk_lock_clean(&asr->lock);
    wtk_lock_clean(&asr->lex_lock);

    wtk_free(asr);
}

int qtk_asr_start(qtk_asr_t *asr) {
    int i, ret = 0;

    asr->waited = 0;
    asr->notifyed = 0;
    asr->valid = 1;
    asr->usrec_priority = 0;
    asr->rec_priority = 0;
    asr->usrec = 0;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_route_start(asr->routes[i]);
    }

    return ret;
}

int qtk_asr_feed(qtk_asr_t *asr, char *data, int bytes, int is_end) {
    int i;

    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_route_feed(asr->routes[i], data, bytes, is_end);
    }
    return 0;
}

static int qtk_asr_is_complete(qtk_asr_t *asr) {
    int ret;
    //	wtk_debug("notifyed  = %d n_iasrs = %d waited = %d wait = %d pos =
    //%d\n", 		asr->notifyed, 		asr->n_iasrs, 		asr->waited, 		asr->wait, 		asr->rec->pos
    //		);
    wtk_lock_lock(&asr->lock);
    if (asr->notifyed >= asr->n_iasrs) {
        ret = 1;
        goto end;
    }

    if (asr->waited == asr->wait && asr->rec->pos > 0) {
        ret = 1;
        goto end;
    }

    ret = 0;
end:
    wtk_lock_unlock(&asr->lock);
    return ret;
}

wtk_string_t qtk_asr_get_result(qtk_asr_t *asr) {
    wtk_string_t v;

    while (asr->valid) {
        if (qtk_asr_is_complete(asr)) {
            asr->valid = 0;
            break;
        }
        wtk_msleep(10);
    }
    wtk_string_set(&v, asr->rec->data, asr->rec->pos);
    return v;
}

void qtk_asr_reset(qtk_asr_t *asr) {
    int i;

    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_route_reset(asr->routes[i]);
    }
    wtk_strbuf_reset(asr->rec);
}

void qtk_asr_cancel(qtk_asr_t *asr) {
    int i;

    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_route_cancel(asr->routes[i]);
    }
    asr->valid = 0;
}

void qtk_asr_on_result(qtk_asr_t *asr, char *data, int bytes, float fs, float fe, int type)
{
    if(asr->result_notify)
    {
        asr->result_notify(asr->restult_ths, data, bytes, fs, fe, type);
    }
}

void qtk_asr_on_hint(qtk_asr_t* asr,char* data,int bytes){
    if(asr->hint_notify){
        if(asr->cfg->use_lex){
            wtk_lock_lock(&asr->lex_lock);
            if(asr->lex){
                wtk_strbuf_reset(asr->lex_buf);
                wtk_strbuf_push(asr->lex_buf,data,bytes);
                //double t1=time_get_ms();
                qtk_asr_lex_process(asr,"hint");
                //wtk_debug("YYY time use ==%lf\n",time_get_ms()-t1);
                asr->hint_notify(asr->hint_ths,asr->lex_buf->data,asr->lex_buf->pos);
            }
            wtk_lock_unlock(&asr->lex_lock);
        }else{
            asr->hint_notify(asr->hint_ths,data,bytes);
        }
    }
}
//spx最终结果回调
void qtk_asr_on_spxfinal(qtk_asr_t* asr,char* data,int bytes){
    if(asr->spxfinal_notify){
        if(asr->cfg->use_lex){
            wtk_lock_lock(&asr->lex_lock);
            if(asr->lex){
                wtk_strbuf_reset(asr->lex_buf);
                wtk_strbuf_push(asr->lex_buf,data,bytes);
                //double t1=time_get_ms();
                qtk_asr_lex_process(asr,"rec");
                //wtk_debug("YYY time use ==%lf\n",time_get_ms()-t1);
                asr->spxfinal_notify(asr->spxfinal_ths,asr->lex_buf->data,asr->lex_buf->pos);
            }
            wtk_lock_unlock(&asr->lex_lock);
        }else{
            asr->spxfinal_notify(asr->spxfinal_ths,data,bytes);
        }
    }
}

void qtk_asr_set_spxfinal_notify(qtk_asr_t *asr, void *ths,
                             qtk_iasr_set_hint_notify_f notify) {
    asr->spxfinal_ths=ths;
    asr->spxfinal_notify=notify;
    int i;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_set_spxfinal_notify(asr->iasrs[i], asr, (qtk_iasr_set_hint_notify_f)qtk_asr_on_spxfinal);
    }
}

void qtk_asr_set_hint_notify(qtk_asr_t *asr, void *ths,
                             qtk_iasr_set_hint_notify_f notify) {
    asr->hint_ths=ths;
    asr->hint_notify=notify;
    int i;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_set_hint_notify(asr->iasrs[i], asr, (qtk_iasr_set_hint_notify_f)qtk_asr_on_hint);
    }
}

void qtk_asr_set_result_notify(qtk_asr_t *asr, void *ths, qtk_iasr_set_result_f notify)
{
    asr->restult_ths = ths;
    asr->result_notify = notify;
    int i;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_set_result_notify(asr->iasrs[i], asr, (qtk_iasr_set_result_f)qtk_asr_on_result);
    }  
}

void qtk_asr_set_hw_notify(qtk_asr_t *asr, void *ths,
                           qtk_iasr_set_hw_notify_f notify_f) {
    int i;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_set_hw_notify(asr->iasrs[i], ths, notify_f);
    }
}

int qtk_asr_hw_upload(qtk_asr_t *asr, char *res_fn, int flag) {
    int i;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_hw_upload(asr->iasrs[i], res_fn, flag);
    }
    return 0;
}

int qtk_asr_hw_update(qtk_asr_t *asr, char *res_fn, int flag) {
    int i;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_hw_update(asr->iasrs[i], res_fn, flag);
    }
    return 0;
}

int qtk_asr_hw_get_hotword(qtk_asr_t *asr) {
    int i;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_get_hotword(asr->iasrs[i]);
    }
    return 0;
}

void qtk_asr_set_res(qtk_asr_t *asr, char *data, int len) {
    int i;

    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_set_res(asr->iasrs[i], data, len);
    }
}
void qtk_asr_set_coreType(qtk_asr_t *asr, char *data, int len) {
    int i;

    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_set_coreType(asr->iasrs[i], data, len);
    }
}

void qtk_asr_set_skip_space(qtk_asr_t *asr, int skip_space) {
    int i;

    asr->skip_space = skip_space;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_set_skip_space(asr->iasrs[i], skip_space);
    }
}

void qtk_asr_set_xbnf(qtk_asr_t *asr, char *xbnf, int len) {
    int i;
    for (i = 0; i < asr->n_iasrs; ++i) {
        qtk_iasr_set_xbnf(asr->iasrs[i], xbnf, len);
    }
}

void qtk_asr_set_idle_time(qtk_asr_t *asr, int itime)
{
    int i;
    for(i=0; i < asr->n_iasrs; ++i){
        qtk_iasr_set_idle_time(asr->iasrs[i], itime);
    }
}

int qtk_asr_update_cmds(qtk_asr_t* asr,char* words,int len){
	int i;
	int ret;
	for(i=0;i<asr->n_iasrs;++i){
		ret=qtk_iasr_update_cmds(asr->iasrs[i], words, len);
		if(ret!=0)return ret;
	}
	return 0;
}
