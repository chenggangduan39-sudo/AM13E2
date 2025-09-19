#include "qtk_uart_cfg.h"

int qtk_uart_cfg_init(qtk_uart_cfg_t *cfg)
{
    wtk_string_set(&cfg->dev, 0, 0);
    wtk_string_set(&cfg->log_fn, 0, 0);
    cfg->uart_log = NULL;
    cfg->baude = 115200;
    cfg->bits = 8;
    cfg->parity = 0;
    cfg->stop = 1;
    cfg->c_flow = 0;
    cfg->step_size = 1536;
    cfg->use_uart_log = 0;
    
    // cfg->doa.stage=0;
    // cfg->doa.prefix=0;
    // cfg->doa.type=0;
    // cfg->doa.size=0;
    // cfg->doa.id=0;
    // memset(cfg->doa.data,0,sizeof(cfg->doa.data)); 
    // cfg->doa.check=0;
    // cfg->doa.suffix=0;
    // cfg->doa.available=0;
    // cfg->doa.index=0;
    // memset(cfg->doa.buffer,0,sizeof(cfg->doa.buffer));

    return 0;
}
int qtk_uart_cfg_clean(qtk_uart_cfg_t *cfg)
{
    return 0;
}
int qtk_uart_cfg_update_local(qtk_uart_cfg_t *cfg, wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = main;
    wtk_local_cfg_update_cfg_string_v(lc, cfg, dev, v);
    wtk_local_cfg_update_cfg_string_v(lc, cfg, log_fn, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, baude, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, bits, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, parity, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, stop, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, c_flow, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, step_size, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_uart_log, v);

    wtk_debug("dev=%.*s\n",cfg->dev.len,cfg->dev.data);

    return 0;
}
int qtk_uart_cfg_update(qtk_uart_cfg_t *cfg)
{
    return 0;
}
