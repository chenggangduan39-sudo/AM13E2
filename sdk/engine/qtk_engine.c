#include "qtk_engine.h"
#include "sdk/engine/asr/qtk_easr.h"
#include <time.h>

#define QTK_ENGINE_YEAR 123
#define QTK_ENGINE_MONTH 7
#define QTK_ENGINE_DAY 6

#define QTK_ALLOW_CNTTIMES 100*60*60*24*3   // Milliseconds, second, minute, hours days
#ifdef USE_CNTLIMIT
static int qtk_count=0;
#endif
int qtk_engine_allow_use() {
#ifdef USE_CNTLIMIT
    if (qtk_count++ > QTK_ALLOW_CNTTIMES)
    	return -1;
    return 0;
#else
    time_t timep;
    struct tm *p;
    int delta = 0;

    time(&timep);
    p = gmtime(&timep);
    wtk_debug("%d-%d-%d.\n", p->tm_year, p->tm_mon, p->tm_mday);
    delta = (p->tm_year - QTK_ENGINE_YEAR) * 365 +
            (p->tm_mon - QTK_ENGINE_MONTH) * 30 + (p->tm_mday - QTK_ENGINE_DAY);
    return delta > 30 ? -1 : 0;
#endif
}

int qtk_engine_count(int count, char *binfn)
{
	wtk_rbin2_t *rbin;
    wtk_rbin2_item_t *item;
    int ret;
    char *cfg_fn="./cfg";
    wtk_rbin2_t *rbwrite;
    wtk_string_t name;
	int cc;
	wtk_strbuf_t *inbuf;

	wtk_debug("binfn=%s\n",binfn);

    rbin = wtk_rbin2_new();
	ret = wtk_rbin2_read(rbin, binfn);
	if(ret != 0){
        wtk_debug("error read\n");
        ret = -1;
        goto end;
	}
	item = wtk_rbin2_get2(rbin, cfg_fn, strlen(cfg_fn));
	if(!item){
		wtk_debug("%s not found\n", cfg_fn);
		ret = -1;
        goto end;
	}
    printf("count=[%.*s]\n",item->data->len,item->data->data);
	cc=wtk_str_atoi2(item->data->data,item->data->len,NULL);
	if(cc > count)
	{
        ret = -1;
        goto end;
	}

	cc++;
    wtk_string_set(&name,"cfg",sizeof("cfg")-1);

    rbwrite = wtk_rbin2_new();
    char *in;
    in = wtk_itoa(cc);

	wtk_heap_add_large(rbwrite->heap,in,strlen(in));
	wtk_rbin2_add2(rbwrite,&name,in,strlen(in));
    wtk_rbin2_write(rbwrite, binfn);

    wtk_rbin2_delete(rbwrite);
    ret=0;
end:
    wtk_rbin2_delete(rbin);
	return ret;
}

int qtk_engine_get_authvalue(char * vach, int *len)
{
    char avl[]={0xFF,0xFF,0xFF,0xFF,0x01,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0xD1,0xD0,0x9C,0x99,
0x98,0x19,0x00,0x00,0x00,0x17,0x00,0x00,0x00,0x8E,0x9B,0x8D,0x9A,0x9E,0x92,0x9A,
0x8D,0xA0,0x89,0xCE,0xD1,0xCF,0xA0,0xCD,0xCF,0xCD,0xCA,0xCF,0xCC,0xCE,0xCF,0xF5};
    wtk_rbin2_t *rbin;
    wtk_rbin2_item_t *item;
    char *cfg_fn="./cfg";
    int ret=0;

    rbin = wtk_rbin2_new_str(avl,sizeof(avl));

    item=wtk_rbin2_get2(rbin,cfg_fn,strlen(cfg_fn));
	if(!item)
	{
		wtk_debug("get %s failed\n",cfg_fn);
		ret=-1;goto end;
	}
    *len = item->data->len;

    memcpy(vach, item->data->data, item->data->len);

    wtk_rbin2_delete(rbin);
end:
    if(ret < 0)
    {
        *len=0;
        return -1;
    }
    return 0;
}

int qtk_engine_usb_auth(char *auth, int len, char *binfn)
{
	wtk_rbin2_t *rbin;
    wtk_rbin2_item_t *item;
    int ret;
    char *cfg_fn="./cfg";

	wtk_debug("binfn=%s\n",binfn);

    rbin = wtk_rbin2_new();
	ret = wtk_rbin2_read(rbin, binfn);
	if(ret != 0){
        wtk_debug("error read\n");
	}
	item = wtk_rbin2_get2(rbin, cfg_fn, strlen(cfg_fn));
	if(!item){
		wtk_debug("%s not found\n", cfg_fn);
		ret = -1;
	}
    // wtk_debug("auth=%.*s binauth=%.*s\n",len,auth,item->data->len,item->data->data);
    if(strncmp(auth, item->data->data, item->data->len) == 0)
    {
        wtk_rbin2_delete(rbin);
	    return 0;
    }
    wtk_rbin2_delete(rbin);
	return -1;
}

#ifdef USE_ASR
#include "asr/qtk_easr.h"
static qtk_engine_action_t asr_actions = {
    (qtk_engine_new_func)qtk_easr_new,
    (qtk_engine_del_func)qtk_easr_delete,
    (qtk_engine_start_func)qtk_easr_start,
    (qtk_engine_feed_func)qtk_easr_feed,
    NULL,
    (qtk_engine_reset_func)qtk_easr_reset,
    (qtk_engine_cancel_func)qtk_easr_cancel,
    (qtk_engine_set_notify_func)qtk_easr_set_notify,
    (qtk_engine_set_func)qtk_easr_set,
    (qtk_engine_set_xbnf_func)qtk_easr_set_xbnf,
    NULL,
    NULL,
    (qtk_engine_get_result_func)	 qtk_easr_get_result,
};

#include "csr/qtk_ecsr.h"
static qtk_engine_action_t csr_actions = {
    (qtk_engine_new_func)qtk_ecsr_new,
    (qtk_engine_del_func)qtk_ecsr_delete,
    (qtk_engine_start_func)qtk_ecsr_start,
    (qtk_engine_feed_func)qtk_ecsr_feed,
    NULL,
    (qtk_engine_reset_func)qtk_ecsr_reset,
    (qtk_engine_cancel_func)qtk_ecsr_cancel,
    (qtk_engine_set_notify_func)qtk_ecsr_set_notify,
    (qtk_engine_set_func)qtk_ecsr_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif
#ifdef USE_TTS
#include "tts/qtk_etts.h"
static qtk_engine_action_t tts_actions = {
    (qtk_engine_new_func)qtk_etts_new,
    (qtk_engine_del_func)qtk_etts_delete,
    (qtk_engine_start_func)qtk_etts_start,
    (qtk_engine_feed_func)qtk_etts_feed,
    NULL,
    (qtk_engine_reset_func)qtk_etts_reset,
    (qtk_engine_cancel_func)qtk_etts_cancel,
    (qtk_engine_set_notify_func)qtk_etts_set_notify,
    (qtk_engine_set_func)qtk_etts_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif
#ifdef USE_VAD
#include "vad/qtk_evad.h"
static qtk_engine_action_t vad_actions = {
    (qtk_engine_new_func)qtk_evad_new,
    (qtk_engine_del_func)qtk_evad_delete,
    (qtk_engine_start_func)qtk_evad_start,
    (qtk_engine_feed_func)qtk_evad_feed,
    NULL,
    (qtk_engine_reset_func)qtk_evad_reset,
    (qtk_engine_cancel_func)qtk_evad_cancel,
    (qtk_engine_set_notify_func)qtk_evad_set_notify,
    (qtk_engine_set_func)qtk_evad_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif
#ifdef USE_KVAD
#include "kvad/qtk_ekvad.h"
static qtk_engine_action_t kvad_actions = {
    (qtk_engine_new_func)qtk_ekvad_new,
    (qtk_engine_del_func)qtk_ekvad_delete,
    (qtk_engine_start_func)qtk_ekvad_start,
    (qtk_engine_feed_func)qtk_ekvad_feed,
    NULL,
    (qtk_engine_reset_func)qtk_ekvad_reset,
    (qtk_engine_cancel_func)qtk_ekvad_cancel,
    (qtk_engine_set_notify_func)qtk_ekvad_set_notify,
    (qtk_engine_set_func)qtk_ekvad_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif
#ifdef USE_SEMDLG
#include "semdlg/qtk_esemdlg.h"
static qtk_engine_action_t semdlg_actions = {
    (qtk_engine_new_func)qtk_esemdlg_new,
    (qtk_engine_del_func)qtk_esemdlg_delete,
    (qtk_engine_start_func)qtk_esemdlg_start,
    (qtk_engine_feed_func)qtk_esemdlg_feed,
    NULL,
    (qtk_engine_reset_func)qtk_esemdlg_reset,
    NULL,
    (qtk_engine_set_notify_func)qtk_esemdlg_set_notify,
    (qtk_engine_set_func)qtk_esemdlg_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif
#ifdef USE_BFIO
#include "bfio/qtk_ebfio.h"
static qtk_engine_action_t bfio_actions = {
    (qtk_engine_new_func)qtk_ebfio_new,
    (qtk_engine_del_func)qtk_ebfio_delete,
    (qtk_engine_start_func)qtk_ebfio_start,
    (qtk_engine_feed_func)qtk_ebfio_feed,
    NULL,
    (qtk_engine_reset_func)qtk_ebfio_reset,
    (qtk_engine_cancel_func)qtk_ebfio_cancel,
    (qtk_engine_set_notify_func)qtk_ebfio_set_notify,
    (qtk_engine_set_func)qtk_ebfio_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_BFIO2
#include "ebfio/qtk_enginebfio.h"
static qtk_engine_action_t bfio2_actions = {
    (qtk_engine_new_func)qtk_enginebfio_new,
    (qtk_engine_del_func)qtk_enginebfio_delete,
    (qtk_engine_start_func)qtk_enginebfio_start,
    (qtk_engine_feed_func)qtk_enginebfio_feed,
    NULL,
    (qtk_engine_reset_func)qtk_enginebfio_reset,
    (qtk_engine_cancel_func)qtk_enginebfio_cancel,
    (qtk_engine_set_notify_func)qtk_enginebfio_set_notify,
    (qtk_engine_set_func)qtk_enginebfio_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_VBOXEBF3
#include "vboxebf3/qtk_evboxebf3.h"
static qtk_engine_action_t evboxebf3_actions = {
    (qtk_engine_new_func)qtk_evboxebf3_new,
    (qtk_engine_del_func)qtk_evboxebf3_delete,
    (qtk_engine_start_func)qtk_evboxebf3_start,
    (qtk_engine_feed_func)qtk_evboxebf3_feed,
    NULL,
    (qtk_engine_reset_func)qtk_evboxebf3_reset,
    (qtk_engine_cancel_func)qtk_evboxebf3_cancel,
    (qtk_engine_set_notify_func)qtk_evboxebf3_set_notify,
    (qtk_engine_set_func)qtk_evboxebf3_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif
#ifdef USE_EVAL
#include "eval/qtk_eeval.h"
static qtk_engine_action_t eval_actions = {
    (qtk_engine_new_func)qtk_eeval_new,
    (qtk_engine_del_func)qtk_eeval_delete,
    (qtk_engine_start_func)qtk_eeval_start,
    (qtk_engine_feed_func)qtk_eeval_feed,
    NULL,
    (qtk_engine_reset_func)qtk_eeval_reset,
    (qtk_engine_cancel_func)qtk_eeval_cancel,
    (qtk_engine_set_notify_func)qtk_eeval_set_notify,
    (qtk_engine_set_func)qtk_eeval_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif
#ifdef USE_WAKEUP
#include "wakeup/qtk_ewakeup.h"
static qtk_engine_action_t wakeup_actions = {
    (qtk_engine_new_func)qtk_ewakeup_new,
    (qtk_engine_del_func)qtk_ewakeup_delete,
    (qtk_engine_start_func)qtk_ewakeup_start,
    (qtk_engine_feed_func)qtk_ewakeup_feed,
    NULL,
    (qtk_engine_reset_func)qtk_ewakeup_reset,
    NULL,
    (qtk_engine_set_notify_func)qtk_ewakeup_set_notify,
    (qtk_engine_set_func)qtk_ewakeup_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif
#ifdef USE_TINYBF
#include "sdk/engine/tinybf/qtk_etinybf.h"
static qtk_engine_action_t tinybf_actions = {
    (qtk_engine_new_func)qtk_etinybf_new,
    (qtk_engine_del_func)qtk_etinybf_delete,
    (qtk_engine_start_func)qtk_etinybf_start,
    (qtk_engine_feed_func)qtk_etinybf_feed,
    NULL,
    (qtk_engine_reset_func)qtk_etinybf_reset,
    NULL,
    (qtk_engine_set_notify_func)qtk_etinybf_set_notify,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_AEC
// #include "sdk/engine/aec/qtk_eaec.h"
// static qtk_engine_action_t aec_actions = {
//     (qtk_engine_new_func)qtk_eaec_new,
//     (qtk_engine_del_func)qtk_eaec_delete,
//     (qtk_engine_start_func)qtk_eaec_start,
//     (qtk_engine_feed_func)qtk_eaec_feed,
//     (qtk_engine_reset_func)qtk_eaec_reset,
//     NULL,
//     (qtk_engine_set_notify_func)qtk_eaec_set_notify,
//     NULL,
//     NULL,
//     NULL,
//     NULL,
//     NULL,
// };
#include "eaec/qtk_engineaec.h"
static qtk_engine_action_t aec_actions = {
    (qtk_engine_new_func)        qtk_engineaec_new,
    (qtk_engine_del_func)        qtk_engineaec_delete,
    (qtk_engine_start_func)      qtk_engineaec_start,
    (qtk_engine_feed_func) 	     qtk_engineaec_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_engineaec_reset,
    (qtk_engine_cancel_func)     qtk_engineaec_cancel,
    (qtk_engine_set_notify_func) qtk_engineaec_set_notify,
    (qtk_engine_set_func)		 qtk_engineaec_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_AGC
#include "eagc/qtk_engineagc.h"
static qtk_engine_action_t agc_actions = {
    (qtk_engine_new_func)        qtk_engineagc_new,
    (qtk_engine_del_func)        qtk_engineagc_delete,
    (qtk_engine_start_func)      qtk_engineagc_start,
    (qtk_engine_feed_func) 	     qtk_engineagc_feed,
    (qtk_engine_feed2_func)      qtk_engineagc_feed2,
    (qtk_engine_reset_func)      qtk_engineagc_reset,
    (qtk_engine_cancel_func)     qtk_engineagc_cancel,
    (qtk_engine_set_notify_func) qtk_engineagc_set_notify,
    (qtk_engine_set_func)		 qtk_engineagc_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_MASKBFNET
#include "emaskbfnet/qtk_emaskbfnet.h"
static qtk_engine_action_t maskbfnet_actions = {
    (qtk_engine_new_func)        qtk_emaskbfnet_new,
    (qtk_engine_del_func)        qtk_emaskbfnet_delete,
    (qtk_engine_start_func)      qtk_emaskbfnet_start,
    (qtk_engine_feed_func) 	     qtk_emaskbfnet_feed,
    (qtk_engine_feed2_func) 	 qtk_emaskbfnet_feed2,
    (qtk_engine_reset_func)      qtk_emaskbfnet_reset,
    (qtk_engine_cancel_func)     qtk_emaskbfnet_cancel,
    (qtk_engine_set_notify_func) qtk_emaskbfnet_set_notify,
    (qtk_engine_set_func)		 qtk_emaskbfnet_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_WDEC
#include "wdec/qtk_ewdec.h"
static qtk_engine_action_t wdec_actions = {
    (qtk_engine_new_func)qtk_ewdec_new,
    (qtk_engine_del_func)qtk_ewdec_delete,
    (qtk_engine_start_func)qtk_ewdec_start,
    (qtk_engine_feed_func)qtk_ewdec_feed,
    NULL,
    (qtk_engine_reset_func)qtk_ewdec_reset,
    NULL,
    (qtk_engine_set_notify_func)qtk_ewdec_set_notify,
    (qtk_engine_set_func)qtk_ewdec_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_VBOXEBF
#include "vboxebf/qtk_evboxebf.h"
static qtk_engine_action_t vboxebf_actions = {
    (qtk_engine_new_func)        qtk_evboxebf_new,
    (qtk_engine_del_func)        qtk_evboxebf_delete,
    (qtk_engine_start_func)      qtk_evboxebf_start,
    (qtk_engine_feed_func) 	     qtk_evboxebf_feed,
    (qtk_engine_feed2_func) 	 qtk_evboxebf_feed2,
    (qtk_engine_reset_func)      qtk_evboxebf_reset,
    (qtk_engine_cancel_func)     qtk_evboxebf_cancel,
    (qtk_engine_set_notify_func) qtk_evboxebf_set_notify,
    (qtk_engine_set_func)		 qtk_evboxebf_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_GDENOISE
#include "egdenoise/qtk_egdenoise.h"
static qtk_engine_action_t gdenoise_actions = {
    (qtk_engine_new_func)        qtk_egdenoise_new,
    (qtk_engine_del_func)        qtk_egdenoise_delete,
    (qtk_engine_start_func)      qtk_egdenoise_start,
    (qtk_engine_feed_func) 	     qtk_egdenoise_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_egdenoise_reset,
    (qtk_engine_cancel_func)     qtk_egdenoise_cancel,
    (qtk_engine_set_notify_func) qtk_egdenoise_set_notify,
    (qtk_engine_set_func)		 qtk_egdenoise_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_ESTIMATE
#include "eestimate/qtk_eestimate.h"
static qtk_engine_action_t estimate_actions = {
    (qtk_engine_new_func)        qtk_eestimate_new,
    (qtk_engine_del_func)        qtk_eestimate_delete,
    (qtk_engine_start_func)      qtk_eestimate_start,
    (qtk_engine_feed_func) 	     qtk_eestimate_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_eestimate_reset,
    (qtk_engine_cancel_func)     qtk_eestimate_cancel,
    (qtk_engine_set_notify_func) qtk_eestimate_set_notify,
    (qtk_engine_set_func)		 qtk_eestimate_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_GAINNETBF
#include "egainnetbf/qtk_egainnetbf.h"
static qtk_engine_action_t gainnetbf_actions = {
    (qtk_engine_new_func)        qtk_egainnetbf_new,
    (qtk_engine_del_func)        qtk_egainnetbf_delete,
    (qtk_engine_start_func)      qtk_egainnetbf_start,
    (qtk_engine_feed_func) 	     qtk_egainnetbf_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_egainnetbf_reset,
    (qtk_engine_cancel_func)     qtk_egainnetbf_cancel,
    (qtk_engine_set_notify_func) qtk_egainnetbf_set_notify,
    (qtk_engine_set_func)		 qtk_egainnetbf_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_CONSIST
#include "consist/qtk_econsist.h"
static qtk_engine_action_t consist_actions = {
    (qtk_engine_new_func)			qtk_econsist_new,
    (qtk_engine_del_func)			qtk_econsist_delete,
    (qtk_engine_start_func)      	qtk_econsist_start,
    (qtk_engine_feed_func)			qtk_econsist_feed,
    NULL,
    (qtk_engine_reset_func)		qtk_econsist_reset,
    NULL,
    (qtk_engine_set_notify_func) qtk_econsist_set_notify,
    (qtk_engine_set_func)        qtk_econsist_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_SSL
#include "ssl/qtk_essl.h"
static qtk_engine_action_t ssl_actions = {
    (qtk_engine_new_func)        qtk_essl_new,
    (qtk_engine_del_func)        qtk_essl_delete,
    (qtk_engine_start_func)      qtk_essl_start,
    (qtk_engine_feed_func) 	     qtk_essl_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_essl_reset,
    (qtk_engine_cancel_func)     qtk_essl_cancel,
    (qtk_engine_set_notify_func) qtk_essl_set_notify,
    (qtk_engine_set_func)		 qtk_essl_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_SOUNDSCREEN
#include "esoundscreen/qtk_esoundscreen.h"
static qtk_engine_action_t soundscreen_actions = {
    (qtk_engine_new_func)        qtk_esoundscreen_new,
    (qtk_engine_del_func)        qtk_esoundscreen_delete,
    (qtk_engine_start_func)      qtk_esoundscreen_start,
    (qtk_engine_feed_func) 	     qtk_esoundscreen_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_esoundscreen_reset,
    (qtk_engine_cancel_func)     qtk_esoundscreen_cancel,
    (qtk_engine_set_notify_func) qtk_esoundscreen_set_notify,
    (qtk_engine_set_func)		 qtk_esoundscreen_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_QFORM
#include "eqform/qtk_eqform9.h"
static qtk_engine_action_t qform_actions = {
    (qtk_engine_new_func)        qtk_eqform9_new,
    (qtk_engine_del_func)        qtk_eqform9_delete,
    (qtk_engine_start_func)      qtk_eqform9_start,
    (qtk_engine_feed_func) 	     qtk_eqform9_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_eqform9_reset,
    (qtk_engine_cancel_func)     qtk_eqform9_cancel,
    (qtk_engine_set_notify_func) qtk_eqform9_set_notify,
    (qtk_engine_set_func)		 qtk_eqform9_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_EQFORM
#include "eeqform/qtk_eeqform.h"
static qtk_engine_action_t eqform_actions = {
    (qtk_engine_new_func)        qtk_eeqform_new,
    (qtk_engine_del_func)        qtk_eeqform_delete,
    (qtk_engine_start_func)      qtk_eeqform_start,
    (qtk_engine_feed_func) 	     qtk_eeqform_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_eeqform_reset,
    (qtk_engine_cancel_func)     qtk_eeqform_cancel,
    (qtk_engine_set_notify_func) qtk_eeqform_set_notify,
    (qtk_engine_set_func)		 qtk_eeqform_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_QKWS
#include "eqkws/qtk_eqkws.h"
static qtk_engine_action_t qkws_actions = {
    (qtk_engine_new_func)        qtk_eqkws_new,
    (qtk_engine_del_func)        qtk_eqkws_delete,
    (qtk_engine_start_func)      qtk_eqkws_start,
    (qtk_engine_feed_func) 	     qtk_eqkws_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_eqkws_reset,
    (qtk_engine_cancel_func)     qtk_eqkws_cancel,
    (qtk_engine_set_notify_func) qtk_eqkws_set_notify,
    (qtk_engine_set_func)		 qtk_eqkws_set,
    NULL,
    (qtk_engine_get_fn_func)	 qtk_eqkws_get_fn,
    (qtk_engine_get_prob_func)	 qtk_eqkws_get_prob,
    (qtk_engine_get_result_func)	 qtk_eqkws_get_result,
};
#endif

#ifdef USE_CSRSC
#include "ecsrsc/qtk_ecsrsc.h"
static qtk_engine_action_t csrsc_actions = {
    (qtk_engine_new_func)        qtk_ecsrsc_new,
    (qtk_engine_del_func)        qtk_ecsrsc_delete,
    (qtk_engine_start_func)      qtk_ecsrsc_start,
    (qtk_engine_feed_func) 	     qtk_ecsrsc_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_ecsrsc_reset,
    (qtk_engine_cancel_func)     qtk_ecsrsc_cancel,
    (qtk_engine_set_notify_func) qtk_ecsrsc_set_notify,
    (qtk_engine_set_func)		 qtk_ecsrsc_set,
    NULL,
    (qtk_engine_get_fn_func)	 qtk_ecsrsc_get_fn,
    NULL,
    NULL,
};
#endif

#ifdef USE_RESAMPLE
#include "eresample/qtk_eresample.h"
static qtk_engine_action_t eresample_actions = {
    (qtk_engine_new_func)qtk_eresample_new,
    (qtk_engine_del_func)qtk_eresample_delete,
    (qtk_engine_start_func)qtk_eresample_start,
    (qtk_engine_feed_func)qtk_eresample_feed,
    NULL,
    (qtk_engine_reset_func)qtk_eresample_reset,
    (qtk_engine_cancel_func)qtk_eresample_cancel,
    (qtk_engine_set_notify_func)qtk_eresample_set_notify,
    (qtk_engine_set_func)qtk_eresample_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif


#ifdef USE_EIMG
#include "img/qtk_eimg.h"
static qtk_engine_action_t eimg_actions = {
    (qtk_engine_new_func)        qtk_eimg_new,
    (qtk_engine_del_func)        qtk_eimg_delete,
    (qtk_engine_start_func)      qtk_eimg_start,
    (qtk_engine_feed_func) 	     qtk_eimg_feed,
    NULL,
    (qtk_engine_reset_func)      qtk_eimg_reset,
    NULL,
    (qtk_engine_set_notify_func) qtk_eimg_set_notify,
    (qtk_engine_set_func)		 qtk_eimg_set,
    NULL,
    NULL,
    NULL,
    NULL,
};
#endif

#ifdef USE_HONGHE
#define QTK_ENGINE_VBOXEBF_STR "aecnspickup"//HongHe
#else
#define QTK_ENGINE_VBOXEBF_STR "vboxebf"
#endif
#define QTK_ENGINE_GDENOISE_STR "gdenoise"
#define QTK_ENGINE_ESTIMATE_STR "estimate"
#define QTK_ENGINE_GAINNETBF_STR "gainnetbf" //"gainnetbf"
#define QTK_ENGINE_CONSIST_STR	"consist"
#define QTK_ENGINE_SSL_STR	"ssl"
#define QTK_ENGINE_SOUNDSCREEN_STR "soundscreen"
#define QTK_ENGINE_QFORM_STR "qform"
#define QTK_ENGINE_QKWS_STR "kws"
#define QTK_ENGINE_CSRSC_STR "csrsc"
#define QTK_ENGINE_EQFORM_STR "eqform"

#define QTK_ENGINE_ASR_STR "asr"
#define QTK_ENGINE_CSR_STR "csr"
#define QTK_ENGINE_TTS_STR "tts"
#define QTK_ENGINE_VAD_STR "vad"
#define QTK_ENGINE_SEMDLG_STR "semdlg"
#define QTK_ENGINE_BFIO_STR "bfio"
#define QTK_ENGINE_BFIO2_STR "bfio2"
#define QTK_ENGINE_VBOXEBF3_STR "vboxebf3"
#define QTK_ENGINE_EVAL_STR "eval"
#define QTK_ENGINE_EWAKEUP "wakeup"
#define QTK_ENGINE_EIMG_STR "img"
#define QTK_ENGINE_KVAD_STR "kvad"
#define QTK_ENGINE_TINYBF_STR "tinybf"
#define QTK_ENGINE_AEC_STR "aec"
#define QTK_ENGINE_AGC_STR "agc"
#define QTK_ENGINE_MASKBFNET_STR "maskbfnet"
#define QTK_ENGINE_EWDEC_STR "wdec"
#define QTK_ENGINE_RESAMPLE_STR "resample"

static void qtk_engine_init(qtk_engine_t *e) {
    e->session = NULL;
    e->params = NULL;

    e->actions = NULL;
    e->handler = NULL;
}

static int qtk_engine_init_action(qtk_engine_t *e) {
    wtk_string_t *v;

    v = wtk_local_cfg_find_string_s(e->params->main, "role");
    if (!v) {
        wtk_log_log0(e->session->log, "not set engine role.");
        return -1;
    }
#ifdef USE_ASR
    if (wtk_string_cmp_s(v, QTK_ENGINE_ASR_STR) == 0) {
        e->actions = &asr_actions;
        e->type = QTK_ENGINE_ASR;
        return 0;
    }

    if (wtk_string_cmp_s(v, QTK_ENGINE_CSR_STR) == 0) {
        e->actions = &csr_actions;
        e->type = QTK_ENGINE_CSR;
        return 0;
    }
#endif
#ifdef USE_TTS
    if (wtk_string_cmp_s(v, QTK_ENGINE_TTS_STR) == 0) {
        e->actions = &tts_actions;
        e->type = QTK_ENGINE_TTS;
        return 0;
    }
#endif
#ifdef USE_VAD
    if (wtk_string_cmp_s(v, QTK_ENGINE_VAD_STR) == 0) {
        e->actions = &vad_actions;
        e->type = QTK_ENGINE_VAD;
        return 0;
    }
#endif
#ifdef USE_KVAD
    if (wtk_string_cmp_s(v, QTK_ENGINE_KVAD_STR) == 0) {
        e->actions = &kvad_actions;
        e->type = QTK_ENGINE_KVAD;
        return 0;
    }
#endif
#ifdef USE_SEMDLG
    if (wtk_string_cmp_s(v, QTK_ENGINE_SEMDLG_STR) == 0) {
        e->actions = &semdlg_actions;
        e->type = QTK_ENGINE_SEMDLG;
        return 0;
    }
#endif
#ifdef USE_BFIO
    if (wtk_string_cmp_s(v, QTK_ENGINE_BFIO_STR) == 0) {
        e->actions = &bfio_actions;
        e->type = QTK_ENGINE_BFIO;
        return 0;
    }
#endif
#ifdef USE_BFIO2
    if (wtk_string_cmp_s(v, QTK_ENGINE_BFIO2_STR) == 0) {
        e->actions = &bfio2_actions;
        e->type = QTK_ENGINE_BFIO2;
        return 0;
    }
#endif
#ifdef USE_EVAL
    if (wtk_string_cmp_s(v, QTK_ENGINE_EVAL_STR) == 0) {
        e->actions = &eval_actions;
        e->type = QTK_ENGINE_EVAL;
        return 0;
    }
#endif
#ifdef USE_VBOXEBF3
    if (wtk_string_cmp_s(v, QTK_ENGINE_VBOXEBF3_STR) == 0) {
        e->actions = &evboxebf3_actions;
        e->type = QTK_ENGINE_VBOXEBF3;
        return 0;
    }
#endif
#ifdef USE_WAKEUP
    if (wtk_string_cmp_s(v, QTK_ENGINE_EWAKEUP) == 0) {
        e->actions = &wakeup_actions;
        e->type = QTK_ENGINE_WAKEUP;
        return 0;
    }
#endif

#ifdef USE_TINYBF
    if (wtk_string_cmp_s(v, QTK_ENGINE_TINYBF_STR) == 0) {
        e->actions = &tinybf_actions;
        e->type = QTK_ENGINE_TINYBF;
        return 0;
    }
#endif

#ifdef USE_AEC
    if (wtk_string_cmp_s(v, QTK_ENGINE_AEC_STR) == 0) {
        e->actions = &aec_actions;
        e->type = QTK_ENGINE_AEC;
        return 0;
    }
#endif

#ifdef USE_AGC
    if (wtk_string_cmp_s(v, QTK_ENGINE_AGC_STR) == 0) {
        e->actions = &agc_actions;
        e->type = QTK_ENGINE_AGC;
        return 0;
    }
#endif

#ifdef USE_MASKBFNET
    if (wtk_string_cmp_s(v, QTK_ENGINE_MASKBFNET_STR) == 0) {
        e->actions = &maskbfnet_actions;
        e->type = QTK_ENGINE_MASKBFNET;
        return 0;
    }
#endif

#ifdef USE_WDEC
    if (wtk_string_cmp_s(v, QTK_ENGINE_EWDEC_STR) == 0) {
        e->actions = &wdec_actions;
        e->type = QTK_ENGINE_WDEC;
        return 0;
    }
#endif

#ifdef USE_VBOXEBF
	if(wtk_string_cmp_s(v, QTK_ENGINE_VBOXEBF_STR) == 0){
		e->actions = &vboxebf_actions;
		e->type = QTK_ENGINE_VBOXEBF;
		return 0;
	}
#endif

#ifdef USE_GDENOISE
	if(wtk_string_cmp_s(v, QTK_ENGINE_GDENOISE_STR) == 0){
		e->actions = &gdenoise_actions;
		e->type = QTK_ENGINE_GDENOISE;
		return 0;
	}
#endif

#ifdef USE_ESTIMATE
	if(wtk_string_cmp_s(v, QTK_ENGINE_ESTIMATE_STR) == 0){
		e->actions = &estimate_actions;
		e->type = QTK_ENGINE_ESTIMATE;
		return 0;
	}
#endif

#ifdef USE_GAINNETBF
	if(wtk_string_cmp_s(v, QTK_ENGINE_GAINNETBF_STR) == 0){
		e->actions = &gainnetbf_actions;
		e->type = QTK_ENGINE_GAINNETBF;
		return 0;
	}
#endif

#ifdef USE_CONSIST
	if(wtk_string_cmp_s(v, QTK_ENGINE_CONSIST_STR) == 0) {
		e->actions = &consist_actions;
		e->type = QTK_ENGINE_CONSIST;
		return 0;
	}
#endif

#ifdef USE_SSL
	if(wtk_string_cmp_s(v, QTK_ENGINE_SSL_STR) == 0) {
		e->actions = &ssl_actions;
		e->type = QTK_ENGINE_SSL;
		return 0;
	}
#endif

#ifdef USE_SOUNDSCREEN
	if(wtk_string_cmp_s(v, QTK_ENGINE_SOUNDSCREEN_STR) == 0){
		e->actions = &soundscreen_actions;
		e->type = QTK_ENGINE_SOUNDSCREEN;
		return 0;
	}
#endif

#ifdef USE_QFORM
	if(wtk_string_cmp_s(v, QTK_ENGINE_QFORM_STR) == 0){
		e->actions = &qform_actions;
		e->type = QTK_ENGINE_QFORM;
		return 0;
	}
#endif

#ifdef USE_QKWS
	if(wtk_string_cmp_s(v, QTK_ENGINE_QKWS_STR) == 0){
		e->actions = &qkws_actions;
		e->type = QTK_ENGINE_QKWS;
		return 0;
	}
#endif

#ifdef USE_CSRSC
	if(wtk_string_cmp_s(v, QTK_ENGINE_CSRSC_STR) == 0){
		e->actions = &csrsc_actions;
		e->type = QTK_ENGINE_CSRSC;
		return 0;
	}
#endif

#ifdef USE_EQFORM
	if(wtk_string_cmp_s(v, QTK_ENGINE_EQFORM_STR) == 0){
		e->actions = &eqform_actions;
		e->type = QTK_ENGINE_EQFORM;
		return 0;
	}
#endif

#ifdef USE_RESAMPLE
	if(wtk_string_cmp_s(v, QTK_ENGINE_RESAMPLE_STR) == 0){
		e->actions = &eresample_actions;
		e->type = QTK_ENGINE_RESAMPLE;
		return 0;
	}
#endif

#ifdef USE_EIMG
	if(wtk_string_cmp_s(v, QTK_ENGINE_EIMG_STR) == 0){
		e->actions = &eimg_actions;
		e->type = QTK_ENGINE_EIMG;
		return 0;
	}
#endif

    wtk_log_log0(e->session->log, "can't find engine role.");
    return -1;
}

qtk_engine_t *qtk_engine_new(qtk_session_t *session, char *params) {
    qtk_engine_t *e;
    int ret;

#ifdef USE_TIMELIMIT
    if (qtk_engine_allow_use() != 0) {
        wtk_log_log0(session->log, "The dynamic library is out of day limit.");
        _qtk_error(session, _QTK_OUT_TIMELIMIT);
        return NULL;
    }
#endif

    if (!session) {
        return NULL;
    }

#ifndef USE_SESSION_NOCHECK
    ret = qtk_session_check(session);
    if (ret != 0) {
        wtk_log_warn0(session->log, "session check failed");
        return NULL;
    }
#endif

    e = (qtk_engine_t *)wtk_malloc(sizeof(qtk_engine_t));
    qtk_engine_init(e);

    e->session = session;
    e->params = wtk_cfg_file_new();
    ret = wtk_cfg_file_feed(e->params, params, strlen(params));
    if (ret != 0) {
        wtk_log_warn(session->log, "engine params als failed [%s]", params);
        _qtk_error(session, _QTK_ENGINE_PARAMS_ERR);
        goto end;
    }

#ifdef USE_QTKCOUNT
	char tmp[128]={0};
	wtk_string_t *v;
	v = wtk_local_cfg_find_string_s(e->params->main,"cfg");
	if(!v) {
		wtk_log_log0(e->session->log,"not set engine cfg.");
		return NULL;
	}
	int pos=v->len-1;
	while(1)
	{
		if(v->data[pos] == '/')
		{
			break;
		}
		pos--;
	}
	sprintf(tmp,"%.*s%s",pos+1,v->data,"aecsys.bin");

	if(qtk_engine_count(2000, tmp) != 0) {
		wtk_log_log0(session->log,"The dynamic library is out of number limit.");
		_qtk_error(session,_QTK_OUT_TIMELIMIT);
		goto end;
	}
	system("sync");
#endif

#ifdef USE_USB_AUTH
    char autmp[128]={0};
#ifdef USE_HAIXIN
    sprintf(autmp,"%s","/mnt/vendor/certificate/hisense_mtk9679_aecsys.bin");
#else
	wtk_string_t *v;
	v = wtk_local_cfg_find_string_s(e->params->main,"cfg");
	if(!v) {
		wtk_log_log0(e->session->log,"not set engine cfg.");
		return -1;
	}
	int pos=v->len-1;
	while(1)
	{
		if(v->data[pos] == '/')
		{
			break;
		}
		pos--;
	}
	sprintf(autmp,"%.*s%s",pos+1,v->data,"usbsys.bin");
#endif
    int avl=0;
    char tmpav[1024]={0};
    qtk_engine_get_authvalue(tmpav, &avl);
    if(qtk_engine_usb_auth(tmpav, avl, autmp) != 0) {
		wtk_log_log0(session->log,"This dynamic library has not been authorized.");
		_qtk_error(session,_QTK_OUT_TIMELIMIT);
        goto end;
	}
	system("sync");
#endif
    
    ret = qtk_engine_init_action(e);
    if (ret != 0) {
        wtk_log_warn(session->log, "role invalid [%s]", params);
        _qtk_error(session, _QTK_ENGINE_ROLE_INVALID);
        goto end;
    }

    e->handler = e->actions->new_func(session, e->params->main);
    if (!e->handler) {
        ret = -1;
        goto end;
    }
    ret = 0;
end:
    if (ret != 0) {
        qtk_engine_delete(e);
        e = NULL;
    }
    return e;
}

int qtk_engine_delete(qtk_engine_t *e) {
    if (e->handler) {
        e->actions->del_func(e->handler);
    }

    if (e->params) {
        wtk_cfg_file_delete(e->params);
    }

    wtk_free(e);

    return 0;
}

int qtk_engine_start(qtk_engine_t *e) {
    int ret;

#ifndef USE_SESSION_NOCHECK
    ret = qtk_session_check(e->session);
    if (ret != 0) {
        wtk_log_warn0(e->session->log, "session check failed");
        return -1;
    }
#endif

    return e->actions->start_func(e->handler);
}

int qtk_engine_reset(qtk_engine_t *e) {
    return e->actions->reset_func(e->handler);
}

int qtk_engine_feed(qtk_engine_t *e, char *data, int bytes,
                    qtk_feed_type_t is_end) {
#ifdef USE_CNTLIMIT
    if (qtk_engine_allow_use() != 0) {
        wtk_log_log0(e->session->log, "The library is out of day limit.");
        _qtk_error(e->session, _QTK_OUT_TIMELIMIT);
        return NULL;
    }
#endif
    return e->actions->feed_func(e->handler, data, bytes, is_end);
}

int qtk_engine_feed2(qtk_engine_t *e, char *input, int in_bytes, char *output, int *out_bytes, qtk_feed_type_t is_end)
{
    return e->actions->feed2_func(e->handler, input, in_bytes, output, out_bytes, is_end);
}

int qtk_engine_cancel(qtk_engine_t *e) {
    if (!e->actions->cancel_func) {
        wtk_log_warn0(e->session->log, "cancel invalid");
        _qtk_warning(e->session, _QTK_ENGINE_CANCEL_INVALID);
        return -1;
    }

    return e->actions->cancel_func(e->handler);
}

int qtk_engine_set(qtk_engine_t *e, char *params) {
    return e->actions->set_func(e->handler, params, strlen(params));
}

int qtk_engine_set_xbnf(qtk_engine_t *e, char *data, int len) {
    if (!e->actions->set_xbnf_func) {
        wtk_log_warn0(e->session->log, "set xbnf invalid");
        return -1;
    }
    return e->actions->set_xbnf_func(e->handler, data, len);
}

int qtk_engine_get(qtk_engine_t *e, char *param, qtk_var_t *v) {return 0; }

void qtk_engine_get_result(qtk_engine_t *e, qtk_var_t *var)
{
    e->actions->get_result_func(e->handler, var);
}

char *qtk_engine_get_fn(qtk_engine_t *e)
{
    return e->actions->get_fn_func(e->handler);
}

float qtk_engine_get_prob(qtk_engine_t *e)
{
    return e->actions->get_prob_func(e->handler);
}

int qtk_engine_set_notify(qtk_engine_t *e, void *ths,
                          qtk_engine_notify_f notify_f) {
    e->actions->set_notify_func(e->handler, ths, notify_f);
    return 0;
}

#ifdef USE_ASR
int qtk_engine_update_cmds(qtk_engine_t *e,char* words){
    wtk_debug("==================>>>>>>>>>>>>>>>>>>>words len=%d\n",strlen(words));
	int ret = qtk_easr_update_cmds(e->handler,words,strlen(words));
	return ret;
}
#endif
