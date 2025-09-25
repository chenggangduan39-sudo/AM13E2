#ifndef __QTK_UART_OTA_CFG_H__
#define __QTK_UART_OTA_CFG_H__
#include "sdk/dev/uart/qtk_uart_cfg.h"

#ifdef _cplusplus
extern "C"{
#endif

typedef struct qtk_uart_ota_cfg qtk_uart_ota_cfg_t;

struct qtk_uart_ota_cfg{
	qtk_uart_cfg_t uart;
};

int qtk_uart_ota_cfg_init(qtk_uart_ota_cfg_t *cfg);
int qtk_uart_ota_cfg_clean(qtk_uart_ota_cfg_t *cfg);
int qtk_uart_ota_cfg_update_local(qtk_uart_ota_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_uart_ota_cfg_update(qtk_uart_ota_cfg_t *cfg);

#ifdef _cplusplus
}
#endif
#endif
