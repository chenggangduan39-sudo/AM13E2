// #include "qtk_uart_client.h"

// static double tm_s = 0;
// static double tm_e = 0;
// #define USE_3308
// #define USE_LOG
// #ifdef USE_3308
// static int once=0;
// static int once2=1;
// static char buf[12]={0};
// static int result=0;
// static uint8_t result_data;
// static struct{
// 		key_t key;
//     	SHM *shmaddr;
//     	int shmid;
//     	pid_t pid_client;
// }cp;
// #endif
// #define USE_AM13E2 
// qtk_uart_recv_frame_t current_frame = {0};
// int qdreamer_audio_check_request;  				// 0-无请求, 1-扬声器检测, 2-麦克风检测
// int qdreamer_audio_check_result;   				//  0-正常, 1-静音, 2-爆音
// uint64_t qdreamer_audio_check_start_time;        // 检测开始时间戳
// int qdreamer_audio_check_running;      			// 检测状态标志
// static int qtk_uart_client_netlink(qtk_uart_client_t *uc);
// void qtk_uart_client_on_data(qtk_uart_client_t *uc, char *data, int bytes);
// void qtk_uart_client_on_enrcheck(qtk_uart_client_t *uc, float enr_thresh, float enr, int on_run);
// int qtk_uart_client_trsn_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
// int qtk_uart_client_recorder_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
// int qtk_uart_client_msg_run(qtk_uart_client_t *uc, wtk_thread_t *thread);

// static qtk_uart_client_msg_t* qtk_uart_client_msg_new(qtk_uart_client_t *uc);
// static int qtk_uart_client_msg_delete(qtk_uart_client_msg_t *msg);
// static qtk_uart_client_msg_t* qtk_uart_client_pop_msg(qtk_uart_client_t *uc);
// static void qtk_uart_client_push_msg(qtk_uart_client_t *uc,qtk_uart_client_msg_t *msg);
// static void qtk_uart_client_clean_q(qtk_uart_client_t *uc, wtk_blockqueue_t *queue);
// void qtk_uart_client_update_params(qtk_uart_client_t *uc, char *data, int len);
// int qtk_uart_client_uart_param(qtk_uart_client_t *uc, wtk_strbuf_t *buf);
// static void qtk_uart_client_feed_notice(qtk_uart_client_t *uc, int notice);
// static int qtk_uart_client_get_code_string(qtk_uart_client_t *uc, int statID, wtk_strbuf_t *buf);
// // void qtk_uart_client_on_uart(qtk_uart_client_t *uc, qtk_uart_type_t state, char *data, int len);
// void handle_uart_frame(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *frame);
// static void send_response(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *req_frame,uint8_t*data,uint16_t data_len);

// typedef void (*uart_callback_t)(qtk_uart_type_t type, void *userdata);

// void msleep(unsigned long mSec)
// {
// 	struct timeval tv;
	
// 	tv.tv_sec=0;
// 	tv.tv_usec=mSec;
// 	select(0, NULL, NULL, NULL, &tv);
// }

// #ifdef USE_3308
// void myfun(int sig, siginfo_t *info, void *ptr)
// {
// 	// if (once2)
// 	// {
// 	// 	once=0;
//     // 	cp.pid_client = cp.shmaddr->pid;
// 	// 	wtk_debug("====>>>>cp.pid_client=%d   getpid=%d\n",cp.pid_client,getpid());
// 	// 	once2=0;
// 	// }
// 	once=1;
// 	wtk_debug("=====>>>>>>>>>>>>>>>>>>>>>myfun=%d\n",sig);
//     return;
// }

// int qtk_proc_read(qtk_uart_client_t *uc)
// {
// 	double tm;
// 	// 读取程序
// 	// char buf[12]={0};
// 	wtk_debug("====>>>>qtk_proc_read start\n");
// 	tm = time_get_ms();
// 	while(1)
// 	{
// 		// sleep(1);
// 		msleep(100000);
// 		tm = time_get_ms() - tm;
// 		if(tm > 1500)
// 		{
// 			return -1;
// 		}
// 		if(once ==1)
// 			break;
// 	}
// 	once=0;
// 	wtk_debug("read:%s\n", cp.shmaddr->buf);
// 	result=atoi(cp.shmaddr->buf);
// 	wtk_debug("=====>>>> result=%d\n",result);
// 	return 0;
// }

// int qtk_proc_read2(qtk_uart_client_t *uc)
// {
// 	// 读取程序
// 	wtk_debug("====>>>>qtk_proc_read2 start\n");
// 	while(1)
// 	{
// 		sleep(1);
// 		if(once ==1)
// 			break;
// 	}
// 	once=0;
// 	wtk_debug("read:%s\n", cp.shmaddr->buf);
// 	memset(buf,0,sizeof(buf));
// 	memcpy(buf,cp.shmaddr->buf,strlen(cp.shmaddr->buf));
	
// 	wtk_debug("====>>>>buf=[%s]\n",buf);
// 	return 0;
// }


// int qtk_proc_write(qtk_uart_client_t *uc,char *data,int len)
// {
// 	// 写入程序
// 	wtk_debug("input>[%.*s]\n",len,data);
// 	memset(cp.shmaddr->buf,0,MAX);
// 	memcpy(cp.shmaddr->buf, data, len);
// 	// wtk_debug("input>[%.*s]\n",len,cp.shmaddr->buf);
// 	// kill(cp.pid_client, SIGUSR1);
// 	union sigval tmp;
// 	tmp.sival_int = 100;
//  // 给进程 pid，发送 SIGINT 信号，并把 tmp 传递过去
// 	sigqueue(cp.pid_client, SIGRTMIN, tmp);
// 	wtk_debug("====>>>>qtk_proc_write end once=%d\n",once);
// 	return 0;
// }
// int qtk_uart_client_get_arg(qtk_uart_client_t *uc, qtk_uart_type_t state) 
// {
// 	int s=-1;
//     if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0)
// 	{
// 		FILE *fn;
// 		fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r");
// 		char buf[1024];
// 		char buf2[32]={0};
// 		char *pv;
// 		int ret, val;
// 		float scale;

// 		wtk_debug("==================>>>>>>>%d\n",sizeof(buf));
// 		ret = fread(buf, 1, sizeof(buf), fn);

// 		wtk_debug("============>>>>>>ret=%d [%.*s]\n",ret,ret,buf);
// 		switch (state)
// 		{
// 			case QTK_UART_TYPE_RECV_GET_DENOISE_SWITCH:
// 			     pv = strstr(buf,"VBOX3_ANS=");
// 				 s=atoi(pv + 10);
// 				break;
// 			case QTK_UART_TYPE_RECV_GET_GAIN_CONTROL_SWITCH:
// 				pv = strstr(buf,"VBOX3_AGC=");
// 				s=atoi(pv + 10);
// 			break;
// 			case QTK_UART_TYPE_RECV_GET_ECHO_INTENSITY_SWITCH:
// 				pv = strstr(buf, "VBOX3_AEC=");
//                 if (pv) {
//                     s = atoi(pv + 10);
//                     if (s == 0) {
//                         // s = -1; 
//                     } 
//                     else {
//                         pv = strstr(buf, "vbox3_aec_level=");
//                         if (pv) {
//                             int level = atoi(pv + 16);
//                             if (level >= 1 && level <= 3) {
//                                 s = level;
//                             } else {
//                                 s = 0x00;
//                             }
//                         } else {
//                             s = 0x00;
//                         }
//                     }
//                 }
// 			break;
// 			case QTK_UART_TYPE_RECV_GET_MICROPHONE_VOLUME_VALUE:
// 				pv = strstr(buf, "mic_shift2=");
// 				wtk_debug("pv=%s\n",pv);
// 				scale = wtk_str_atof(pv + 11, ret-(pv-buf)-12);
				
// 				wtk_debug("mic_shift=%0.2f %f %f\n",scale,((scale-1.0)/(15.5-1.2))*100,roundf(((scale-1.0)/(15.5-1.2))*100));
// 				s=roundf(((scale-1.0)/(15.5-1.2))*100);
// 			break;
// 			case QTK_UART_TYPE_RECV_GET_SPEKER_VOLUME_VALUE:
// 			    system("cat /sys/bus/i2c/devices/3-0069/volume > woofvolume.txt");
// 			    system("cat /sys/bus/i2c/devices/3-0069/volume > tweetervolume.txt");
// 				if(access("/oem/qdreamer/qsound/tweetervolume.txt",F_OK) == 0)
// 				{
// 					pv=file_read_buf("/oem/qdreamer/qsound/tweetervolume.txt",&val);
// 					s=atoi(pv);
// 				}
// 			default:
// 				break;
// 		}
		
// 		fclose(fn);
// 		fn = NULL;
// 	}
// 	return s;
// }

// #endif

// qtk_uart_client_t *qtk_uart_client_new(qtk_uart_client_cfg_t *cfg, wtk_log_t *log)
// {
// 	qtk_uart_client_t *uc;
// 	char *data = NULL;
// 	int len;
// 	int ret;

// 	signal(SIGPIPE, SIG_IGN);
// 	uc = (qtk_uart_client_t *)wtk_malloc(sizeof(qtk_uart_client_t));
// 	memset(uc, 0, sizeof(qtk_uart_client_t));
// 	uc->cfg = cfg;
// 	uc->log = log;
// 	uc->version = wtk_strbuf_new(64, 1);
// 	uc->utype = QTK_UART_STATE_RECV_START;
// 	uc->parser = wtk_json_parser_new();
// 	uc->msg_run=0;
// 	uc->trsn_run=0;
// #ifdef USE_LOG
// 	uc->log = wtk_log_new("/data/qtk_uart_client.log");
// #endif
// 	wtk_blockqueue_init(&uc->input_q);
// 	wtk_blockqueue_init(&uc->msg_q);
// 	wtk_lockhoard_init(&uc->msg_hoard,offsetof(qtk_uart_client_msg_t,hoard_n),10,
// 			(wtk_new_handler_t)qtk_uart_client_msg_new,
// 			(wtk_delete_handler_t)qtk_uart_client_msg_delete,
// 			uc
// 			);
//     wtk_thread_init(&uc->trsn_thread,(thread_route_handler)qtk_uart_client_trsn_run,(void*)uc);
// 	wtk_thread_set_name(&uc->trsn_thread,"trsn");

// 	wtk_thread_init(&uc->msg_thread,(thread_route_handler)qtk_uart_client_msg_run,(void *)uc);
// 	wtk_thread_set_name(&uc->msg_thread, "msg");

// 	if(uc->cfg->use_uart){
// 		int i=0;
// 		while(i < 10)
// 		{
// 			uc->uart = qtk_uart_new(&(uc->cfg->uart));
// 			if(uc->uart){break;}
// 			if(!uc->uart){sleep(1);}
// 			i++;
// 		}
// 		if(!uc->uart){
// 			wtk_debug("upload new failed.\n");
// 			wtk_log_warn0(uc->log, "upload new failed.");
// 			ret = -1;
// 			goto end;
// 		}

// 		uc->uart_buf = wtk_strbuf_new(3200, 1);
// 		uc->uart_buf2 = wtk_strbuf_new(256, 1);
// 	}
// 	uc->mac = wtk_strbuf_new(256,0);

// #ifdef USE_3308
// 	///////////////////////
// 	cp.shmaddr=NULL;
// 	// 创建一个提取码
// #ifdef USE_KTC3308
//     if ((cp.key = ftok("/ktc/version.txt", 'z')) == -1)
// #else
// 	if ((cp.key = ftok("/oem/qdreamer/qsound/version.txt", 'z')) == -1)
// #endif
//     {
//         perror("ftok");
//         return -1;
//     }
//     printf("===>>>key %d\n",cp.key);
//     // 创建一个共享空间
//     if ((cp.shmid = shmget(cp.key, SIZE, IPC_CREAT | 0666)) == -1)
//     {
//         perror("shmget");
//         return -1;
//     }
//     printf("===>>>shmid %d\n",cp.shmid);
//     // 设置共享空间地址
//     if ((cp.shmaddr = shmat(cp.shmid, NULL, 0)) == (SHM *)-1)
//     {
//         perror("shmat");
//         return -1;
//     }

//     // 注册信号
// 	cp.pid_client = cp.shmaddr->pid;
//     cp.shmaddr->pid = getpid();
// 	wtk_debug("====>>>>cp.pid_client=%d   getpid=%d\n",cp.pid_client,getpid());
// 	// signal(SIGUSR2, myfun);
// 	// kill(cp.pid_client,SIGUSR2);
// 	struct sigaction act, oact;
// 	act.sa_sigaction = myfun; //指定信号处理回调函数
// 	sigemptyset(&act.sa_mask); // 阻塞集为空
// 	act.sa_flags = SA_SIGINFO; // 指定调用 signal_handler
//  // 注册信号 SIGINT
// 	sigaction(SIGRTMIN+2, &act, &oact);

// 	union sigval tmp;
// 	tmp.sival_int = 100;
//  // 给进程 pid，发送 SIGINT 信号，并把 tmp 传递过去
// 	sigqueue(cp.pid_client, SIGRTMIN+2, tmp);
// #endif

// #ifdef USE_AM34
// // 生成唯一的key
//     uc->key = ftok("/oem/uhan/qsound/is_sound.txt", 65);

//     // 创建消息队列
//     uc->msg_id = msgget(uc->key, 0666 | IPC_CREAT);
// 	if (uc->msg_id == -1)// 错误处理：msgget调用成功返回消息队列标识符，调用失败返回-1
// 	{
//         wtk_debug("msgget failed with error: %d\n", errno);
//     }
// #endif
// 	/////////////////////

// 	wtk_debug("================>>>>>>>>>new ok!\n");
// 	ret = 0;
// end:
// 	if(ret != 0){
// 		qtk_uart_client_delete(uc);
// 		uc = NULL;
// 	}
// 	return uc;
// }

// int qtk_uart_client_delete(qtk_uart_client_t *uc)
// {
// 	wtk_debug("uc delete.\n");
// 	wtk_lockhoard_clean(&uc->msg_hoard);
	
// 	if(uc->uart){
// 		qtk_uart_delete(uc->uart);
// 	}

// 	if(uc->uart_buf){
// 		wtk_strbuf_delete(uc->uart_buf);
// 	}
// 	if(uc->uart_buf2){
// 		wtk_strbuf_delete(uc->uart_buf2);
// 	}

// 	if(uc->version) {
// 		wtk_strbuf_delete(uc->version);
// 	}
// 	if(uc->mac)
// 	{
// 		wtk_strbuf_delete(uc->mac);
// 	}
// 	if(uc->parser)
// 	{
// 		wtk_json_parser_delete(uc->parser);
// 	}

// #ifdef USE_3308
// 	// 删除共享空间
//     shmdt(cp.shmaddr);
//     shmctl(cp.shmid, IPC_RMID, NULL);
// #endif

// #ifdef USE_AM34
// 	msgctl(uc->msg_id, IPC_RMID, NULL);
// 	perror("msgctl failed");
// #endif
// 	wtk_free(uc);
// 	return 0;
// }

// int qtk_uart_client_start(qtk_uart_client_t *uc)
// {
// 	if(0 == uc->msg_run)
// 	{
// 		uc->msg_run = 1;
// 		wtk_thread_start(&uc->msg_thread);
// 	}

//     if(0 == uc->trsn_run)
//     {
//     	uc->trsn_run = 1;
//         wtk_thread_start(&uc->trsn_thread);
//     }

// 	return 0;
// }

// int qtk_uart_client_stop(qtk_uart_client_t *uc)
// {
// 	if(1 == uc->trsn_run)
//     {
//     	uc->trsn_run = 0;
//         wtk_blockqueue_wake(&uc->input_q);
//         wtk_thread_join(&uc->trsn_thread);
// 		qtk_uart_client_clean_q(uc, &uc->input_q);
//     }
// 	if(1 == uc->msg_run)
// 	{
// 		uc->msg_run = 0;
// 		wtk_blockqueue_wake(&uc->msg_q);
// 		wtk_thread_join(&uc->msg_hoard);
// 		qtk_uart_client_clean_q(uc, &uc->msg_q);
// 	}
// 	return 0;
// }

// int qtk_uart_client_grep_start(char *data, int len, int *alen)
// {
// 	int pos=0;
// 	char *start="start";
// 	while(pos < len-strlen(start))
// 	{
// 		if(strncmp(data+pos, start, strlen(start)) == 0)
// 		{
// 			wtk_debug("[%.*s]\n",strlen(start),data+pos);
// 			memcpy(alen, data+pos+strlen(start), sizeof(int));
// 			return pos+RECV_FRAME_HEAD;
// 		}
// 		pos++;
// 	}
// 	return -1;
// }

// int qtk_uart_client_msg_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
// {
// 	qtk_uart_client_msg_t *msg;
// 	wtk_queue_node_t *qn;
// 	// qtk_uart_recv_frame_t *frame;
// 	while (uc->msg_run)
// 	{
// 		qn = wtk_blockqueue_pop(&uc->msg_q, -1, NULL);
// 		if(!qn){continue;}
// 		msg = data_offset2(qn, qtk_uart_client_msg_t, q_n);

// 		// qtk_uart_client_on_uart(uc, msg->statID, msg->buf->data, msg->buf->pos);
// 		// handle_uart_frame(uc,frame);

// 		// if(msg)
// 		// {
// 		// 	qtk_uart_client_push_msg(uc, msg);
// 		// }
// 		if(&current_frame)
// 		{
// 			qtk_uart_client_push_msg(uc, &current_frame);
// 		}
// 	}
	
// }

// int qtk_uart_client_trsn_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
// {
//     uint8_t byte;
//     int ret;
//     uart_parse_state_t state = PARSE_STATE_HEADER1;

//     qtk_uart_recv_frame_t current_frame = {0};
//     uint16_t data_len = 0;
//     uint16_t data_index = 0;
//     uint16_t crc_expected = 0;
//     uint8_t crc_data[256]; // 最大支持 256 字节数据
//     int crc_pos = 0;
//     int event_byte = 0;
//     int len_byte = 0;
//     int crc_byte = 0;

//     while (uc->trsn_run) {
//         ret = qtk_uart_read(uc->uart, (char*)&byte, 1); // 逐字节读取
//         if (ret <= 0) {
//             usleep(10000); // 10ms
//             continue;
//         }

//         printf(" %02X ",byte); // 调试用

//         switch (state) {
//             case PARSE_STATE_HEADER1:
//                 if (byte == REQUEST_FRAME_HEADER_0) {
//                     memset(&current_frame, 0, sizeof(current_frame));
//                     current_frame.frame_header[0] = byte;
//                     state = PARSE_STATE_HEADER2;
//                 }
//                 break;

//             case PARSE_STATE_HEADER2:
//                 if (byte == REQUEST_FRAME_HEADER_1) {
//                     current_frame.frame_header[1] = byte;
//                     state = PARSE_STATE_EVENT_CODE;
//                 } else {
//                     state = PARSE_STATE_HEADER1;
//                 }
//                 break;

//             case PARSE_STATE_EVENT_CODE:
//                 current_frame.event_code[event_byte++] = byte;
//                 if (event_byte >= 2) {
//                     event_byte = 0;
//                     state = RESP_STATE_DATA_LENGTH;
//                 }
//                 break;

//            case RESP_STATE_DATA_LENGTH:
//                 current_frame.data_length[0] = byte; 
//                 state = RESP_STATE_DATA_LENGTH_2;
//                 break;

//             case RESP_STATE_DATA_LENGTH_2:
//                 current_frame.data_length[1] = byte;
//                 data_len = (current_frame.data_length[1] << 8) | current_frame.data_length[0]; // 小端模式
//                 printf("RESP_STATE_DATA_LENGTH:date_len : %d\n", data_len);
//                 if (data_len > 0) {
//                     current_frame.data = (uint8_t*)malloc(data_len);
//                     if (!current_frame.data) {
//                         state = PARSE_STATE_HEADER1;
//                         break;
//                     }
//                 }else{
// 					state = RESP_STATE_CHECKSUM;
// 					break;;
// 				}
//                 data_index = 0;
//                 state = RESP_STATE_DATA;
//                 break;
//             case RESP_STATE_DATA:
//                 if (data_index < data_len) {
//                     current_frame.data[data_index++] = byte;
//                 }
//                 printf("data_index : %d\n",data_index);
//                 if (data_index >= data_len) {
//                     printf("RESP_STATE_DATA:date_len : %d\n",data_len);
//                     state = RESP_STATE_CHECKSUM;
//                 }
//                 break;

//             case RESP_STATE_CHECKSUM:
//                 current_frame.checksum[0] = byte;
//                 printf("current_frame.checksum[0]: %02X\n",current_frame.checksum[0] ); 
//                 state = RESP_STATE_CHECKSUM_2;

//                 break;
//             case RESP_STATE_CHECKSUM_2:
//                 current_frame.checksum[1] = byte;
//                 printf("current_frame.checksum[1]: %02X\n",current_frame.checksum[1] ); 
//                 state = RESP_STATE_FOOTER;
//                 break;
//             case RESP_STATE_FOOTER:
//                 if (byte == FRAME_FOOTER) {
//                     current_frame.frame_footer = byte;
//                     printf("Received Checksum: %02X %02X\n", 
//                     current_frame.checksum[0], 
//                     current_frame.checksum[1]);
// 					wtk_debug("--------------------->>>>>>>>>>>>\n");
//                     // 构造 CRC 数据
//                     crc_pos = 0;
//                     crc_data[crc_pos++] = current_frame.event_code[0];
//                     crc_data[crc_pos++] = current_frame.event_code[1];
// 					wtk_debug("--------------------->>>>>>>>>>>>\n");
//                     crc_data[crc_pos++] = current_frame.data_length[0];
//                     crc_data[crc_pos++] = current_frame.data_length[1];
// 					wtk_debug("--------------------->>>>>>>>>>>>\n");
//                     for (int i = 0; i < data_len; i++) {
//                         crc_data[crc_pos++] = current_frame.data[i];
//                     }
// 					wtk_debug("--------------------->>>>>>>>>>>>\n");
// 					// wtk_debug("current_frame.data[]: %02X\n",current_frame.data[0]);
//                     uint16_t calc_crc = calculateModbusCRC(crc_data, crc_pos);
//                     uint16_t recv_crc = current_frame.checksum[0] | (current_frame.checksum[1] << 8);

//                     if (calc_crc == recv_crc) {
//                         handle_uart_frame(uc, &current_frame);
//                     } else {
//                         printf("CRC error: %04X vs %04X\n", recv_crc, calc_crc);
//                     }
//                 }

//                 // 清理
//                 if (current_frame.data) {
//                     free(current_frame.data);
//                     current_frame.data = NULL;
//                 }
//                 state = PARSE_STATE_HEADER1;
//                 break;
//         }
//     }

//     return 0;
// }

// void qtk_uart_client_update_params(qtk_uart_client_t *uc, char *data, int len)
// {
// 	wtk_json_parser_t *parser = NULL;
// 	wtk_json_item_t *json = NULL;
// 	wtk_queue_node_t *qn;
// 	int reboot = 0;
// 	int reset_cache = 0;
// 	int ret;
// 	char sdate[24];
// 	char stime[24];

// 	if(len <= 0) {
// 		return;
// 	}

// 	parser = wtk_json_parser_new();
// 	if(!parser){
// 		return ;
// 	}

// 	wtk_debug("[%.*s]\n",len,data);
// 	ret = wtk_json_parser_parse(parser, data, len);
// 	if(ret == 0) {
// 		json = wtk_json_obj_get_s(parser->json->main, "record_tm");
// 		if(json && json->type == WTK_JSON_NUMBER) {
// 			uc->record_tm = json->v.number;
// 		}
		
// 		json = wtk_json_obj_get_s(parser->json->main, "channel");
// 		if(json && json->type == WTK_JSON_NUMBER) {
// 			uc->record_chn = json->v.number;
// 		}

// 	} else {
// 		wtk_debug("[WARN] uart recv params parser failed.\n");
// 		if(uc->log) {
// 			wtk_log_warn0(uc->log, "[WARN] *** device recv params parser failed, perhaps wrong json string");
// 		}
// 	}
// 	wtk_json_parser_delete(parser);
// }

// #ifdef USE_3308

// #endif

// int qtk_uart_client_set_volume(qtk_uart_client_t * uc, char *buf, int len)
// {
//     int isok = 0;
//     if (access(UART_CFG_PATH, F_OK) == 0 && access(UART_CFG_PATH, W_OK) == 0)
//     {
//         FILE *uf;
//         char tmpbuf[1024] = {0};
//         char buf2[24] = {0};
//         char *pv;
//         int ret, val;
//         int use_agc = -1;

//         // 打开配置文件
//         uf = fopen(UART_CFG_PATH, "r+");
//         ret = fread(tmpbuf, sizeof(tmpbuf), 1, uf);
//         wtk_debug("================fread=[%s]\n", tmpbuf);
//         pv = strstr(tmpbuf, "VBOX3_AGC=");
//         use_agc = atoi(pv + 10);
//         wtk_debug("=====================>>>>>>use_agc=%d\n", use_agc);
//         if (!use_agc)
//         {
//             if (len == 1)
//             {
//                 float set;
//                 set = (15.5 - 1.2) * (buf[0] / 100.0) + 1.0;
//                 wtk_debug("==============>>>>SET_MICVOLUME=%f\n", set);
//                 pv = strstr(tmpbuf, "mic_shift2=");
//                 if (pv)
//                 {
//                     wtk_debug("============>>>>>>>>old=[%s]\n", pv);
//                     sprintf(buf2, "mic_shift2=%0.1f;", set);
//                     memset(pv, 0, strlen(pv));
//                     memcpy(pv, buf2, strlen(buf2));
//                     fseek(uf, 0, SEEK_SET);
//                     ret = fwrite(tmpbuf, strlen(tmpbuf), 1, uf);
//                     wtk_debug("================fwrite=%d [%.*s]\n", ret, strlen(tmpbuf), tmpbuf);
//                 }
//             }
//         }
//         else
//         {
//             isok = -2;
//         }
//         fflush(uf);
//         fclose(uf);
//         system("sync");
//         uf = NULL;
//     }
//     else
//     {
//         isok = -2;  // 文件不可访问
//     }

//     return isok;
// }

// int qtk_uart_client_isRunning()
// {
// 	int ret = 0;
// 	int count=0;
// 	FILE *fstream=NULL;

// 	char buff[1024] = {0};
// 	char input[1024] = {0};

// 	wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>\n");
// #ifdef USE_AM60
// 	snprintf(input, 1024, "ps -ef | grep -w \"Qsound_am60\" | wc -l");
// #else
// #ifdef USE_802A
// 	snprintf(input, 1024, "ps -ef | grep -w \"Qsound_802\" | wc -l");
// #else
// #ifdef USE_BMC
// 	snprintf(input, 1024, "ps -ef | grep -w \"Qsound_bmc\" | wc -l");
// #else
// 	snprintf(input, 1024, "ps -ef | grep -w \"Qsound_3308\" | wc -l");
// #endif
// #endif
// #endif
// 	if(NULL==(fstream=popen(input, "r")))
// 	{
// 		fprintf(stderr,"execute command failed: %s", strerror(errno));
// 		return -1;
// 	}

// 	while(NULL!=fgets(buff, sizeof(buff), fstream))
// 	{
// 		wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>\n");
// 		count = atoi(buff);
// 		wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>count=%d\n",count);
// 		if(count > 2)
// 		{
// 			ret = 1;
// 		}
// 	}
// 	pclose(fstream);
// 	return ret;
// }

// #ifndef  KTC_K72A
// int uart_skip_channels[7]={7,6,5,4,3,2,1};
// #else
// int uart_skip_channels[2]={10,11};
// #endif
// void handle_uart_frame(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *frame)
// {
// 	FILE *fn;
// 	int ret;
// 	char tmpbuf[1024]={0};
// 	char *pc;
// 	int s;
// 	int getresult;
// 	int ucnt=0;
// 	char set_buf[128]={0};
// 	char *upresult;
// 	int ulen;
// 	int timeout_s = 10;
// 	int resp;
// 	int len;
// 	uint8_t response;
//     uint16_t data_len = frame->data_length[0] | (frame->data_length[1] << 8);
//     uint16_t event_code = frame->event_code[1] | (frame->event_code[0] << 8);
//     wtk_debug("Received event_code: 0x%04X\n", event_code);
// 	wtk_debug("-------=========================================---->>>>>\n");
//     switch (event_code) {
//         case 0x0101:  // QTK_UART_TYPE_RECV_SPEAKER_JUDGMENT - 扬声器音频检测判断
// 			getresult = qtk_uart_client_isRunning();
// 			wtk_debug("------------------------------------->>>>>\n");
// 			if (getresult == 1) {
// 				wtk_debug("------------------------------------->>>>>\n");
// 				ret = 1;
// 				ucnt = 0;
// 				while (ret != 0) {
// 					qtk_proc_write(uc,"SPK_AUDIO_CHECK",16);
// 					ret=qtk_proc_read(uc);
// 					if(++ucnt > 1){break;}
// 						wtk_debug("------------------------------------->>>>>\n");
// 				}
// 			}
// 			wtk_debug("------------------------------------->>>>>\n");
// 			time_t startspk_time = time(NULL);
// 			wtk_debug("------------------------------------->>>>>\n");
// 			while (time(NULL) - startspk_time < timeout_s) {
// 				if (access("/oem/qdreamer/qsound/spkcheck_result.txt", F_OK) == 0){
// 					// 文件存在，读取文件内容
// 					system("chmod 777 /oem/qdreamer/qsound/spkcheck_result.txt");
// 					upresult = file_read_buf("/oem/qdreamer/qsound/spkcheck_result.txt", &ulen);
// 					resp = atoi(upresult);
// 					wtk_debug("------------------------resp = %d\n",resp);
// 					if (resp == 1) {
// 						response = 0x01;
// 						send_response(uc, frame, &response,1); // 静音
// 					} else if (resp == 2) {
// 						response = 0x02;
// 						send_response(uc, frame, &response,1); // 爆音
// 					} else {
// 						response = 0x00;
// 						send_response(uc, frame, &response,1); // 正常
// 					}
// 					system("rm spkcheck_result.txt");
// 					wtk_log_log0(uc->log,"SPK_AUDIO_CHECK finished!\n");
// 					break;
// 				}
// 				usleep(10000); // 10ms
// 			}
// 			wtk_log_log0(uc->log,"ERROR:SPK_AUDIO_CHECK,timeout!\n");
// 			break;
//         case 0x0102:  // QTK_UART_TYPE_RECV_MIC_JUDGMENT - mic音频检测判断
// 			getresult = qtk_uart_client_isRunning();
// 			wtk_debug("------------------------------------->>>>>\n");
// 			if (getresult == 1) {
// 				wtk_debug("------------------------------------->>>>>\n");
// 				ret = 1;
// 				ucnt = 0;
// 				while (ret != 0) {
// 					qtk_proc_write(uc,"MIC_AUDIO_CHECK",16);
// 					ret=qtk_proc_read(uc);
// 					if(++ucnt > 1){break;}
// 						wtk_debug("------------------------------------->>>>>\n");
// 				}
// 			}
// 			wtk_debug("------------------------------------->>>>>\n");
// 			time_t startmic_time = time(NULL);
// 			wtk_debug("------------------------------------->>>>>\n");
// 			while (time(NULL) - startmic_time < timeout_s) {
// 				if (access("/oem/qdreamer/qsound/miccheck_result.txt", F_OK) == 0){
// 					system("chmod 777 /oem/qdreamer/qsound/miccheck_result.txt");
// 					upresult = file_read_buf("/oem/qdreamer/qsound/miccheck_result.txt", &ulen);
// 					resp = atoi(upresult);
// 					wtk_debug("------------------------resp = %d\n",resp);
// 					if (resp == 1) {
// 						response = 0x01;
// 						send_response(uc, frame, &response,1); // 静音
// 					} else if (resp == 2) {
// 						response = 0x02;
// 						send_response(uc, frame, &response,1); // 爆音
// 					} else {
// 						response = 0x00;
// 						send_response(uc, frame, &response,1); // 正常
// 					}
// 					system("rm miccheck_result.txt");
// 					wtk_log_log0(uc->log,"MIC_AUDIO_CHECK finished!\n");
// 					break;
// 				}
// 				usleep(10000); // 10ms
// 			}
// 			wtk_log_log0(uc->log,"ERROR:MIC_AUDIO_CHECK timeout!\n");
// 			break;
//         case 0x0103:  // QTK_UART_TYPE_RECV_MIC_JUDGMENT_OUTPUT_EQ_ADJUSTMENT - 输出EQ设置
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x0105:  // QTK_UART_TYPE_RECV_SET_DENOISE_SWITCH - 智能降噪开关设置
// 			wtk_debug("-------------------------------->>>>>>>>>>>>>>>>>>>>\n");
// 			if(frame->data[0]==0x00){
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					if(frame->data[0]==0x00){
// 						pc=strstr(tmpbuf,"VBOX3_ANS=");
// 					}
// 					s=pc-tmpbuf;
// 					tmpbuf[s+10]='0';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					fflush(fn);
// 				}

// 				getresult = qtk_uart_client_isRunning();
// 				if(getresult == 1){
// 					ret =1;
// 					ucnt=0;
// 					while(ret != 0){
// 						qtk_proc_write(uc,"SET_MICANC_OFF",14);
// 						ret=qtk_proc_read(uc);
// 						if(++ucnt > 1){break;}
// 					}
// 				}else{
// 					result=0;
// 				}
// 				if(ret == -1){
// 					result = -2;
// 				}
// 				wtk_log_log0(uc->log,"close anc!\n");
// 			}else if (frame->data[0]==0x01){
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					if(frame->data[0]==0x01){
// 						pc=strstr(tmpbuf,"VBOX3_ANS=");
// 					}
// 					s=pc-tmpbuf;
// 					tmpbuf[s+10]='1';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					fflush(fn);
// 				}

// 				getresult = qtk_uart_client_isRunning();
// 				if(getresult == 1){
// 					ret =1;
// 					ucnt=0;
// 					while(ret != 0){
// 						qtk_proc_write(uc,"SET_MICANC_ON",13);
// 						ret=qtk_proc_read(uc);
// 						if(++ucnt > 1){break;}
// 					}
// 				}else{
// 					result=0;
// 				}
// 				if(ret == -1){
// 					result = -2;
// 				}
// 				wtk_log_log0(uc->log,"open anc!\n");

// 			}
// 			system("sync");
// 			wtk_strbuf_reset(uc->uart_buf2);
// 			fclose(fn);
// 			fn=NULL;
// 			system("sync");
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x0104:  // QTK_UART_TYPE_RECV_GET_DENOISE_SWITCH - 智能降噪开关获取
// 			result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_DENOISE_SWITCH);
//             wtk_debug("anc status: %d\n", result);
//             // wtk_strbuf_reset(uc->uart_buf2);
// 			if(result == 0){
// 				response = 0x01;
// 			}else if (result == 1){
// 				response = 0x02;
// 			}
//             wtk_debug("anc status: 0x%02X\n", response);
//             send_response(uc, frame, &response,1);
// 			response = 0x00;
// 			wtk_log_log0(uc->log,"Get AGC_switch success!\n");
//             break;
            
//         case 0x0107:  // QTK_UART_TYPE_RECV_SET_GAIN_CONTROL_SWITCH - 自动增益开关设置
// 			if(frame->data[0]==0x00){
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					if(frame->data[0]==0x00){
// 						pc=strstr(tmpbuf,"VBOX3_AGC=");
// 					}
// 					s=pc-tmpbuf;
// 					tmpbuf[s+10]='0';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					fflush(fn);
// 				}

// 				getresult = qtk_uart_client_isRunning();
// 				if(getresult == 1){
// 					ret =1;
// 					ucnt=0;
// 					while(ret != 0){
// 						qtk_proc_write(uc,"SET_MICAGC_OFF",15);
// 						ret=qtk_proc_read(uc);
// 						if(++ucnt > 1){break;}
// 					}
// 				}else{
// 					result=0;
// 				}
// 				if(ret == -1){
// 					result = -2;
// 				}
// 				wtk_log_log0(uc->log,"close agc function!\n");
// 			}else if (frame->data[0]==0x01){
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					if(frame->data[0]==0x01){
// 						pc=strstr(tmpbuf,"VBOX3_AGC=");
// 					}
// 					s=pc-tmpbuf;
// 					tmpbuf[s+10]='1';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					fflush(fn);
// 				}

// 				getresult = qtk_uart_client_isRunning();
// 				if(getresult == 1){
// 					ret =1;
// 					ucnt=0;
// 					while(ret != 0){
// 						qtk_proc_write(uc,"SET_MICAGC_ON",13);
// 						ret=qtk_proc_read(uc);
// 						if(++ucnt > 1){break;}
// 					}
// 				}else{
// 					result=0;
// 				}
// 				if(ret == -1){
// 					result = -2;
// 				}
// 				wtk_log_log0(uc->log,"open agc function!\n");
// 			}
// 			system("sync");
// 			wtk_strbuf_reset(uc->uart_buf2);
// 			fclose(fn);
// 			fn=NULL;
// 			system("sync");
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x0106:  // QTK_UART_TYPE_RECV_GET_GAIN_CONTROL_SWITCH - 自动增益开关获取
//             result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_GAIN_CONTROL_SWITCH);
//             wtk_debug("agc status: %d\n", result);
//             wtk_strbuf_reset(uc->uart_buf2);
//             if(result == 0){
// 				response = 0x01;
// 			}else if (result == 1){
// 				response = 0x02;
// 			}
//             send_response(uc, frame, &response,1);
// 			response = 0x00;
// 			wtk_log_log0(uc->log,"Get AGC_switch sucess!\n");
//             break;     
//         case 0x0109:  // QTK_UART_TYPE_RECV_SET_ECHO_INTENSITY_SWITCH - 回声抑制强度设置
// 		if (frame->data[0] == 0x00) {
// 			if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0) {
// 				fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r+");
// 				ret = fread(tmpbuf, sizeof(tmpbuf), 1, fn);
// 				wtk_debug("================fread=[%s]\n", tmpbuf);
// 				pc = strstr(tmpbuf, "VBOX3_AEC=");
// 				s = pc - tmpbuf;
// 				tmpbuf[s + 10] = '0';
// 				fseek(fn, 0, SEEK_SET);
// 				ret = fwrite(tmpbuf, strlen(tmpbuf), 1, fn);
// 				wtk_debug("+========>>>>>>s=%d pc=%c\n", s, *pc);
// 				fflush(fn);
// 			}
			
// 			// 发送关闭指令
// 			getresult = qtk_uart_client_isRunning();
// 			if (getresult == 1) {
// 				ret = 1;
// 				ucnt = 0;
// 				while (ret != 0) {
// 					qtk_proc_write(uc, "SET_MICAEC_OFF", 14);
// 					ret = qtk_proc_read(uc);
// 					if (++ucnt > 1) break;
// 				}
// 			}
// 			wtk_log_log0(uc->log,"关闭回声抑制功能!\n");
// 		} 
// 		else if (frame->data[0] == 0x01 || frame->data[0] == 0x02 || frame->data[0] == 0x03) {
// 			uint8_t aec_level = frame->data[0]; // 0x01=低, 0x02=中, 0x03=高
// 			if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0) {
// 				fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r+");
// 				ret = fread(tmpbuf, sizeof(tmpbuf), 1, fn);
// 				wtk_debug("================fread=[%s]\n", tmpbuf);
// 				pc = strstr(tmpbuf, "VBOX3_AEC=");
// 				if (pc) {
// 					s = pc - tmpbuf;
// 					tmpbuf[s + 10] = '1';
// 				}
// 				pc = strstr(tmpbuf, "vbox3_aec_level=");
// 				if (pc) {
// 					s = pc - tmpbuf;
// 					tmpbuf[s + 16] = '0' + (aec_level - 0x00); // 转换为字符'1'/'2'/'3'
// 				}
// 				fseek(fn, 0, SEEK_SET);
// 				ret = fwrite(tmpbuf, strlen(tmpbuf), 1, fn);
// 				fflush(fn);
// 				wtk_debug("已设置AEC等级: %d\n", aec_level);
// 			}
// 			getresult = qtk_uart_client_isRunning();
// 			if (getresult == 1) {
// 				ret = 1;
// 				ucnt = 0;
// 				while (ret != 0) {
// 					char cmd_buf[32];
// 					if (aec_level == 0x01){ 
// 						snprintf(cmd_buf, sizeof(cmd_buf), "SET_MICAEC_ON_LOW");
// 						wtk_log_log0(uc->log,"设置回声抑制强度：低!\n");
// 					}
// 					else if (aec_level == 0x02){
// 						snprintf(cmd_buf, sizeof(cmd_buf), "SET_MICAEC_ON_MID");
// 						wtk_log_log0(uc->log,"设置回声抑制强度：中!\n");
// 					}
// 					else if(aec_level == 0x03) {
// 						snprintf(cmd_buf, sizeof(cmd_buf), "SET_MICAEC_ON_HIGH");
// 						wtk_log_log0(uc->log,"设置回声抑制强度：高!\n");
// 					}
// 					qtk_proc_write(uc, cmd_buf, strlen(cmd_buf));
// 					ret = qtk_proc_read(uc);
// 					if (++ucnt > 1) break;
// 				}
// 			}
// 		}
// 			system("sync");
// 			wtk_strbuf_reset(uc->uart_buf2);
// 			fclose(fn);
// 			fn=NULL;
// 			system("sync");
//             send_response(uc, frame, NULL,NULL);
//             break;     
//         case 0x0108:  // QTK_UART_TYPE_RECV_GET_ECHO_INTENSITY_SWITCH - 回声抑制强度获取
//          	result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_ECHO_INTENSITY_SWITCH);
// 			// uint8_t strong_echo = (result == -1) ? 0x00 : (uint8_t)result;
// 			wtk_debug("AEC Response:AEC level %d\n", result);
// 			if(result == 0){
// 				response = 0x01;
// 			}else if (result == 1){
// 				response = 0x02;
// 			}else if (result == 2){
// 				response = 0x03;
// 			}else if (result == 3){
// 				response = 0x04;
// 			}
//             send_response(uc, frame, &response,1);
// 			response = 0x00;
//             break;
            
//         case 0x010A:  // QTK_UART_TYPE_RECV_GET_LIST_AUDIO_INPUT_PORTS - 获取音频输入口列表
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x010B:  // QTK_UART_TYPE_RECV_GET_LIST_AUDIO_OUTPUT_PORTS - 获取音频输出口列表
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x010C:  // QTK_UART_TYPE_RECV_ENDABLE_AND_DISABLE_MIC - 启用禁用MIC
// 			if(frame->data[0]==0x00)
// 			{
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					pc=strstr(tmpbuf,"USE_MIC=");
// 					s=pc-tmpbuf;
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					tmpbuf[s+8]='1';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					fflush(fn);
// 				}
// 			}else if(frame->data == 0x01){
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					pc=strstr(tmpbuf,"USE_MIC=");
// 					s=pc-tmpbuf;
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					tmpbuf[s+8]='0';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					fflush(fn);
// 				}
// 			}
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x010D:  // QTK_UART_TYPE_RECV_ENDABLE_AND_DISABLE_SPK - 启用禁用SPK
// 			if(frame->data[0]==0x00)
// 			{
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					pc=strstr(tmpbuf,"USE_SPK=");
// 					s=pc-tmpbuf;
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					tmpbuf[s+8]='1';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					fflush(fn);
// 				}
// 			}else if(frame->data == 0x01){
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					pc=strstr(tmpbuf,"USE_SPK=");
// 					s=pc-tmpbuf;
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					tmpbuf[s+8]='0';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					fflush(fn);
// 				}
// 			}
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x010E:  // QTK_UART_TYPE_RECV_LINE_IN_CONTROL - line in本地输出控制(本地扩音)
// 			if(frame->data[0] == 0x00)
// 			{
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					pc=strstr(tmpbuf,"USE_LINEIN=");
// 					s=pc-tmpbuf;
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					tmpbuf[s+11]='1';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					fflush(fn);
// 				}
// 			}else if(frame->data[0] == 0x01){
// 				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
// 					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
// 					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
// 					wtk_debug("================fread=[%s]\n",tmpbuf);
// 					pc=strstr(tmpbuf,"USE_LINEIN=");
// 					s=pc-tmpbuf;
// 					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
// 					tmpbuf[s+11]='0';
// 					fseek(fn,0,SEEK_SET);
// 					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
// 					fflush(fn);
// 				}
// 			}
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x010F:  // QTK_UART_TYPE_RECV_SET_INPUT_TYPE - 设置输入口类型
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x011A:  // QTK_UART_TYPE_RECV_GET_VOLUME_VALUE - 获取音量值
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x0110:  // QTK_UART_TYPE_RECV_GET_MICROPHONE_VOLUME_VALUE - 读取麦克风音量档位
//             result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_MICROPHONE_VOLUME_VALUE);
// 			wtk_debug("MIC volume_value: 0x%02X\n", (uint8_t)result);
// 			send_response(uc, frame, (uint8_t)result,1); // 直接发送单字节
//             break;
            
//         case 0x0112:  // QTK_UART_TYPE_RECV_SET_MICROPHONE_VOLUME_VALUE - 设置麦克风音量档位
// 			getresult = qtk_uart_client_isRunning();
// 			if(getresult == 1){
// 				ret =1;
// 				ucnt=0;
// 				while(ret != 0){
// 					ret=sprintf(set_buf,"SET_MICVOLUME%d",frame->data[1]);
// 					qtk_proc_write(uc,set_buf,ret);//先判断是否启用agc
// 					ret=qtk_proc_read(uc);
// 					if(++ucnt > 1){break;}
// 				}
// 			}else{
// 				result = qtk_uart_client_set_volume(uc, (char*)&frame->data[1], 1);
// 			}
// 			if(ret == -1){
// 				result = -2;
// 			}
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x0111:  // QTK_UART_TYPE_RECV_GET_SPEKER_VOLUME_VALUE - 读取扬声器音量档位
//             result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_SPEKER_VOLUME_VALUE);
// 			wtk_debug("SPK volume value: 0x%02X\n", (uint8_t)result);
// 			result_data = (uint8_t)result;
// 			send_response(uc, frame, &result,1); // 直接发送单字节
//             break;
            
//         case 0x0113:  // QTK_UART_TYPE_RECV_SET_SPEKER_VOLUME_VALUE - 设置扬声器音量档位
//             getresult = qtk_uart_client_isRunning();
// 			if(getresult == 1){
// 			ret =1;
// 			ucnt=0;
// 			while(ret != 0){
// 				ret=sprintf(set_buf,"%s%.*s","SET_SPKVOLUME",1,(char*)frame->data[1]);
// 				qtk_proc_write(uc,set_buf,ret);//先判断是否启用agc 
// 				ret=qtk_proc_read(uc);
// 				if(++ucnt > 1){break;}
// 			}
// 			}else{
// 				// result = qtk_uart_client_set_volume(uc, (char*)frame->data[1], 1);
// 				sprintf(set_buf,"echo %d > /sys/bus/i2c/devices/3-006d/volume",frame->data[1]);
// 				system(set_buf);
// 				sprintf(set_buf,"echo %d > /sys/bus/i2c/devices/3-0069/volume",frame->data[1]);
// 				system(set_buf);
// 			}
// 			if(ret == -1){
// 				result = -2;
// 			}
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x0114:  // QTK_UART_TYPE_RECV_LOG_REPORTING - 日志上报
// 			system("tar -cvf /data/qtk_uart_client.log.tar /data/qtk_uart_client.log");
// 			char *data=file_read_buf("/data/qtk_uart_client.log.tar",&len);
//             send_response(uc, frame, data,len);
//             break;
            
//         case 0x0115:  // QTK_UART_TYPE_RECV_ALARM_REPORTING - 告警上报
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x0116:  // QTK_UART_TYPE_RECV_AUDIO_STATUS_CHANGE_NOTIFICATION - 音频状态变化通知
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x0117:  // QTK_UART_TYPE_RECV_AUDIO_INPUT_AND_OUTPUT_DEVICE_UNPLUGGING - 音频输入输出设备拔出通知
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x0118:  // QTK_UART_TYPE_RECV_SPEAKER_CONTROL - 扬声器控制
// 			 getresult = qtk_uart_client_isRunning();
// 			if(getresult == 1){
// 			ret =1;
// 			ucnt=0;
// 			while(ret != 0){
// 				ret=sprintf(set_buf,"%s%.*s","SPEAKER_CONTROL",1,(char*)frame->data[1]);
// 				qtk_proc_write(uc,set_buf,ret);//先判断是否启用agc 
// 				ret=qtk_proc_read(uc);
// 				if(++ucnt > 1){break;}
// 			}
//             send_response(uc, frame, NULL,NULL);
//             break;
            
//         case 0x0119:  // QTK_UART_TYPE_RECV_LOG_COLLECTION - 日志收集
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x011B:  // QTK_UART_TYPE_RECV_AUDIO_INPUT_AND_OUTPUT_DEVICE_INSERTIOIN - 音频输入输出设备插入通知
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x011C:  // QTK_UART_TYPE_RECV_GET_OUTPUT_EQ_MODE - 获取输出EQ模式
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x011D:  // QTK_UART_TYPE_RECV_SET_OUTPUT_EQ_MODE - 设置输出EQ模式
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x011E:  // QTK_UART_TYPE_RECV_GET_LINEOUT_MODE - 获取lineout输出模式
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         case 0x011F:  // QTK_UART_TYPE_RECV_SET_LINEOUT_MODE - 设置lineout输出模式
//             send_response(uc, frame, 0x00,1);
//             break;
            
//         default:
//             send_response(uc, frame, 0xFF,1); // 无效命令
//             break;
//     }
// }
// static void qtk_uart_client_clean_q(qtk_uart_client_t *uc, wtk_blockqueue_t *queue)
// {
// 	qtk_uart_client_msg_t *msg;
// 	wtk_queue_node_t *qn;
// 	int len=queue->length;
// 	int i=0;

// 	while(i<len) {
// 		qn = wtk_blockqueue_pop(queue,0,NULL);
// 		if(!qn) {
// 			break;
// 		}
// 		msg = data_offset2(qn,qtk_uart_client_msg_t,q_n);
// 		qtk_uart_client_push_msg(uc,msg);
// 		i++;
// 	}
// }

// static qtk_uart_client_msg_t* qtk_uart_client_msg_new(qtk_uart_client_t *uc)
// {
// 	qtk_uart_client_msg_t *msg;

// 	msg = (qtk_uart_client_msg_t*)wtk_malloc(sizeof(*msg));
// 	if(!msg) {
// 		return NULL;
// 	}
// 	msg->buf = wtk_strbuf_new(2560,1);
// 	if(!msg->buf) {
// 		wtk_free(msg);
// 		return NULL;
// 	}

// 	return msg;
// }

// static int qtk_uart_client_msg_delete(qtk_uart_client_msg_t *msg)
// {
// 	wtk_strbuf_delete(msg->buf);
// 	wtk_free(msg);
// 	return 0;
// }

// static qtk_uart_client_msg_t* qtk_uart_client_pop_msg(qtk_uart_client_t *uc)
// {
// 	qtk_uart_client_msg_t *msg;

// 	msg = wtk_lockhoard_pop(&uc->msg_hoard);
// 	if(!msg) {
// 		return NULL;
// 	}
// 	msg->statID = 0;
// 	wtk_strbuf_reset(msg->buf);
// 	return msg;
// }

// static void qtk_uart_client_push_msg(qtk_uart_client_t *uc,qtk_uart_client_msg_t *msg)
// {
// 	wtk_lockhoard_push(&uc->msg_hoard,msg);
// }

// int qtk_uart_client_feed(qtk_uart_client_t *uc, char *data, int len)
// {
// 	return 0;
// }

// int qtk_uart_client_uart_param(qtk_uart_client_t *uc, wtk_strbuf_t *buf)
// {
// 	wtk_json_item_t *item;
// 	wtk_json_t *json;
// 	wtk_strbuf_t *tmp = NULL;

// 	json = wtk_json_new();
// 	item = wtk_json_new_object(json);
// 	tmp = wtk_strbuf_new(256, 0);

// 	wtk_json_obj_add_ref_number_s(json, item, "dev_no", uc->cfg->dev_no);

// 	wtk_json_obj_add_str2_s(json, item, "mac", uc->mac->data, uc->mac->pos);
// 	wtk_json_obj_add_str2_s(json, item, "version", uc->version->data, uc->version->pos);

// 	tm_e = time_get_ms();
// 	wtk_json_obj_add_ref_number_s(json, item, "delay", (int)(tm_e-tm_s));
// 	tm_s = 0;
// 	tm_e = 0;
// 	// qtk_get_battery_file(tmp);
// 	// wtk_json_obj_add_str2_s(json, item, "batt", tmp->data, tmp->pos);
// 	// wtk_strbuf_reset(tmp);

// 	wtk_json_item_print(item, tmp);
// 	wtk_strbuf_push(buf, tmp->data, tmp->pos);
// 	wtk_strbuf_delete(tmp);
// 	wtk_json_delete(json);
// 	return 0;
// }

// static void qtk_uart_client_feed_notice(qtk_uart_client_t *uc, int notice)
// {
// 	qtk_uart_client_msg_t *msg;

// 	msg = qtk_uart_client_pop_msg(uc);

// 	msg->statID = notice;

// 	wtk_blockqueue_push(&uc->input_q, &msg->q_n);
// }
// int parse_uart_frame(const uint8_t *data, int len, qtk_uart_recv_frame_t *frame)
// {
// 	wtk_debug("parse_uart_frame\n");
//     if (len < 9) {
//         return -1; // 帧不完整
//     }
//     if (data[0] != REQUEST_FRAME_HEADER_0 || 
//         data[1] != REQUEST_FRAME_HEADER_1) {
//         return -2; // 帧头错误
//     }
//     if (data[len-1] != FRAME_FOOTER) {
//         return -3; // 帧尾错误
//     }
//     memcpy(frame->frame_header, data, 2);
//     memcpy(frame->event_code, data + 2, 2);
//     memcpy(frame->data_length, data + 4, 2);
//     memcpy(frame->checksum, data + len - 3, 2); 
//     frame->frame_footer = data[len-1];
//     // 解析数据部分
//     uint16_t data_len =  frame->data_length[0]| (frame->data_length[1] << 8);
//     if (data_len > 0) {
//         // 检查数据长度是否匹配
//         if (len != 9 + data_len) {
//             return -4; // 长度不匹配
//         }
//         frame->data = (uint8_t *)malloc(data_len);
//         if (!frame->data) {
//             return -5; // 内存分配失败
//         }
//         memcpy(frame->data, data + 6, data_len);
//     } else {
//         frame->data = NULL;
//     }
    
//     return 0;
// }
// static void send_response(qtk_uart_client_t *uc, 
//                                 qtk_uart_recv_frame_t *req_frame,
//                                 uint8_t *data,
// 							    uint16_t data_len)
// {
// 	qtk_uart_recv_frame_t resp = {
//         .frame_header = {RESPONSE_FRAME_HEADER_0, RESPONSE_FRAME_HEADER_1},
//         .event_code = {req_frame->event_code[0], req_frame->event_code[1]},
//         .data_length = {data_len & 0xFF, (data_len >> 8) & 0xFF},
//         .data = (uint8_t *)data,
//         .frame_footer = FRAME_FOOTER
//     };
//     int crc_data_len = 4 + data_len;
//     uint8_t *crc_data = (uint8_t *)malloc(crc_data_len);    
//     if (crc_data) {
//         int pos = 0;
//         memcpy(crc_data + pos, resp.event_code, 2);
//         pos += 2;
//         memcpy(crc_data + pos, resp.data_length, 2);
//         pos += 2;
//         if (data_len > 0) {
//             memcpy(crc_data + pos, data, data_len);
//         }
        
//         uint16_t crc = calculateModbusCRC(crc_data, crc_data_len);
//         resp.checksum[0] = crc & 0xFF;
//         resp.checksum[1] = (crc >> 8) & 0xFF;
        
//         free(crc_data);
//     } else {
//         resp.checksum[0] = 0x00;
//         resp.checksum[1] = 0x00;
//     }
//     int frame_len = 9 + data_len; // 2头 + 2事件 + 2长度 + 数据 + 2CRC + 1尾
//     uint8_t *send_buf = (uint8_t *)malloc(frame_len);
    
//     if (send_buf) {
//         int pos = 0;
//         memcpy(send_buf + pos, resp.frame_header, 2);
//         pos += 2;
//         memcpy(send_buf + pos, resp.event_code, 2);
//         pos += 2;
//         memcpy(send_buf + pos, resp.data_length, 2);
//         pos += 2;
//         if (data_len > 0) {
//             memcpy(send_buf + pos, data, data_len);
//             pos += data_len;
//         }
//         memcpy(send_buf + pos, resp.checksum, 2);
//         pos += 2;
//         send_buf[pos++] = resp.frame_footer;
//         int ret = qtk_uart_write2(uc->uart, (char*)send_buf, pos);
//         wtk_debug("Send log frame: len=%d\n", ret);
//         wtk_debug("Header: %02X %02X\n", resp.frame_header[0], resp.frame_header[1]);
//         wtk_debug("Event: %02X %02X\n", resp.event_code[0], resp.event_code[1]);
//         wtk_debug("Length: %02X %02X (%d bytes)\n", 
//                  resp.data_length[0], resp.data_length[1], data_len);
//         if (data_len > 0) {
//             wtk_debug("Data: [%.*s]\n", data_len > 32 ? 32 : data_len, data);
//         }
//         wtk_debug("CRC: %02X %02X\n", resp.checksum[0], resp.checksum[1]);
//         wtk_debug("Footer: %02X\n", resp.frame_footer);
//         free(send_buf);
// 	}
// }
// unsigned short calculateModbusCRC(unsigned char *data, int length) {
// 	unsigned short crc = 0xFFFF;// 初始化 CRC 值
//     unsigned char b;
//     for (int i = 0; i < length; i++) {
//         crc ^= data[i];
//         // 对每个字节进行 8 次右移操作
//         for (int j = 0; j < 8; j++) {
//             if (crc & 0x0001) {
//                 crc >>= 1;  // 右移 1 位
//                 crc ^= 0xA001;  // 异或常数 0xA001
//             } else {
//                 crc >>= 1;  // 右移 1 位
//             }
//         }
//     }
//     return crc; 
// }