#include "qtk_tts_symbols_id.h"

qtk_tts_symbols_id_t* qtk_tts_symbols_id_new(qtk_tts_symbols_id_cfg_t *cfg)
{
    qtk_tts_symbols_id_t *sym = NULL;

    sym = wtk_malloc(sizeof(*sym));
    sym->cfg = cfg;

    return sym;
}
int qtk_tts_symbols_id_delete(qtk_tts_symbols_id_t *symbols)
{
    if(symbols == NULL)
        goto end;

    wtk_free(symbols);
end:
    return 0;
}

int qtk_tts_symbols_id_get_id(qtk_tts_symbols_id_t *symbols,char *sym,int len)
{
    int id = -1;
    wtk_string_t **strs = NULL;
    int i = 0,nstr = 0;
    nstr = symbols->cfg->symbols->nslot;
    strs = symbols->cfg->symbols->slot;
    for(i = 0;i < nstr; ++i){
        if(len == strs[i]->len && !wtk_string_cmp(strs[i],sym,len)){
            id = i;
            break;
        }
    }
    return id;
}