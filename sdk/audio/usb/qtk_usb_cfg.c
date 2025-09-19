#include "qtk_usb_cfg.h"

int qtk_usb_cfg_init(qtk_usb_cfg_t *cfg)
{
	cfg->vendor_id=0x0483;
	cfg->product_id=0x5740;
	cfg->in_point=0x81;
	cfg->out_point=0x01;

	cfg->buf_time=20;
	cfg->buf_bytes=0;

	cfg->sample_rate=16000;
	cfg->bytes_per_sample=2;
	cfg->channel=8;
	cfg->nskip=0;
	cfg->skip_channels=NULL;

	cfg->ply_cache=5;
	cfg->rcd_cache=5;
	cfg->timeout=100;
	cfg->ply_step=64;

	cfg->use_asy_ply=0;
	cfg->use_asy_rcd=0;

	cfg->mic_gain = 0xD3;
	cfg->cb_gain = 0xB1;
	cfg->ply_volume = 0x90;
	cfg->use_adjust = 0;
	cfg->use_3_cb_start = 0;
	cfg->use_hotplug = 0;

	cfg->debug=1;
	return 0;
}


int qtk_usb_cfg_clean(qtk_usb_cfg_t *cfg)
{
	wtk_free(cfg->skip_channels);
	return 0;
}

int qtk_usb_cfg_update_local(qtk_usb_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_array_t *a;
	int i;
//	char vid[64];
//	int pid;

	a=wtk_local_cfg_find_array_s(lc,"skip_channels");
	if(a)
	{
		cfg->skip_channels=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nskip=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->skip_channels[i]=wtk_str_atoi(v->data,v->len);
		}
	}
	wtk_local_cfg_update_cfg_i(lc,cfg,vendor_id,v);
//	wtk_debug("====>%d\n",cfg->vendor_id);
////	sprintf(vid,"%#x",cfg->vendor_id);
//	wtk_debug("=====>vid=%s\n",vid);
////	cfg->vendor_id = *vid;
	wtk_local_cfg_update_cfg_i(lc,cfg,product_id,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,in_point,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,out_point,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,buf_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rcd_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ply_cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,timeout,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ply_step,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,mic_gain,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cb_gain,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ply_volume,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_asy_rcd,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_asy_ply,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_3_cb_start,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_adjust,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hotplug,v);
	return 0;
}

int qtk_usb_cfg_update(qtk_usb_cfg_t *cfg)
{
	if(cfg->use_adjust) {
		if(cfg->mic_gain > 0xFE) {
			cfg->mic_gain = 0xFE;
		} else if (cfg->mic_gain < 0xB0) {
			cfg->mic_gain = 0xB0;
		}

		if(cfg->cb_gain > 0xFE) {
			cfg->mic_gain = 0xFE;
		} else if (cfg->cb_gain < 0xB0) {
			cfg->cb_gain = 0xB0;
		}

		if(cfg->ply_volume > 0xA9) {
			cfg->ply_volume = 0xA9;
		} else if (cfg->ply_volume < 0x90) {
			cfg->ply_volume = 0x90;
		}
	}
	cfg->buf_bytes=cfg->buf_time*cfg->channel*cfg->sample_rate*cfg->bytes_per_sample/1000;
	//wtk_debug("buf->bytes = %d.\n",cfg->buf_bytes);
	return 0;
}
