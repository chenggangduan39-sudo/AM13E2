#include "wtk/bfio/qform/beamnet/wtk_beamnet_cfg.h"

int wtk_beamnet_cfg_init(wtk_beamnet_cfg_t *cfg) {

	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;
	cfg->nmic=0;
	cfg->mic_pos=NULL;
	cfg->out_channels=0;
	cfg->feature_len=15;
	cfg->sv=340;
	cfg->feature_type=1;

    cfg->wins=1024;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->seperator));
    qtk_onnxruntime_cfg_init(&(cfg->beamformer));
#endif
	cfg->use_onnx = 1;
    return 0;
}

int wtk_beamnet_cfg_clean(wtk_beamnet_cfg_t *cfg) {
	int i;
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->sp_channel)
	{
		wtk_free(cfg->sp_channel);
	}
	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->nmic;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->seperator));
    qtk_onnxruntime_cfg_clean(&(cfg->beamformer));
#endif

    return 0;
}

int wtk_beamnet_cfg_update(wtk_beamnet_cfg_t *cfg) {
	int ret;

	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	cfg->out_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
#ifdef ONNX_DEC
	if(cfg->use_onnx){
		ret = qtk_onnxruntime_cfg_update(&(cfg->seperator));
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
		ret = qtk_onnxruntime_cfg_update(&(cfg->beamformer));
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
	ret=0;
end:
	return ret;
}

int wtk_beamnet_cfg_update2(wtk_beamnet_cfg_t *cfg, wtk_source_loader_t *sl) {
	int ret;

	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	cfg->out_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
#ifdef ONNX_DEC
	if(cfg->use_onnx){
		ret = qtk_onnxruntime_cfg_update2(&(cfg->seperator), sl->hook);
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
		ret = qtk_onnxruntime_cfg_update2(&(cfg->beamformer), sl->hook);
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
	ret=0;
end:
	return ret;
}

int wtk_beamnet_cfg_update_local(wtk_beamnet_cfg_t *cfg, wtk_local_cfg_t *m) {
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,feature_len,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sv,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,feature_type,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_onnx,v);

	a=wtk_local_cfg_find_array_s(lc,"mic_channel");
	if(a)
	{
		cfg->mic_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nmicchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}

	a=wtk_local_cfg_find_array_s(lc,"sp_channel");
	if(a)
	{
		cfg->sp_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nspchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->sp_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}
	lc=wtk_local_cfg_find_lc_s(m,"mic");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;

		cfg->mic_pos=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->nmic=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->mic_pos[cfg->nmic]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[cfg->nmic][i]=wtk_str_atof(v->data,v->len);
				// wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
			}
			++cfg->nmic;
		}
		if(cfg->nmic!=cfg->nmicchannel)
		{
			wtk_debug("error: nmic=%d!=nmicchannel=%d\n",cfg->nmic,cfg->nmicchannel);
			exit(1);
		}
	}

#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(m, "seperator");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->seperator), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
	lc = wtk_local_cfg_find_lc_s(m, "beamformer");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->beamformer), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
#endif
    ret = 0;
end:
    return ret;
}

wtk_beamnet_cfg_t* wtk_beamnet_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_beamnet_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_beamnet_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_beamnet_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_beamnet_cfg_delete(wtk_beamnet_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_beamnet_cfg_t* wtk_beamnet_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_beamnet_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_beamnet_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_beamnet_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_beamnet_cfg_delete_bin(wtk_beamnet_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
