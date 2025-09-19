/*
 * qtk_tts_module.c
 *
 *  Created on: Feb 23, 2023
 *      Author: dm
 */

#include "qtk/soundtouch/qtk_soundtouch_api.h"
#include "qtk_tts_module.h"
#include "qtk_tts_module_api.h"

typedef struct{
	wtk_main_cfg_t* main_cfg;
	qtk_tts_module_cfg_t* cfg;
	qtk_tts_module_t* m;
	unsigned use_bin:1;
}qtk_tts_module_api_t;

static void qtk_tts_param_init(qtk_tts_param_t* param)
{
	param->vol = 1.0;
	param->tempo = 1.0;
	param->pitch = 1.0;
	param->rate = 1.0;
}

static int qtk_tts_param_changed(qtk_tts_param_t* param)
{
	if (param->tempo !=1.0 || param->pitch != 1.0 || param->rate != 1.0)
		return 1;

	return 0;
}

static void qtk_tts_vol_changed(short *data, int size, float vol)
{
	int i;
	float v;

	if (vol !=1.0)
	{
		for(i=0; i < size; i++){
			v = data[i] * vol;
			data[i] = v > 32767 ? 32767: v;
		}
	}
}

static int qtk_tts_soundstretch(qtk_tts_module_t* tts, short* data, int len, int is_end)
{
	int i, nsample=0;
	short buf[SOUNDTOUCH_BUFF_SIZE];

    i=0;
	while (i<len)
	{
		nsample = SOUNDTOUCH_BUFF_SIZE > len? len: SOUNDTOUCH_BUFF_SIZE;
		qtk_soundtouch_api_feed(tts->soundtouch, buf, nsample);

		if (tts->notify)
		{
	        do{
	        	nsample=qtk_soundtouch_api_recv(tts->soundtouch, buf, SOUNDTOUCH_BUFF_SIZE);
	    		if (nsample <= 0)break;
				qtk_tts_vol_changed(buf, nsample, tts->param.vol);
				if (is_end && i + SOUNDTOUCH_BUFF_SIZE >= len)
					tts->notify(tts->user_data, buf, nsample, 1);
				else
					tts->notify(tts->user_data, buf, nsample, 0);
	        } while (1);
		}
		i+=SOUNDTOUCH_BUFF_SIZE;
		data += SOUNDTOUCH_BUFF_SIZE;
	}

	if (is_end){
        do{
        	nsample=qtk_soundtouch_api_recv(tts->soundtouch, buf, SOUNDTOUCH_BUFF_SIZE);
    		if (nsample <= 0)break;
			qtk_tts_vol_changed(buf, nsample, tts->param.vol);
			if (is_end && i + SOUNDTOUCH_BUFF_SIZE >= len)
				tts->notify(tts->user_data, buf, nsample, 1);
			else
				tts->notify(tts->user_data, buf, nsample, 0);
        } while (1);
	}

	return 0;
}

static int qtk_tts_module_notify(void* ths, short* data, int len, int is_end)
{
	qtk_tts_module_t* tts=(qtk_tts_module_t*)ths;

	if (tts->notify)
	{
		if (qtk_tts_param_changed(&(tts->param)))
			qtk_tts_soundstretch(tts, data, len, is_end);
		else
		{
			qtk_tts_vol_changed(data, len, tts->param.vol);
			tts->notify(tts->user_data, data, len, is_end);
		}
	}

	return 0;
}

qtk_tts_module_t* qtk_tts_module_new(qtk_tts_module_cfg_t* cfg)
{
	qtk_tts_module_t* m;
	int ret=0;

	m = wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;

    m->parse = qtk_tts_parse_new(&(cfg->parse));
    if (NULL==m->parse){
    	ret=-1;	goto end;
    }

    m->syn = qtk_tts_syn_new(&(cfg->syn));
    if (NULL==m->syn){
    	ret=-1; goto end;
    }
    qtk_tts_syn_set_notify(m->syn, (qtk_tts_syn_notify_f)qtk_tts_module_notify, m);

    m->soundtouch = qtk_soundtouch_api_new(NULL);
    qtk_tts_param_init(&(m->param));
end:
	if (ret!=0){
    	qtk_tts_module_delete(m);
    	m=NULL;
	}
	return m;
}


#include "wtk/core/math/wtk_mat.h"
int tts_mod_test[56]={
		109, 66, 106, 16, 77, 104, 7, 33, 108, 26, 8, 94, 105, 18,
		                                          33, 104, 26, 14, 85, 104, 12, 91, 107, 19, 42, 107, 4, 77,
		                                          105, 28, 109, 15, 94, 106, 26, 10, 85, 105, 14, 57, 104, 19,
		                                          49, 106, 14, 35, 104, 26, 18, 29, 105, 12, 87, 107, 28, 109
};

int qtk_tts_module_feed(qtk_tts_module_t* m, char* data, int len)
{
	int ret=0;
	//text parser
	ret = qtk_tts_parse_process(m->parse, data, len);
	if (ret!=0)goto end;
	ret = qtk_tts_syn_feed(m->syn, m->parse->id_vec, m->parse->nid);

	//test example
//	wtk_veci_t*id_vec;
//	id_vec = wtk_veci_new(56);
//	id_vec->p = tts_mod_test;
//	ret = qtk_tts_syn_feed(m->syn, &id_vec, 1);
//	wtk_veci_delete(id_vec);

end:
	return ret;
}

int qtk_tts_module_reset(qtk_tts_module_t* m)
{
	int ret=0;

	ret = qtk_tts_parse_reset(m->parse);
	if (ret!=0) goto end;
	ret = qtk_tts_syn_reset(m->syn);
	if (ret!=0) goto end;
end:

	return ret;
}

void qtk_tts_module_delete(qtk_tts_module_t* m)
{
	if (m->parse)
		qtk_tts_parse_delete(m->parse);
	if (m->syn)
		qtk_tts_syn_delete(m->syn);
	if (m->soundtouch)
		qtk_soundtouch_api_delete(m->soundtouch);

    wtk_free(m);
}

void qtk_tts_module_setNotify(qtk_tts_module_t* m, qtk_tts_module_notify_f notify, void* user_data)
{
	m->notify = notify;
	m->user_data = user_data;
}

void qtk_tts_module_setVol(qtk_tts_module_t* m, float vol)
{
	m->param.vol = vol;
}

void qtk_tts_module_setPitch(qtk_tts_module_t* m, float pitch)
{
	m->param.pitch = pitch;
    if (pitch != 1.0)
    	qtk_soundtouch_api_setpitch(m->soundtouch, pitch);
}

void qtk_tts_module_setTempo(qtk_tts_module_t* m, float tempo)
{
	m->param.tempo = tempo;
    if (tempo != 1.0)
    	qtk_soundtouch_api_settemp(m->soundtouch, tempo);
}

void qtk_tts_module_setVolChanged(qtk_tts_module_t* m, float volDelta)
{
	m->param.vol = m->param.vol * (1+volDelta * 0.01);
}

void qtk_tts_module_setPitchChanged(qtk_tts_module_t* m, float pitchDelta)
{
    qtk_soundtouch_api_setPitchSemiTones(m->soundtouch, pitchDelta);
}

void qtk_tts_module_setRateChanged(qtk_tts_module_t* m, float rateDelta)
{
	qtk_soundtouch_api_setRateChange(m->soundtouch, rateDelta);
}


void qtk_tts_module_setTempoChanged(qtk_tts_module_t* m, float tempoDelta)
{
    qtk_soundtouch_api_settemp(m->soundtouch, tempoDelta);
}

TTS_DLL_API void* qtk_tts_module_api_new(char *fn, int use_bin)
{
    qtk_tts_module_api_t* api;
    int ret;

    api = (qtk_tts_module_api_t*)calloc(1, sizeof(*api));
    api->use_bin = use_bin;
    if (use_bin)
    {
    	api->cfg = qtk_tts_module_cfg_new_bin(fn, 0);
    }
    else
    {
        api->main_cfg = wtk_main_cfg_new_type(qtk_tts_module_cfg,fn);
        if (!api->main_cfg){
        	printf("cfg res error\n");
        	ret=-1;
        	goto end;
        }
        api->cfg = api->main_cfg->cfg;
    }
    if (NULL == api->cfg)
    {
    	printf("cfg res error\n");
    	ret=-1;goto end;
    }
    api->m = qtk_tts_module_new(api->cfg);
    if (NULL==api->m)
    {
    	ret = -1;goto end;
    }
    ret = 0;

end:
	if (ret!=0){
    	qtk_tts_module_api_delete(api);
    	api = NULL;
	}

    return api;
}

TTS_DLL_API int qtk_tts_module_api_feed(void* api, char* data, int len)
{
	return qtk_tts_module_feed(((qtk_tts_module_api_t*)api)->m, data, len);
}

TTS_DLL_API int qtk_tts_module_api_reset(void* api)
{
	return qtk_tts_module_reset(((qtk_tts_module_api_t*)api)->m);
}

TTS_DLL_API void qtk_tts_module_api_delete(void *api)
{
	qtk_tts_module_api_t* ma=(qtk_tts_module_api_t*)api;

	if (ma->m)
		qtk_tts_module_delete(((qtk_tts_module_api_t*)api)->m);
    if(ma->main_cfg)
    	wtk_main_cfg_delete(((qtk_tts_module_api_t*)api)->main_cfg);
    else if(ma->cfg)
    	qtk_tts_module_cfg_delete_bin(((qtk_tts_module_api_t*)api)->cfg);

    wtk_free(api);
}

TTS_DLL_API void qtk_tts_module_api_setNotify(void* api, qtk_tts_notify_f notify, void* user_data)
{
	qtk_tts_module_setNotify(((qtk_tts_module_api_t*)api)->m, notify, user_data);
}

TTS_DLL_API void qtk_tts_module_api_setVol(void* api, float vol)
{
	qtk_tts_module_setVol(((qtk_tts_module_api_t*)api)->m, vol);
}

TTS_DLL_API void qtk_tts_module_api_setPitch(void* api, float pitch)
{
	qtk_tts_module_setVolChanged(((qtk_tts_module_api_t*)api)->m, pitch);
}

TTS_DLL_API void qtk_tts_module_api_setTempo(void* api, float tempo)
{
	qtk_tts_module_setTempo(((qtk_tts_module_api_t*)api)->m, tempo);
}

TTS_DLL_API void qtk_tts_module_api_setVolChanged(void* api, float volDelta)
{
	qtk_tts_module_setVolChanged(((qtk_tts_module_api_t*)api)->m, volDelta);
}

TTS_DLL_API void qtk_tts_module_api_setPitchChanged(void* api, float pitchDelta)
{
	qtk_tts_module_setPitchChanged(((qtk_tts_module_api_t*)api)->m, pitchDelta);
}

TTS_DLL_API void qtk_tts_module_api_setTempoChanged(void* api, float tempoDelta)
{
	qtk_tts_module_setTempoChanged(((qtk_tts_module_api_t*)api)->m, tempoDelta);
}

TTS_DLL_API void qtk_tts_module_api_setRateChanged(void* api, float rateDelta)
{
	qtk_tts_module_setRateChanged(((qtk_tts_module_api_t*)api)->m, rateDelta);
}
