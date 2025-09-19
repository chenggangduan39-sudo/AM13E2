#include "qtk_session.h"
#include "auth/qtk_auth.h"
#include "cldhub/qtk_cldhub.h"
#include "usrid/qtk_usrid.h"
#include "cldlog/qtk_cldlog.h"
#include "srvsel/qtk_srvsel.h"
#ifdef USE_ENC8838
#include "sdk/dev/enc/qtk_enc_ver.h"
#endif
#ifdef USE_KEROS
#include "sdk/codec/keros/qtk_keros.h"
#endif

#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#ifdef WIN32
int load_socket() {
    WSADATA wsadata;
    WORD wVersionRequested;
    int err;

    wVersionRequested = MAKEWORD(2, 2);
    err = WSAStartup(wVersionRequested, &wsadata);
    if (err != 0) {
        return -1;
    }
    if (LOBYTE(wsadata.wVersion) != 2 || HIBYTE(wsadata.wVersion) != 2) {
        WSACleanup();
        return 0;
    }
    return 0;
}
#endif

static qtk_srvsel_t *srvsel;
void qtk_session_print_version(qtk_session_t *session)
{
	char buf[256];
	wtk_get_build_timestamp(buf);
	wtk_log_log(session->log,"======>version:SDK_yk.0.2.0 build at %s<======",buf);
}
void qtk_session_on_httperr(qtk_session_t *session)
{
	int ret;
	if(session->opt.use_srvsel){
		ret = qtk_srvsel_process(srvsel);
		if(0 == ret){
			qtk_cldhub_update_hostport(session->cldhub,srvsel->host,srvsel->port);
		}
		qtk_srvsel_clean(srvsel);
	}else{
		static int count=0;
		qtk_cldhub_update_hostport(session->cldhub,session->opt.host,session->opt.port);
	}
}
static int qtk_session_run_srvsel(qtk_session_t *session,wtk_thread_t *t)
{
	int ret;

	if(session->opt.use_srvsel) {
		while(session->run) {
			ret = qtk_srvsel_process(srvsel);
			if(ret == 0) {
				qtk_option_set_host(&session->opt,srvsel->host->data,srvsel->host->len);
				qtk_option_set_port(&session->opt,srvsel->port->data,srvsel->port->len);
				break;
			} else {
				if(session->srvsel_sem_flg){
					wtk_sem_release(&session->srvsel_sem,1);
					session->srvsel_sem_flg = 0;
				}
				_qtk_error(session,_QTK_SRVSEL_FAILED);
				wtk_msleep(1000);
			}
		}
		qtk_srvsel_clean(srvsel);
	}

	if(session->opt.use_cldhub) {
		session->cldhub = qtk_cldhub_new(session);
		qtk_cldhub_set_err_notify(session->cldhub,session,(qtk_httpc_request_err_notify_f)qtk_session_on_httperr);
	}

	if(session->srvsel_sem_flg){
		wtk_sem_release(&session->srvsel_sem,1);
		session->srvsel_sem_flg = 0;
	}
	return 0;
}

#ifdef USE_AUTH
static int qtk_session_run_auth(qtk_session_t *session,wtk_thread_t *t)
{
	qtk_auth_t *auth;
	qtk_auth_result_t rlt = QTK_AUTH_SUCCESS;

	auth = qtk_auth_new(session);
        rlt = qtk_auth_process(auth);
        qtk_auth_delete(auth);

        wtk_log_log(session->log,"auth rlt = %d",rlt);
	if(rlt != QTK_AUTH_SUCCESS) {
		_qtk_error(session,_QTK_AUTH_FAILED);
		session->err = 1;
	}

	wtk_sem_release(&session->auth_sem,1);
	return 0;
}
#endif

static int qtk_session_run(qtk_session_t *session,wtk_thread_t *t)
{
	wtk_thread_t thread[2];

	if(!session->opt.rsa) {
		qtk_option_new_rsa(&session->opt);
	}

	if(session->opt.use_srvsel || session->opt.use_cldhub || session->opt.use_cldlog)
	{
		wtk_thread_init(thread+0,(thread_route_handler)qtk_session_run_srvsel,session);
		wtk_thread_set_name(thread+0,"session srvsel");
		wtk_thread_start(thread+0);
	}

#ifdef USE_AUTH
	wtk_thread_init(thread+1,(thread_route_handler)qtk_session_run_auth,session);
	wtk_thread_set_name(thread+1,"session auth");
	wtk_thread_start(thread+1);
#else
	wtk_sem_release(&session->auth_sem,1);
#endif

	if(session->opt.use_srvsel || session->opt.use_cldhub || session->opt.use_cldlog)
	{
		wtk_thread_join(thread+0);
		wtk_thread_clean(thread+0);
	}

#ifdef USE_AUTH
	wtk_thread_join(thread+1);
	wtk_thread_clean(thread+1);
#endif

	return 0;
}
void qtk_session_srvsel_on_timer(qtk_session_t *session)
{
	int ret;

	ret = qtk_srvsel_process(srvsel);
	if(0 == ret){
		qtk_cldhub_update_hostport(session->cldhub,srvsel->host,srvsel->port);
	}
	qtk_srvsel_clean(srvsel);
}

/**
 * appid\secretkey\usrid check
 */

int qtk_session_check_option(qtk_session_t *session)
{
	if(!session->opt.appid||session->opt.appid->len<=0){
		wtk_log_log0(session->log,"no appid");
		_qtk_error(session,_QTK_APPID_INVALID);
		return -1;
	}
	if(!session->opt.secretkey||session->opt.secretkey->len<=0){
		wtk_log_log0(session->log,"no secretkey");
		_qtk_error(session,_QTK_SECRETKEY_INVALID);
		return -1;
	}
	if(session->opt.use_srvsel)
	{
		if(!session->opt.usrid||session->opt.usrid->len<=0){
			wtk_log_log0(session->log,"no usrid");
			_qtk_error(session,_QTK_USRID_INVALID);
			return -1;
		}
	}
	return 0;
}
qtk_session_t* qtk_session_new(char *params,qtk_errcode_level_t level,void *ths,qtk_errcode_handler errhandler)
{
	qtk_session_t *session;
	wtk_cfg_file_t *cfile = NULL;
	qtk_usrid_t *usrid;
	wtk_string_t v;
	int ret;

	session = (qtk_session_t*)wtk_malloc(sizeof(qtk_session_t));
	qtk_option_init(&session->opt);
	session->cldhub = NULL;
	session->log = NULL;
	session->clog = NULL;
	session->timer = NULL;
	session->ec = NULL;
	session->err = 0;
	session->run = 0;
	session->srvsel_sem_flg = 0;
	session->auth_sem_flg = 0;

	/**
	 * init errcode first
	 */
	session->ec = qtk_errcode_new(1);
	qtk_errcode_set_level(session->ec,level);
	qtk_errcode_set_handler(session->ec,ths,errhandler);
	/**
	 * als params
	 */
	cfile = wtk_cfg_file_new();
	ret = wtk_cfg_file_feed(cfile,params,strlen(params));
	if(ret != 0) {
		session->err = 1;
		_qtk_error(session,_QTK_SESSION_PARAMS_ERR);
		goto end;
	}
	/**
	 * update option
	 */
	qtk_option_update_params(&session->opt,cfile->main);

	/**
	 * init log;
	 */
	// wtk_debug("+++++=use_log = %d  log_fn = %s\n", session->opt.use_log, session->opt.log_fn->data);
	if(session->opt.log_fn && session->opt.use_log) {
		session->log = wtk_log_new(session->opt.log_fn->data);
		qtk_session_print_version(session);
		if(!session->log) {
			wtk_debug(">>>>>>log new failed.\n");
			_qtk_error(session,_QTK_LOG_INVALID);
		}
		qtk_errcode_set_log(session->ec,session->log);
	}
	/**
	 * init cldlog;
	 */
	if(session->opt.use_cldlog) {
		session->clog = qtk_cldlog_new(session);
		if(session->clog) {
			qtk_cldlog_start(session->clog);
		}
	}
	/**
	 * init timer;
	 */
#ifndef USE_SESSION_NOCHECK
	if(session->opt.use_timer) {
		session->timer = qtk_timer_new(session->log);
		if(!session->timer) {
			wtk_log_warn0(session->log,"timer new failed\n");
		}
	}
#endif
	/**
	 * get user id
	 */
	if(session->opt.use_srvsel || session->opt.use_cldhub)
	{
		usrid = qtk_usrid_new(session);
		v = qtk_usrid_get(usrid);
		qtk_option_set_usrid(&session->opt,v.data,v.len);
		qtk_usrid_delete(usrid);
	}

	if(session->opt.use_srvsel) {
		srvsel = qtk_srvsel_new(session);
		if(!srvsel){
			wtk_log_warn0(session->log,"srvsel new failed");
			ret = -1;
			goto end;
		}
	}
/**
 *  offline verify
 */
#ifdef USE_VERIFY
#include "verify/qtk_verify.h"
	ret = qtk_verify_proc(session->opt.usrid->data,session->opt.usrid->len);
	if(ret != 0) {
		printf("No permission\n");
		exit(1);
	}

	ret = qtk_verify_proc_limitdays(15);
	if(ret != 0) {
		printf("Out of limit day\n");
		exit(1);

	}
#endif
	/**
	 * start thread  (auth  || srvsel + cldhub)
	 */
#ifndef USE_SESSION_NOCHECK
	wtk_sem_init(&session->auth_sem,0);
	wtk_sem_init(&session->srvsel_sem,0);
	if(session->opt.use_cldhub || session->opt.use_srvsel)
	{
		wtk_thread_init(&session->thread,(thread_route_handler)qtk_session_run,session);
		wtk_thread_set_name(&session->thread,"session");
	}
#endif

	ret = 0;
end:
	if(cfile) {
		wtk_cfg_file_delete(cfile);
	}

	return session;
}

void qtk_session_delete(qtk_session_t *session)
{
	if(session->run) {
		qtk_session_stop(session);
	}
#ifndef USE_SESSION_NOCHECK
	if(session->opt.use_cldhub || session->opt.use_srvsel)
	{
		wtk_thread_clean(&session->thread);
	}
	wtk_sem_clean(&session->auth_sem);
	wtk_sem_clean(&session->srvsel_sem);
#endif

	if(srvsel){
		qtk_srvsel_delete(srvsel);
	}
	if(session->cldhub) {
		qtk_cldhub_delete(session->cldhub);
	}
	qtk_option_clean(&session->opt);
	if(session->ec){
		qtk_errcode_delete(session->ec);
	}
	if(session->timer) {
		qtk_timer_delete(session->timer);
	}
	if(session->log) {
		wtk_log_delete(session->log);
		//项目中已经出现了不指定为NULL造成的崩溃
		session->log=NULL;
	}
	if(session->clog) {
		qtk_cldlog_stop(session->clog);
		qtk_cldlog_delete(session->clog);
	}
	wtk_free(session);
}

void qtk_session_start(qtk_session_t *session)
{
	if(session->opt.use_cldhub || session->opt.use_srvsel)
	{
		if(session->run) {
			return;
		}
		session->run = 1;
#ifndef USE_SESSION_NOCHECK
		if(session->timer) {
			qtk_timer_start(session->timer);
		}
		session->auth_sem_flg = 1;
		session->srvsel_sem_flg = 1;
		wtk_thread_start(&session->thread);
		wtk_sem_acquire(&session->srvsel_sem,-1);
		wtk_sem_acquire(&session->auth_sem,-1);
		
		if(session->opt.use_srvsel&&session->timer){
			qtk_timer_add2(session->timer,session->opt.srvsel_update_period,-1,session,(qtk_timer_notify_func)qtk_session_srvsel_on_timer);
		}
#endif
	}
//	qtk_option_print(&session->opt,session->log);
}

void qtk_session_stop(qtk_session_t *session)
{
	if(!session->run) {
		return;
	}
#ifndef USE_SESSION_NOCHECK
	if(session->opt.use_srvsel)
	{
		session->run = 0;
		wtk_thread_join(&session->thread);
	}
	if(session->timer) {
		qtk_timer_stop(session->timer);
	}
#endif
}

void qtk_session_set_usrid(qtk_session_t *session,char *usrid,int len)
{
	qtk_option_set_usrid(&session->opt,usrid,len);
}

char *qtk_session_get_i2c_path(char *params)
{
	int len=strlen(params);
	int i=0,idxs=0,type=1;
	char *data;
	if(len <= 10){return NULL;}
	i=0;
	while (i<len)
	{
		data=params+i;
		switch (type)
		{
		case 1:
			if(strncmp(data, "i2c_path=", strlen("i2c_path=")) == 0)
			{
				idxs=i+8;
				type=2;
				i+=7;
			}
			break;
		case 2:
			if(strncmp(data, ";", strlen(";")) == 0)
			{
				char *path=wtk_malloc(i-idxs+1);
				memset(path, 0, i-idxs+1);
				printf("i=%d idxs=%d %.*s\n",i,idxs,i-idxs,params+idxs);
				memcpy(path, params+idxs, i-idxs);
				return path;
			}
			break;
		default:
			break;
		}
		i++;
	}
	
	return NULL;
}


qtk_session_t* qtk_session_init(char *params,qtk_errcode_level_t level,void *ths,qtk_errcode_handler errhandler)
{
	qtk_session_t *session=NULL;
	int ret=-1;
#ifdef USE_USBCRYPTO
	printf("Initializing session...\n");
    // 1. 调用 USB 加密验证
    int ret = usb_crypto_verify();
    if (ret != 0) {
        fprintf(stderr, "USB加密验证失败！错误码: %d\n", ret);
        goto end;
    }
	printf("USB 加密成功!!");
#endif
#ifdef USE_ENC8838
	char *i2cpath=NULL;
	i2cpath = qtk_session_get_i2c_path(params);
	ret = qtk_enc_ver(i2cpath);
	if(i2cpath != NULL)
	{
		wtk_free(i2cpath);
	}
	if(ret < 0)
	{
		goto end;
	}
#endif

#ifdef USE_KEROS
	ret = qtk_check_keros(NULL);
	if(ret!=0)
	{
		wtk_debug("mod keros auth failed!");
		return NULL;
	}
#endif

#ifdef WIN32
	load_socket();
#endif

	char buf[256];
	wtk_get_build_timestamp(buf);
	wtk_debug("BUILD AT %s\n",buf);

	session = qtk_session_new(params,level,ths,errhandler);
#ifndef USE_SESSION_NOCHECK
	ret =qtk_session_check_option(session);
	if(ret != 0){
		session->err = 1;
		goto end;
	}
#endif
	qtk_option_print(&session->opt,session->log);
	if(session){
		qtk_session_start(session);
	}
	ret = 0;
end:
	if(ret != 0){
		if(session){
			qtk_session_delete(session);
			session = NULL;
		}
	}
	return session;
}

void qtk_session_exit(qtk_session_t *session)
{
	qtk_session_stop(session);
	qtk_session_delete(session);
}

int qtk_session_check(qtk_session_t *session)
{
	if(session->err) {
		return -1;
	}
	return 0;
}

void qtk_session_feed_errcode(qtk_session_t *session,qtk_errcode_level_t level,int errcode, char *extraErrdata, int len)
{
	qtk_errcode_feed(session->ec,level,errcode, extraErrdata, len);
}

int qtk_session_read_errcode(qtk_session_t *session,qtk_errcode_level_t *level,int *errcode)
{
	return qtk_errcode_read(session->ec,level,errcode);
}

void qtk_session_set_errHandler(qtk_session_t *session,void *ths,qtk_errcode_handler handler)
{
	qtk_errcode_set_handler(session->ec,ths,handler);
}

void qtk_session_set_errLevel(qtk_session_t *session,qtk_errcode_level_t level)
{
	qtk_errcode_set_level(session->ec,level);
}

char* qtk_session_strerr(qtk_session_t *session,int errcode)
{
	return qtk_errcode_tostring(session->ec,errcode);
}


