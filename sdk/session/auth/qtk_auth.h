#ifndef QTK_AUTH_QTK_AUTH
#define QTK_AUTH_QTK_AUTH

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/os/wtk_log.h"
#include "wtk/core/sha1.h"
#include "wtk/core/wtk_base64.h"

#include "qtk_rsa.h"
#include "sdk/qtk_def.h"
#include "sdk/httpc/qtk_httpc.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QTK_AUTH_REQ_HOST    "cloud.qdreamer.com"
#define QTK_AUTH_REQ_PORT    "80"
#define QTK_AUTH_REQ_URI     "/api/v1/auth/"
#define QTK_AUTH_REQ_TIMEOUT 2000

#define QTK_AUTH_FAIL_TRYS 2
#define QTK_AUTH_FAIL_WAIT 200

typedef enum{
	QTK_AUTH_SUCCESS = 0,      //验证成功
	QTK_AUTH_TIMEOUT,          //服务器反馈超时
	QTK_AUTH_ERRSIGN,		   //服务器反馈验证信息错误
	QTK_AUTH_ERROR,            //服务器错误
	QTK_AUTH_NOLICENSE,        //服务器反馈license超出限制
	QTK_AUTH_DATELIMIT,		   //out of date line
	QTK_AUTH_FAILED,           //服务器端返回其余任何code，默认为failed
	QTK_AUTH_NETERR,           //网络未连通
	QTK_AUTH_ERRRET,		   //网络反馈不是正确的格式
	QTK_AUTH_DIFFDEVICE,       //配置文件中缓存的设备信息和获取的设备信息不匹配
	QTK_AUTH_ERRSIGN_LC,       //服务器传回sign,本地验证未通过
}qtk_auth_result_t;

typedef struct qtk_auth qtk_auth_t;
struct qtk_auth
{
	qtk_session_t *session;
	wtk_json_parser_t *parser;
	wtk_strbuf_t *buf;
	struct{
		qtk_httpc_cfg_t cfg;
		qtk_httpc_t *httpc;
	}http;
	time_t time_diff;
	time_t send_time;
	qtk_auth_result_t rlt;
	unsigned use_http:1;
};

qtk_auth_t* qtk_auth_new(qtk_session_t *session);
qtk_auth_t* qtk_auth_new2(qtk_session_t *session,int use_http);
void qtk_auth_delete(qtk_auth_t *auth);

wtk_string_t qtk_auth_mk(qtk_auth_t *auth);
wtk_string_t qtk_auth_mk2(qtk_auth_t *auth);
wtk_string_t qtk_auth_req(qtk_auth_t *auth,char *data,int bytes);
void qtk_auth_check(qtk_auth_t *auth,char *data,int bytes,int use_rsa);
qtk_auth_result_t qtk_auth_get_result(qtk_auth_t *auth);

qtk_auth_result_t qtk_auth_process(qtk_auth_t *auth);

#ifdef __cplusplus
};
#endif
#endif
