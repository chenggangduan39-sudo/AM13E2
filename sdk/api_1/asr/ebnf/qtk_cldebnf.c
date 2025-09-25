#include "qtk_cldebnf.h"

void qtk_cldebnf_on_http_head(qtk_cldebnf_t *cnf, qtk_http_response_t *rep) {
    switch (rep->status) {
    case 200:
        wtk_log_log0(cnf->session->log, "ebnf exist");
        //		qtk_cldlog_log0(cnf->session->clog,"ebnf exit");
        cnf->state = QTK_CLDEBNF_VALID;
        break;
    case 404:
        wtk_log_log0(cnf->session->log, "ebnf not exist");
        //		qtk_cldlog_log0(cnf->session->clog,"ebnf not exist");
        cnf->state = QTK_CLDEBNF_INIT;
        break;
    default:
        wtk_log_warn(cnf->session->log, "ebnf server error status = %d",
                     rep->status);
        //		qtk_cldlog_warn(cnf->session->clog,"ebnf server error
        //status = %d",rep->status);
        cnf->state = QTK_CLDEBNF_INVALID;
        _qtk_error(cnf->session, _QTK_CLDEBNF_SERVER_ERROR);
        break;
    }
}

void qtk_cldebnf_on_http_post(qtk_cldebnf_t *cnf, qtk_http_response_t *rep) {
    wtk_json_parser_t *parser;
    wtk_json_item_t *item;

    if (rep->status != 200) {
        wtk_log_warn(cnf->session->log, "rep status = %d", rep->status);
        //		qtk_cldlog_warn(cnf->session->clog,"rep status =
        //%d",rep->status);
        _qtk_error(cnf->session, _QTK_CLDEBNF_SERVER_ERROR);
        cnf->state = QTK_CLDEBNF_INVALID;
        return;
    }

    if (rep->body->pos > 0) {
        parser = wtk_json_parser_new();
        wtk_json_parser_reset(parser);
        wtk_json_parser_parse(parser, rep->body->data, rep->body->pos);
        if (!parser->json->main) {
            cnf->state = QTK_CLDEBNF_INVALID;
            wtk_json_parser_delete(parser);
            wtk_log_warn(cnf->session->log, "rep body not json [%.*s]",
                         rep->body->pos, rep->body->data);
            //			qtk_cldlog_warn(cnf->session->clog,"rep body not json
            //[%.*s]",rep->body->pos,rep->body->data);
            return;
        }

        item = wtk_json_obj_get_s(parser->json->main, "code");
        if (!item || item->type != WTK_JSON_NUMBER) {
            cnf->state = QTK_CLDEBNF_INVALID;
            wtk_json_parser_delete(parser);
            wtk_log_warn(cnf->session->log, "rep code not number [%.*s]",
                         rep->body->pos, rep->body->data);
            //			qtk_cldlog_warn(cnf->session->clog,"rep code not number
            //[%.*s]",rep->body->pos,rep->body->data);
            return;
        }

        if (((int)item->v.number) != 0) {
            wtk_log_warn(cnf->session->log, "rep code = %ld [%.*s]",
                         item->v.number, rep->body->pos, rep->body->data);
            //			qtk_cldlog_warn(cnf->session->clog,"rep code = %ld
            //[%.*s]",item->v.number,rep->body->pos,rep->body->data);
            cnf->state = QTK_CLDEBNF_INVALID;
            wtk_json_parser_delete(parser);
            return;
        }
    } else {
        wtk_log_warn0(cnf->session->log, "rep body is empty");
        //		qtk_cldlog_warn0(cnf->session->clog,"rep body is
        //empty");
        cnf->state = QTK_CLDEBNF_INVALID;
        return;
    }

    cnf->state = QTK_CLDEBNF_VALID;
}

void qtk_cldebnf_add_req_hdr1(qtk_cldebnf_t *cnf, wtk_strbuf_t *buf) {
    wtk_strbuf_push_f(buf, "HOST:%.*s:%.*s\r\n", cnf->cfg->httpc.host.len,
                      cnf->cfg->httpc.host.data, cnf->cfg->httpc.port.len,
                      cnf->cfg->httpc.port.data);
}

void qtk_cldebnf_add_req_hdr2(qtk_cldebnf_t *cnf, wtk_strbuf_t *buf) {
    wtk_strbuf_push_f(buf, "HOST:%.*s:%.*s\r\n", cnf->cfg->httpc.host.len,
                      cnf->cfg->httpc.host.data, cnf->cfg->httpc.port.len,
                      cnf->cfg->httpc.port.data);
    wtk_strbuf_push_s(buf, "Content-Type: text/plain\r\n");
}

int qtk_cldebnf_run(qtk_cldebnf_t *cnf, wtk_thread_t *t) {
    char tmp[32];
    int ret;
    int len;
    wtk_strbuf_t *buf;
    char *s, *e;
    typedef enum {
        URL_VERSION_V1 = 49,
        URL_VERSION_V2,
        URL_VERSION_V3,
        URL_VERSION_V4,
        URL_VERSION_V5,
    } url_version_t;
    url_version_t i = URL_VERSION_V1;

    len = cnf->ebnf_data->len + 2;
    buf = wtk_strbuf_new(len, 1);
    s = cnf->cfg->url_pre.data;
    e = cnf->cfg->url_pre.data + cnf->cfg->url_pre.len;
    while (s < e) {
        if (*s == 'v') {
            s++;
            i = *s;
            break;
        }
        s++;
    }
    switch (i) {
    case URL_VERSION_V1:
        break;
    case URL_VERSION_V2:
        wtk_strbuf_push_s(buf, "v2");
        break;
    case URL_VERSION_V3:
        wtk_strbuf_push_s(buf, "v3");
        break;
    case URL_VERSION_V4:
        wtk_strbuf_push_s(buf, "v4");
        break;
    case URL_VERSION_V5:
        wtk_strbuf_push_s(buf, "v5");
        break;
    }

    wtk_strbuf_push(buf, cnf->ebnf_data->data, cnf->ebnf_data->len);
    md5_hex(buf->data, buf->pos, tmp);
    cnf->md5Id = wtk_string_dup_data(tmp, 32);
    wtk_log_log(cnf->session->log, "md5Id = %.*s\n", cnf->md5Id->len,
                cnf->md5Id->data);
    //	qtk_cldlog_log(cnf->session->clog,"md5Id =
    //%.*s\n",cnf->md5Id->len,cnf->md5Id->data);
    wtk_strbuf_delete(buf);

    wtk_strbuf_reset(cnf->url);
    wtk_strbuf_push(cnf->url, cnf->cfg->url_pre.data, cnf->cfg->url_pre.len);
    wtk_strbuf_push(cnf->url, cnf->md5Id->data, cnf->md5Id->len);
    wtk_string_set(&cnf->cfg->httpc.url, cnf->url->data, cnf->url->pos);
    wtk_log_log(cnf->session->log, "httpc.url = %.*s\n",
                cnf->cfg->httpc.url.len, cnf->cfg->httpc.url.data);
    //	qtk_cldlog_log(cnf->session->clog,"httpc.url =
    //%.*s\n",cnf->cfg->httpc.url.len,cnf->cfg->httpc.url.data);

    while (cnf->run) {
        qtk_httpc_set_handler(
            cnf->httpc, cnf,
            (qtk_httpc_request_handler_f)qtk_cldebnf_on_http_head);
        ret = qtk_httpc_head(cnf->httpc, NULL, 0, cnf,
                             (qtk_httpc_add_hdr_f)qtk_cldebnf_add_req_hdr1);
        qtk_httpc_connect_reset(cnf->httpc);
        if (ret != 0) {
            wtk_sem_release(&cnf->sem, 1);
            wtk_msleep(cnf->cfg->interval);
            continue;
        }
        if (cnf->state != QTK_CLDEBNF_INIT) {
            break;
        }

        qtk_httpc_set_handler(
            cnf->httpc, cnf,
            (qtk_httpc_request_handler_f)qtk_cldebnf_on_http_post);
        ret = qtk_httpc_post(cnf->httpc, cnf->ebnf_data->data,
                             cnf->ebnf_data->len, cnf,
                             (qtk_httpc_add_hdr_f)qtk_cldebnf_add_req_hdr2);
        qtk_httpc_connect_reset(cnf->httpc);
        if (ret != 0) {
            wtk_sem_release(&cnf->sem, 1);
            wtk_msleep(cnf->cfg->interval);
            continue;
        }
        break;
    }

    wtk_sem_release(&cnf->sem, 1);
    return 0;
}

void qtk_cldebnf_init(qtk_cldebnf_t *cnf) {
    cnf->cfg = NULL;
    cnf->session = NULL;

    cnf->httpc = NULL;
    cnf->ebnf_data = NULL;

    cnf->md5Id = NULL;
    cnf->run = 0;
    cnf->state = QTK_CLDEBNF_INIT;
}

qtk_cldebnf_t *qtk_cldebnf_new(qtk_cldebnf_cfg_t *cfg, qtk_session_t *session) {
    qtk_cldebnf_t *cnf;

    cnf = (qtk_cldebnf_t *)wtk_malloc(sizeof(qtk_cldebnf_t));
    qtk_cldebnf_init(cnf);

    cnf->cfg = cfg;
    cnf->session = session;

    cnf->httpc = qtk_httpc_new(&cfg->httpc, NULL, session);
    cnf->url = wtk_strbuf_new(32, 1);

    wtk_thread_init(&cnf->thread, (thread_route_handler)qtk_cldebnf_run, cnf);
    wtk_thread_set_name(&cnf->thread, "cldebnf");

    wtk_sem_init(&cnf->sem, 0);

    return cnf;
}

void qtk_cldebnf_delete(qtk_cldebnf_t *cnf) {
    if (cnf->run) {
        cnf->run = 0;
        wtk_thread_join(&cnf->thread);
    }
    wtk_thread_clean(&cnf->thread);
    wtk_sem_clean(&cnf->sem);

    qtk_httpc_delete(cnf->httpc);
    if (cnf->ebnf_data) {
        wtk_string_delete(cnf->ebnf_data);
    }
    if (cnf->md5Id) {
        wtk_string_delete(cnf->md5Id);
    }
    if (cnf->url) {
        wtk_strbuf_delete(cnf->url);
    }
    wtk_free(cnf);
}

void qtk_cldebnf_process(qtk_cldebnf_t *cnf, char *data, int len) {
    cnf->ebnf_data = wtk_string_dup_data(data, len);
    cnf->run = 1;
    wtk_thread_start(&cnf->thread);
    wtk_sem_acquire(&cnf->sem, -1);
}

wtk_string_t *qtk_cldebnf_check(qtk_cldebnf_t *cnf) {
    if (cnf->state == QTK_CLDEBNF_VALID) {
        return cnf->md5Id;
    } else {
        return NULL;
    }
}
