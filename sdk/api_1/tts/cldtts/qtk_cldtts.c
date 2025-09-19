#include "qtk_cldtts.h"
static char qtk_cldtts_comma[3] = {0xef, 0xbc, 0x8c};
static char qtk_cldtts_period[3] = {0xe3, 0x80, 0x82};
static char qtk_cldtts_semi[3] = {0xef, 0xbc, 0x9b};

static wtk_string_t qtk_cldtts_cutstr[] = {
    {(char *)qtk_cldtts_comma, 3},  // 0 -->，
    {(char *)qtk_cldtts_period, 3}, // 1 -->。
    {(char *)qtk_cldtts_semi, 3},   // 2 -->；
    wtk_string(","),                // 3
    wtk_string("."),                // 4
    wtk_string(";"),                // 5
    wtk_string("?"),                // 6
    wtk_string("!"),                // 6
};

static int qtk_cldtts_is_punc(char *data, int len) {
    int i, n;

    n = sizeof(qtk_cldtts_cutstr) / sizeof(wtk_string_t);
    for (i = 0; i < n; ++i) {
        if (len == qtk_cldtts_cutstr[i].len) {
            if (memcmp(qtk_cldtts_cutstr[i].data, data, len) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static void qtk_cldtts_init(qtk_cldtts_t *c) {
    c->cfg = NULL;
    c->session = NULL;

    c->spx = NULL;

    c->notify_f = NULL;
    c->notify_ths = NULL;

    c->stop_hint = 0;
}

qtk_cldtts_t *qtk_cldtts_new(qtk_cldtts_cfg_t *cfg, qtk_session_t *session) {
    qtk_cldtts_t *c;

    c = (qtk_cldtts_t *)wtk_malloc(sizeof(qtk_cldtts_t));
    qtk_cldtts_init(c);

    c->cfg = cfg;
    c->session = session;

    c->spx = qtk_spx_new(&cfg->spx, session);

    return c;
}

void qtk_cldtts_delete(qtk_cldtts_t *c) {
    qtk_spx_delete(c->spx);
    wtk_free(c);
}

void qtk_cldtts_start(qtk_cldtts_t *c) {}

void qtk_cldtts_reset(qtk_cldtts_t *c) {
    c->stop_hint = 0;
    qtk_spx_reset(c->spx);
}

static int qtk_cldtts_sub_process(qtk_cldtts_t *c, char *data, int bytes) {
    wtk_json_parser_t *parser;
    wtk_json_item_t *item;
    qtk_spx_msg_t *msg;
    int ret;
    int b;

    ret = qtk_spx_start(c->spx, data, bytes, 0);
    if (ret != 0) {
        goto end;
    }

    b = 1;
    while (b) {
        msg = qtk_spx_get_result(c->spx);
        if (msg == NULL) {
            ret = -1;
            goto end;
        }
        switch (msg->type) {
        case QTK_SPX_MSG_APPEND:
            break;
        case QTK_SPX_MSG_END:
            b = 0;
            break;
        case QTK_SPX_MSG_ERR:
            parser = wtk_json_parser_new();
            wtk_json_parser_parse(parser, msg->body->data, msg->body->pos);
            if (!parser->json->main) {
                ret = -1;
                goto end;
            }
            item = wtk_json_obj_get_s(parser->json->main, "errId");
            if (item && item->type == WTK_JSON_NUMBER) {
                qtk_session_feed_errcode(c->session, QTK_ERROR, _QTK_SERVER_ERR,
                                         msg->body->data, msg->body->pos);
            }
            qtk_spx_push_msg(c->spx, msg);
            ret = -1;
            goto end;
            break;
        default:
            qtk_spx_push_msg(c->spx, msg);
            ret = -1;
            goto end;
            break;
        }

        if (msg->body->pos <= 0) {
            qtk_spx_push_msg(c->spx, msg);
            continue;
        }

        if (c->stop_hint) {
            qtk_spx_push_msg(c->spx, msg);
            continue;
        }

        c->notify_f(c->notify_ths, msg->body->data, msg->body->pos);
        qtk_spx_push_msg(c->spx, msg);
    }

    ret = 0;
end:
    return ret;
}

int qtk_cldtts_process(qtk_cldtts_t *c, char *txt, int bytes) {
    char *s, *e, *last, ch;
    int n, sub_bytes;
    int ret;

    if (!c->cfg->use_split) {
        wtk_log_log(c->session->log, "tts txt [%.*s]", bytes, txt);
        return qtk_cldtts_sub_process(c, txt, bytes);
    }

    s = last = txt;
    e = txt + bytes;
    while (s < e) {
        ch = *s;
        n = wtk_utf8_bytes(ch);
        if (qtk_cldtts_is_punc(s, n)) {
            sub_bytes = s + n - last;

            if (c->stop_hint) {
                wtk_log_log(c->session->log, "skip txt [%.*s]", sub_bytes,
                            last);
            } else {
                wtk_log_log(c->session->log, "tts txt [%.*s]", sub_bytes, last);
                ret = qtk_cldtts_sub_process(c, last, sub_bytes);
                // wtk_msleep(5000);
                if (ret != 0) {
                    goto end;
                }
            }
            last = s + n;
        }
        s += n;
    }

    sub_bytes = e - last;
    if (sub_bytes > 0) {
        if (c->stop_hint) {
            wtk_log_log(c->session->log, "skip txt [%.*s]", sub_bytes, last);
        } else {
            wtk_log_log(c->session->log, "tts txt [%.*s]", sub_bytes, last);
            ret = qtk_cldtts_sub_process(c, last, sub_bytes);
            if (ret != 0) {
                goto end;
            }
        }
    }
    ret = 0;
end:
    return ret;
}

void qtk_cldtts_set_notify(qtk_cldtts_t *c, void *notify_ths,
                           qtk_cldtts_notify_f notify_f) {
    c->notify_f = notify_f;
    c->notify_ths = notify_ths;
}

void qtk_cldtts_set_stop_hint(qtk_cldtts_t *c) {
    wtk_log_log0(c->session->log, "set stop hint");
    c->stop_hint = 1;
    qtk_spx_cancel(c->spx);
}

void qtk_cldtts_set_speed(qtk_cldtts_t *c, float speed) {
    qtk_spx_set_speed(c->spx, speed);
}

void qtk_cldtts_set_pitch(qtk_cldtts_t *c, float pitch) {
    qtk_spx_set_pitch(c->spx, pitch);
}

void qtk_cldtts_set_volume(qtk_cldtts_t *c, float volume) {
    qtk_spx_set_volume(c->spx, volume);
}

void qtk_cldtts_set_coreType(qtk_cldtts_t *c, char *data, int len) {
    qtk_spx_set_coreType(c->spx, data, len);
}
void qtk_cldtts_set_res(qtk_cldtts_t *c, char *data, int len) {
    qtk_spx_set_res(c->spx, data, len);
}
void qtk_cldtts_set_useStream(qtk_cldtts_t *c, int useStream) {
    qtk_spx_set_useStream(c->spx, useStream);
}