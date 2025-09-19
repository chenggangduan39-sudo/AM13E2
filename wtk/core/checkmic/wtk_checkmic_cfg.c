#include "wtk_checkmic_cfg.h"

int wtk_checkmic_cfg_init(wtk_checkmic_cfg_t *cfg)
{
    cfg->channel = 2;
    cfg->mic_pos = NULL;
    cfg->rate = 16000;

    return 0;
}

int wtk_checkmic_cfg_clean(wtk_checkmic_cfg_t *cfg)
{
    int i;

	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->channel;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
    return 0;
}

int wtk_checkmic_cfg_update_local(wtk_checkmic_cfg_t *cfg, wtk_local_cfg_t *lc)
{
    wtk_string_t *v;
    wtk_local_cfg_t *m;

    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    
    m = lc;
    lc = wtk_local_cfg_find_lc_s(m, "mic_pos");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;

		cfg->mic_pos=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->channel=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->mic_pos[cfg->channel]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[cfg->channel][i]=wtk_str_atof(v->data,v->len);
			}
			++cfg->channel;
		}
	}

    return 0;
}

int wtk_checkmic_cfg_update(wtk_checkmic_cfg_t *cfg)
{
    return 0;
}