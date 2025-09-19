#include "sdk/dev/uart/qtk_uart.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/bfio/compress/qtk_signal_decompress.h"
#include "sdk/codec/qtk_msg.h"
#include "wtk/core/wtk_wavfile.h"
#include "sdk/codec/qtk_audio_conversion.h"

typedef enum{
	TEST_UART_RECV_START,
	TEST_UART_RECV_TYPE,
	TEST_UART_RECV_DATA,
	TEST_UART_RECV_END,
}test_uart_type;
#define RECV_HEAD_LEN 1036
char recv_buf[2048];

qtk_signal_decompress_cfg_t *decompress_cfg=NULL;
qtk_signal_decompress_t *decompress=NULL;
wtk_strbuf_t *deoutbuf=NULL;
wtk_blockqueue_t decompress_queue;
wtk_blockqueue_t play_queue;
qtk_msg_t *testmsg=NULL;
char *out_fn="./tmp";
int is_decompress_run=0;
int is_play_run=0;

int uart_play_decompress_grep_str(char *str, char *data, int len)
{
	int pos=0;
	char *head=str;

	while(pos < len)
	{
		if(memcmp(data+pos, head, strlen(head)) == 0)
		{
			return pos;
		}
		pos++;
	}
	return -1;
}

int uart_play_player_run(void *data,wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	char pcmd[1024]={0};
	wtk_wavfile_t *wavfile=NULL;
	wavfile = wtk_wavfile_new(8000);
	char ofn[1024]={0};
	int ocnt=0;

	while(is_play_run){
		qn= wtk_blockqueue_pop(&play_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, 15.0);
		// file_write_buf(out_fn, msg_node->buf->data, msg_node->buf->pos);
		snprintf(ofn, sizeof(ofn), "%s_%d.wav", out_fn, ocnt++);
		wtk_wavfile_open(wavfile, ofn);
		wtk_wavfile_write(wavfile, msg_node->buf->data, msg_node->buf->pos);
		wtk_wavfile_close(wavfile);
		sprintf(pcmd, "aplay -Ddefault %s", ofn);
		system(pcmd);
		printf("player ending.\n");

		qtk_msg_push_node(testmsg, msg_node);
	}
	wtk_wavfile_delete(wavfile);
	return 0;
}

int uart_play_decompress_run(void *data,wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node, *msg_node2;
	wtk_queue_node_t *qn;
	wtk_strbuf_t *decompressbuf=wtk_strbuf_new(1024,0);
	wtk_strbuf_t *tmpbuf=wtk_strbuf_new(1024,0);
	int ret=-1,is_start=1,is_end=0;
	wtk_strbuf_reset(decompressbuf);
	wtk_strbuf_reset(tmpbuf);

	char *headstr="start";
	char *endstr="end";
	int head_len=strlen(headstr);
	int end_len=strlen(endstr);

	while(is_decompress_run){
		qn= wtk_blockqueue_pop(&decompress_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		wtk_strbuf_push(tmpbuf, msg_node->buf->data, msg_node->buf->pos);
		// wtk_debug("====================>>>>>>>>>>>>>>>>is start=%d is_end=%d tmpbuf->pos=%d\n", is_start, is_end, tmpbuf->pos);
		if(is_start)
		{
			ret = uart_play_decompress_grep_str(headstr, tmpbuf->data, tmpbuf->pos);
			if(ret >= 0)
			{
				// wtk_debug("decompressbuf->pos=%d tmpbuf->pos=%d\n", decompressbuf->pos, tmpbuf->pos);
				int start_pos=ret;
				ret = uart_play_decompress_grep_str(endstr, tmpbuf->data, tmpbuf->pos);
				// wtk_debug("==========>>>>>>>>>>>>>>>>is start=%d is_end=%d ret=%d start_pos=%d\n", is_start, is_end, ret, start_pos);
				if(ret >= 0 && ret > start_pos)
				{
					wtk_strbuf_push(decompressbuf, tmpbuf->data+start_pos+head_len, ret-head_len-start_pos);
					wtk_strbuf_pop(tmpbuf, NULL, ret+end_len);
				}else{
					wtk_strbuf_push(decompressbuf, tmpbuf->data+start_pos+head_len, tmpbuf->pos-start_pos-head_len-end_len);
					wtk_strbuf_pop(tmpbuf, NULL, tmpbuf->pos-end_len);
				}
				is_start = 0;
				is_end=0;
			}else{
				wtk_strbuf_pop(tmpbuf, NULL, tmpbuf->pos - head_len);
			}
		}else{
			ret = uart_play_decompress_grep_str(endstr, tmpbuf->data, tmpbuf->pos);
			if(ret >= 0)
			{
				// wtk_debug("-------------isend=%d endlen=%d\n",ret,end_len);
				if(ret > 0)
				{
					wtk_strbuf_push(decompressbuf, tmpbuf->data, ret);
				}
				wtk_strbuf_pop(tmpbuf, NULL, ret+end_len);
				is_end=1;
				is_start = 1;
			}else{
				// wtk_debug("decompressbuf->pos=%d tmpbuf->pos=%d\n", decompressbuf->pos, tmpbuf->pos);
				wtk_strbuf_push(decompressbuf, tmpbuf->data, tmpbuf->pos - end_len);
				wtk_strbuf_pop(tmpbuf, NULL, tmpbuf->pos - end_len);
			}
		}
		if(is_end)
		{
			//TODO push data to decompress engine
			int64_t codec_len[4]={0};
			int64_t *res;
			memcpy(codec_len, decompressbuf->data, sizeof(int64_t)*4);
			// wtk_debug("=================>>>>>>>>>>>>>>>>>>>>>len=%d  %d/%d/%d/%d\n", decompressbuf->pos, codec_len[0], codec_len[1], codec_len[2], codec_len[3]);
			// int i,j;
			// int adatalen=0;
			// res = (int64_t *)(decompressbuf->data+sizeof(int64_t)*4);
			// for(i=0;i<4;++i)
			// {
			// 	printf("data[%d]={",i);
			// 	for(j=0;j<codec_len[i];++j)
			// 	{
			// 		printf("%d ",res[adatalen+j]);
			// 	}
			// 	printf("}\n");
			// 	adatalen+=codec_len[i];
			// }
			printf("================ Decompressing start =================\n");
			qtk_signal_decompress_feed(decompress, (int64_t *)(decompressbuf->data+sizeof(int64_t)*4), codec_len);
			qtk_signal_decompress_get_result(decompress);
			printf("================ Decompressing end.  =================\n");
			
			// wtk_debug("========================>>>deoutbuf->pos=%d\n", deoutbuf->pos);
			if(deoutbuf->pos > 0)
			{
				msg_node2 = qtk_msg_pop_node(testmsg);
				wtk_strbuf_push(msg_node2->buf, deoutbuf->data, deoutbuf->pos);
				wtk_blockqueue_push(&play_queue, &msg_node2->qn);
			}

			wtk_strbuf_reset(deoutbuf);
			qtk_signal_decompress_reset(decompress);
			wtk_strbuf_reset(decompressbuf);
			is_end = 0;
		}

		qtk_msg_push_node(testmsg, msg_node);
	}
	if(decompressbuf)
	{
		wtk_strbuf_delete(decompressbuf);
	}
	if(tmpbuf)
	{
		wtk_strbuf_delete(tmpbuf);
	}
	return 0;
}

void uart_play_decompress_on_data(wtk_strbuf_t *buf, short *output, int len)
{
	// wtk_debug("==========#############=======>%d\n", len);
	wtk_strbuf_push(buf, (char *)output, len<<1);
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
	wtk_thread_t thread_decompress;
	wtk_thread_t thread_play;
	char *cfg_fn=NULL;
	char *decfg_fn=NULL;
	int ret,pos,alen;

	arg  = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_str_s(arg, "c", &cfg_fn);
	if(ret != 0){
		wtk_debug("usage: ./a.out -c cfg -dc decompress.bin\n");
		return -1;
	}
	ret = wtk_arg_get_str_s(arg, "dc", &decfg_fn);
	if(ret != 0){
		wtk_debug("usage: ./a.out -c cfg -dc decompress.bin\n");
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
	deoutbuf = wtk_strbuf_new(1024, 1.0);

	decompress_cfg = qtk_signal_decompress_cfg_new_bin(decfg_fn);
	if(!decompress_cfg){
		wtk_debug("decompress_cfg new failed.\n");
		goto end;
	}
	decompress = qtk_signal_decompress_new(decompress_cfg);
	if(!decompress){
		wtk_debug("decompress new failed.\n");
		goto end;
	}
	qtk_signal_decompress_set_notify(decompress, deoutbuf, (qtk_signal_decompress_notify_f)uart_play_decompress_on_data);
	testmsg = qtk_msg_new();
	wtk_blockqueue_init(&decompress_queue);
	wtk_blockqueue_init(&play_queue);
	wtk_thread_init(&thread_decompress, (thread_route_handler)uart_play_decompress_run, decompress);
	is_decompress_run=1;
	wtk_thread_start(&thread_decompress);
	wtk_thread_init(&thread_play, (thread_route_handler)uart_play_player_run, decompress);
	is_play_run = 1;
	wtk_thread_start(&thread_play);

	outbuf=wtk_strbuf_new(1024, 1.0);
	databuf=wtk_strbuf_new(1024, 1.0);
	wtk_strbuf_reset(databuf);
	test_uart_type type=TEST_UART_RECV_START;
	int datalen=0;
	qtk_msg_node_t *msg_node;

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
						// wtk_debug("databuf->pos=%d outbuf->pos=%d alen=%d pos=%d datalen=%d\n", databuf->pos, outbuf->pos, alen, pos, datalen);
						wtk_strbuf_push(databuf, outbuf->data+alen+6, datalen);
						wtk_strbuf_pop(outbuf, NULL, alen+datalen+6+6);
						datalen = 0;
						type=TEST_UART_RECV_END;
					}else{
						// wtk_debug("databuf->pos=%d outbuf->pos=%d alen=%d datalen=%d\n", databuf->pos, outbuf->pos, alen, datalen);
						wtk_strbuf_push(databuf, outbuf->data+alen+6, outbuf->pos-alen-6);
						datalen = datalen - (outbuf->pos-alen-6);
						wtk_strbuf_pop(outbuf, NULL, outbuf->pos);
						type=TEST_UART_RECV_DATA;
					}
				}else{
					// wtk_debug("============================>>>>>>pos=%d alen=%d\n", pos, alen);
					wtk_strbuf_pop(outbuf, NULL, alen-4);
				}
				break;
			case TEST_UART_RECV_DATA:
				pos = grep_end(outbuf->data, outbuf->pos, &alen);
				if(pos >= 0)
				{
					// wtk_debug("end=[");
					// int i;
					// for(i=0; i<outbuf->pos; ++i)
					// {
					// 	printf("%02X ", outbuf->data[i]);
					// }
					// printf("]\n");
					// wtk_debug("==================>>>>>>>>>>>>pos=%d outbuf->pos=%d databuf->pos=%d datalen=%d alen=%d\n", pos, outbuf->pos, databuf->pos, datalen, alen);
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
					// wtk_debug("==================>>>>>>>>>>>>databuf->pos=%d outbuf->pos=%d datalen=%d\n",databuf->pos ,outbuf->pos, datalen);
				}
				break;
			default:
				break;
			}
			if(type == TEST_UART_RECV_END)
			{
				wtk_debug("recv data len=%d outbuf->pos=%d\n", databuf->pos, outbuf->pos);
				// fwrite(databuf->data, 1, databuf->pos, ofp);
				// fflush(ofp);
				msg_node = qtk_msg_pop_node(testmsg);
				wtk_strbuf_push(msg_node->buf, databuf->data, databuf->pos);
				wtk_blockqueue_push(&decompress_queue, &msg_node->qn);
				wtk_strbuf_reset(databuf);
				type=TEST_UART_RECV_START;
			}
		}
	}

end:
	is_decompress_run=0;
	wtk_blockqueue_wake(&decompress_queue);
	wtk_thread_join(&thread_decompress);
	wtk_blockqueue_clean(&decompress_queue);
	
	is_play_run=0;
	wtk_blockqueue_wake(&play_queue);
	wtk_thread_join(&thread_play);
	wtk_blockqueue_clean(&play_queue);

	qtk_msg_delete(testmsg);
	if(outbuf)
	{
		wtk_strbuf_delete(outbuf);
	}
	if(databuf)
	{
		wtk_strbuf_delete(databuf);
	}
	if(deoutbuf)
	{
		wtk_strbuf_delete(deoutbuf);
	}
	if(uart){
		qtk_uart_delete(uart);
	}
	if(main_cfg){
		wtk_main_cfg_delete(main_cfg);
	}
	if(decompress){
		qtk_signal_decompress_delete(decompress);
	}
	if(decompress_cfg){
		qtk_signal_decompress_cfg_delete(decompress_cfg);
	}
	if(arg){
		wtk_arg_delete(arg);
	}
	return 0;
}
