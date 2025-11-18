/*
 * wtk_asr_api.c
 *
 *  Created on: 2024年3月27日
 *      Author: root
 */
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/wfst/qtk_asr_wrapper_cfg.h"
#include "wtk/asr/wfst/qtk_asr_wrapper.h"
#include "wtk/asr/api/qvoice_asr_api.h"
#ifdef USE_CNTLIMIT
#define MAX_COUNT_LIMIT 30*60*60*10
static long qtk_count=0;
#endif
typedef struct qvoice_asr_api{
	qtk_asr_wrapper_t *dec;
	qtk_asr_wrapper_cfg_t *cfg;
	wtk_vad_cfg_t *vcfg;
    wtk_vad_t *vad;
	wtk_main_cfg_t *main_cfg;
	wtk_main_cfg_t *main_vcfg;
	wtk_strbuf_t *buf;
	wtk_queue_t vqueue;
	int wakeup_cnt;
	int task;    //0:wakeup, 1: cmd
	int frame_cnt;
	int use_wakeup;
	int use_cmd;
	int vad_frame_state;
//	int skip_frame;
//	int skip_frame_cnt;
}qvoice_asr_api_t;

void* qvoice_asr_api_new(char* cfg_fn, char* vcfg_fn)
{
	qvoice_asr_api_t *api=NULL;
	wtk_main_cfg_t *main_cfg=NULL;
	int ret;
	int bin=1;

	ret=0;
	api = (qvoice_asr_api_t*) wtk_calloc(sizeof(*api), 1);
	api->buf = wtk_strbuf_new(128, 1);
	if (bin)
	{
		api->cfg = qtk_asr_wrapper_cfg_new_bin(cfg_fn);
		if (vcfg_fn)
			api->vcfg = wtk_vad_cfg_new_bin2(vcfg_fn);
	}else{
		main_cfg=wtk_main_cfg_new_type(qtk_asr_wrapper_cfg,cfg_fn);
		if(!main_cfg)
		{
			wtk_debug("load configure failed.\n");
			goto end;
		}
		api->main_cfg = main_cfg;
		api->cfg=(qtk_asr_wrapper_cfg_t*)main_cfg->cfg;
		//VAD
		if (vcfg_fn)
		{
			main_cfg=wtk_main_cfg_new_type(wtk_vad_cfg,vcfg_fn);
			if(!main_cfg)
			{
				wtk_debug("load VAD configure failed.\n");
				goto end;
			}
			api->main_vcfg = main_cfg;
			api->vcfg=(wtk_vad_cfg_t*)main_cfg->cfg;
			if (!api->vcfg)
			{
				wtk_debug("[qvoice]: create vad cfg failed.\n");
				ret=-1;
				goto end;
			}
		}
	}

	if (!api->cfg)
	{
		wtk_debug("[qvoice]: create k2 cfg failed.\n");
		ret=-1;
		goto end;
	}

	api->dec=qtk_asr_wrapper_new(api->cfg);
	if(!api->dec)
	{
		wtk_debug("[qvoice]: create k2 failed.\n");
		ret=-1;
		goto end;
	}
	if (api->vcfg)
	{
		wtk_queue_init(&api->vqueue);
		api->vad = wtk_vad_new(api->vcfg,&api->vqueue);
		if(!api->vad)
		{
			wtk_debug("[qvoice]: create vad failed.\n");
			ret=-1;
			goto end;
		}
	}
end:
	if (ret!=0)
	{
		qvoice_asr_api_delete(api);
		api=NULL;
	}
	return (void*)api;
}

int qvoice_asr_api_start(void* h)
{
	qvoice_asr_api_t* api;

	api = (qvoice_asr_api_t*)h;
	if (api->vad)
	{
		return wtk_vad_start(api->vad);
	}
	else
		return qtk_asr_wrapper_start(api->dec);
}

int st=0,end=0;
int qvoice_asr_api_feed(void* h, char* data, int size, int isend)
{
	qvoice_asr_api_t* api;
	wtk_vframe_t *f;
	wtk_queue_node_t *qn;
	int ret;

#ifdef USE_CNTLIMIT
	if  (qtk_count++ > MAX_COUNT_LIMIT)
	{
		wtk_debug("[Error]: overcome maximum use times\n");
		return -1;
	}
#endif
	api = (qvoice_asr_api_t*)h;
	if (api->vad)
	{
		wtk_vad_feed(api->vad, data, size, isend);
		while(1){
			qn = wtk_queue_pop(&api->vqueue);
			if(!qn){
				break;
			}
//			if (api->skip_frame_cnt > 0)
//			{
//				api->skip_frame_cnt--;
//				break;
//			}
			f = data_offset(qn,wtk_vframe_t,q_n);

			switch (api->vad_frame_state)
			{
			case wtk_vframe_sil:
				if(f->state != wtk_vframe_sil)
				{
					st = api->frame_cnt;
					qtk_asr_wrapper_set_vadindex(api->dec,f->index);
					qtk_asr_wrapper_start(api->dec);
					ret = qtk_asr_wrapper_feed(api->dec,(char*)f->wav_data,sizeof(short)*api->vad->cfg->dnnvad.parm.frame_step,0);
					if(ret == 1)
					{
						api->wakeup_cnt++;
//						api->skip_frame_cnt = api->skip_frame;
//						wtk_debug("wakeup\n");
						if (api->use_cmd)
						{
							qtk_asr_wrapper_feed(api->dec, 0, 0, 1);
							qtk_asr_wrapper_reset(api->dec);
							qtk_asr_wrapper_start(api->dec);
						}
					}else
					{
						if (api->use_cmd && api->use_wakeup > 0)
							api->task = 1;
						//do this operation for decrease delay.
//						if (api->use_cmd && api->wakeup_flag==1)
//						{
//							//asr
//							if (api->use_cmd && api->wakeup_flag > 0)
//							{
//								wtk_string_t v;
//								qtk_asr_wrapper_get_result(api->dec, &v);
//								if(v.len > 0){
//									printf("%.*s\n",v.len,v.data);
//								}
//								wtk_strbuf_push(api->buf, v.data, v.len);
//							}
//						}
					}
					api->vad_frame_state=wtk_vframe_speech;
				}
				break;
			case wtk_vframe_speech:
				if(f->state != wtk_vframe_speech){
					end = api->frame_cnt;
					api->vad_frame_state = wtk_vframe_sil;
					if (api->use_cmd && api->wakeup_cnt > 0 && api->task == 0)
						continue;
					ret = qtk_asr_wrapper_feed(api->dec,0,0,1);
//					wtk_debug("%d %d\n",st,end);
					if(ret == 1){
						api->wakeup_cnt++;
//						api->skip_frame_cnt = 0;   //to sil, don't skip
//						wtk_debug("wakeup\n");
					}else{
						//asr
						if (api->use_cmd && api->task==1 && api->wakeup_cnt > 0)
						{
							wtk_string_t v;
							qtk_asr_wrapper_get_result(api->dec, &v);
//							if(v.len > 0){
//								printf("%.*s\n",v.len,v.data);
//							}
							wtk_strbuf_push(api->buf, v.data, v.len);
						}
					}
					qtk_asr_wrapper_reset(api->dec);
				}else{
//					printf("use_cmd=%d wakeup_cnt:%d task:%d\n", api->use_cmd, api->wakeup_cnt, api->task);
					if (api->use_cmd && api->wakeup_cnt > 0 && api->task == 0)
						continue;
					ret = qtk_asr_wrapper_feed(api->dec,(char*)f->wav_data,sizeof(short)*api->vad->cfg->dnnvad.parm.frame_step,0);
					if(ret == 1){
						api->wakeup_cnt++;
//						api->skip_frame_cnt = api->skip_frame;
//						wtk_debug("wakeup\n");
						if (api->use_cmd)
						{
							qtk_asr_wrapper_feed(api->dec, 0, 0, 1);
							qtk_asr_wrapper_reset(api->dec);
//							qtk_asr_wrapper_start(api->dec);
						}
					}
				}
				break;
			case wtk_vframe_speech_end:
				break;
			default:
				break;
			}
			++api->frame_cnt;
//				wtk_fnnvad_push_vframe(api->vad->route.dnnvad, f);
			wtk_vad_push_vframe(api->vad,f);
		}
		return 0;
	}
	else
		return qtk_asr_wrapper_feed(api->dec, data, size, isend);
}

int qvoice_asr_api_setContext(void* h,char *data,int bytes)
{
	qvoice_asr_api_t* api;

	api = (qvoice_asr_api_t*)h;
	return qtk_asr_wrapper_set_context(api->dec, data, bytes);
}

int qvoice_asr_api_setContext_asr(void* h,char *data,int bytes)
{
	qvoice_asr_api_t* api;

	api = (qvoice_asr_api_t*)h;
	api->use_cmd = 1;

	return qtk_asr_wrapper_set_context_asr(api->dec, data, bytes);
}

int qvoice_asr_api_setContext_wakeup(void* h,char *data,int bytes)
{
	qvoice_asr_api_t* api;

	api = (qvoice_asr_api_t*)h;
	api->use_wakeup = 1;

	return qtk_asr_wrapper_set_context_wakeup(api->dec, data, bytes);
}

int qvoice_asr_api_get_result(void* h, char*data, int len)
{
	qvoice_asr_api_t* api;
	api = (qvoice_asr_api_t*)h;
	wtk_string_t v;
	int l;

	qtk_asr_wrapper_get_result(api->dec, &v);
	l = v.len < len? v.len: len-1;
	memcpy(data, v.data, l);

	return l;
}

int qvoice_asr_api_get_wake(void* h)
{
	qvoice_asr_api_t* api;
	api = (qvoice_asr_api_t*)h;

	if (api->wakeup_cnt > 0)
		return 1;
	else
		return 0;
}

int qvoice_asr_api_reset(void* h)
{
	qvoice_asr_api_t* api;

	api = (qvoice_asr_api_t*)h;
	api->vad_frame_state = wtk_vframe_sil;
	api->task=0;
	api->use_wakeup=0;
	api->wakeup_cnt=0;
//	api->skip_frame = 5;
	wtk_strbuf_reset(api->buf);
	qtk_asr_wrapper_reset(api->dec);

	return 0;
}

void qvoice_asr_api_delete(void* h)
{
	qvoice_asr_api_t* api;

	api = (qvoice_asr_api_t*)h;
	wtk_strbuf_delete(api->buf);
	if(api->dec)
	{
		qtk_asr_wrapper_delete(api->dec);
	}
	if (api->main_cfg)
	{
		wtk_main_cfg_delete(api->main_cfg);
	}else if(api->cfg)
	{
		qtk_asr_wrapper_cfg_delete_bin(api->cfg);
	}

	if(api->vad)
	{
		wtk_vad_delete(api->vad);
	}
	if(api->main_vcfg)
	{
		wtk_main_cfg_delete(api->main_vcfg);
	}else if (api->vcfg)
	{
		wtk_vad_cfg_delete_bin(api->vcfg);
	}

	wtk_free(api);
}


