#include "qtk_usb.h"

#ifdef USE_USB
static long count;
qtk_usb_rcdnode_t* qtk_usb_rcdnode_new(qtk_usb_t *u)
{
	qtk_usb_rcdnode_t *node;

	node=(qtk_usb_rcdnode_t*)wtk_malloc(sizeof(qtk_usb_rcdnode_t));
	node->v=wtk_string_new(u->cfg->buf_bytes);
	return node;
}

int qtk_usb_rcdnode_delete(qtk_usb_rcdnode_t *node)
{
	wtk_string_delete(node->v);
	wtk_free(node);
	return 0;
}

qtk_usb_plytask_t* qtk_usb_plynode_new(qtk_usb_t *u)
{
	qtk_usb_plytask_t *node;

	node=(qtk_usb_plytask_t*)wtk_malloc(sizeof(qtk_usb_plytask_t));
	node->buf=wtk_strbuf_new(u->cfg->buf_bytes,1);
	node->transfer=libusb_alloc_transfer(0);
	node->u=u;
	return node;
}

void qtk_usb_plynode_delete(qtk_usb_plytask_t *node)
{
	libusb_free_transfer(node->transfer);
	wtk_strbuf_delete(node->buf);
	wtk_free(node);
}

qtk_usb_rcdnode_t* qtk_usb_rcdnode_pop(qtk_usb_t *u)
{
	return wtk_lockhoard_pop(&(u->rcd_hoard));
}

void qtk_usb_rcdnode_push(qtk_usb_t *u,qtk_usb_rcdnode_t *node)
{
	wtk_lockhoard_push(&(u->rcd_hoard),node);
}

int qtk_usb_send_cmd(qtk_usb_t *u,int cmd)
{
	int ret;
	unsigned char byte;

    ret=libusb_control_transfer(u->dev_hd,
            LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE,
            cmd,
            0,
            0,
            &byte, 1, 1000);
    if(ret>0){
    	ret=0;
    }else{
    	ret=-1;
    }
    return ret;
}

int qtk_usb_cmd_by_channel_rate(int channel,int rate)
{
    int ret=0;

    switch(channel){
    case 1:
        ret=ret|0x30;
        break;
    case 2:
        ret=ret|0x40;
        break;
    }

    switch(rate){
    case 8000:
        ret|=1;
        break;
    case 11025:
        ret|=2;
        break;
    case 16000:
        ret|=3;
        break;
    case 22050:
        ret|=4;
        break;
    case 32000:
        ret|=5;
        break;
    case 44100:
        ret|=6;
        break;
    case 47250:
        ret|=7;
        break;
    case 48000:
        ret|=8;
        break;
    case 50000:
        ret|=9;
        break;
    case 96000:
        ret|=10;
        break;
    case 192000:
        ret|=11;
        break;
    default:
        break;
    }

    return ret;
}

void qtk_usb_rcdtask_cb(struct libusb_transfer *transfer)
{
	qtk_usb_rcdnode_t *node;
	qtk_usb_rcdtask_t *task;

//	wtk_debug("actual = %d.\n",transfer->actual_length)
	if(transfer->actual_length <= 0)
	{
		return;
	}
	task=(qtk_usb_rcdtask_t*)transfer->user_data;
	//wtk_debug("rcd_hint = %d.\n",task->u->rcd_hint)
	if(!task->u->rcd_hint)
	{
		if(task->u->run)
		{
			libusb_submit_transfer(transfer);
		}
		return;
	}
	node=qtk_usb_rcdnode_pop(task->u);
	memcpy(node->v->data,transfer->buffer,transfer->actual_length);
	node->v->len=transfer->actual_length;

	wtk_blockqueue_push(&(task->u->rcd_q),&(node->q_n));
	if(task->u->run)
	{
		libusb_submit_transfer(transfer);
	}
}

qtk_usb_rcdtask_t* qtk_usb_rcdtask_new(qtk_usb_t *u)
{
	qtk_usb_rcdtask_t *task;

	task=(qtk_usb_rcdtask_t*)wtk_malloc(sizeof(qtk_usb_rcdtask_t));
	task->u=u;
	task->len=u->cfg->buf_bytes;
	task->data=(char*)wtk_malloc(task->len);
	task->transfer=libusb_alloc_transfer(0);
	//wtk_debug("task->len = %d.\n",task->len);
	libusb_fill_bulk_transfer(task->transfer,u->dev_hd,u->cfg->in_point,
		(unsigned char*)task->data, task->len, (libusb_transfer_cb_fn)qtk_usb_rcdtask_cb, task, 0);
	return task;
}

void qtk_usb_rcdtask_delete(qtk_usb_rcdtask_t *task)
{
	libusb_free_transfer(task->transfer);
	wtk_free(task->data);
	wtk_free(task);
}

void qtk_usb_init_asyrcd(qtk_usb_t *u)
{
	int i;

	wtk_lockhoard_init(&(u->rcd_hoard),offsetof(qtk_usb_rcdnode_t,hoard_n),100,
			(wtk_new_handler_t)qtk_usb_rcdnode_new,(wtk_delete_handler_t)qtk_usb_rcdnode_delete,u);
	wtk_blockqueue_init(&(u->rcd_q));
	u->rcdtasks=(qtk_usb_rcdtask_t**)wtk_malloc(sizeof(qtk_usb_rcdtask_t*)*u->cfg->rcd_cache);
	//wtk_debug("rcd cache = %d.\n",u->cfg->rcd_cache);
	for(i=0;i<u->cfg->rcd_cache;++i)
	{
		u->rcdtasks[i]=qtk_usb_rcdtask_new(u);
		libusb_submit_transfer(u->rcdtasks[i]->transfer);
	}
	u->rcd_hint=0;
}

void qtk_usb_exit_asyrcd(qtk_usb_t *u)
{
	int i;
	struct timeval tv2 = { 0, 0};

	for(i=0;i<u->cfg->rcd_cache;++i)
	{
		libusb_cancel_transfer(u->rcdtasks[i]->transfer);
		libusb_handle_events_timeout(u->ctx,&tv2);
		qtk_usb_rcdtask_delete(u->rcdtasks[i]);
	}
	wtk_blockqueue_clean(&(u->rcd_q));
	wtk_lockhoard_clean(&(u->rcd_hoard));
	wtk_free(u->rcdtasks);
}

void qtk_usb_init_asyply(qtk_usb_t *u)
{
	int i;

	wtk_blockqueue_init(&(u->ply_q));
	u->plytasks=(qtk_usb_plytask_t**)wtk_malloc(sizeof(qtk_usb_plytask_t*)*u->cfg->ply_cache);
	for(i=0;i<u->cfg->ply_cache;++i)
	{
		u->plytasks[i]=qtk_usb_plynode_new(u);
		wtk_blockqueue_push(&(u->ply_q),&(u->plytasks[i]->q_n));
	}
}

void qtk_usb_exit_asyply(qtk_usb_t *u)
{
	int i;
	struct timeval tv2 = { 0, 0};

	for(i=0;i<u->cfg->ply_cache;++i)
	{
		libusb_cancel_transfer(u->plytasks[i]->transfer);
		libusb_handle_events_timeout(u->ctx,&tv2);
		qtk_usb_plynode_delete(u->plytasks[i]);
	}
	wtk_free(u->plytasks);
	wtk_blockqueue_clean(&(u->ply_q));
}


#ifdef WIN32
int qtk_usb_run(qtk_usb_t *u,wtk_thread_t *t)
{
	int ret;

	while (u->run)
	{
		ret = libusb_handle_events(u->ctx);
		if (ret != LIBUSB_SUCCESS)
		{
			break;
		}
	}
	return 0;
}
#else
int qtk_usb_run(qtk_usb_t *u,wtk_thread_t *t)
{
	struct timeval tv;
	struct timeval tv2 = { 0, 0};
	int ret;
	const struct libusb_pollfd **fds;
	int fd;
	int i;
	fd_set *r_set,*w_set,*e_set;
	fd_set *tr_set,*tw_set,*te_set;

	r_set=(fd_set*)wtk_calloc(1,sizeof(fd_set));
	w_set=(fd_set*)wtk_calloc(1,sizeof(fd_set));
	e_set=(fd_set*)wtk_calloc(1,sizeof(fd_set));
	FD_ZERO(r_set);
	tr_set=(fd_set*)wtk_calloc(1,sizeof(fd_set));
	tw_set=(fd_set*)wtk_calloc(1,sizeof(fd_set));
	te_set=(fd_set*)wtk_calloc(1,sizeof(fd_set));
	fds=libusb_get_pollfds(u->ctx);
	i=0;
	fd=0;
	while(1)
	{
		if(!fds[i]){
			break;
		}
		//wtk_debug("%d.\n",fds[i]->fd);
		if(fds[i]->fd>fd){
			fd=fds[i]->fd;
		}else{
			++i;continue;
		}
		if(fds[i]->events & 0x004){
			FD_SET(fds[i]->fd,w_set);
		}
		if(fds[i]->events & 0x001){
			FD_SET(fds[i]->fd,r_set);
		}
		FD_SET(fds[i]->fd,e_set);
		++i;
	}
	fd+=1;
	while(u->run)
	{
		*tr_set = *r_set;
		*tw_set = *w_set;
		*te_set = *e_set;
		tv.tv_sec=0;
		tv.tv_usec=500000;
		ret=select(fd,tr_set,tw_set,te_set,&tv);
//		wtk_debug("ret=%d.\n",ret);
		if(ret>0)
		{
			ret=libusb_handle_events_timeout(u->ctx,&tv2);
		}
	}
	ret=libusb_handle_events_timeout(u->ctx,&tv2);
	wtk_free(r_set);
	wtk_free(w_set);
	wtk_free(e_set);
	wtk_free(tr_set);
	wtk_free(tw_set);
	wtk_free(te_set);
	libusb_free_pollfds(fds);
	return 0;
}
#endif

void qtk_usb_plytask_cb(struct libusb_transfer *transfer)
{
	qtk_usb_plytask_t *task;

	task=(qtk_usb_plytask_t*)transfer->user_data;
	if(task->u->run)
	{
		wtk_blockqueue_push(&(task->u->ply_q),&(task->q_n));
	}
}

int qtk_usb_play_asy(qtk_usb_t *u,char *data,int bytes)
{
	qtk_usb_plytask_t *node;
	wtk_queue_node_t *qn = NULL;
	int ret=0;
	char *s,*e;
	int n;

	s=data;
	e=data+bytes;
	while(s<e)
	{
		n=min(u->cfg->ply_step,e-s);
		qn = wtk_blockqueue_pop(&u->ply_q,u->cfg->buf_time*2,NULL);
		if(!qn) {
			return -1;
		}
		node = data_offset2(qn,qtk_usb_plytask_t,q_n);
		wtk_strbuf_reset(node->buf);
		wtk_strbuf_push(node->buf,s,n);
		libusb_fill_bulk_transfer(node->transfer,u->dev_hd,u->cfg->out_point,
				(unsigned char*)node->buf->data,node->buf->pos,
				(libusb_transfer_cb_fn)qtk_usb_plytask_cb, node, 0);
		ret=libusb_submit_transfer(node->transfer);
		if(ret!=0)
		{
			ret=-1;
			break;
		}
		s+=n;
	}
	if(ret==0){
		ret=bytes;
	}
	return ret;
}

int qtk_usb_play_syn(qtk_usb_t *u,char *data,int bytes)
{
	int cnt;
	int actual,ret=0;
	char *s,*e;
	int n;

	s=data;e=data+bytes;
	while(s<e)
	{
		cnt=0;
		n=min(u->cfg->ply_step,e-s);
		do{
			ret=libusb_bulk_transfer(u->dev_hd,u->cfg->out_point,(unsigned char*)s,n,&actual,u->cfg->timeout);
			++cnt;
		}while(ret!=0 && cnt <10);
		if(ret!=0){
			ret=-1;
			break;
		}
		s+=actual;
	}
	if(ret==0)
	{
		ret=bytes;
	}
	return ret;
}

wtk_strbuf_t* qtk_usb_record_asy(qtk_usb_t *u)
{
	wtk_queue_node_t *qn;
	qtk_usb_rcdnode_t *node;
	wtk_strbuf_t  *buf=u->rcdtmp_buf;

	wtk_strbuf_reset(buf);
	qn=wtk_blockqueue_pop(&(u->rcd_q),100,NULL);
	if(qn){
		node=data_offset2(qn,qtk_usb_rcdnode_t,q_n);
		wtk_strbuf_push(buf,node->v->data,node->v->len);
		qtk_usb_rcdnode_push(u,node);
	}
	return buf;
}

wtk_strbuf_t* qtk_usb_record_syn(qtk_usb_t *u)
{
	wtk_strbuf_t *buf=u->rcdtmp_buf;
	int ret,actual;

	actual=0;
//	wtk_debug("====================================================> bulk\n");
	ret=libusb_bulk_transfer(u->dev_hd,u->cfg->in_point,(unsigned char*)buf->data,buf->length,&actual,2000);
//	wtk_debug("====================================================> ret = %d\n",ret);
	if(ret==0){
		buf->pos=actual;
	}else{
		buf->pos=0;
	}
	return buf;
}
#endif

qtk_usb_t* qtk_usb_new(qtk_usb_cfg_t *cfg,qtk_session_t *session,void *notify_ths,qtk_usb_notify_f notify_f)
{
#ifdef USE_USB
	qtk_usb_t *u;
	int ret;

	u=(qtk_usb_t*)wtk_malloc(sizeof(qtk_usb_t));


	u->cfg=cfg;
	u->session=session;
	u->notify_ths = notify_ths;
	u->notify_f = notify_f;
	u->rcd_buf = NULL;
	u->ctx = NULL;
	u->rcdtmp_buf = NULL;
	u->err=0;
	ret = wtk_lock_init(&u->lock);
	if(ret != 0) {
		wtk_log_warn0(u->session->log,"init lock failed\n");
		goto end;
	}
	ret = libusb_init(&u->ctx);
	if(ret != 0) {
		wtk_log_warn0(u->session->log,"libusb init failed\n");
		goto end;
	}
	u->rcd_buf=wtk_strbuf_new(cfg->buf_bytes,1);
	u->rcdtmp_buf=wtk_strbuf_new(cfg->buf_bytes,1);
	ret = qtk_usb_start(u);
		if(ret != 0) {
			wtk_log_warn0(u->session->log,"usb start failed\n");
			goto end;
		}

		ret = 0;
	end:
		if(ret != 0) {
			qtk_usb_delete(u);
			u = NULL;
		}
		return u;
#else
	wtk_log_warn0(session->log,"USE_USB not specified");
	_qtk_error(session,_QTK_USB_UNSUPPORT);
	return 0;
#endif
}

int qtk_usb_delete(qtk_usb_t *u)
{
#ifdef USE_USB
	if(u->run) {
		qtk_usb_stop(u);
	}
	if(u->rcd_buf) {
		wtk_strbuf_delete(u->rcd_buf);
	}
	if(u->rcdtmp_buf) {
		wtk_strbuf_delete(u->rcdtmp_buf);
	}
	if(u->ctx) {
		libusb_exit(u->ctx);
	}
	wtk_lock_clean(&u->lock);
	wtk_free(u);
#endif
	return 0;
}

int qtk_usb_play_start(qtk_usb_t *u,char *sd_name,int sample_rate,int channel,int bytes_per_sample)
{
#ifdef USE_USB
	int ret;
	wtk_log_log(u->session->log,"sampl_rate %d channel %d bytes_per_sample %d",
			sample_rate,channel,bytes_per_sample
			);

	u->channel=channel;
	if(channel > 2){
		ret=-1;goto end;
	}
	if(sample_rate!=16000){
		ret=-1;goto end;
	}
	if(bytes_per_sample!=2){
		ret=-1;goto end;
	}
	//start  recovery
	ret=qtk_usb_cmd_by_channel_rate(channel,sample_rate);
	qtk_usb_send_cmd(u,ret);
	//wtk_debug("u->cfg->use_adjust = %d. u->cfg->ply_volume = %d\n",u->cfg->use_adjust,u->cfg->ply_volume);
	wtk_log_log(u->session->log,"use_adjust %d ply_volume %d use_3_cb_start %d",
			u->cfg->use_adjust,u->cfg->ply_volume,u->cfg->use_3_cb_start
			);

	if(u->cfg->use_adjust) {
		qtk_usb_send_cmd(u,u->cfg->ply_volume);
	}
	if(u->cfg->use_3_cb_start) {
		//wtk_debug("start callback.\n");
		qtk_usb_send_cmd(u,3);
	}
	ret=0;
end:
	return ret;
#else
	return 0;
#endif
}

int qtk_usb_play_write(qtk_usb_t *u,char *data,int bytes)
{
#ifdef USE_USB
	short *pv=NULL,*pv1;
	int i,j,n;
	int ret;

	if(u->channel==1)
	{
		pv=(short*)wtk_malloc(sizeof(short)*bytes);
		pv1=(short*)data;
		n=bytes>>1;
		for(i=0,j=0;i<n;++i,j+=2)
		{
			pv[j]=pv[j+1]=pv1[i];
		}
		data=(char*)pv;
		bytes=bytes<<1;
	}

	if(u->cfg->use_asy_ply)
	{
		ret=qtk_usb_play_asy(u,data,bytes);
	}else{
		ret=qtk_usb_play_syn(u,data,bytes);
	}
	if(pv)
	{
		if(ret>0){
			ret=ret>>1;
		}
		wtk_free(pv);
	}
	if(ret<=0){
//		wtk_debug("===>播放write failed 开始重启\n");
//		wtk_log_log(u->session->log,"usb play write failed && ret = %d\n",ret);
//		u->notify_f(u->notify_ths,1);
	}
	return ret;
#else
	return 0;
#endif
}

void qtk_usb_play_stop(qtk_usb_t *u)
{
#ifdef USE_USB
	wtk_log_log0(u->session->log,"play stop");
	//qtk_usb_send_cmd(u,2);
#endif
}

int qtk_usb_record_start(qtk_usb_t *u)
{
#ifdef USE_USB
	wtk_log_log(u->session->log,"u->cfg->use_adjust = %d. mic_gain = %d. cb_gain = %d",
			u->cfg->use_adjust,u->cfg->mic_gain,u->cfg->cb_gain
			);
	if(u->cfg->use_adjust)
	{
		qtk_usb_send_cmd(u,0);
		qtk_usb_send_cmd(u,u->cfg->mic_gain);
		qtk_usb_send_cmd(u,1);
		qtk_usb_send_cmd(u,u->cfg->cb_gain);
	}
	u->rcd_hint=1;
#endif
	return 0;
}

wtk_strbuf_t* qtk_usb_record_read(qtk_usb_t *u)
{
#ifdef USE_USB
	wtk_strbuf_t *tmp_buf,*buf;
	short *pv,*pv1;
	int i,n,channel,j,k,m,ki,b;
	int *skip_channel=NULL;
	int nskip;

	if(u->cfg->use_asy_rcd){
		tmp_buf=qtk_usb_record_asy(u);
	}else{
		tmp_buf=qtk_usb_record_syn(u);
	}

	skip_channel=u->cfg->skip_channels;
	//wtk_debug("%p\n",skip_channel);
	if(skip_channel)
	{
		buf=u->rcd_buf;
		nskip=u->cfg->nskip;
		pv=(short*)tmp_buf->data;
		pv1=(short*)buf->data;
		channel=u->cfg->channel;
		n=tmp_buf->pos/(2*channel);
		k=m=0;
		for(i=0;i<n;++i)
		{
			for(j=0;j<channel;++j)
			{
				b=0;
				for(ki=0;ki<nskip;++ki)
				{
					if(j==skip_channel[ki])
					{
						b=1;
						break;
					}
				}
				if(b)
				{
					++m;
				}else{
					pv1[k++]=pv[m++];
				}
			}
		}
		buf->pos=k*2;
//		wtk_debug("=====>pos  = %d\n",buf->pos);
		count += buf->pos;
//		wtk_debug("=====>count  = %ld\n",count);
		if(count > 1000000){
//			wtk_debug("========================>录音   主动开始重启\n");
//			u->notify_f(u->notify_ths,1);
			count = 0;
		}
		//wtk_debug("k=%d.\n",k);
	}else{
		buf=tmp_buf;
	}
	if(buf->pos==0){
//		wtk_debug("===>录音 read failed 开始重启\n");
		wtk_log_log(u->session->log,"usb record reed failed && buf->pos = %d\n",buf->pos);
//		u->notify_f(u->notify_ths,1);
	}
	return buf;
#else
	return 0;
#endif
}

void qtk_usb_record_stop(qtk_usb_t *u)
{
#ifdef USE_USB
	u->rcd_hint=0;
#endif
}

#ifdef USE_USB
static int qtk_usb_open_dev(qtk_usb_t *u)
{
	libusb_device **devs = NULL;
	int devs_num;
	int ret;
	//int endpoints[]={0x81,0x01,0x82};
	//int endpoints[]={0x81,0x01,0x82};
	int endpoints[]={0x00,0x01};
	int i,n;

	devs_num = libusb_get_device_list(u->ctx,&devs);
	if(devs_num <= 0) {
		wtk_debug("usb device get failed\n");
		ret = -1;
		goto end;
	}
	u->dev_hd = libusb_open_device_with_vid_pid(u->ctx,u->cfg->vendor_id,u->cfg->product_id);
	if(!u->dev_hd) {
		if(u->cfg->debug){
			wtk_debug("open usb [%#x  %#x] failed\n",u->cfg->vendor_id,u->cfg->product_id);
		}
		_qtk_error(u->session,_QTK_USB_OPEN_FAILED);
		ret = -1;
		goto end;
	}

	n = sizeof(endpoints) / sizeof(int);
	for(i=0;i<n;++i) {
		ret = libusb_kernel_driver_active(u->dev_hd,endpoints[i]);
		if(ret == 1) {
			ret = libusb_detach_kernel_driver(u->dev_hd,endpoints[i]);
			if(ret != 0) {
				wtk_debug("detach ret = %d\n",ret);
				goto end;
			}
		}

		ret = libusb_claim_interface(u->dev_hd,endpoints[i]);
		if(ret != 0) {
			wtk_debug("claim %#x failed\n",endpoints[i]);
			_qtk_error(u->session,_QTK_USB_OPEN_FAILED);
			goto end;
		}
	}

	ret = 0;
end:
	if(devs) {
		libusb_free_device_list(devs,1);
	}
	return ret;
}

static void qtk_usb_close_dev(qtk_usb_t *u)
{
	//int endpoints[]={0x81,0x01,0x82};
	int endpoints[]={0x00,0x01};
	int i,n;

	if(u->dev_hd)
	{
		n = sizeof(endpoints) / sizeof(int);
		for(i=0;i<n;++i) {
			libusb_release_interface(u->dev_hd,endpoints[i]);
			libusb_attach_kernel_driver(u->dev_hd,endpoints[i]);
		}
		libusb_close(u->dev_hd);
		u->dev_hd = NULL;
	}
}

#endif
int qtk_usb_start(qtk_usb_t *u)
{
	int ret = 0;

#ifdef USE_USB
	ret = wtk_lock_lock(&u->lock);
	if(ret != 0) {
		return -1;
	}

	ret = qtk_usb_open_dev(u);
	if(ret != 0) {
		u->err = 1;
		goto end;
	}

	if(u->cfg->use_asy_rcd) {
		qtk_usb_init_asyrcd(u);
	}
	if(u->cfg->use_asy_ply) {
		qtk_usb_init_asyply(u);
	}

	u->run = 1;
	if(u->cfg->use_asy_rcd || u->cfg->use_asy_ply) {
		wtk_thread_init(&u->thread,(thread_route_handler)qtk_usb_run,u);
		wtk_thread_set_name(&u->thread,"usb");
		ret = wtk_thread_start(&u->thread);
		if(ret != 0) {
			wtk_debug("usb thread start failed\n");
			goto end;
		}
	}

	ret = 0;
end:
	wtk_lock_unlock(&u->lock);
#endif
	return ret;
}

int qtk_usb_stop(qtk_usb_t *u)
{
#ifdef USE_USB
	int ret;

	ret = wtk_lock_lock(&u->lock);
	if(ret != 0) {
		return -1;
	}
	if(u->err == 1){
		qtk_usb_close_dev(u);
		wtk_lock_unlock(&u->lock);
		return 0;
	}
	u->run = 0;
	if((u->cfg->use_asy_rcd || u->cfg->use_asy_ply)) {
		wtk_thread_join(&u->thread);
		wtk_thread_clean(&u->thread);
	}
	if(u->cfg->use_asy_ply) {
		qtk_usb_exit_asyply(u);
	}
	if(u->cfg->use_asy_rcd) {
		qtk_usb_exit_asyrcd(u);
	}
	qtk_usb_close_dev(u);
	wtk_lock_unlock(&u->lock);
#endif
	return 0;
}
