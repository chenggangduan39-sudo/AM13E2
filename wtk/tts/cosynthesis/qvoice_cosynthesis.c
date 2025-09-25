#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/tts/cosynthesis/wtk_cosynthesis.h"
#include "wtk/tts/cosynthesis/qvoice_cosynthesis.h"

#ifdef WIN32
#ifndef DLLEXPORT
#define DLLEXPORT __declspec(dllexport)
#endif
#else
#ifndef DLLEXPORT
#define DLLEXPORT __attribute__((visibility("default")))
#endif
#endif

typedef struct
{
	void* cfg;
    wtk_cosynthesis_t *cs;
    int use_bin;
}cosynthesis_t;


void* qvoice_cosynthesis_cfg_new(char* cfn)
{
	wtk_main_cfg_t *main_cfg=NULL;
    main_cfg=wtk_main_cfg_new_type(wtk_cosynthesis_cfg,cfn);
    if (main_cfg && NULL==main_cfg->cfg)
    {
            wtk_main_cfg_delete(main_cfg);
            main_cfg=NULL;
    }

    return (void*)main_cfg;
}

DLLEXPORT void* qvoice_cosynthesis_new(char* cfn)
{
    cosynthesis_t *syn;
    syn = wtk_calloc(1,sizeof(cosynthesis_t));
    syn->cfg = qvoice_cosynthesis_cfg_new(cfn);
    syn->use_bin=0;
    if (syn->cfg)
    {
    	syn->cs = wtk_cosynthesis_new(((wtk_main_cfg_t*)syn->cfg)->cfg);
    	if(NULL==syn->cs)
    	{
    		wtk_free(syn);
    		syn=NULL;
    	}
    }

    return (void*)syn;
}

DLLEXPORT void *qvoice_cosynthesis_newbin(char* cfn)
{
    cosynthesis_t *syn;
    syn = wtk_calloc(1,sizeof(cosynthesis_t));
    syn->cfg = wtk_cosynthesis_cfg_new_bin(cfn, 0);
    syn->use_bin=1;
    if (syn->cfg)
    {
    	syn->cs = wtk_cosynthesis_new(syn->cfg);
        if(NULL==syn->cs)
        {
        	wtk_free(syn);
        	syn=NULL;
        }
    }

    return (void*)syn;
}

DLLEXPORT void qvoice_cosynthesis_delete(void *c)
{
    cosynthesis_t *syn;

    if (c)
    {
        syn = (cosynthesis_t *)c;
        if (syn->use_bin)
        {
            wtk_cosynthesis_delete(syn->cs);
            wtk_cosynthesis_cfg_delete_bin(syn->cfg);
        }else
        {
        	wtk_cosynthesis_delete(syn->cs);
        	wtk_main_cfg_delete(syn->cfg);
        }
        wtk_free(syn);
    }

}

DLLEXPORT void qvoice_cosynthesis_reset(void *c)
{
    cosynthesis_t *syn;
    syn = (cosynthesis_t *)c;
    wtk_cosynthesis_reset(syn->cs);
}

DLLEXPORT int qvoice_cosynthesis_process(void *c, const char *s, const char* sep)
{
    cosynthesis_t *syn;
    syn = (cosynthesis_t *)c;
    return wtk_cosynthesis_feedm(syn->cs,(char*)s, strlen(s), sep);
}

DLLEXPORT void qvoice_cosynthesis_set_notify(void *c, qvoice_cosynthesis_notify notify, void *upval)
{
    cosynthesis_t *syn;
    syn = (cosynthesis_t *)c;
    syn->cs->notify = notify;
    syn->cs->ths = upval;    
}

DLLEXPORT cosynthesis_info_t* qvoice_cosynthesis_getOutput(void *c)
{
    cosynthesis_t *syn;
    syn = (cosynthesis_t *)c;
    return (cosynthesis_info_t*)wtk_cosynthesis_getOutput (syn->cs);
}
