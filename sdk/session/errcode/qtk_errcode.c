#include "qtk_errcode.h" 

static qtk_errcode_node_t* qtk_errcode_node_new(qtk_errcode_t *ec)
{
	qtk_errcode_node_t *node;

	node = (qtk_errcode_node_t*)wtk_malloc(sizeof(qtk_errcode_node_t));
	node->extraErrdata = wtk_strbuf_new(64,0);
	return node;
}

static int qtk_errcode_node_delete(qtk_errcode_node_t *node)
{
	wtk_strbuf_delete(node->extraErrdata);
	wtk_free(node);
	return 0;
}

static qtk_errcode_node_t* qtk_errcode_pop_node(qtk_errcode_t *ec)
{
	return (qtk_errcode_node_t*)wtk_lockhoard_pop(&ec->err_hoard);
}

static void qtk_errcode_push_node(qtk_errcode_t *ec,qtk_errcode_node_t *node)
{
	wtk_lockhoard_push(&ec->err_hoard,node);
}

static void qtk_errcode_init_hash(qtk_hash_t *hash) {
        qtk_hash_add(hash, _QTK_SESSION_PARAMS_ERR,
                     (void *)"Session params format is illegal");       // 11001
        qtk_hash_add(hash, _QTK_LOG_INVALID, (void *)"Log new failed"); // 11002
        qtk_hash_add(hash, _QTK_APPID_INVALID,
                     (void *)"Appid is not setted in session params"); // 11003
        qtk_hash_add(
            hash, _QTK_SECRETKEY_INVALID,
            (void *)"Secretkey is not setted in session params"); // 11004
        qtk_hash_add(hash, _QTK_USRID_INVALID,
                     (void *)"Userid not valid"); // 11005
        qtk_hash_add(hash, _QTK_SRVSEL_FAILED,
                     (void *)"Dynamic allocation of IP falied"); // 11006
        qtk_hash_add(hash, _QTK_AUTH_FAILED,
                     (void *)"Permission to verify failed"); // 11007
        qtk_hash_add(hash, _QTK_AUTH_TIMEOUT,
                     (void *)"Verification timeout return from auth "
                             "server[server code 1]"); // 11008
        qtk_hash_add(hash, _QTK_AUTH_ERRSIGN,
                     (void *)"Verification information error return from auth "
                             "server[server code 2]"); // 11009
        qtk_hash_add(
            hash, _QTK_AUTH_ERROR,
            (void *)"Error return from auth server[server code 3]"); // 11010
        qtk_hash_add(hash, _QTK_AUTH_NOLICENSE,
                     (void *)"License beyond the quantity limit return from "
                             "auth server[server code 4]"); // 11011
        qtk_hash_add(hash, _QTK_AUTH_DATELIMIT,
                     (void *)"License is out data line return from auth "
                             "server[server code 5] "); // 11012
        qtk_hash_add(hash, _QTK_AUTH_UNKNOWNERR,
                     (void *)"Other unknown error return auth server"); // 11013
        qtk_hash_add(hash, _QTK_AUTH_ERRSIGN_LC,
                     (void *)"The server returns sign, local authentication "
                             "failed"); // 11014

        qtk_hash_add(
            hash, _QTK_OUT_TIMELIMIT,
            (void *)"The dynamic library is out of day limit"); // 12001
        qtk_hash_add(hash, _QTK_ENGINE_PARAMS_ERR,
                     (void *)"Engine params format is illegal"); // 12002
        qtk_hash_add(hash, _QTK_ENGINE_ROLE_INVALID,
                     (void *)"Role is not setted in engine params"); // 12003
        qtk_hash_add(
            hash, _QTK_CFG_NOTSET,
            (void *)"Cfg filepath is not setted in engine params"); // 12004
        qtk_hash_add(hash, _QTK_CFG_NOTEXIST,
                     (void *)"Cfg file is not exist"); // 12005
        qtk_hash_add(hash, _QTK_CFG_UNREADABLE,
                     (void *)"Cfg file is unreadable"); // 12006
        qtk_hash_add(
            hash, _QTK_MIC_NOTMATCH,
            (void *)"AEC mic position is not match with config"); // 12007
        qtk_hash_add(hash, _QTK_MIC_POS_FMTERR,
                     (void *)"AEC mic position set format error"); // 12008
        qtk_hash_add(hash, _QTK_CFG_NEW_FAILED,
                     (void *)"Config new failed"); // 12009
        qtk_hash_add(hash, _QTK_INSTANSE_NEW_FAILED,
                     (void *)"Engine instanse new failed"); // 12010
        qtk_hash_add(hash, _QTK_ENGINE_CANCEL_INVALID,
                     (void *)"Engine not support cancel interface"); // 12011
        qtk_hash_add(hash, _QTK_ENGINE_SET_INVALID,
                     (void *)"Engine not support set params"); // 12012
        qtk_hash_add(hash, _QTK_ENGINE_NOTIFY_INVALID,
                     (void *)"Engine notify not setted"); // 12013
        qtk_hash_add(hash, _QTK_MODULE_ROLE_INVALID,
                     (void *)"Module role not exist"); // 12014

        qtk_hash_add(hash, _QTK_SPX_TIMEOUT,
                     (void *)"Cloud SPX request is timeout"); // 13001
        qtk_hash_add(hash, _QTK_ASR_TIMEOUT,
                     (void *)"Cloud ASR request is timeout"); // 13002
        qtk_hash_add(hash, _QTK_TTS_TIMEOUT,
                     (void *)"Cloud TTS request is timeout"); // 13003
        qtk_hash_add(hash, _QTK_NLP_TIMEOUT,
                     (void *)"Cloud NLP request is timeout"); // 13004
        qtk_hash_add(hash, _QTK_EVAL_TIMEOUT,
                     (void *)"Cloud EVAL request is timeout"); // 13005
        qtk_hash_add(
            hash, _QTK_ASR_NOT_CREDIBLE,
            (void *)"ASR result conf is less than anticipation"); // 13006
        qtk_hash_add(hash, _QTK_ENGINE_NO_RESPONSE,
                     (void *)"Engine no response"); // 13007
        qtk_hash_add(hash, _QTK_ASR_AUDIO_SHORT,
                     (void *)"Asr input audio too short"); // 13008
        qtk_hash_add(hash, _QTK_ASR_INVALID_AUDIO,
                     (void *)"Asr input audio is invalid"); // 13009
        qtk_hash_add(
            hash, _QTK_ASR_CUMULA_FULL,
            (void *)"Network is not good and asr cumula full"); // 13010

        qtk_hash_add(hash, _QTK_CLDEBNF_SERVER_ERROR,
                     (void *)"Cldebnf invalid"); // 13051
        qtk_hash_add(hash, _QTK_EVAL_EVALTEXT_NOTSET,
                     (void *)"Eval text not set"); // 13052
        qtk_hash_add(hash, _QTK_EVAL_EVALTEXT_NULL,
                     (void *)"Eval text is null"); // 13053

        qtk_hash_add(hash, _QTK_USB_HOTPLUG_UNVALID,
                     (void *)"USB hotplug is not support");           // 14001
        qtk_hash_add(hash, _QTK_USB_LEFT, (void *)"USB is pull out"); // 14002
        qtk_hash_add(hash, _QTK_USB_OPEN_FAILED,
                     (void *)"USB open failed"); // 14003
        qtk_hash_add(hash, _QTK_USB_CLAIM_FAILED,
                     (void *)"USB claim failed"); // 14004
        qtk_hash_add(
            hash, _QTK_USB_UNSUPPORT,
            (void *)"USB is not supportted in this dynamic library"); // 14005
        qtk_hash_add(hash, _QTK_PLAYER_START_FAILED,
                     (void *)"Audio player start failed"); // 14006
        qtk_hash_add(hash, _QTK_PLAYER_WRITE_FAILED,
                     (void *)"Audio player write fauled"); // 14007
        qtk_hash_add(hash, _QTK_RECORDER_START_FAILED,
                     (void *)"Audio recorder start failed"); // 14008
        qtk_hash_add(hash, _QTK_RECORDER_READ_FAILED,
                     (void *)"Audio recorder read failed"); // 14009
        qtk_hash_add(hash, _QTK_PCM_NOT_READY,
                     (void *)"PCM is not ready"); // 14010
        qtk_hash_add(hash, _QTK_PCM_START_FAILED,
                     (void *)"PCM start failed"); // 14011
        qtk_hash_add(hash, _QTK_PCM_READ_FAILED,
                     (void *)"PCM read failed"); // 14012
        qtk_hash_add(hash, _GET_ASOUND_CARD_FAILED,
                     (void *)"Get asound card failed");                 // 14013
        qtk_hash_add(hash, _QTK_USB_ARRIVED, (void *)"USB is arrived"); // 14014
        qtk_hash_add(hash, _QTK_DEV_RESET, (void *)"reset usb devices"); // 14015

        qtk_hash_add(hash, _QTK_HTTP_SND_FAILED,
                     (void *)"HTTP send failed"); // 15001
        qtk_hash_add(hash, _QTK_HTTP_RCV_FAILED,
                     (void *)"HTTP recv failed"); // 15002
        qtk_hash_add(hash, _QTK_HTTP_CONNECT_FAILED,
                     (void *)"HTTP connect failed"); // 15003
        qtk_hash_add(hash, _QTK_HTTP_AUTH_FAILED,
                     (void *)"HTTP pathways auth failed"); // 15004
        qtk_hash_add(hash, _QTK_HTTP_RESPONSE_FMTERR,
                     (void *)"HTTP response format is illegal"); // 15005
        qtk_hash_add(hash, _QTK_NETWORK_BROKEN,
                     (void *)"network broken"); // 15006
        qtk_hash_add(hash, _QTK_DNS_PARSE_FAILED,
                     (void *)"dns parse failed"); // 15007
        qtk_hash_add(hash, _QTK_SOCKET_NEW_FAILED,
                     (void *)"HTTP socket new failed"); // 15008
        qtk_hash_add(hash, _QTK_SOCKET_ERROR,
                     (void *)"HTTP socket error"); // 15009
        qtk_hash_add(hash, _QTK_SOCKET_HANGUP,
                     (void *)"HTTP socker hang up"); // 15010
        qtk_hash_add(hash, _QTK_HTTP_RECONNECT_FAILED,
                     (void *)"HTTP reconnect failed"); // 15011

        qtk_hash_add(hash, _QTK_HOTWORD_OP_INVALID,
                     (void *)"Hotword upload is not set, so operation is "
                             "invalid"); // 16001
        qtk_hash_add(hash, _QTK_HOTWORD_TXT_NOT_FOUND,
                     (void *)"Hotword txt is not existed"); // 16002
        qtk_hash_add(hash, _QTK_SERVER_ERR,
                     (void *)"{\"err from server\":"); // 17001
}

static int qtk_errcode_run(qtk_errcode_t *ec,wtk_thread_t *t)
{
	qtk_errcode_node_t *node;
	wtk_queue_node_t *qn;
	char tmp[1024];

	while(ec->run) {
		qn = wtk_blockqueue_pop(&ec->err_q,-1,NULL);
		if(!qn) {
			break;
		}
		node = data_offset2(qn,qtk_errcode_node_t,q_n);

		//wtk_debug("log %p level %d code %d\n",ec->log,node->level,node->errcode);
		if(ec->handler && node->level >= ec->level) {
			snprintf(tmp, 1024, "%s%.*s",qtk_errcode_tostring(ec,node->errcode),node->extraErrdata->pos, node->extraErrdata->data);
			ec->handler(ec->handler_ths,node->errcode,tmp);
		} else if (node->level >= QTK_ERROR) {
                        wtk_log_log(ec->log, "exit err level %d errcode %d",
                                    node->level, node->errcode);
                } else {
			wtk_log_log(ec->log,"skip level %d errcode %d",node->level,node->errcode);
		}

		qtk_errcode_push_node(ec,node);
	}

	return 0;
}

static int qtk_errcode_start(qtk_errcode_t *ec)
{
	ec->run = 1;
	return wtk_thread_start(&ec->thead);
}

static void qtk_errcode_stop(qtk_errcode_t *ec)
{
	if(!ec->run) {
		return;
	}

	ec->run = 0;
	wtk_blockqueue_wake(&ec->err_q);
	wtk_thread_join(&ec->thead);
}

static void qtk_errcode_init(qtk_errcode_t *ec)
{
	ec->log = NULL;
	ec->level = QTK_ERROR;
	ec->run = 0;
	ec->use_thread = 0;

	ec->hash = NULL;
	ec->handler = NULL;
	ec->handler_ths = NULL;
}

qtk_errcode_t* qtk_errcode_new(int use_thread)
{
	qtk_errcode_t *ec;

	ec = (qtk_errcode_t*)wtk_malloc(sizeof(qtk_errcode_t));
	qtk_errcode_init(ec);
	ec->use_thread = use_thread;

	wtk_lockhoard_init(&ec->err_hoard,offsetof(qtk_errcode_node_t,hoard_n),10,
			(wtk_new_handler_t)qtk_errcode_node_new,
			(wtk_delete_handler_t)qtk_errcode_node_delete,
			ec
			);
	wtk_blockqueue_init(&ec->err_q);

	if(ec->use_thread) {
		wtk_thread_init(&ec->thead,(thread_route_handler)qtk_errcode_run,ec);
		wtk_thread_set_name(&ec->thead,"errcode");
	}

	ec->hash = qtk_hash_new(13);
	qtk_errcode_init_hash(ec->hash);

	return ec;
}

void qtk_errcode_delete(qtk_errcode_t *ec)
{

	if(ec->use_thread) {
		qtk_errcode_stop(ec);
		wtk_thread_clean(&ec->thead);
	}else{
		wtk_blockqueue_wake(&ec->err_q);
	}

	wtk_blockqueue_clean(&ec->err_q);
	wtk_lockhoard_clean(&ec->err_hoard);

	if(ec->hash) {
		qtk_hash_delete(ec->hash);
	}

	wtk_free(ec);
}

void qtk_errcode_set_handler(qtk_errcode_t *ec,
		void *handler_ths,
		qtk_errcode_handler handler
		)
{
	ec->handler_ths = handler_ths;
	ec->handler = handler;

	if(ec->use_thread) {
		qtk_errcode_start(ec);
	}
}

void qtk_errcode_set_level(qtk_errcode_t *ec,qtk_errcode_level_t level)
{
	ec->level = level;
}

void qtk_errcode_set_log(qtk_errcode_t *ec,wtk_log_t *log)
{
	ec->log = log;
}

void qtk_errcode_feed(qtk_errcode_t *ec,qtk_errcode_level_t level,int code, char *extraErrdata, int len)
{
	qtk_errcode_node_t *node;

	node = qtk_errcode_pop_node(ec);
	node->level = level;
	node->errcode = code;
	wtk_strbuf_reset(node->extraErrdata);
	if(extraErrdata){
		wtk_strbuf_push(node->extraErrdata, extraErrdata, len);
	}
	wtk_blockqueue_push(&ec->err_q,&node->q_n);
}

int qtk_errcode_read(qtk_errcode_t *ec,qtk_errcode_level_t *level,int *code)
{
	qtk_errcode_node_t *node;
	wtk_queue_node_t *qn;

pop_again:
	qn = wtk_blockqueue_pop(&ec->err_q,-1,NULL);
	if(!qn) {
		*level = 0;
		*code = 0;
		return -1;
	}

	node = data_offset2(qn,qtk_errcode_node_t,q_n);
	if(node->level < ec->level) {
		qtk_errcode_push_node(ec,node);
		wtk_log_log(ec->log,"skip level %d errcode %d",node->level,node->errcode);
		goto pop_again;
	}

	*level = node->level;
	*code = node->errcode;
	qtk_errcode_push_node(ec,node);
	return 0;
}

char* qtk_errcode_tostring(qtk_errcode_t *ec,int code)
{
	char *strerr = NULL;

	if(ec->hash) {
		strerr = (char*)qtk_hash_find(ec->hash,code);
	}

	return strerr;
}

