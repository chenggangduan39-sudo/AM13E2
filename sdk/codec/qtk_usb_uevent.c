#include "qtk_usb_uevent.h"
#include "wtk/core/wtk_strbuf.h"

#define UEVENT_BUFFER_SIZE 2048  
#define USE_RECVMSG	1
#define USE_RECV	2
#define RECV_METHOD USE_RECVMSG

static char* qtk_usb_uevent_param_str[] = {
		"ACTION",							//0   开始
		"DEVPATH",							//1   设备路径
		"SUBSYSTEM",						//2   子系统
		"USB_STATE",						//3   usb状态
		"STREAM_DIRECTION",					//4   音频流方向
		"STREAM_STATE",						//5   流状态
        "SAMPLE_RATE",                      //6   采样率
		"SEQNUM",							//7   字符长度
};

int qtk_usb_uevent_on_run(qtk_usb_uevent_t *uu, wtk_thread_t *t);
int qtk_usb_uevent_on_msg(qtk_usb_uevent_t *uu, wtk_thread_t *t);

static int qtk_usb_uevent_init_netlink_sock(struct sockaddr_nl *nl)  
{  
	const int buffersize = 4096;  
	int ret;  

	nl->nl_family = AF_NETLINK;  
	nl->nl_pid = 0;  
	nl->nl_groups =1;  

	int s = socket(AF_NETLINK, SOCK_RAW, NETLINK_KOBJECT_UEVENT);  
	if (s < 0)   
	{  
	  perror("socket");  
	  return -1;  
	}  
	//setsockopt(s, SOL_SOCKET, SO_RCVBUF, &buffersize, sizeof(buffersize));  
	  ret = bind(s, (struct sockaddr *)nl, sizeof(struct sockaddr_nl));  
	  if (ret < 0)   
	  {  
		  perror("bind");  
		  close(s);  
		  return -1;  
	  }  

	return s;  
}

int qtk_usb_uevent_is_state(qtk_usb_uevent_t *uu, char *data,int len)
{
	int i=0,j=0;
	int keylen=0;
	int headcnt=0;

	while(i<len)
	{
		// printf("%d=[%c]\n",i,data[i]);
		if(data[i]=='=')
		{
			keylen=i-headcnt;
		}else if(data[i]=='\n')
		{
			if(keylen > 0)
			{
				// printf("%d %d %d=[%.*s] = [%.*s]\n",i,headcnt,keylen,keylen,data+headcnt,i-headcnt-keylen-1,data+headcnt+keylen+1);
                if(strncmp(qtk_usb_uevent_param_str[0], data+headcnt, keylen) == 0)
                {
                    printf("==>[%.*s]\n",i-headcnt-keylen-1,data+headcnt+keylen+1);
                    uu->action=data+headcnt+keylen+1;
                }else if(strncmp(qtk_usb_uevent_param_str[1], data+headcnt, keylen) == 0)
                {
                    printf("==>[%.*s]\n",i-headcnt-keylen-1,data+headcnt+keylen+1);
                    uu->devpath=data+headcnt+keylen+1;
                }else if(strncmp(qtk_usb_uevent_param_str[2], data+headcnt, keylen) == 0)
                {
                    printf("==>[%.*s]\n",i-headcnt-keylen-1,data+headcnt+keylen+1);
                    uu->subsystem=data+headcnt+keylen+1;
                    if(strncmp("u_audio",data+headcnt+keylen+1,i-headcnt-keylen-1) == 0)
                    {
                        uu->use_audio=1;
                    }else if(strncmp("android_usb",data+headcnt+keylen+1,i-headcnt-keylen-1) == 0)
                    {
                        uu->use_android_usb=1;
                    }
                }else if(strncmp(qtk_usb_uevent_param_str[3], data+headcnt, keylen) == 0)
                {
                    printf("==>[%.*s]\n",i-headcnt-keylen-1,data+headcnt+keylen+1);
                    uu->usb_state=data+headcnt+keylen+1;
                    if(strncmp(uu->usb_state, "DISCONNECTED", strlen("DISCONNECTED")) == 0)
                    {
                        if(uu->notify)
                        {
                            uu->notify(uu->ths, QTK_USB_STATE_SYSTEM_REBOOT, 0);
                        }
                    }else if(strncmp(uu->usb_state, "CONFIGURED", strlen("CONFIGURED")) == 0 && uu->use_android_usb==1)
                    {
                        if(uu->notify)
                        {
                            uu->use_android_usb=0;
                            uu->notify(uu->ths, QTK_USB_STATE_PULL_UP, 0);
                        }
                    }
                }else if(strncmp(qtk_usb_uevent_param_str[4], data+headcnt, keylen) == 0)
                {
                    printf("==>[%.*s]\n",i-headcnt-keylen-1,data+headcnt+keylen+1);
                    uu->stream_direction=data+headcnt+keylen+1;
                    if(strncmp("IN",data+headcnt+keylen+1,i-headcnt-keylen-1) == 0)
                    {
                        uu->use_stream_direction=1;
                    }
                }else if(strncmp(qtk_usb_uevent_param_str[5], data+headcnt, keylen) == 0)
                {
                    printf("==>[%.*s]\n",i-headcnt-keylen-1,data+headcnt+keylen+1);
                    uu->stream_state=data+headcnt+keylen+1;
                    if(strncmp("ON",data+headcnt+keylen+1,i-headcnt-keylen-1) == 0)
                    {
                        uu->use_stream_state=1;
                        if(uu->use_sample_rate && uu->use_audio && uu->use_stream_direction && uu->use_stream_state)
                        {
                            uu->notify(uu->ths, QTK_USB_STATE_PLAYER_START, uu->sample_rate);
                        }
                    }else if(strncmp("OFF",data+headcnt+keylen+1,i-headcnt-keylen-1) == 0)
                    {
                        if(uu->use_audio && uu->use_stream_direction)
                        {
                            uu->use_stream_state=0;
                            uu->use_sample_rate=0;
                            uu->use_audio=0;
                            uu->use_stream_direction=0;
                            uu->notify(uu->ths, QTK_USB_STATE_PLAYER_STOP, 0);
                        }
                    }
                }else if(strncmp(qtk_usb_uevent_param_str[6], data+headcnt, keylen) == 0)
                {
                    printf("==>[%.*s]\n",i-headcnt-keylen-1,data+headcnt+keylen+1);
                    uu->sample_rate=atoi(data+headcnt+keylen+1);
                    uu->use_sample_rate=1;
                    if(uu->use_audio && uu->use_stream_direction && uu->use_stream_state)
                    {
                        uu->notify(uu->ths, QTK_USB_STATE_PLAYER_START, uu->sample_rate);
                    }
                }
			}
			headcnt=i+1;
		}
		i++;
	}

	return 0;
}

qtk_usb_uevent_t * qtk_usb_uevent_new()
{
    qtk_usb_uevent_t *uu=NULL;
    uu=(qtk_usb_uevent_t *)wtk_malloc(sizeof(qtk_usb_uevent_t));
    if(!uu)
    {
        wtk_debug("qtk_usb_uevent new faild\n");
        uu=NULL;
    }
    uu->action=NULL;
    uu->devpath=NULL;
    uu->subsystem=NULL;
    uu->usb_state=NULL;
    uu->stream_direction=NULL;
    uu->stream_state=NULL;
    uu->sample_rate=16000;
    uu->uu_run=0;
    uu->use_audio=0;
    uu->use_android_usb=0;
    uu->use_stream_direction=0;
    uu->use_stream_state=0;
    uu->use_sample_rate=0;

    uu->msg = qtk_msg_new();
    wtk_blockqueue_init(&uu->msg_queue);
    wtk_thread_init(&uu->msg_t, (thread_route_handler)qtk_usb_uevent_on_msg, uu);
    uu->msg_run = 1;
    wtk_thread_start(&uu->msg_t);


    wtk_thread_init(&(uu->uu_t), (thread_route_handler)qtk_usb_uevent_on_run, uu);
    uu->uu_run = 1;
    wtk_thread_start(&uu->uu_t);

    return uu;
}
void qtk_usb_uevent_delete(qtk_usb_uevent_t *uu)
{
    uu->uu_run;
    wtk_thread_join(&uu->uu_t);

    uu->msg_run = 0;
    wtk_blockqueue_wake(&uu->msg_queue);
    wtk_thread_join(&uu->msg_t);

    wtk_blockqueue_clean(&uu->msg_queue);
    qtk_msg_delete(uu->msg);
    wtk_free(uu);
}

void qtk_usb_uevent_set_notify(qtk_usb_uevent_t *uu, void *ths, qtk_usb_uevent_notify_f notify)
{
    uu->ths = ths;
    uu->notify = notify;
}

int qtk_usb_uevent_on_msg(qtk_usb_uevent_t *uu, wtk_thread_t *t)
{
    qtk_msg_node_t *msg_node;
    wtk_queue_node_t *qn;

    while(uu->msg_run)
    {
		qn= wtk_blockqueue_pop(&uu->msg_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

        int ret=qtk_usb_uevent_is_state(uu, msg_node->buf->data, msg_node->buf->pos);

        qtk_msg_push_node(uu->msg, msg_node);
    }
}

int qtk_usb_uevent_on_run(qtk_usb_uevent_t *uu, wtk_thread_t *t)
{
    qtk_msg_node_t *msg_node;
    int len = 0; 
	char buf[UEVENT_BUFFER_SIZE * 2] = {0};/* Netlink message buffer */  
	struct sockaddr_nl nl;
	bzero((void *)&nl, (size_t)(sizeof(struct sockaddr_nl))); 

	int uevent_sock = qtk_usb_uevent_init_netlink_sock(&nl); 
	
#if (RECV_METHOD == USE_RECVMSG) || (RECV_METHOD == USE_NOBIND)
    struct iovec iov;

    struct msghdr msg;
    memset(&msg,0,sizeof(msg));
    iov.iov_base=(void *)buf;
    iov.iov_len=sizeof(buf);
    msg.msg_name=(void *)&nl;
    msg.msg_namelen=sizeof(nl);
    msg.msg_iov=&iov;
    msg.msg_iovlen=1;
#endif

	while(uu->uu_run)  
	{  
		bzero(buf,4096);
    #if (RECV_METHOD == USE_RECVMSG)
        len=recvmsg(uevent_sock,&msg,0);
        printf("recvmsg:");
    #else
        len= recv(uevent_sock, &buf, 4096, 0);  
        printf("recv:");
    #endif 
		int i = 0;
        for(i=0;i<len;i++)
        {
            if(*(buf+i)=='\0')
            {
                buf[i]='\n';
            }
        }

		printf("len:%d ---%s\n", len,buf);
		// int ret=qtk_usb_uevent_is_state(uu, buf, len);
        msg_node = qtk_msg_pop_node(uu->msg);
        wtk_strbuf_push(msg_node->buf, buf, len);
        wtk_blockqueue_push(&uu->msg_queue, &msg_node->qn);
	}

}
