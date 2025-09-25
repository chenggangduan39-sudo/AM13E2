#include "wtk_equalizer_cfg.h"

int wtk_equalizer_cfg_init(wtk_equalizer_cfg_t *cfg)
{
    // cfg->octave = 1.0;
    cfg->octave = 2.0/3.0;
    cfg->sfreq = 16000.0f;
    cfg->extra_filter = 1;
    cfg->value = NULL;
    cfg->rate=NULL;
    cfg->band_count = 0;
    
    return 0;
}

int wtk_equalizer_cfg_clean(wtk_equalizer_cfg_t *cfg)
{
    if(cfg->value)
    {
        wtk_free(cfg->value);
    }
    if(cfg->rate)
    {
        wtk_free(cfg->rate);
    }

    return 0;
}

int wtk_equalizer_cfg_update_local(wtk_equalizer_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    int ret = 0,i = 0;
    wtk_string_t *v = NULL;
    wtk_array_t *array = NULL;

    wtk_local_cfg_update_cfg_f(lc,cfg,sfreq,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,octave,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,extra_filter,v);

	array=wtk_local_cfg_find_array_s(lc,"rate");
    if(array)
    {
        cfg->band_count=array->nslot;
        cfg->rate=wtk_malloc(sizeof(float)*cfg->band_count);
        for(i=0;i<cfg->band_count;++i)
        {
            v = ((wtk_string_t**)array->slot)[i];
            cfg->rate[i]  = wtk_str_atof(v->data,v->len);
        }
    }
    array = wtk_local_cfg_find_array_s(lc,"value");
    if(array == NULL)
    {
        wtk_debug("error:equalizer value don't have\n");
        ret = -1;
        goto end;
    }
    if(cfg->band_count != array->nslot)
    {
        wtk_debug("error:equalizer value don't have\n");
        ret = -1;
        goto end;
    }
    cfg->value = wtk_malloc(sizeof(float)*cfg->band_count);
    for(i = 0; i < cfg->band_count; ++i)
    {
        v = ((wtk_string_t**)array->slot)[i];
        cfg->value[i]  = wtk_str_atof(v->data,v->len);
    }
end:
    return ret;
}

int wtk_equalizer_cfg_update(wtk_equalizer_cfg_t *cfg)
{
    int ret = 0;
    return ret;
}