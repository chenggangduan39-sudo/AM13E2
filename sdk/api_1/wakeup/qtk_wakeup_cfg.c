#include "qtk_wakeup_cfg.h"
int qtk_wakeup_cfg_init(qtk_wakeup_cfg_t *cfg) {
    cfg->cfg_fn = NULL;
    cfg->img = NULL;
    cfg->kvwake_cfg = NULL;

    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;
    cfg->img_main_cfg = NULL;
    cfg->use_bin = 0;
    cfg->use_img = 1;
    cfg->use_kvadwake = 0;

    return 0;
}

int qtk_wakeup_cfg_clean(qtk_wakeup_cfg_t *cfg) {
    // wtk_debug("================>>>>>>>main_cfg=%p cfg=%p\n",cfg->img_main_cfg,cfg->img_main_cfg->cfg);
    if (cfg->img) {
    	if (cfg->use_bin)
    		qtk_img_cfg_delete_bin(cfg->img);
    	else
    		wtk_main_cfg_delete(cfg->img_main_cfg);
    }
    if(cfg->kvwake_cfg){
        cfg->use_bin ? wtk_mbin_cfg_delete(cfg->mbin_cfg) : wtk_main_cfg_delete(cfg->main_cfg);
    }
    return 0;
}

int qtk_wakeup_cfg_update_local(qtk_wakeup_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret=0;

    lc = main;
    wtk_local_cfg_update_cfg_str(lc, cfg, cfg_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_img, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_kvadwake, v);

    if (!cfg->cfg_fn) {
        ret = -1;
        goto end;
    }

end:
    return ret;
}

int qtk_wakeup_cfg_update(qtk_wakeup_cfg_t *cfg) {
	wtk_main_cfg_t *main_cfg=0;
    int ret;

    if (cfg->cfg_fn) {
        if(cfg->use_img){
            if (cfg->use_bin)
                cfg->img = qtk_img_cfg_new_bin(cfg->cfg_fn);
            else{
                main_cfg=wtk_main_cfg_new_type(qtk_img_cfg,cfg->cfg_fn);
                if(!main_cfg)
                {
                    wtk_debug("load configure failed.\n");
                    ret=-1; goto end;
                }
                cfg->img_main_cfg = main_cfg;
                cfg->img = (qtk_img_cfg_t*)main_cfg->cfg;
                // wtk_debug("================>>>>>>>main_cfg=%p cfg=%p\n",cfg->img_main_cfg,cfg->img_main_cfg->cfg);
            }
            if (!cfg->img) {
                ret = -1;
                goto end;
            }
        }
        if(cfg->use_kvadwake){
            if(cfg->use_bin){
                cfg->mbin_cfg = wtk_mbin_cfg_new_type(wtk_kvadwake_cfg, cfg->cfg_fn, "./cfg");
                if(cfg->mbin_cfg){
                    cfg->kvwake_cfg = (wtk_kvadwake_cfg_t *)(cfg->mbin_cfg->cfg);
                    ret= 0;
                }
            }else{
                cfg->main_cfg = wtk_main_cfg_new_type(wtk_kvadwake_cfg, cfg->cfg_fn);
                if(cfg->main_cfg){
                    cfg->kvwake_cfg = (wtk_kvadwake_cfg_t *)(cfg->main_cfg->cfg);
                    ret = 0;
                }
            }
        }
    } else {
        ret = -1;
        goto end;
    }
    ret = 0;
end:
    return ret;
}

int qtk_wakeup_cfg_update_img(qtk_wakeup_cfg_t *cfg, wtk_source_t *s) {
    int ret = 0;
    wtk_rbin2_item_t *item;

    item = (wtk_rbin2_item_t *)s->data;
    if(cfg->use_img){
        cfg->img = qtk_img_cfg_new_bin3(item->rb->fn, item->pos);
        if (!cfg->img) {
            ret = -1;
        }
    }else if(cfg->use_kvadwake){
        cfg->mbin_cfg = wtk_mbin_cfg_new_type2(item->pos, wtk_kvadwake_cfg, item->rb->fn, "./cfg");
        if(cfg->mbin_cfg){
            cfg->kvwake_cfg = (wtk_kvadwake_cfg_t *)(cfg->mbin_cfg->cfg);
            ret= 0;
        }
    }
    return ret;
}

int qtk_wakeup_cfg_update2(qtk_wakeup_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret;

    if (cfg->cfg_fn) {
        ret = wtk_source_loader_load(
            sl, cfg, (wtk_source_load_handler_t)qtk_wakeup_cfg_update_img,
            cfg->cfg_fn);
        if (ret != 0) {
            goto end;
        }
    } else {
        ret = -1;
        goto end;
    }
    ret = 0;
end:
    return ret;
}

qtk_wakeup_cfg_t *qtk_wakeup_cfg_new(char *cfg_fn) {
    qtk_wakeup_cfg_t *cfg;
    wtk_main_cfg_t *main_cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_wakeup_cfg, cfg_fn);
    cfg = (qtk_wakeup_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void qtk_wakeup_cfg_delete(qtk_wakeup_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_wakeup_cfg_t *qtk_wakeup_cfg_new_bin(char *bin_fn) {
    qtk_wakeup_cfg_t *cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_wakeup_cfg, bin_fn, "./cfg");
    cfg = (qtk_wakeup_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void qtk_wakeup_cfg_delete_bin(qtk_wakeup_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
