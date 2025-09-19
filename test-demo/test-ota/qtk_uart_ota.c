#include "qtk_uart_ota.h"

int qtk_uart_ota_read_entry(qtk_uart_ota_t *uc, wtk_thread_t *thread);
int qtk_uart_ota_process_entry(qtk_uart_ota_t *uc, wtk_thread_t *thread);
int md5sum_check(qtk_uart_ota_t *ota);
int ota_upgrade(qtk_uart_ota_t *ota);
int do_send_data(qtk_uart_ota_t *ota, ota_send_type_t type, char *data, int len);

qtk_uart_ota_t *qtk_uart_ota_new(qtk_uart_ota_cfg_t *cfg)
{
	qtk_uart_ota_t *ota;
	int ret;

	ota = (qtk_uart_ota_t *)wtk_calloc(1, sizeof(qtk_uart_ota_t));
	if (!ota) {
		wtk_debug("malloc failed.\n");
		return NULL;
	}
	ota->cfg = cfg;

	ota->uart = qtk_uart_new(&ota->cfg->uart);
	if (!ota->uart) {
		wtk_debug("uart new failed.\n");
		ret = -1;
		goto end;
	}
	ota->buf = wtk_strbuf_new(1024, 0);
	ota->md5 = wtk_strbuf_new(1024, 0);
	ota->msg = qtk_msg_new();
	wtk_blockqueue_init(&ota->queue);


    wtk_thread_init(&ota->read_thread,(thread_route_handler)qtk_uart_ota_read_entry, ota);
	wtk_thread_set_name(&ota->read_thread, "uart_read");

    wtk_thread_init(&ota->process_thread,(thread_route_handler)qtk_uart_ota_process_entry, ota);
	wtk_thread_set_name(&ota->process_thread, "uart_data_process");

	ret = 0;
end:
	if (ret != 0) {
		qtk_uart_ota_delete(ota);
		ota = NULL;
	}
	return ota;
}

void qtk_uart_ota_delete(qtk_uart_ota_t *ota)
{
	if (ota->msg) {
		qtk_msg_delete(ota->msg);
	}
	if (ota->buf) {
		wtk_strbuf_delete(ota->buf);
	}
	if (ota->md5) {
		wtk_strbuf_delete(ota->md5);
	}
	if (ota->uart) {
		qtk_uart_delete(ota->uart);
	}
	wtk_free(ota);
}

int qtk_uart_ota_start(qtk_uart_ota_t *ota)
{
	if (ota->start) {
		return 0;
	}
	ota->process_thread_run = 1;
	wtk_thread_start(&ota->process_thread);
	ota->read_thread_run = 1;
	wtk_thread_start(&ota->read_thread);
	ota->start = 1;
	return 0;
}

int qtk_uart_ota_stop(qtk_uart_ota_t *ota)
{
	if (ota->start == 0) {
		return 0;
	}
	ota->read_thread_run = 0;
	wtk_thread_join(&ota->read_thread);
	ota->process_thread_run = 0;
	wtk_blockqueue_wake(&ota->queue);
	wtk_thread_join(&ota->process_thread);
	ota->start = 0;
	return 0;
}

#define HEAD_LEN (9)
typedef enum {
	READ_DATA_HEAD, // "start"(5字节) + len(4字节)
	READ_DATA_DATA,
}read_data_type_t;
int packageLen;


int qtk_uart_data_parse(qtk_uart_ota_t *ota, char *buf, int bytes)
{
	static read_data_type_t data_type = READ_DATA_HEAD;
	int consumeLen;
	int len;
	char *data;
	qtk_msg_node_t *node;
	
	data = buf;
	len = bytes;
	// wtk_debug("++++++++++++++++++\n");
	while (len > 0) {
		switch (data_type) {
			case READ_DATA_HEAD:
				consumeLen  = min(HEAD_LEN - ota->buf->pos, len);
				if(consumeLen == 0) {
					exit(0);
				}
				wtk_strbuf_push(ota->buf, data, consumeLen);
				len -= consumeLen;
				data += consumeLen;
				if (ota->buf->pos == HEAD_LEN) {
					// wtk_debug(">>>>>>>>>>%s\n", ota->buf->data);
					if (strncmp(ota->buf->data, "start", 5) == 0) {
						packageLen = *(int *)(ota->buf->data + 5);
						// wtk_debug("packageLen = %d\n", packageLen);
						wtk_strbuf_reset(ota->buf);
						data_type = READ_DATA_DATA;
					} else {
						wtk_strbuf_reset(ota->buf);
						len += (consumeLen - 1);
						data -= (consumeLen - 1);
					}
				}
				break;
			case READ_DATA_DATA:
				consumeLen = min(packageLen - ota->buf->pos, len);
				// wtk_debug("consumeLen = %d\n", consumeLen);
				wtk_strbuf_push(ota->buf, data, consumeLen);
				len -= consumeLen;
				data += consumeLen;
				if (ota->buf->pos == packageLen) {
					// wtk_debug(">>>>>recv package over\n");
					node = qtk_msg_pop_node(ota->msg);
					if (node) {
						wtk_strbuf_push(node->buf, ota->buf->data, ota->buf->pos);
						wtk_blockqueue_push(&ota->queue, &node->qn);
					}
					wtk_strbuf_reset(ota->buf);
					data_type = READ_DATA_HEAD;
				}
				break;
		}
	}

	return 0;
}


int qtk_uart_ota_read_entry(qtk_uart_ota_t *ota, wtk_thread_t *thread)
{
	qtk_msg_node_t *node;
	char read_buf[1024];
	int ret;

	while (ota->read_thread_run) {
		ret = qtk_uart_read(ota->uart, read_buf, sizeof(read_buf));
		if (ret > 0) {
			qtk_uart_data_parse(ota, read_buf, ret);
		}
	}
}


int qtk_uart_ota_process_entry(qtk_uart_ota_t *ota, wtk_thread_t *thread)
{
	wtk_queue_node_t *qn;
	qtk_msg_node_t *node;
	msg_type_t msg_type;
	int package_recv_len = 0;
	FILE *f = 0;


	while (ota->process_thread_run) {
		qn = wtk_blockqueue_pop(&ota->queue, -1, NULL);
		if (!qn) {
			break;
		}
		node = data_offset2(qn, qtk_msg_node_t, qn);
		msg_type = node->buf->data[0];
		// wtk_debug("======= msg_type = %d\n", msg_type);
		switch (msg_type) {
			case MSG_TYPE_GET_VERSION:
				wtk_debug("get version\n");
				if(access("/oem/qdreamer/qsound/version.txt",F_OK) == 0) {
					int len;
					char*data=file_read_buf("/oem/qdreamer/qsound/version.txt",&len);
					do_send_data(ota, OTA_SEND_VERSION, data, len);
				}
			
				break;
			case MSG_TYPE_OTA_DATA_START:
				wtk_debug("ota data start, md5sum: %.*s\n", node->buf->pos - 1, node->buf->data+1);
				package_recv_len = 0;
				// kill Qsound_3308
				system("/oem/qdreamer/qsound/kill_sound.sh");
				wtk_strbuf_reset(ota->md5);
				wtk_strbuf_push(ota->md5, node->buf->data+1, node->buf->pos);
				f = fopen("/tmp/3308_ota_packge.zip", "wb+");
				if (!f) {
					wtk_debug("open failed.\n");
				}

				break;
			case MSG_TYPE_OTA_DATA_DATA:
				// wtk_debug("ota data data\n");
				package_recv_len += node->buf->pos - 1;
				if (f) {
					fwrite(node->buf->data + 1, node->buf->pos -1, 1, f);
					fflush(f);
				}
				break;
			case MSG_TYPE_OTA_DATA_END:
				wtk_debug("ota data end  package_len = %d\n", package_recv_len);
				if (f) {
					fclose(f);
					f = NULL;
				}
				if (md5sum_check(ota) == 0) {
					ota_upgrade(ota);
				}
				break;
		}

		qtk_msg_push_node(ota->msg, node);
	}
	return 0;
}

int md5sum_check(qtk_uart_ota_t *ota)
{
	char buffer[33] = {0};
	int rlt;

	wtk_debug(">>>>>>>>>>>>md5sum check\n");
	FILE *fp=popen("md5sum /tmp/3308_ota_packge.zip","r");
	memset(buffer,0,sizeof(buffer));
	if (fgets(buffer, sizeof(buffer), fp) != NULL)
	{
	    printf("buffer:%s\n",buffer,strlen(buffer));
		rlt = strncmp(ota->md5->data, buffer, 32);
	}
	pclose(fp);
	if (rlt == 0) {
		wtk_debug("md5sum check ok\n");
		return 0;
	} else {
		wtk_debug("md5sum check failed.\n");
		rlt = -1;
		do_send_data(ota, OTA_SEND_OTA_RLT, (char *)&rlt, 1);
		return -1;
	}
}

int ota_upgrade(qtk_uart_ota_t *ota)
{
	int rlt;
	wtk_debug(">>>>>>>>>>>>>ota upgrade\n");
	system("cp /oem/qdreamer/qsound/update.sh /tmp/");
	system("cp /oem/qdreamer/qsound/ab_update.sh /tmp/");
	system("cp /oem/RkLunch.sh /tmp/");
	system("chmod 777 /tmp/update.sh");
	system("chmod 777 /tmp/ab_update.sh");
	system("sync");
	system("/tmp/update.sh");
	wtk_debug("xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx \n");
	system("sync");
	int ulen;
	char *upresult=file_read_buf("/tmp/upgrade.log",&ulen);
			
	wtk_debug("============>>>>>>>>>>>read ota result!\n");
	if(strstr(upresult,"check /dev/block/by-name/boot_b ok") && strstr(upresult,"check /dev/block/by-name/system_b ok"))
	{
		rlt = 0;
		wtk_debug("==>>rk ota success boot rootfs b partition.\n")
			//system("reboot");
	}else if(strstr(upresult,"check /dev/block/by-name/boot_a ok") && strstr(upresult,"check /dev/block/by-name/system_a ok"))
	{
		wtk_debug("12222222222222222222222222222\n");
		rlt = 0;
		wtk_debug("==>>rk ota success boot rootfs a partition.\n")
			//system("reboot");
	} else {
		rlt = -1;
	}
	wtk_free(upresult);
	do_send_data(ota, OTA_SEND_OTA_RLT, (char *)&rlt, 1);
	if (rlt == 0 ){
		wtk_msleep(100);
		system("reboot");
	}
}

int do_send_data(qtk_uart_ota_t *ota, ota_send_type_t type, char *data, int len)
{
	int ret;
	wtk_strbuf_t *buf = wtk_strbuf_new(1024, 0);
	wtk_strbuf_push(buf, (char *)&type, 1);
	if (len > 0) {
		qtk_strbuf_push(buf, data, len);
	}
	ret = qtk_uart_write2(ota->uart, buf->data, buf->pos);
	wtk_strbuf_delete(buf);
	if (ret == buf->pos) {
		return 0;
	} else {
		return -1;
	}
}
