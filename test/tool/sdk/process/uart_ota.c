#include "test-demo/test-ota/qtk_uart_ota.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

int main(int argc, char **argv)
{
	wtk_main_cfg_t *main_cfg = NULL;
	wtk_arg_t *arg = NULL;
	qtk_uart_ota_cfg_t *cfg;
	qtk_uart_ota_t *ota = NULL;
	char *cfg_fn;
	int ret;

	arg = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_str_s(arg, "c", &cfg_fn);
	if(ret != 0){
		wtk_debug("cfg not set\n");
		goto end;
	}

	main_cfg = wtk_main_cfg_new_type(qtk_uart_ota_cfg, cfg_fn);
	if(!main_cfg){
		wtk_debug("main_cfg new failed.\n");
		goto end;
	}
	cfg = (qtk_uart_ota_cfg_t *)main_cfg->cfg;
	ota = qtk_uart_ota_new(cfg);
	if(!ota){
		wtk_debug("uc new failed.\n");
		goto end;
	}
	qtk_uart_ota_start(ota);
	while (1) {
		wtk_msleep(2000);
	}

end:
	if (arg) {
		wtk_arg_delete(arg);
	}
	return 0;
}
