#include "qtk_hotword.h"

void qtk_hw_update_token(qtk_hw_t *hw) {
    time_t t;
    int iat, exp;
    char tmp[64];
    int ret;

    iat = time(&t) - 500;
    exp = iat + 3600; // 验证时效为1h
    ret = snprintf(tmp, 64, "{\"exp\":%d,\"iat\":%d}", exp, iat);
    wtk_string_set(&hw->token_payload, tmp, ret);
    wtk_log_log(hw->session->log, "payload: %.*s\n", hw->token_payload.len,
                hw->token_payload.data);
    //	wtk_debug("payload: %.*s\n", hw->token_payload.len,
    //hw->token_payload.data);
    qtk_jwt_token_get(hw->token, QTK_HOTWORD_TOKEN_HEADER,
                      strlen(QTK_HOTWORD_TOKEN_HEADER), tmp, ret,
                      QTK_HOTWORD_TOKEN_SIGNATURE,
                      strlen(QTK_HOTWORD_TOKEN_SIGNATURE));
    wtk_log_log(hw->session->log, "token: %.*s\n", hw->token->pos,
                hw->token->data);
}

void qtk_hw_init(qtk_hw_t *hw) {
    hw->cfg = NULL;
    hw->session = NULL;
    hw->httpc = NULL;
    hw->json = NULL;
    hw->token = NULL;
    hw->upload_res = NULL;
    hw->update_res = NULL;
    hw->ths = NULL;
    hw->notify = NULL;
    wtk_string_set(&hw->token_payload, 0, 0);
}

qtk_hw_t *qtk_hw_new(qtk_hw_cfg_t *cfg, qtk_session_t *session) {
    qtk_hw_t *hw;

    hw = (qtk_hw_t *)wtk_malloc(sizeof(qtk_hw_t));
    qtk_hw_init(hw);
    hw->cfg = cfg;
    hw->session = session;
    hw->httpc = qtk_httpc_new(&(hw->cfg->httpc_cfg), NULL, hw->session);
    hw->token = wtk_strbuf_new(128, 1);
    hw->upload_res = wtk_strbuf_new(128, 1);
    hw->json = wtk_json_new();
    qtk_hw_update_token(hw);

    return hw;
}

int qtk_hw_delete(qtk_hw_t *hw) {
    if (hw->token) {
        wtk_strbuf_delete(hw->token);
    }
    if (hw->upload_res) {
        wtk_strbuf_delete(hw->upload_res);
    }
    if (hw->httpc) {
        qtk_httpc_delete(hw->httpc);
    }
    if (hw->update_res) {
        wtk_string_delete(hw->update_res);
    }
    if (hw->json) {
        wtk_json_delete(hw->json);
    }
    wtk_free(hw);
    return 0;
}

// http post 回调，上抛 response
void qtk_hw_on_http_post(qtk_hw_t *hw, qtk_http_response_t *rep) {
    wtk_log_log(hw->session->log, "status = %d.", rep->status);
    // qtk_http_response_print(rep);
    if (rep->body->pos > 0) {
        if (hw->notify) {
            hw->notify(hw->ths, rep->body->data, rep->body->pos);
        }
    }
}

void qtk_hw_add_req_hdr1(qtk_hw_t *hw, wtk_strbuf_t *buf) {
    wtk_strbuf_push_f(buf, "HOST:%.*s:%.*s\r\n", hw->cfg->httpc_cfg.host.len,
                      hw->cfg->httpc_cfg.host.data, hw->cfg->httpc_cfg.port.len,
                      hw->cfg->httpc_cfg.port.data);
    wtk_strbuf_push_s(buf, "Authorization: Bearer ");
    wtk_strbuf_push(buf, hw->token->data, hw->token->pos);
    wtk_strbuf_push_s(buf, "\ncontent-type: application/json\r\n");
}

void qtk_hw_add_req_hdr2(qtk_hw_t *hw, wtk_strbuf_t *buf) {
    wtk_strbuf_push_f(buf, "HOST:%.*s:%.*s\r\n", hw->cfg->httpc_cfg.host.len,
                      hw->cfg->httpc_cfg.host.data, hw->cfg->httpc_cfg.port.len,
                      hw->cfg->httpc_cfg.port.data);
    wtk_strbuf_push_s(buf, "Authorization: Bearer ");
    wtk_strbuf_push(buf, hw->token->data, hw->token->pos);
    wtk_strbuf_push_s(buf, "\ncontent-type: text/plain\r\n");
}

void qtk_hw_add_req_hdr3(qtk_hw_t *hw, wtk_strbuf_t *buf) {
    wtk_strbuf_push_f(buf, "HOST:%.*s:%.*s\r\n", hw->cfg->httpc_cfg.host.len,
                      hw->cfg->httpc_cfg.host.data, hw->cfg->httpc_cfg.port.len,
                      hw->cfg->httpc_cfg.port.data);
    wtk_strbuf_push_s(buf, "Authorization: Bearer ");
    wtk_strbuf_push(buf, hw->token->data, hw->token->pos);
    wtk_strbuf_push_s(buf, "\r\n");
}

// 上传
int qtk_hw_upload(qtk_hw_t *hw, char *res_fn, int flag) {
    wtk_json_item_t *item;
    int ret;
    int len;
    char *data;

    if (!flag) {
        ret = wtk_file_exist(res_fn);
        if (ret) {
            wtk_log_log0(hw->session->log, "hotword txt is not exist.");
            _qtk_error(hw->session, _QTK_HOTWORD_TXT_NOT_FOUND);
            return -1;
        }

        data = file_read_buf(res_fn, &len);
    } else {
        data = res_fn;
        len = strlen(data);
    }

    item = wtk_json_new_object(hw->json);
    wtk_json_obj_add_str2_s(hw->json, item, "app_id",
                            hw->session->opt.appid->data,
                            hw->session->opt.appid->len);
    wtk_json_obj_add_str2_s(hw->json, item, "device_id",
                            hw->session->opt.usrid->data,
                            hw->session->opt.usrid->len);
    wtk_json_obj_add_str2_s(hw->json, item, "word", data, len - 1);

    wtk_json_item_print(item, hw->upload_res);
    wtk_string_set(&hw->cfg->httpc_cfg.url, QTK_HOTWORD_URL,
                   strlen(QTK_HOTWORD_URL));
    qtk_httpc_connect_reset(hw->httpc);
    qtk_httpc_set_handler(hw->httpc, hw,
                          (qtk_httpc_request_handler_f)qtk_hw_on_http_post);
    ret = qtk_httpc_post(hw->httpc, hw->upload_res->data, hw->upload_res->pos,
                         hw, (qtk_httpc_add_hdr_f)qtk_hw_add_req_hdr1);
    qtk_httpc_connect_reset(hw->httpc);
    if (!flag) {
        wtk_free(data);
    }

    return 0;
}

// 更新
int qtk_hw_update(qtk_hw_t *hw, char *res_fn, int flag) {
    char url[256] = {0};
    int ret;
    int len;
    char *data;

    if (!flag) {
        ret = wtk_file_exist(res_fn);
        if (ret) {
            wtk_log_log0(hw->session->log, "hotword update txt not exist.");
            _qtk_error(hw->session, _QTK_HOTWORD_TXT_NOT_FOUND);
            return -1;
        }

        data = file_read_buf(res_fn, &len);
    } else {
        data = res_fn;
        len = strlen(data);
    }
    hw->update_res = wtk_string_dup_data(data, len - 1);
    snprintf(url, 256, "%s/%s/%s", QTK_HOTWORD_URL,
             hw->session->opt.appid->data, hw->session->opt.usrid->data);
    wtk_string_set(&hw->cfg->httpc_cfg.url, url, strlen(url));
    qtk_httpc_connect_reset(hw->httpc);
    qtk_httpc_set_handler(hw->httpc, hw,
                          (qtk_httpc_request_handler_f)qtk_hw_on_http_post);
    ret = qtk_httpc_put(hw->httpc, hw->update_res->data, hw->update_res->len,
                        hw, (qtk_httpc_add_hdr_f)qtk_hw_add_req_hdr2);
    qtk_httpc_connect_reset(hw->httpc);

    if (!flag) {
        wtk_free(data);
    }
    return 0;
}

// 获取
int qtk_hw_get_hotword(qtk_hw_t *hw) {
    char url[256] = {0};
    snprintf(url, 256, "%s/%s/%s", QTK_HOTWORD_URL,
             hw->session->opt.appid->data, hw->session->opt.usrid->data);
    wtk_string_set(&hw->cfg->httpc_cfg.url, url, strlen(url));
    qtk_httpc_connect_reset(hw->httpc);
    qtk_httpc_set_handler(hw->httpc, hw,
                          (qtk_httpc_request_handler_f)qtk_hw_on_http_post);
    qtk_httpc_get(hw->httpc, NULL, 0, hw,
                  (qtk_httpc_add_hdr_f)qtk_hw_add_req_hdr3);
    qtk_httpc_connect_reset(hw->httpc);

    return 0;
}

void qtk_hw_set_notify(qtk_hw_t *hw, void *ths, qtk_hw_notify_f notify) {
    hw->ths = ths;
    hw->notify = notify;
}
