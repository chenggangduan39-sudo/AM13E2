#include "qtk_uart_client_cfg.h"

int qtk_uart_client_cfg_init(qtk_uart_client_cfg_t *cfg)
{
	qtk_uart_cfg_init(&(cfg->uart));
	cfg->maincfg = NULL;
	cfg->use_uart = 0;
	cfg->log_debug = 1;
	cfg->dev_no = 0;
	cfg->pingsuan_run = 0;
	return 0;
}
int qtk_uart_client_cfg_clean(qtk_uart_client_cfg_t *cfg)
{
	qtk_uart_cfg_clean(&(cfg->uart));

	return 0;
}
int qtk_uart_client_cfg_update_local(qtk_uart_client_cfg_t *cfg, wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc = wtk_local_cfg_find_lc_s(main, "uart");
	if(lc){
		qtk_uart_cfg_update_local(&(cfg->uart), lc);
	}
	wtk_local_cfg_update_cfg_b(main, cfg, use_uart, v);
	wtk_local_cfg_update_cfg_b(main, cfg, pingsuan_run, v);

	wtk_local_cfg_update_cfg_b(main,cfg, log_debug,v);
	wtk_local_cfg_update_cfg_i(main,cfg, dev_no,v);

	return 0;
}

int qtk_uart_client_cfg_update(qtk_uart_client_cfg_t *cfg)
{

	qtk_uart_cfg_update(&(cfg->uart));

	return 0;
}

qtk_uart_client_cfg_t *qtk_uart_client_cfg_new(char *fn)
{
	qtk_uart_client_cfg_t *uartcfg;
	wtk_main_cfg_t *maincfg;

	maincfg = wtk_main_cfg_new_type(qtk_uart_client_cfg, fn);
	uartcfg = (qtk_uart_client_cfg_t *)(maincfg->cfg);
	uartcfg->maincfg = maincfg;
	return uartcfg;
}

void qtk_uart_client_cfg_delete(qtk_uart_client_cfg_t *uart)
{
	if(uart->maincfg){
		wtk_main_cfg_delete(uart->maincfg);
	}
}