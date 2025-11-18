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

int grep_start(char *data, int len, int *alen)
{
	
	if(len >= 4)
	{
		memcpy(alen, data, sizeof(int));
		wtk_debug("read data length, alen=%d\n", *alen);
		return 0; 
	}
	return -1;
}

int grep_end(char *data, int len)
{
	int pos=0;
	char *endstr="end";
	while(pos < len-strlen(endstr))
	{
		if(strncmp(data+pos, endstr, strlen(endstr)) == 0)
		{
			return pos;
		}
		pos++;
	}
	return -1;
}

int main(int argc, char **argv)
{
	wtk_arg_t *arg = NULL;
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_uart_cfg_t *cfg=NULL;
	qtk_uart_t *uart=NULL;
	wtk_strbuf_t *outbuf=NULL;
	char *cfg_fn=NULL;
	char *out_fn=NULL;
	int ret,pos,alen,nx,errcnt=0;
	int posend=0;
	test_uart_type utype=TEST_UART_RECV_START;
	
	arg  = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_str_s(arg, "c", &cfg_fn);
	if(ret != 0){
		wtk_debug("usage: ./a.out -c cfg -o out_fn\n");
		return -1;
	}
	ret = wtk_arg_get_str_s(arg, "o", &out_fn);
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
	outbuf=wtk_strbuf_new(1024, 1.0);

	while(1)
	{
		memset(recv_buf, 0, sizeof(recv_buf));
		switch (utype)
		{
		case TEST_UART_RECV_START:
			ret = qtk_uart_read(uart, recv_buf, RECV_HEAD_LEN);
			wtk_debug("=====================>>>>>>>>>>[%.*s]\n",ret,recv_buf);
			if(ret > 0)
			{
				wtk_strbuf_push(outbuf, recv_buf, ret);
				pos = grep_start(outbuf->data, outbuf->pos, &alen);
				if(pos > 0)
				{
					pos=RECV_HEAD_LEN-(outbuf->pos-pos);
					wtk_strbuf_reset(outbuf);
					wtk_strbuf_push(outbuf, recv_buf+pos,RECV_HEAD_LEN-pos);
					posend=RECV_HEAD_LEN-pos;
					wtk_debug("======================>>>start alen=%d / %d/%d\n",alen,pos,ret);
					utype=TEST_UART_RECV_DATA;
				}else
				{
					if(outbuf->pos > 2*RECV_HEAD_LEN)
					{
						wtk_strbuf_pop(outbuf, NULL, outbuf->pos-(2*RECV_HEAD_LEN));
					}
				}
				errcnt=0;
			}else{
				errcnt++;
			}
			break;
		case TEST_UART_RECV_TYPE:
			break;
		case TEST_UART_RECV_DATA:
			nx=min(sizeof(recv_buf), alen-posend);
			ret = qtk_uart_read(uart, recv_buf, nx);
			if(ret > 0){
				posend+=ret;
				// wtk_debug("read:[%.*s]\n", ret, recv_buf);
				wtk_strbuf_push(outbuf, recv_buf, ret);
				// wtk_debug("=================>>>>>>>>>>%d\n",outbuf->pos);
				if(outbuf->pos == alen)
				{
					utype=TEST_UART_RECV_END;
				}
				errcnt=0;
			}else{
				errcnt++;
			}
			break;
		case TEST_UART_RECV_END:
			ret = qtk_uart_read(uart, recv_buf, strlen("end"));
			if(ret > 0)
			{
				wtk_debug("[%.*s]\n",ret,recv_buf);
				file_write_buf(out_fn, outbuf->data, outbuf->pos);
				utype=TEST_UART_RECV_START;
				wtk_strbuf_reset(outbuf);
				errcnt=0;
			}else{
				errcnt++;
			}
			break;		
		default:
			break;
		}
		if(errcnt > 1)
		{
			if(utype != TEST_UART_RECV_START)
			{
				wtk_strbuf_reset(outbuf);
				utype = TEST_UART_RECV_START;
			}
		}
	}

end:
	if(outbuf)
	{
		wtk_strbuf_delete(outbuf);
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
