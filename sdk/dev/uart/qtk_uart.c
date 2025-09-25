#include "qtk_uart.h"

qtk_uart_t *qtk_uart_new(qtk_uart_cfg_t *cfg)
{
	qtk_uart_t *s;
	int ret;

	s = (qtk_uart_t *)wtk_calloc(1, sizeof(qtk_uart_t));
    s->use_log = 0;
    s->cfg=cfg;
    if(cfg->uart_log == NULL && s->cfg->use_uart_log)
    {
        cfg->uart_log = wtk_log_new(cfg->log_fn.data);
        s->use_log = 1;
    }
	s->fd = open(cfg->dev.data,O_RDWR|O_NOCTTY|O_NDELAY);
    if(s->fd == -1){
        wtk_debug("open uart failed: %s \n", strerror(errno));
        wtk_log_log(s->cfg->uart_log, "open uart failed: %s \n", strerror(errno));
        ret = -1;
        goto end;
    }
    
    /*清除串口非阻塞标志*/
    if(fcntl(s->fd,F_SETFL,0) < 0)
    {
    	wtk_debug("fcntl failed!\n");
        wtk_log_log0(s->cfg->uart_log, "fcntl failed!\n");
        ret = -1;
        goto end;
    }
    ret = qtk_uart_set_config(s,cfg->baude, cfg->c_flow, cfg->bits, cfg->parity, cfg->stop);
    if(ret != 0)
    {
        wtk_debug("set config failed!\n");
        wtk_log_log0(s->cfg->uart_log, "set config failed!\n");
        ret = -1;
        goto end;
    }
    ret = 0;
end:
   	  if(ret != 0){
   		qtk_uart_delete(s);
   		s = NULL;
   	  }
   	  return s;
}
int qtk_uart_delete(qtk_uart_t *s)
{
	if(s->fd > 0){
        assert(s->fd);
	    close(s->fd);
	}
    if(s->use_log && s->cfg->uart_log)
    {
        wtk_log_delete(s->cfg->uart_log);
    }
	wtk_free(s);
	return 0;
}
int qtk_uart_set_baud(int fd, int baudrate)
{
    struct termios opt;
    memset(&opt, 0, sizeof(opt));
    if(tcgetattr(fd, &opt) == -1) {
        wtk_debug("get fd cofiguration failed\n");
        return -1;
    }
    switch(baudrate) {
        case 300:	
            cfsetispeed(&opt,B300); 
            cfsetospeed(&opt,B300);
            opt.c_cflag |= B300 | CBAUD;
            break;
		case 600:	
            cfsetispeed(&opt,B600);
            cfsetospeed(&opt,B600);
            opt.c_cflag |= B600 | CBAUD;
            break;
		case 1200:	
            cfsetispeed(&opt,B1200);
            cfsetospeed(&opt,B1200);
            opt.c_cflag |= B1200 | CBAUD;
            break;
		case 1800:
            cfsetispeed(&opt,B1800);
            cfsetospeed(&opt,B1800);
            opt.c_cflag |= B1800 | CBAUD;
            break;
		case 2400:	
            cfsetispeed(&opt,B2400);
            cfsetospeed(&opt,B2400);
            opt.c_cflag |= B2400 | CBAUD;
            break;
		case 4800:	
            cfsetispeed(&opt,B4800);
            cfsetospeed(&opt,B4800);
            opt.c_cflag |= B4800 | CBAUD;
            break;
		case 9600:  
		    cfsetispeed(&opt,B9600);
            cfsetospeed(&opt,B9600);
            opt.c_cflag |= B9600 | CBAUD;
            break;
		case 19200:	
            cfsetispeed(&opt,B19200);
            cfsetospeed(&opt,B19200);
            opt.c_cflag |= B19200 | CBAUD;
            break;
		case 38400:	
            cfsetispeed(&opt,B38400);
            cfsetospeed(&opt,B38400);
            opt.c_cflag |= B38400 | CBAUD;
            break;
		case 115200:	
            cfsetispeed(&opt,B115200);
            cfsetospeed(&opt,B115200);
            opt.c_cflag |= B115200 | CBAUDEX;
            break;
		case 230400:	
            cfsetispeed(&opt,B230400);
            cfsetospeed(&opt,B230400);
            opt.c_cflag |= B230400 | CBAUDEX;
            break;
		case 460800:	
            cfsetispeed(&opt,B460800);
            cfsetospeed(&opt,B460800);
            opt.c_cflag |= B460800 | CBAUDEX;
            break;
		case 500000:	
            cfsetispeed(&opt,B500000);
            cfsetospeed(&opt,B500000);
            opt.c_cflag |= B500000 | CBAUDEX;
            break;
		case 576000:	
            cfsetispeed(&opt,B576000);
            cfsetospeed(&opt,B576000);
            opt.c_cflag |= B576000 | CBAUDEX;
            break;
		case 921600:	
            cfsetispeed(&opt,B921600);
            cfsetospeed(&opt,B921600);
            opt.c_cflag |= B921600 | CBAUDEX;
            break;
		case 1000000:	
            cfsetispeed(&opt,B1000000);
            cfsetospeed(&opt,B1000000);
            opt.c_cflag |= B1000000 | CBAUDEX;
            break;
		case 1152000:	
            cfsetispeed(&opt,B1152000);
            cfsetospeed(&opt,B1152000);
            opt.c_cflag |= B1152000 | CBAUDEX;
            break;
		case 1500000:	
            cfsetispeed(&opt,B1500000);
            cfsetospeed(&opt,B1500000);
            opt.c_cflag |= B1500000 | CBAUDEX;
            break;
		case 2000000:	
            cfsetispeed(&opt,B2000000);
            cfsetospeed(&opt,B2000000);
            opt.c_cflag |= B2000000 | CBAUDEX;
            break;
		case 2500000:	
            cfsetispeed(&opt,B2500000);
            cfsetospeed(&opt,B2500000);
            opt.c_cflag |= B2500000 | CBAUDEX;
            break;
		case 3000000:	
            cfsetispeed(&opt,B3000000);
            cfsetospeed(&opt,B3000000);
            opt.c_cflag |= B3000000 | CBAUDEX;
            break;
		case 3500000:	
            cfsetispeed(&opt,B3500000);
            cfsetospeed(&opt,B3500000);
            opt.c_cflag |= B3500000 | CBAUDEX;
			break;
    }
    if(tcflush(fd, TCOFLUSH) == -1) {
        wtk_debug("configure flush failed\n");
        return -1;
    }
    if(tcsetattr(fd, TCSANOW, &opt) == -1) {
        wtk_debug("configure serial port failed\n");
        return -1;
    }
    return 0;
}

int qtk_uart_set_bits(int fd, int data_bit, int stop_bit, int parity, int flow_ctrl)
{
    struct termios opt;
    memset(&opt, 0, sizeof(opt));
   if(tcgetattr(fd, &opt) == -1) {
        wtk_debug("get fd cofiguration failed\n");
        return -1;
    }
    /* Enable or Disable Hardware flow control. */
	switch(flow_ctrl){
		case 0:
			opt.c_cflag &= ~CRTSCTS;
			break;
		case 1:
			opt.c_cflag |= CRTSCTS;
			break;
		case 2:
			 opt.c_cflag |= IXON|IXOFF|IXANY;
			 break;
	}
    /* 设置校检位 */
    switch(parity) {
        case 0: // 无校检
            opt.c_cflag &= ~PARENB;
            opt.c_iflag &= ~INPCK;
            break;
        case 1: // 奇校检
            opt.c_cflag |= (PARODD | PARENB);
            opt.c_iflag |= (INPCK | ISTRIP);
            break;
        case 2: // 偶校检
            opt.c_cflag |= ~PARENB;
            opt.c_cflag &= ~PARODD;
            opt.c_iflag |= (INPCK | ISTRIP);
            break;
        case 3: //空格
            opt.c_cflag &= ~PARENB;
            opt.c_cflag &= ~CSTOPB;
        default:
            wtk_debug("skip check flag : %d\n", parity);
            break;
    }

    /* 设置停止位 */
    switch(stop_bit) {
        case 1: 
            opt.c_cflag &= ~CSTOPB;  // 1位停止位
            break;
        case 2:
            opt.c_cflag |= CSTOPB;//CSTOPB：使用两位停止位
            break;
        default:
            wtk_debug("unsupported control flag : %d\n", stop_bit);
            break;
    }

    /* 设置数据位 */
    opt.c_cflag &= ~CSIZE;   // 屏蔽其他标志 & 0060
    switch(data_bit) {
        case 5:
            opt.c_cflag |= CS5;
            break;
        case 6:
            opt.c_cflag |= CS6;
            break;
        case 7:
            opt.c_cflag |= CS7;
            break;
        case 8:
            opt.c_cflag |= CS8;
            break;
        default:
            wtk_debug("data flag exclude %d\n", data_bit);
				break;
    }
    
    /* Hardware Control Options - Set local mode and Enable receiver to receive characters */
    opt.c_cflag     |= (CLOCAL | CREAD );// 保证程序不占用串口、保证程序可以从串口读数据
    /* Terminal Control options */
        /* - Disable signals. Disable canonical input processing. Disable echo. */
    opt.c_lflag     &= ~(ICANON | IEXTEN | ECHO | ISIG); /* Line options - Raw input 设置本地模式为原始模式*/
        
	opt.c_iflag     &= ~(ICRNL | INPCK | ISTRIP | BRKINT | IXON | IXOFF | IXANY);
    /* Output processing - Disable post processing of output. */
    opt.c_oflag     &= ~OPOST;      /* Output options - Raw output 设置输出模式为原始输出*/
    /* Control Characters - Min. no. of characters 设置最小接受字符*/
    opt.c_cc[VMIN]  = 0;
    /* Character/Packet timeouts. 设置等待时间*/
    opt.c_cc[VTIME] = 3;

    /* 如果发生数据移除，只发送数据，但是不进行写操作 */
    if(tcflush(fd, TCOFLUSH) == -1) {
        wtk_debug("configure flush failed\n");
        return -1;
    }
    if(tcsetattr(fd, TCSANOW, &opt) == -1) {
        wtk_debug("configure serial port failed\n");
        return -1;
    }
    return 0;
}
int qtk_uart_set_config(qtk_uart_t * s, int baude,int c_flow,int bits,char parity,int stop)
{
	wtk_debug("baude = %d\n", baude);
    int ret=0;
	ret = qtk_uart_set_baud(s->fd, baude);
    if(ret!= 0){goto end;}
	ret = qtk_uart_set_bits(s->fd, bits, stop, parity, c_flow);
    
end:
	return ret;
}

ssize_t safe_read(int fd,void *vptr,size_t n)
{
    size_t nleft;
    ssize_t nread;
    char *ptr;

    ptr=vptr;
    nleft=n;

    while(nleft > 0)
    {
        if((nread = read(fd,ptr,nleft)) <0)
        {
            if(errno == EINTR){
                nread = 0;
			}else if(nread == 0){
				break;
			}
		}
		// wtk_debug("nread = %d\n", nread);
        nleft -= nread;
        ptr += nread;
    }
    return (n-nleft);
}
int qtk_uart_read(qtk_uart_t *s, char *buf, int len)
{
	ssize_t cnt = 0;
    fd_set rfds;
    struct timeval time;
	int ret;

    FD_ZERO(&rfds);
    FD_SET(s->fd,&rfds);

    time.tv_sec = 1;
    time.tv_usec = 0;

    ret = select(s->fd+1,&rfds,NULL,NULL,&time);
    switch(ret)
    {
        case -1:
            fprintf(stderr,"select error!\n");
            wtk_log_log(s->cfg->uart_log, "select error %d\n",ret);
            return -1;
        case 0:
            fprintf(stderr,"time over!\n");
            return -1;
        default:
			 //wtk_debug(">>>>len = %d\n", len);
            cnt = safe_read(s->fd,buf,len);
            if(cnt == -1)
            {
                fprintf(stderr,"read error!\n");
                wtk_log_log(s->cfg->uart_log, "read error %d",cnt);
                return -1;
            }
			// wtk_debug("recv cnt = %d: ");
			// for (int i = 0; i < cnt; i++) {
			// 	printf("%02x ", buf[i]);
			// }
			// printf("\n");

			//wtk_debug("recv:ret = %d %.*s\n",(int)cnt, (int)cnt, buf);
            return cnt;
    }
}

ssize_t safe_write(int fd, const void *vptr, size_t n)
{
    size_t  nleft;
    ssize_t nwritten;
    const char *ptr;

    ptr = vptr;
    nleft = n;
    while(nleft > 0)
    {
        if((nwritten = write(fd, ptr, nleft)) < 0)
        {
            if(nwritten < 0&&errno == EINTR)
                nwritten = 0;
            else
                return -1;
        }
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n);
}
int qtk_uart_write(qtk_uart_t *s, const char *data, int len)
{
 	ssize_t cnt = 0;
    // wtk_debug(">>>>>>>>>>>>>>>>write: %.*s\n", len, data);
    cnt = safe_write(s->fd,data,len);
    if(cnt == -1)
    {
        fprintf(stderr,"write error!\n");
        wtk_log_log(s->cfg->uart_log, "write error %d",cnt);
        return -1;
    }

    return cnt;
}

int qtk_uart_write2(qtk_uart_t *s, const char *data, int len)
{
    int pos=0,errcnt=0;
    int ret,nx,step=s->cfg->step_size;//25600;
    wtk_debug("step=%d\n",step);
    #if 0
	    qtk_uart_write(s,(char *)(&len), sizeof(len));
    #endif
	while(pos < len)
	{
		nx=min(step,len-pos);
		ret = qtk_uart_write(s, data+pos, nx);
	//	wtk_debug("write %d pos = %d\n", ret,pos);
		if(ret < 0){
			wtk_debug("write failed.\n");
            wtk_log_log(s->cfg->uart_log, "write error %d",ret);
            errcnt++;
            if(errcnt > 10)
            {
                return -1;
            }
            continue;
		}
		pos += nx;
		wtk_msleep(1);
	}
#if 0
    return pos+9;
#endif
    return pos;
}

int qtk_uart_doa_updata(qtk_uart_t *uart)
{
    qtk_doa_t doa;
    doa.stage=0;
    doa.prefix=0;
    doa.type=0;
    doa.size=0;
    doa.id=0;
    memset(doa.data,0,sizeof(doa.data)); 
    doa.check=0;
    doa.suffix=0;
    doa.available=0;
    doa.index=0;
    memset(doa.buffer,0,sizeof(doa.buffer));
    uart->doa=&doa;
    // doa = (qtk_doa_t *)wtk_calloc(1, sizeof(qtk_doa_t));
}
