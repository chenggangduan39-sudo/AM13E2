#include "qtk_tts_symbols_id_cfg.h"

int qtk_tts_symbols_id_cfg_init(qtk_tts_symbols_id_cfg_t *cfg)
{
    cfg->symbols = NULL;
    cfg->is_en = 0;
    return 0;
}
int qtk_tts_symbols_id_cfg_clean(qtk_tts_symbols_id_cfg_t *cfg)
{
    return 0;
}

int qtk_tts_symbols_id_cfg_update_local(qtk_tts_symbols_id_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *main_cfg = lc;

    wtk_local_cfg_update_cfg_b(main_cfg,cfg,is_en,v);
    cfg->symbols=wtk_local_cfg_find_array_s(main_cfg,"symbols");

    return 0;
}

int qtk_tts_symbols_id_cfg_update(qtk_tts_symbols_id_cfg_t *cfg)
{
    return 0;
}