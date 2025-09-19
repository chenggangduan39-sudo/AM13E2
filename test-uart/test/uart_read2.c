#include "qtk/qtk_uart.h"
#include "qtk/core/qtk_strbuf.h"

typedef enum{
	TEST_UART_RECV_START,
	TEST_UART_RECV_TYPE,
	TEST_UART_RECV_DATA,
	TEST_UART_RECV_END,
}test_uart_type;
#define RECV_HEAD_LEN 9
char recv_buf[100];

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

int main(int argc, char **argv)
{
	qtk_uart_t *uart=NULL;
	qtk_strbuf_t *outbuf=NULL;
	char *outdata=(char *)malloc(32);
	int ret,pos,alen;
        int baude=115200;
        char *dev;

        if(argc < 2)
        {
                printf("./a.out /dev/ttyACM0 115200\n");
                exit(0);
        }
        dev=argv[1];

        if(argc == 3)
        {
                baude=atoi(argv[2]);
        }

        printf("dev=%s baude=%d\n",dev,baude);

	uart = qtk_uart_new(dev, baude, 8, 0, 1, 0);
	if(!uart){
		printf("uart new failed.\n");
		goto end;
	}
	outbuf=qtk_strbuf_new(1024, 1.0);
	memset(outdata, 0, 32);

	while(1)
	{
		memset(recv_buf, 0, sizeof(recv_buf));
		ret = qtk_uart_read(uart, recv_buf, RECV_HEAD_LEN);
		if(ret > 0)
		{
			qtk_strbuf_push(outbuf, recv_buf, ret);
			pos = grep_start(outbuf->data, outbuf->pos, &alen, outdata);
			if(pos >= 0)
			{
				int theta=-1;
				if(outdata[1] == 0x00)
				{
					theta = merge_high_low_bytes((unsigned char)(outdata[3]), (unsigned char)(outdata[2]));
					printf("=================>>>>>>>>>>theta=%d\n", theta);
				}else if(outdata[1] == 0x01)
				{
					theta = merge_high_low_bytes((unsigned char)(outdata[3]), (unsigned char)(outdata[2]));
					if(theta == 500)
					{
						printf("=================>>>>>>>>>>wakeup\n");
					}
				}
				qtk_strbuf_pop(outbuf, NULL, pos+9);
			}else
			{
				if(alen > 0)
				{
					qtk_strbuf_pop(outbuf, NULL, alen);
				}
			}
		}
	}

end:
	if(outdata)
	{
		free(outdata);
	}
	if(outbuf)
	{
		qtk_strbuf_delete(outbuf);
	}
	if(uart){
		qtk_uart_delete(uart);
	}
	return 0;
}
