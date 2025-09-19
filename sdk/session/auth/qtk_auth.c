#include "qtk_auth.h"

void qtk_auth_init(qtk_auth_t *auth)
{
	auth->session = NULL;

	auth->parser = NULL;
	auth->buf    = NULL;

	auth->time_diff = 0;
	auth->send_time = 0;

	auth->rlt = -1;
}

qtk_auth_t* qtk_auth_new(qtk_session_t *session)
{
	return qtk_auth_new2(session,1);
}

qtk_auth_t* qtk_auth_new2(qtk_session_t *session,int use_http)
{
	qtk_auth_t *auth;

	auth = (qtk_auth_t*)wtk_malloc(sizeof(qtk_auth_t));

	qtk_auth_init(auth);
	auth->session = session;
	auth->use_http = use_http;

	if(use_http) {
		qtk_httpc_cfg_init(&auth->http.cfg);
		wtk_string_set_s(&auth->http.cfg.host,QTK_AUTH_REQ_HOST);
		wtk_string_set_s(&auth->http.cfg.port,QTK_AUTH_REQ_PORT);
		wtk_string_set_s(&auth->http.cfg.url,QTK_AUTH_REQ_URI);
		qtk_httpc_cfg_update_opt(&auth->http.cfg,NULL,NULL,NULL,session->opt.dns_fn,QTK_AUTH_REQ_TIMEOUT);
		qtk_httpc_cfg_update(&auth->http.cfg);

		auth->http.httpc = qtk_httpc_new(&auth->http.cfg,NULL,session);
	}

	auth->parser = wtk_json_parser_new();
	auth->buf = wtk_strbuf_new(1024,1);
	return auth;
}

void qtk_auth_delete(qtk_auth_t *auth)
{
	if(auth->use_http) {
		qtk_httpc_delete(auth->http.httpc);
		qtk_httpc_cfg_clean(&auth->http.cfg);
	}

	if(auth->parser) {
		wtk_json_parser_delete(auth->parser);
	}

	if(auth->buf) {
		wtk_strbuf_delete(auth->buf);
	}
	wtk_free(auth);
}

int qtk_auth_checkfirst(qtk_auth_t *auth)
{
	qtk_option_t *opt = &auth->session->opt;
	FILE *fp = NULL;
	char buf[128];
	int ret;

	ret = wtk_file_exist(opt->auth_fn->data);
	if(ret == 0) {
		fp = fopen(opt->auth_fn->data,"rb");
		if(!fp) {
			ret = -1;
			goto end;
		}
		ret = fread(buf,1,64,fp);
		if(wtk_string_cmp(opt->usrid,buf,ret) == 0) {
			ret = 0;
			goto end;
		} else {
			ret = -1;
			goto end;
		}
	} else {
		fp = fopen(opt->auth_fn->data,"wb");
		if(!fp) {
			ret = -1;
			goto end;
		}
		fwrite(opt->usrid->data,1,opt->usrid->len,fp);
	}

	ret = 0;
end:
	if(fp) {
		fclose(fp);
	}
	return ret;
}

wtk_string_t qtk_auth_mk(qtk_auth_t *auth)
{
	wtk_string_t v;
	qtk_option_t *opt = &auth->session->opt;
	wtk_strbuf_t *content = auth->buf;
	wtk_strbuf_t *rsa_buf;
	char *base64Buf;
	char sha1[64];
	time_t t;

#if 0
	if(opt->use_auth_cache && opt->auth_fn) {
		int ret;

		ret = qtk_auth_checkfirst(auth);
		if(ret != 0) {
			auth->rlt = QTK_AUTH_DIFFDEVICE;
			wtk_string_set(&v,0,0);
			return v;
		}
	}
#endif

	wtk_strbuf_reset(content);
	time(&t);
	wtk_log_log(auth->session->log,"time_diff = %ld",auth->time_diff);
	t = t + auth->time_diff;
	auth->send_time = t;

    wtk_strbuf_push(content, opt->appid->data,opt->appid->len);
    wtk_strbuf_push(content, opt->usrid->data,opt->usrid->len);
    wtk_strbuf_push_f(content, "%ld", t);
    wtk_strbuf_push(content, opt->secretkey->data,opt->secretkey->len);


    QTK_SHA1_hex(sha1,content->data,content->pos);
    rsa_buf = qtk_rsa_encrypt(opt->rsa,sha1,40);
    base64Buf = wtk_base64_encode_url(rsa_buf->data,rsa_buf->pos);
    wtk_strbuf_reset(content);
    wtk_strbuf_push_s(content, "appId=");
    wtk_strbuf_push(content, opt->appid->data, opt->appid->len);
    wtk_strbuf_push_s(content, "&deviceId=");
    wtk_strbuf_push(content, opt->usrid->data, opt->usrid->len);
    wtk_strbuf_push_s(content, "&time=");
    wtk_strbuf_push_f(content, "%ld", t);
    wtk_strbuf_push_s(content, "&sign=");
    wtk_strbuf_push(content, base64Buf, strlen(base64Buf));
    wtk_free(base64Buf);

    wtk_string_set(&v,content->data,content->pos);
    wtk_log_log(auth->session->log,"auth msg send to server[%.*s]",v.len,v.data);
//    wtk_debug("%.*s\n",v.len,v.data);
    return v;
}

wtk_string_t qtk_auth_mk2(qtk_auth_t *auth)
{
	wtk_string_t v;
	wtk_json_t *json;
	wtk_json_item_t *man;
	qtk_option_t *opt = &auth->session->opt;
	wtk_strbuf_t *content = auth->buf;
	wtk_strbuf_t *rsa_buf;
	char *base64Buf;
	char sha1[64];
	char t_str[32];
	time_t t;
	int ret;
#if 0
	if(opt->use_auth_cache && opt->auth_fn) {
		ret = qtk_auth_checkfirst(auth);
		if(ret != 0) {
			auth->rlt = QTK_AUTH_DIFFDEVICE;
			wtk_debug("diff devcies\n");
			wtk_string_set(&v,0,0);
			return v;
		}
	}
#endif

	wtk_strbuf_reset(content);
	time(&t);
	wtk_log_log(auth->session->log,"time_diff = %ld",auth->time_diff);
	t = t + auth->time_diff;
	auth->send_time = t;

	wtk_strbuf_push(content, opt->appid->data,opt->appid->len);
	wtk_strbuf_push(content, opt->usrid->data,opt->usrid->len);
	wtk_strbuf_push_f(content, "%ld", t);
	wtk_strbuf_push(content, opt->secretkey->data,opt->secretkey->len);

	QTK_SHA1_hex(sha1,content->data,content->pos);
	rsa_buf = qtk_rsa_encrypt(opt->rsa,sha1,40);
	base64Buf = wtk_base64_encode(rsa_buf->data,rsa_buf->pos);

	json = wtk_json_new();
	man = wtk_json_new_object(json);

	wtk_json_obj_add_str_s(json,man,"appId",opt->appid);
	wtk_json_obj_add_str_s(json,man,"deviceId",opt->usrid);
	ret = snprintf(t_str,32,"%ld",t);
	wtk_json_obj_add_str2_s(json,man,"time",t_str,ret);
	//wtk_json_obj_add_ref_number_s(json,man,"time",t);
	wtk_json_obj_add_str2_s(json,man,"sign",base64Buf,strlen(base64Buf));

	wtk_json_item_print(man,content);
	wtk_json_delete(json);
    wtk_free(base64Buf);
    wtk_string_set(&v,content->data,content->pos);
    wtk_log_log(auth->session->log,"auth msg send to server[%.*s]",v.len,v.data);
    return v;
}

int qtk_auth_add_hdr(qtk_auth_t *auth,wtk_strbuf_t *buf)
{
	char host[128];
	int ret;

	ret = sprintf(host,"Host: %s:%s\r\n",QTK_AUTH_REQ_HOST,QTK_AUTH_REQ_PORT);
	wtk_strbuf_push_s(buf,"Content-Type: application/x-www-form-urlencoded\r\n");
	wtk_strbuf_push(buf,host,ret);
	wtk_strbuf_push_s(buf,"Accept: */*\r\n");
	wtk_strbuf_push_s(buf,"User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:22.0) Gecko/20100101 Firefox/22.0\r\n");
	return 0;
}

void qtk_auth_on_httpc(qtk_auth_t *auth,qtk_http_response_t *rep)
{
	wtk_strbuf_reset(auth->buf);
	wtk_strbuf_push(auth->buf,rep->body->data,rep->body->pos);
}

wtk_string_t qtk_auth_req(qtk_auth_t *auth,char *data,int bytes)
{
	wtk_string_t v;
	int ret;

	if(!auth->use_http) {
		wtk_string_set(&v,0,0);
		return v;
	}

	qtk_httpc_set_handler(auth->http.httpc,auth,(qtk_httpc_request_handler_f)qtk_auth_on_httpc);
	ret = qtk_httpc_post(auth->http.httpc,data,bytes,auth,(qtk_httpc_add_hdr_f)qtk_auth_add_hdr);
        if (ret != 0) {
            auth->rlt = QTK_AUTH_NETERR;
            wtk_string_set(&v, 0, 0);
            return v;
        } else {
            wtk_string_set(&v, auth->buf->data, auth->buf->pos);
        }
        return v;
}

void qtk_auth_skipSpace(char *buf)
{
    char *p1 = buf, *p2 = buf;

    while (*p1) {
        if (*p1 != ' ') {
            *p2 = *p1;
            p2++;
        }
        p1++;
    }
    *p2 = 0;
}

void qtk_auth_check(qtk_auth_t *auth,char *data,int bytes,int use_rsa)
{
	wtk_json_parser_t *parser = auth->parser;
	wtk_json_item_t *code, *info, *appid, *deviceId, *stime, *service, *sign;
	wtk_strbuf_t *buf = auth->buf;
	char checkBuf[1024];
	char shaBuf[32];
	char *base64Buf = NULL;

	wtk_log_log(auth->session->log,"auth rep = [%.*s]",bytes,data);
	wtk_json_parser_reset(parser);
	wtk_json_parser_parse(parser,data,bytes);
	if(!parser->json->main) {
		auth->rlt = QTK_AUTH_ERRRET;
		return;
	}

        code = wtk_json_obj_get_s(parser->json->main, "code");
        stime = wtk_json_obj_get_s(parser->json->main, "time");

        if(use_rsa) {
		info = wtk_json_obj_get_s(parser->json->main, "info");
		appid = wtk_json_obj_get_s(parser->json->main, "appId");
		deviceId = wtk_json_obj_get_s(parser->json->main, "deviceId");
		service = wtk_json_obj_get_s(parser->json->main, "service");
	    if (service && service->type == WTK_JSON_ARRAY)
	    {
	        wtk_strbuf_reset(buf);
	        wtk_json_item_print(service, buf);
	    }
	    sign = wtk_json_obj_get_s(parser->json->main, "sign");
	    sprintf(checkBuf, "{\"code\":%d,\"info\":\"%.*s\",\"appId\":\"%.*s\",\"deviceId\":\"%.*s\",\"service\":%.*s,\"time\":%lld}",
	        (int)code->v.number,
	        info->v.str->len, info->v.str->data,
	        appid->v.str->len, appid->v.str->data,
	        deviceId->v.str->len, deviceId->v.str->data,
	        buf->pos, buf->data,
	        (long long)(stime->v.number));
	    qtk_auth_skipSpace(checkBuf);
	    QTK_SHA1(shaBuf, checkBuf, strlen(checkBuf));
	    base64Buf = wtk_base64_decode(sign->v.str->data, sign->v.str->len);

	    if(qtk_rsa_verify(auth->session->opt.rsa,shaBuf,20,base64Buf,128) == 0) {
	    	auth->rlt = QTK_AUTH_ERRSIGN_LC;
	    	wtk_free(base64Buf);
	    	return;
	    }
	    wtk_free(base64Buf);
	}

    if(code && code->type == WTK_JSON_NUMBER) {
        switch ((int)(code->v.number))
        {
        case 0:
            auth->rlt = QTK_AUTH_SUCCESS;
            break;
        case 1:
            auth->rlt = QTK_AUTH_TIMEOUT;
            auth->time_diff = (long long)(stime->v.number) / 1000 - auth->send_time;
            wtk_log_log(auth->session->log,"time_diff = %ld",auth->time_diff);
            break;
        case 2:
            auth->rlt = QTK_AUTH_ERRSIGN;
            break;
        case 3:
            auth->rlt = QTK_AUTH_ERROR;
            break;
        case 4:
            auth->rlt = QTK_AUTH_NOLICENSE;
            break;
        case 5:
        	auth->rlt = QTK_AUTH_DATELIMIT;
        	break;
        default:
            auth->rlt = QTK_AUTH_FAILED;
            break;
        }
    }
}

qtk_auth_result_t qtk_auth_get_result(qtk_auth_t *auth)
{
	return auth->rlt;
}

qtk_auth_result_t qtk_auth_process(qtk_auth_t *auth)
{
    wtk_string_t v;
    int i;

    i = 0;
    do {
        v = qtk_auth_mk(auth);
        if (v.len <= 0) {
            goto end;
        }

        v = qtk_auth_req(auth, v.data, v.len);
        if (v.len > 0) {
            qtk_auth_check(auth, v.data, v.len, 1);
        }
        switch (auth->rlt) {
        case QTK_AUTH_NETERR:
        case QTK_AUTH_ERRRET:
        case QTK_AUTH_TIMEOUT:
        case QTK_AUTH_ERROR:
        case QTK_AUTH_FAILED:
            break;
        default:
            goto end;
            break;
        }
        ++i;
        if (i >= QTK_AUTH_FAIL_TRYS) {
            break;
        }
        wtk_msleep(QTK_AUTH_FAIL_WAIT);
    } while (1);

end:
	return auth->rlt;
}

