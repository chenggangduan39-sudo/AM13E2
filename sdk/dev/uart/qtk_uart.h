#ifndef __QTK_UART_H__
#define __QTK_UART_H__
#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<unistd.h>
#include<assert.h>
#include<termios.h>
#include<string.h>
#include<sys/time.h>
#include<sys/types.h>
// #include<sys/io.h>
#include<errno.h>
#include <signal.h>
#include "wtk/os/wtk_thread.h"
#include "qtk_uart_cfg.h"
#ifdef __cplusplus
extern "C"{
#endif
#ifndef CBAUD
#define CBAUD	0010017
#endif
#ifndef CBAUDEX
#define CBAUDEX 0010000
#endif
#ifndef CRTSCTS
#define CRTSCTS  020000000000
#endif

#define PREFIX '#'
#define SUFFIX '\n'
#define DATA_TYPE '%'
#define COMMAND_TYPE ' '
#define RESPONSE_TYPE '.'
#define MM268_RX_TYPE 0xA0
#define MM268_TX_TYPE 0xAE

typedef struct qtk_uart qtk_uart_t;
typedef struct qtk_doa qtk_doa_t;

struct qtk_uart{
	qtk_uart_cfg_t *cfg;
	int fd;
	qtk_doa_t *doa; //DOA 
    unsigned use_log:1;
};

struct qtk_doa
{
	int8_t stage;
    uint8_t prefix; //头
    uint8_t type; //类型
    uint8_t size; //长度
    uint8_t id; //id
    uint8_t data[256]; //data
	uint8_t check; //校验
    uint8_t suffix; //尾
    int available;
    int index;
    uint8_t buffer[256 + 8];
};

qtk_uart_t *qtk_uart_new(qtk_uart_cfg_t *cfg);
int qtk_uart_delete(qtk_uart_t *s);
int qtk_uart_set_config(qtk_uart_t * s, int baude,int c_flow,int bits,char parity,int stop);
int qtk_uart_read(qtk_uart_t *s, char *buf, int len);
int qtk_uart_write(qtk_uart_t *s, const char *data, int len);
int qtk_uart_write2(qtk_uart_t *s, const char *data, int len);

int qtk_uart_doa_updata(qtk_uart_t *uart);
#ifdef __cplusplus
}
#endif
#endif
