#include "qtk_recorder_cfg.h"

int qtk_recorder_cfg_init(qtk_recorder_cfg_t *cfg) {
    cfg->snd_name = "default";
    cfg->channel = 1;
    cfg->skip_channels = NULL;
    cfg->nskip = 0;
    cfg->sample_rate = 16000;
    cfg->bytes_per_sample = 2;
    cfg->buf_time = 20;
    cfg->buf_size = 0;

    cfg->mic_gain = 196;
    cfg->use_gain_set = 0;

    return 0;
}

int qtk_recorder_cfg_clean(qtk_recorder_cfg_t *cfg) {
    if (cfg->skip_channels) {
        wtk_free(cfg->skip_channels);
    }
    return 0;
}

int qtk_recorder_cfg_update_local(
    qtk_recorder_cfg_t *cfg, wtk_local_cfg_t *lc) //,int *channels,int nskip)
{
    wtk_string_t *v;
    wtk_array_t *a;
    int i;

    a = wtk_local_cfg_find_array_s(lc, "skip_channels");
    if (a) {
        cfg->skip_channels = (int *)wtk_malloc(sizeof(int) * a->nslot);
        cfg->nskip = a->nslot;

        for (i = 0; i < a->nslot; ++i) {
            v = ((wtk_string_t **)a->slot)[i];
            cfg->skip_channels[i] = wtk_str_atoi(v->data, v->len);
        }
    }
    // cfg->nskip = nskip;
    // printf(">>>>>>>>>>>>>>>>> cfg->nskip in recor_up_local:%d
    // \n",cfg->nskip); cfg->skip_channels =
    // (int*)wtk_malloc(sizeof(int)*nskip);
    // memcpy(cfg->skip_channels,channels,4*nskip);
    wtk_local_cfg_update_cfg_str(lc, cfg, snd_name, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sample_rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, bytes_per_sample, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, buf_time, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, mic_gain, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_gain_set, v);
    return 0;
}

int qtk_recorder_cfg_update(qtk_recorder_cfg_t *cfg) {
    cfg->buf_size = cfg->buf_time * cfg->sample_rate * cfg->bytes_per_sample *
                    cfg->channel / 1000;
    return 0;
}
