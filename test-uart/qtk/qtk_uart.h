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
#include "core/qtk_type.h"


#ifdef __cplusplus
extern "C"{
#endif

#ifndef CBAUD
#define CBAUD	0010017
#endif
#ifndef CBAUDEX
#define CBAUDEX 0010000
#endif
#define CRTSCTS  020000000000

typedef struct qtk_uart qtk_uart_t;
struct qtk_uart{
	int fd;
    /*qtk_uart_cfg*/
    char *dev;
    int baude;
    int c_flow;
    int bits;
    char parity;
    int stop;
};
qtk_uart_t *qtk_uart_new(char *device,int baude,int bits,int parity,int stop,int flow);
int qtk_uart_delete(qtk_uart_t *s);
int qtk_uart_set_config(qtk_uart_t * s, int baude,int c_flow,int bits,char parity,int stop);
int qtk_uart_read(qtk_uart_t *s, char *buf, int len);
int qtk_uart_write(qtk_uart_t *s, const char *data, int len);
int qtk_uart_write2(qtk_uart_t *s, const char *data, int len);


#ifdef __cplusplus
}
#endif
#endif
