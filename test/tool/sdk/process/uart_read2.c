#include "sdk/dev/uart/qtk_uart.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"

typedef enum{
	TEST_UART_RECV_START,
	TEST_UART_RECV_TYPE,
	TEST_UART_RECV_DATA,
	TEST_UART_RECV_END,
}test_uart_type;
#define RECV_HEAD_LEN 1036
char recv_buf[2048];

uint32_t merge_high_low_bytes(uint16_t high, uint16_t low) {
    return (high << 8) | low;
}

int grep_start(char *data, int len, int *alen, char *out)
{
	int pos=0;
	unsigned char start=0xff;
	unsigned char end=0x01;
	while(pos < len)
	{
		if((unsigned char)(data[pos]) == start)
		{
			if(len-pos >= 9)
			{
				if((unsigned char)(data[pos+8]) == end)
				{
					memcpy(out, data+pos, 9);
					return pos;
				}
			}else{
				*alen=pos;
				return -1;
			}
		}
		pos++;
	}
	*alen=pos;
	return -1;
}

int grep_start2(char *data, int len, int *alen)
{
	int pos=0;
	char head[4]={0xEB,0x90,0xAA,0xAA};
	while(pos < len)
	{
		if(memcmp(data+pos, head, 4) == 0)
		{
			short dlen;
			memcpy(&dlen, data+pos+4, 2);
			*alen=pos;
			return dlen;
		}
		pos++;
	}
	*alen=pos;
	return -1;
}

int grep_end(char *data, int len, int *alen)
{
	int pos=0;
	char end[4]={0xED,0x03,0xBF,0xBF};
	while(pos < len)
	{
		if(memcmp(data+pos, end, 4) == 0)
		{
			*alen=pos-2;
			return pos-2;
		}
		pos++;
	}
	*alen=pos;
	return -1;
}

int main(int argc, char **argv)
{
	wtk_arg_t *arg = NULL;
	wtk_main_cfg_t *main_cfg = NULL;
	qtk_uart_cfg_t *cfg=NULL;
	qtk_uart_t *uart=NULL;
	wtk_strbuf_t *outbuf=NULL;
	wtk_strbuf_t *databuf=NULL;
	FILE *ofp=NULL;
	char *cfg_fn=NULL;
	char *out_fn=NULL;
	char *outdata=(char *)wtk_malloc(32);
	int ret,pos,alen;
	
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
	databuf=wtk_strbuf_new(1024, 1.0);
	wtk_strbuf_reset(databuf);
	memset(outdata, 0, 32);
	ofp=fopen(out_fn, "wb+");
	test_uart_type type=TEST_UART_RECV_START;
	int datalen=0;

	while(1)
	{
		memset(recv_buf, 0, sizeof(recv_buf));
		ret = qtk_uart_read(uart, recv_buf, RECV_HEAD_LEN);
		if(ret > 0)
		{
			wtk_strbuf_push(outbuf, recv_buf, ret);
			switch (type)
			{
			case TEST_UART_RECV_START:
				pos = grep_start2(outbuf->data, outbuf->pos, &alen);
				if(pos >= 0)
				{
					datalen = pos;
					if(outbuf->pos >= datalen+alen+6+6)
					{
						wtk_debug("databuf->pos=%d outbuf->pos=%d alen=%d pos=%d datalen=%d\n", databuf->pos, outbuf->pos, alen, pos, datalen);
						wtk_strbuf_push(databuf, outbuf->data+alen+6, datalen);
						wtk_strbuf_pop(outbuf, NULL, alen+datalen+6+6);
						datalen = 0;
						type=TEST_UART_RECV_END;
					}else{
						wtk_debug("databuf->pos=%d outbuf->pos=%d alen=%d datalen=%d\n", databuf->pos, outbuf->pos, alen, datalen);
						wtk_strbuf_push(databuf, outbuf->data+alen+6, outbuf->pos-alen-6);
						datalen = datalen - (outbuf->pos-alen-6);
						wtk_strbuf_pop(outbuf, NULL, outbuf->pos);
						type=TEST_UART_RECV_DATA;
					}
				}else{
					wtk_debug("============================>>>>>>pos=%d alen=%d\n", pos, alen);
					wtk_strbuf_pop(outbuf, NULL, alen-4);
				}
				break;
			case TEST_UART_RECV_DATA:
				pos = grep_end(outbuf->data, outbuf->pos, &alen);
				if(pos >= 0)
				{
					wtk_debug("end=[");
					int i;
					for(i=0; i<outbuf->pos; ++i)
					{
						printf("%02X ", outbuf->data[i]);
					}
					printf("]\n");
					wtk_debug("==================>>>>>>>>>>>>pos=%d outbuf->pos=%d databuf->pos=%d datalen=%d alen=%d\n", pos, outbuf->pos, databuf->pos, datalen, alen);
					if(alen > 2 && datalen > 0)
					{
						wtk_strbuf_push(databuf, outbuf->data, alen-2);
					}
					wtk_strbuf_pop(outbuf, NULL, alen+4);
					type=TEST_UART_RECV_END;
				}else{
					if(outbuf->pos >= datalen)
					{
						wtk_strbuf_push(databuf, outbuf->data, datalen);
						wtk_strbuf_pop(outbuf, NULL, datalen);
						datalen = 0;
					}else{
						wtk_strbuf_push(databuf, outbuf->data, outbuf->pos);
						datalen = datalen - outbuf->pos;
						wtk_strbuf_pop(outbuf, NULL, outbuf->pos);
					}
					wtk_debug("==================>>>>>>>>>>>>databuf->pos=%d outbuf->pos=%d datalen=%d\n",databuf->pos ,outbuf->pos, datalen);
				}
				break;
			default:
				break;
			}
			if(type == TEST_UART_RECV_END)
			{
				wtk_debug("recv data len=%d outbuf->pos=%d\n", databuf->pos, outbuf->pos);
				fwrite(databuf->data, 1, databuf->pos, ofp);
				fflush(ofp);
				wtk_strbuf_reset(databuf);
				type=TEST_UART_RECV_START;
			}
#if 0
			pos = grep_start(outbuf->data, outbuf->pos, &alen, outdata);
			if(pos >= 0)
			{
				int theta=-1;
				if(outdata[1] == 0x00)
				{
					theta = merge_high_low_bytes((unsigned char)(outdata[3]), (unsigned char)(outdata[2]));
					wtk_debug("=================>>>>>>>>>>theta=%d\n", theta);
				}else if(outdata[1] == 0x01)
				{
					theta = merge_high_low_bytes((unsigned char)(outdata[3]), (unsigned char)(outdata[2]));
					if(theta == 500)
					{
						wtk_debug("=================>>>>>>>>>>wakeup\n");
					}
				}
				wtk_strbuf_pop(outbuf, NULL, pos+9);
			}else
			{
				if(alen > 0)
				{
					wtk_strbuf_pop(outbuf, NULL, alen);
				}
			}
#endif
		}
	}

end:
	if(ofp)
	{
		fclose(ofp);
	}
	if(outdata)
	{
		wtk_free(outdata);
	}
	if(outbuf)
	{
		wtk_strbuf_delete(outbuf);
	}
	if(databuf)
	{
		wtk_strbuf_delete(databuf);
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
