#include "sdk/dev/uart/qtk_uart.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"

typedef enum{
	TEST_UART_RECV_START,
	TEST_UART_RECV_TYPE,
	TEST_UART_RECV_DATA,
	TEST_UART_RECV_END,
}test_uart_type;
#define RECV_HEAD_LEN 9
char recv_buf[100];

static uint8_t crc8(const uint8_t *data, int len)
{
    unsigned crc = 0;
    int i, j;
    for (j = len; j; j--, data++)
    {
        crc ^= (*data << 8);
        for (i = 8; i; i--)
        {
            if (crc & 0x8000)
                crc ^= (0x1070 << 3);
            crc <<= 1;
        }
    }
    return (uint8_t)(crc >> 8);
}


int main(int argc, char **argv)
{
	wtk_arg_t *arg = NULL;
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_uart_cfg_t *cfg=NULL;
	qtk_uart_t *uart=NULL;
	wtk_strbuf_t *sendbuf=NULL;
	char *cfg_fn=NULL;
	char *in_fn=NULL;
	int ret,pos,len;
	
	arg  = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_str_s(arg, "c", &cfg_fn);
	if(ret != 0){
		wtk_debug("usage: ./a.out -c cfg -o out_fn\n");
		return -1;
	}
	ret = wtk_arg_get_str_s(arg, "i", &in_fn);
	if(ret != 0){
		wtk_debug("usage: ./a.out -c cfg -o out_fn\n");
		return -1;
	}
	main_cfg = wtk_main_cfg_new_type(qtk_uart_cfg, cfg_fn);
	if(!main_cfg){
		wtk_debug("cfg new failed.\n");
		goto end;
	}
	cfg = (qtk_uart_cfg_t *)main_cfg->cfg;	
	uart = qtk_uart_new(cfg);
	if(!uart){
		wtk_debug("uart new failed.\n");
		goto end;
	}
	sendbuf = wtk_strbuf_new(1024, 1.0);
	char *data=NULL;

	data = file_read_buf(in_fn, &len);
	pos=1024;
	char *ss=data;
	char *ee=data+len;
	char head[4]={0xEB,0x90,0xAA,0xAA};
	char end[4]={0xED,0x03,0xBF,0xBF};
	while(ss < ee)
	{
		wtk_strbuf_reset(sendbuf);
		wtk_strbuf_push(sendbuf, head, 4);
		short slen=pos;
		wtk_strbuf_push(sendbuf, (char *)&slen, 2);
		wtk_strbuf_push(sendbuf, ss, pos);
		short cc = crc8(ss, pos);
		wtk_strbuf_push(sendbuf, (char *)&cc, 2);
		wtk_strbuf_push(sendbuf, end, 4);
		qtk_uart_write(uart, sendbuf->data, sendbuf->pos);
		wtk_debug("send data len:%d\n", sendbuf->pos);
		usleep(1000*32);
		ss+=pos;
	}

end:
	if(sendbuf){
		wtk_strbuf_delete(sendbuf);
	}
	if(uart){
		qtk_uart_delete(uart);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}
	if(arg){
		wtk_arg_delete(arg);
	}
	return 0;
}
