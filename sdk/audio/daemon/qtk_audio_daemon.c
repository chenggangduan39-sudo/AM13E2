#include "qtk_audio_daemon.h"

static qtk_audio_daemon_msg_t* qtk_audio_daemon_msg_new(qtk_audio_daemon_t *ad);
static int qtk_audio_daemon_msg_delete(qtk_audio_daemon_msg_t *msg);
#ifdef USE_USB
static qtk_audio_daemon_msg_t* qtk_audio_daemon_pop_msg(qtk_audio_daemon_t *ad);
static void qtk_audio_daemon_push_msg(qtk_audio_daemon_t *ad,qtk_audio_daemon_msg_t *msg);
static void qtk_audio_daemon_clean_msg_q(qtk_audio_daemon_t *ad);

static void qtk_audio_daemon_notify(qtk_audio_daemon_t *ad,qtk_audio_daemon_cmd_t cmd);
static int qtk_audio_daemon_on_hotplug_cb(struct libusb_context *ctx,
		struct libusb_device *dev,
		libusb_hotplug_event event,
		qtk_audio_daemon_t *ad
		);
static void qtk_audio_daemon_on_pipe(qtk_audio_daemon_t *ad);
#endif
static int qtk_audio_daemon_run(qtk_audio_daemon_t *ad);


qtk_audio_daemon_t* qtk_audio_daemon_new(qtk_audio_daemon_cfg_t *cfg,qtk_session_t *session)
{
	qtk_audio_daemon_t *ad;
	int ret;

	ad = (qtk_audio_daemon_t*)wtk_malloc(sizeof(*ad));
	if(!ad) {
		wtk_debug("audio daemon malloc failed\n");
		return NULL;
	}
	memset(ad,0,sizeof(*ad));

	ad->cfg = cfg;
	ad->session = session;
	ad->vid = cfg->vendor_id;
	ad->pid = cfg->product_id;
	ad->state = QTK_AUDIO_DAEMON_STATE_ARRIVE;

	ret = wtk_pipequeue_init(&ad->msg_q);
	if(ret != 0) {
		goto end;
	}
	wtk_lockhoard_init(&ad->msg_hoard,offsetof(qtk_audio_daemon_msg_t,hoard_n),2,
			(wtk_new_handler_t)qtk_audio_daemon_msg_new,
			(wtk_delete_handler_t)qtk_audio_daemon_msg_delete,
			ad
			);

	wtk_thread_init(&ad->thread,(thread_route_handler)qtk_audio_daemon_run,ad);
	wtk_thread_set_name(&ad->thread,"audio daemon");

	ret = 0;
end:
	if(ret != 0) {
		qtk_audio_daemon_delete(ad);
		ad = NULL;
	}
	return ad;
}

void qtk_audio_daemon_delete(qtk_audio_daemon_t *ad)
{
	if(ad->run) {
		qtk_audio_daemon_stop(ad);
	}
	wtk_thread_clean(&ad->thread);
	wtk_pipequeue_clean(&ad->msg_q);
	wtk_lockhoard_clean(&ad->msg_hoard);

	wtk_free(ad);
}

void qtk_audio_daemon_set_notify(qtk_audio_daemon_t *ad,void *notify_ths,qtk_audio_daemon_notify_func notify_func)
{
	ad->notify_func = notify_func;
	ad->notify_ths = notify_ths;
}

void qtk_audio_daemon_set_vid_pid(qtk_audio_daemon_t *ad,int vid,int pid)
{
	ad->vid = vid;
	ad->pid = pid;
}

int qtk_audio_daemon_start(qtk_audio_daemon_t *ad)
{
#ifdef USE_USB
	const struct libusb_pollfd **fds;
	int ret;
	int i;
	int fd;

	if(ad->run) {
		return -1;
	}
	ret = libusb_init(&ad->ctx);
	if(ret != 0) {
		wtk_debug("libusb init failed\n");
		goto end;
	}

	ret = libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG);
	if(ret == 0) {
		wtk_debug("libusb not support hotplug\n");
		ret = -1;
		goto end;
	} else {
		libusb_hotplug_register_callback(ad->ctx,
				LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED|LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
				0,
				ad->vid,
				ad->pid,
				LIBUSB_HOTPLUG_MATCH_ANY,
				(libusb_hotplug_callback_fn)qtk_audio_daemon_on_hotplug_cb,
				ad,
				&ad->handle
				);
	}
	FD_ZERO(&ad->r_set);
	FD_ZERO(&ad->w_set);
	FD_ZERO(&ad->e_set);
	FD_SET(ad->msg_q.pipe_fd[0],&ad->r_set);
	fds = libusb_get_pollfds(ad->ctx);
	i = 0;
	fd = 0;
	while(1) {
		if(!fds[i]) {
			break;
		}
		if(fds[i]->fd > fd) {
			fd = fds[i]->fd;
		} else {
			++i;
			continue;
		}
		if(fds[i]->events & 0x004) {
			FD_SET(fds[i]->fd,&ad->w_set);
		}
		if(fds[i]->events & 0x001) {
			FD_SET(fds[i]->fd,&ad->r_set);
		}
		FD_SET(fds[i]->fd,&ad->e_set);
		++i;
	}
	libusb_free_pollfds(fds);
	ad->max_fd = max(ad->msg_q.pipe_fd[0],fd) + 1;

	ad->run = 1;
	ret = wtk_thread_start(&ad->thread);
	if(ret != 0) {
		goto end;
	}

	ret = 0;
end:
	return ret;
#else
	return 0;
#endif
}

int qtk_audio_daemon_stop(qtk_audio_daemon_t *ad)
{
#ifdef USE_USB
	if(!ad->run) {
		return -1;
	}
	ad->run = 0;
	wtk_pipequeue_touch_write(&ad->msg_q);
	wtk_thread_join(&ad->thread);

	if(ad->ctx) {
		libusb_hotplug_deregister_callback(ad->ctx,ad->handle);
		libusb_exit(ad->ctx);
		ad->ctx = NULL;
	}
#endif
	return 0;
}

int qtk_audio_daemon_feed(qtk_audio_daemon_t *ad,qtk_audio_daemon_cmd_t cmd)
{
#ifdef USE_USB
	qtk_audio_daemon_msg_t *msg;

	msg = qtk_audio_daemon_pop_msg(ad);
	if(!msg) {
		return -1;
	}
	msg->cmd = cmd;
	wtk_pipequeue_push(&ad->msg_q,&msg->q_n);
#endif
	return 0;
}

int qtk_audio_daemon_reset_dev(qtk_audio_daemon_t *ad)
{
#ifdef USE_USB
	libusb_device **list,*dev;
	struct libusb_device_descriptor desc;
	ssize_t cnt,i;
	uint8_t bnum;
	uint8_t dnum;
	char path[128];
	int fd,ret;

	cnt = libusb_get_device_list(ad->ctx,&list);
	if(cnt > 0) {
		for(i=0;i<cnt;++i) {
			dev = list[i];
			libusb_get_device_descriptor(dev,&desc);
			if(desc.idVendor == ad->vid && desc.idProduct == ad->pid) {
				bnum = libusb_get_bus_number(dev);
				dnum = libusb_get_device_address(dev);
				snprintf(path,128,"/dev/bus/usb/%03d/%03d",bnum,dnum);
//				wtk_debug("usb path = %s\n",path);
				wtk_log_log(ad->session->log,"usb path = %s\n",path);
				fd = open(path,O_WRONLY);
				if(fd >= 0) {
					ret = ioctl(fd,USBDEVFS_RESET,NULL);
//					wtk_debug("=======>ret  =%d   errno = %d\n",ret,errno);
					wtk_log_log(ad->session->log,"=======>ret  =%d   errno = %d\n",ret,errno);
					close(fd);
					if(ret == 0) {
						goto end;
					}
				} else {
					wtk_debug("usb [%s] open failed\n",path);
				}
				break;
			}
		}
	}
	ret = -1;
end:
	if(list){
		libusb_free_device_list(list,1);
	}
	return ret;
#else
	return 0;
#endif
}
static qtk_audio_daemon_msg_t* qtk_audio_daemon_msg_new(qtk_audio_daemon_t *ad)
{
	qtk_audio_daemon_msg_t *msg;

	msg = (qtk_audio_daemon_msg_t*)wtk_malloc(sizeof(*msg));
	return msg;
}

static int qtk_audio_daemon_msg_delete(qtk_audio_daemon_msg_t *msg)
{
	wtk_free(msg);
	return 0;
}
#ifdef USE_USB
static qtk_audio_daemon_msg_t* qtk_audio_daemon_pop_msg(qtk_audio_daemon_t *ad)
{
	return (qtk_audio_daemon_msg_t*)wtk_lockhoard_pop(&ad->msg_hoard);
}

static void qtk_audio_daemon_push_msg(qtk_audio_daemon_t *ad,qtk_audio_daemon_msg_t *msg)
{
	wtk_lockhoard_push(&ad->msg_hoard,msg);
}

static void qtk_audio_daemon_clean_msg_q(qtk_audio_daemon_t *ad)
{
	wtk_queue_node_t *qn;
	qtk_audio_daemon_msg_t *msg;

	while(1) {
		qn = wtk_pipequeue_pop(&ad->msg_q);
		if(!qn) {
			break;
		}
		msg = data_offset(qn,qtk_audio_daemon_msg_t,q_n);
		qtk_audio_daemon_push_msg(ad,msg);
	}
}

static void qtk_audio_daemon_notify(qtk_audio_daemon_t *ad,qtk_audio_daemon_cmd_t cmd)
{
	if(ad->notify_func) {
		ad->notify_func(ad->notify_ths,cmd);
	} else {
		wtk_debug("audio daemon skip cmd %d\n",cmd);
	}
}

static int qtk_audio_daemon_on_hotplug_cb(struct libusb_context *ctx,
		struct libusb_device *dev,
		libusb_hotplug_event event,
		qtk_audio_daemon_t *ad
		)
{
	switch(event) {
	case LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT:
		qtk_audio_daemon_notify(ad,QTK_AUDIO_DAEMON_LEFT);
		ad->state = QTK_AUDIO_DAEMON_STATE_LEFT;
		break;
	case LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED:
		qtk_audio_daemon_notify(ad,QTK_AUDIO_DAEMON_ARRIVE);
		ad->state = QTK_AUDIO_DAEMON_STATE_ARRIVE;
		break;
	}
	return 0;
}

static void qtk_audio_daemon_on_pipe(qtk_audio_daemon_t *ad)
{
	wtk_queue_node_t *qn;
	qtk_audio_daemon_msg_t *msg;

	while(1) {
		qn = wtk_pipequeue_pop(&ad->msg_q);
		if(!qn) {
			break;
		}
		msg = data_offset2(qn,qtk_audio_daemon_msg_t,q_n);
		if(ad->state == QTK_AUDIO_DAEMON_STATE_ARRIVE) {
			qtk_audio_daemon_notify(ad,msg->cmd);
		} else {
			wtk_debug("ignore cmd = %d\n",msg->cmd);
		}
		qtk_audio_daemon_push_msg(ad,msg);
	}
}
#endif
static int qtk_audio_daemon_run(qtk_audio_daemon_t *ad)
{
#ifdef USE_USB
	fd_set tr_set,tw_set,te_set;
	struct timeval tv;
	struct timeval tv2 = {0,0};
	int ret;

	qtk_audio_daemon_clean_msg_q(ad);

	while(ad->run) {
		tr_set = ad->r_set;
		tw_set = ad->w_set;
		te_set = ad->e_set;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		ret = select(ad->max_fd,&tr_set,&tw_set,&te_set,&tv);
		//wtk_debug("ret = %d\n",ret);
		if(ret > 0) {
			if(FD_ISSET(ad->msg_q.pipe_fd[0],&tr_set)) {
				//wtk_debug("pipe\n");
				qtk_audio_daemon_on_pipe(ad);
				ret=libusb_handle_events_timeout_completed(ad->ctx,&tv2,NULL);
			} else {
				ret=libusb_handle_events_timeout_completed(ad->ctx,&tv2,NULL);
			}
		}
	}
	libusb_handle_events_timeout_completed(ad->ctx,&tv2,NULL);
	qtk_audio_daemon_clean_msg_q(ad);
#endif
	return 0;
}
