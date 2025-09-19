#ifndef __QTK_UART_CFG_H__
#define __QTK_UART_CFG_H__
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/os/wtk_log.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_uart_cfg qtk_uart_cfg_t;
// typedef struct qtk_doa qtk_doa_t;

struct qtk_uart_cfg{
    wtk_log_t *uart_log;
    wtk_string_t dev;
    wtk_string_t log_fn;
    int baude;
    int c_flow;
    int bits;
    char parity;
    int stop;
    int step_size;
    unsigned use_uart_log:1;
};

// struct qtk_doa
// {
// 	int8_t stage;
//     uint8_t prefix; //头
//     uint8_t type; //类型
//     uint8_t size; //长度
//     uint8_t id; //id
//     uint8_t data[256]; //data
// 	uint8_t check; //校验
//     uint8_t suffix; //尾
//     int available;
//     int index;
//     uint8_t buffer[256 + 8];
// };

int qtk_uart_cfg_init(qtk_uart_cfg_t *cfg);
int qtk_uart_cfg_clean(qtk_uart_cfg_t *cfg);
int qtk_uart_cfg_update_local(qtk_uart_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_uart_cfg_update(qtk_uart_cfg_t *cfg);

#ifdef __cplusplus
}
#endif
#endif