#include "qtk_estimate.h"

#define FEED_STEP (20 * 32 * 2)
void qtk_estimate_on_data(qtk_estimate_t *vb, short *data, int len, int is_end);
void qtk_estimate_on_aec(qtk_estimate_t *gd, short **data, int len, int is_end);
void qtk_estimate_on_gainnet_aec3(qtk_estimate_t *gd, short **data, short **lasty, int len, int is_end);

qtk_estimate_t *qtk_estimate_new(qtk_estimate_cfg_t *cfg)
{
	qtk_estimate_t *vb;
	int i, ret;

	vb = (qtk_estimate_t *)wtk_calloc(1, sizeof(*vb));
	vb->cfg = cfg;
	vb->ths=NULL;
	vb->notify=NULL;
	vb->eths=NULL;
	vb->enotify=NULL;
	vb->rir_est=NULL;
	vb->channel = 1;

	if(vb->cfg->estimate_cfg){
		vb->rir_est = qtk_rir_estimate2_new(vb->cfg->estimate_cfg);
		if(!vb->rir_est){
			ret = -1;
			goto end;
		}
		vb->channel = vb->cfg->estimate_cfg->channel;
	}

	ret = 0;
end:
	if(ret != 0){
		qtk_estimate_delete(vb);
		vb = NULL;		
	}	
	return vb;
}
int qtk_estimate_delete(qtk_estimate_t *vb)
{
	int i;

	if(vb->rir_est)
	{
		qtk_rir_estimate2_delete(vb->rir_est);
	}
	wtk_free(vb);
	return 0;
}
int qtk_estimate_start(qtk_estimate_t *vb)
{
	qtk_rir_estimate2_start(vb->rir_est);
	return 0;
}
int qtk_estimate_reset(qtk_estimate_t *vb)
{
	qtk_rir_estimate2_reset(vb->rir_est);
	return 0;
}

float *qtk_estimate_code_generate(qtk_estimate_t *vb)
{
	qtk_estimate2_code_generate(vb->rir_est);

	if(vb->enotify)
	{
		qtk_var_t var;
		var.type = QTK_VAR_SOURCE_AUDIO;
		var.v.str.data = (char *)(vb->rir_est->pv_code);
		var.v.str.len = vb->rir_est->cfg->code_len<<1;

		vb->enotify(vb->eths, &var);
	}

	return NULL;
}

static int qtk_estimate_on_feed(qtk_estimate_t *vb, char *data, int bytes, int is_end)
{
	if(bytes > 0)
	{
		short *pv = NULL;
		int len=0;
		pv = (short*)data;
		len = bytes /(vb->channel * sizeof(short));

		qtk_rir_estimate2_feed(vb->rir_est, (short *)data, len, 0);

	}
	if(is_end){
		qtk_rir_estimate2_feed(vb->rir_est, NULL, 0, 1);
		int i=0;
		for(i=0;i<vb->channel;++i){
			//printf("%d\n", vb->rir_est->recommend_delay[i]);
			if(vb->enotify)
			{
				qtk_var_t var;
				var.type = QTK_AUDIO_ESTIMATE;
				var.v.str2.idx = i;
				var.v.str2.len = vb->rir_est->recommend_delay[i];
				vb->enotify(vb->eths, &var);
			}
		}
	}
	return 0;
}

int qtk_estimate_feed(qtk_estimate_t *vb, char *data, int len, int is_end)
{
#if 0
	int pos = 0;
	int step = 0;
	int flen;

	step = FEED_STEP * (vb->channel+vb->cfg->nskip);
	while(pos < len){
		flen = min(step, len-pos);
		qtk_estimate_on_feed(vb, data + pos, flen, 0);
		pos +=flen;
	}
	if(is_end){
		qtk_estimate_on_feed(vb, NULL, 0, 1);
	}
#else
	qtk_estimate_on_feed(vb, data, len, is_end);
#endif
	return 0;
}
void qtk_estimate_set_notify(qtk_estimate_t *vb, void *ths, qtk_estimate_notify_f notify)
{
	vb->ths = ths;
	vb->notify = notify;
}

void qtk_estimate_set_notify2(qtk_estimate_t *vb, void *ths, qtk_engine_notify_f notify)
{
	vb->eths=ths;
	vb->enotify=notify;
}