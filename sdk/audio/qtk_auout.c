#include "qtk_auout.h" 

static qtk_auout_msg_t* qtk_auout_msg_new(qtk_auout_t *auout)
{
	qtk_auout_msg_t *msg;

	msg = (qtk_auout_msg_t*)wtk_malloc(sizeof(qtk_auout_msg_t));
	msg->buf = wtk_strbuf_new(1600,1);

	return msg;
}

static int qtk_auout_msg_delete(qtk_auout_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);

	return 0;
}

static qtk_auout_msg_t* qtk_auout_pop_msg(qtk_auout_t *auout)
{
	qtk_auout_msg_t *msg;

	msg = (qtk_auout_msg_t*)wtk_lockhoard_pop(&auout->msg_hoard);
	wtk_strbuf_reset(msg->buf);

	msg->msg_notify_func = NULL;
	msg->msg_notify_ths = NULL;

	return msg;
}

static void qtk_auout_push_msg(qtk_auout_t *auout,qtk_auout_msg_t *msg)
{
	wtk_lockhoard_push(&auout->msg_hoard,msg);
}

static void qtk_auout_wake_msg(qtk_auout_t *auout)
{
	qtk_auout_msg_t *msg;

	msg = qtk_auout_pop_msg(auout);
	if(!msg) {
		return;
	}
	msg->type = -1;
	wtk_blockqueue_push(&auout->input_q,&msg->q_n);
}

static void qtk_auout_on_pitch(qtk_auout_t *auout,char *data, int bytes)
{
	wtk_strbuf_push(auout->buf,data,bytes);
}

static int qtk_auout_on_msg_start(qtk_auout_t *auout,qtk_auout_msg_t *msg)
{
	int ret;

	wtk_log_log(auout->log,"play start sample_rate %d channel %d bytes_per_sample %d",
			msg->sample_rate,
			msg->channel,
			msg->bytes_per_sample
			);

	ret = auout->actions.auout_start(auout->actions.action_ths,NULL,msg->sample_rate,msg->channel,msg->bytes_per_sample);
	if(ret != 0) {
		goto end;
	}

	if(auout->notify_func) {
		auout->notify_func(auout->notify_ths,QTK_AUOUT_DATA_START,NULL,0);
	}

	ret = 0;
end:
	return ret;
}

static void qtk_auout_volume_weight(qtk_auout_t *auout,char *data,int bytes)
{
	short *ps,*pe;
	int num;

	ps = (short*)data;
	pe = (short*)(data + bytes);
	while(ps < pe) {
		num = (*ps) * auout->volume_shift;
		if(num > 32767) {
			*ps = 32767;
		} else if (num < -32768) {
			*ps = -32768;
		} else {
			*ps = num;
		}
		++ps;
	}
}

static int qtk_auout_on_msg_data(qtk_auout_t *auout,qtk_auout_msg_t *msg)
{
	char *data;
	int bytes;
	int ret,n;
	char *s,*e;

	if(auout->volume_shift != 1.0) {
		qtk_auout_volume_weight(auout,msg->buf->data,msg->buf->pos);
	}

	if(auout->pitch_shift == 1.0) {
		data = msg->buf->data;
		bytes = msg->buf->pos;
	} else {
		wtk_strbuf_reset(auout->buf);
		wtk_pitch_process(auout->pitch,auout->pitch_shift,msg->buf->data,msg->buf->pos);
		data = auout->buf->data;
		bytes = auout->buf->pos;
	}

	s = data;
	e = data + bytes;
	while(s < e && !auout->stop_hint) {
		n = min(e-s,auout->step);
		ret = auout->actions.auout_write(auout->actions.action_ths,s,n);
		if(ret < 0) {
			wtk_log_warn(auout->log,"auout write ret = %d",ret);
			goto end;
		}
		if(auout->notify_func) {
			auout->notify_func(auout->notify_ths,QTK_AUOUT_DATA_WRITE,s,n);
		}
		s += n;
	}

	ret = 0;
end:
	return ret;
}

static void qtk_auout_on_msg_end(qtk_auout_t *auout)
{
	if(auout->cfg->play_wait_end_time) {
		wtk_msleep(auout->cfg->play_wait_end_time);
	}

	auout->actions.auout_stop(auout->actions.action_ths);

	wtk_log_log0(auout->log,"play end");
	if(auout->notify_func) {
		auout->notify_func(auout->notify_ths,QTK_AUOUT_DATA_END,NULL,0);
	}
}


static void qtk_auout_on_msg_stop2(qtk_auout_t *auout)
{
	qtk_auout_msg_t *msg;

	msg = qtk_auout_pop_msg(auout);
	if(!msg) {
		return;
	}

	msg->type = QTK_AUOUT_MSG_START;
	msg->sample_rate = auout->sample_rate;
	msg->bytes_per_sample = auout->bytes_per_sample;
	msg->channel = auout->channel;

	wtk_blockqueue_push_front(&auout->input_q,&msg->q_n);
}

static void qtk_auout_clean_msg_q(qtk_auout_t *auout)
{
	qtk_auout_msg_t *msg;
	wtk_queue_node_t *qn;

	while(1) {
		qn = wtk_blockqueue_pop(&auout->input_q,0,NULL);
		if(!qn) {
			break;
		}
		msg = data_offset2(qn,qtk_auout_msg_t,q_n);
//		wtk_log_log(auout->log,"=====>skip state = %d msg type = %d",auout->state,msg->type);
		if(msg->msg_notify_func) {
			msg->msg_notify_func(msg->msg_notify_ths);
		}
		qtk_auout_push_msg(auout,msg);
	}

}

static int qtk_auout_run(qtk_auout_t *auout,wtk_thread_t *thread)
{
	qtk_auout_msg_t *msg;
	wtk_queue_node_t *qn;
	int ret;

	if(auout->cfg->wait_time) {
		wtk_msleep(auout->cfg->wait_time);
	}

	while(auout->run) {
		qn = wtk_blockqueue_pop(&auout->input_q,-1,NULL);
		if(!qn) {
			break;
		}
		msg = data_offset2(qn,qtk_auout_msg_t,q_n);
		wtk_log_log(auout->log,"state = %d msg type = %d",auout->state,msg->type);

		ret = 0;
		switch(msg->type) {
		case QTK_AUOUT_MSG_START:
			if(auout->state == QTK_AUOUT_INIT) {
				ret = qtk_auout_on_msg_start(auout,msg);
				if(ret == 0) {
					auout->state = QTK_AUOUT_PLAY;
				} else {
					wtk_log_log(auout->log,"auout msg start ret = %d\n",ret);
				}
			}
			break;
		case QTK_AUOUT_MSG_DATA:
			if(auout->state == QTK_AUOUT_PLAY) {
				ret = qtk_auout_on_msg_data(auout,msg);
				if(ret != 0) {
					wtk_log_log(auout->log,"auout msg data ret = %d\n",ret);
					auout->state = QTK_AUOUT_INIT;
				}
			}
			break;
		case QTK_AUOUT_MSG_END:
			if(auout->state == QTK_AUOUT_PLAY) {
				qtk_auout_on_msg_end(auout);
				auout->state = QTK_AUOUT_INIT;
			}
			break;
		case QTK_AUOUT_MSG_STOP:
			if(auout->state == QTK_AUOUT_PLAY) {
				qtk_auout_on_msg_end(auout);
				auout->state = QTK_AUOUT_INIT;
			}
			qtk_auout_clean_msg_q(auout);
			break;
		default:
			break;
		}
		if(msg->msg_notify_func) {
			msg->msg_notify_func(msg->msg_notify_ths);
		}
		qtk_auout_push_msg(auout,msg);

		if(ret != 0 && auout->cfg->err_exit) {
//			wtk_debug("err exit\n");
			goto end;
		}
	}

end:
	if(auout->state == QTK_AUOUT_PLAY) {
		qtk_auout_on_msg_end(auout);
		auout->state = QTK_AUOUT_INIT;
	}
	if(auout->actions.auout_clean) {
		auout->actions.auout_clean(auout->actions.action_ths);
	}
	return 0;
}

static void qtk_auout_init(qtk_auout_t *auout)
{
	auout->cfg = NULL;
	auout->log = NULL;
	auout->pitch = NULL;
	auout->buf = NULL;

	// auout->mp3dec = 0;
	// auout->mp3dec_active = 0;

	auout->buf_time = 10;
	auout->run = 0;
	auout->stop_hint = 0;

	auout->state = QTK_AUOUT_INIT;

	auout->actions.auout_start = NULL;
	auout->actions.auout_write = NULL;
	auout->actions.auout_stop  = NULL;
	auout->actions.auout_clean = NULL;
	auout->actions.action_ths  = NULL;

	auout->notify_func = NULL;
	auout->notify_ths  = NULL;
}

// static void qtk_auout_on_mp3dec(qtk_auout_t *auout,wtk_mp3dec_state_t state,char *data,int bytes);

qtk_auout_t* qtk_auout_new(qtk_auout_cfg_t *cfg,wtk_log_t *log)
{
	qtk_auout_t *auout;

	auout = (qtk_auout_t*)wtk_malloc(sizeof(qtk_auout_t));
	qtk_auout_init(auout);

	auout->cfg = cfg;
	auout->log = log;
	auout->pitch_shift = cfg->pitch_shift;
	auout->volume_shift = cfg->volume_shift;

	auout->pitch = wtk_pitch_new(&cfg->pitch);
	wtk_pitch_set(auout->pitch,auout,(wtk_pitch_noityf_f)qtk_auout_on_pitch);

	wtk_blockqueue_init(&auout->input_q);
	wtk_lockhoard_init(&auout->msg_hoard,offsetof(qtk_auout_msg_t,hoard_n),50,
			(wtk_new_handler_t)qtk_auout_msg_new,
			(wtk_delete_handler_t)qtk_auout_msg_delete,
			auout
			);
	wtk_thread_init(&auout->thread,(thread_route_handler)qtk_auout_run,auout);
	wtk_thread_set_name(&auout->thread,"auout");

	auout->buf = wtk_strbuf_new(3200,1);

	// if(cfg->use_mp3dec) {
	// 	wtk_lock_init(&auout->lock);
	// 	auout->mp3dec = wtk_mp3dec2_new(&cfg->mp3dec,log);
	// 	wtk_mp3dec2_set_notify(auout->mp3dec,auout,(wtk_mp3dec_notify_f)qtk_auout_on_mp3dec);
	// 	wtk_mp3dec2_start(auout->mp3dec);
	// 	auout->mp3dec_active = 0;
	// }

	return auout;
}


void qtk_auout_delete(qtk_auout_t *auout)
{
	if(auout->run) {
		qtk_auout_stop(auout);
	}

	wtk_thread_clean(&auout->thread);
	wtk_blockqueue_clean(&auout->input_q);
	wtk_lockhoard_clean(&auout->msg_hoard);

	wtk_pitch_delete(auout->pitch);
	wtk_strbuf_delete(auout->buf);

	// if(auout->cfg->use_mp3dec) {
	// 	wtk_mp3dec2_stop(auout->mp3dec);
	// 	wtk_mp3dec2_delete(auout->mp3dec);
	// 	wtk_lock_clean(&auout->lock);
	// }

	wtk_free(auout);
}

void qtk_auout_set_callback(qtk_auout_t *auout,
		void *action_ths,
		qtk_auout_start_f auout_start,
		qtk_auout_write_f auout_write,
		qtk_auout_stop_f  auout_stop,
		qtk_auout_clean_f auout_clean
		)
{
	auout->actions.action_ths = action_ths;
	auout->actions.auout_start = auout_start;
	auout->actions.auout_write = auout_write;
	auout->actions.auout_stop  = auout_stop;
	auout->actions.auout_clean = auout_clean;
}

void qtk_auout_set_notify(qtk_auout_t *auout,void *notify_ths,qtk_auout_data_notify_func notify_func)
{
	auout->notify_func = notify_func;
	auout->notify_ths = notify_ths;
	wtk_log_log(auout->log,"notify func %p ths %p",auout->notify_func,auout->notify_ths);
}

void qtk_auout_set_bufTime(qtk_auout_t *auout,int buf_time)
{
	auout->buf_time = buf_time;
	wtk_log_log(auout->log,"buf time %d",buf_time);
}

int qtk_auout_start(qtk_auout_t *auout)
{
	if(auout->run) {
		return -1;
	}

	wtk_log_log0(auout->log,"auout start");
	auout->run = 1;
	return wtk_thread_start(&auout->thread);
}

void qtk_auout_stop(qtk_auout_t *auout)
{
	if(!auout->run) {
		return;
	}

	qtk_auout_stop_play(auout);
	auout->run = 0;
	qtk_auout_wake_msg(auout);
	wtk_thread_join(&auout->thread);
}

void qtk_auout_stop2(qtk_auout_t *auout)
{
	if(!auout->run) {
		return;
	}

	auout->run = 0;
	auout->stop_hint = 1;
	qtk_auout_wake_msg(auout);
	wtk_thread_join(&auout->thread);
	auout->stop_hint = 0;

	if(auout->input_q.length > 0) {
		qtk_auout_on_msg_stop2(auout);
	}
}

void qtk_auout_play_start(qtk_auout_t *auout,int sample_rate,int channel,int bytes_per_sample)
{
	qtk_auout_msg_t *msg;

	msg = qtk_auout_pop_msg(auout);
	msg->type = QTK_AUOUT_MSG_START;
	msg->sample_rate = sample_rate;
	msg->channel = channel;
	msg->bytes_per_sample = bytes_per_sample;
	auout->sample_rate = msg->sample_rate;
	auout->channel = msg->channel;
	auout->bytes_per_sample = msg->bytes_per_sample;
	auout->step = auout->buf_time * msg->sample_rate * msg->channel * msg->bytes_per_sample / 1000;
	wtk_blockqueue_push(&auout->input_q,&msg->q_n);
}

void qtk_auout_play_feed_data(qtk_auout_t *auout,char *data,int bytes)
{
//	wtk_debug(">>>>>>>>>>>>>bytes = %d\n",bytes);
	// if(auout->cfg->use_mp3dec){
	// 	wtk_string_t *v;
	// 	v = wtk_string_dup_data(data,bytes);
	// 	wtk_mp3dec2_feed_data(auout->mp3dec,v);
	// 	wtk_mp3dec2_reset_file(auout->mp3dec);

	// }else
	// {
		qtk_auout_play_data(auout,data,bytes);
	// }
}

void qtk_auout_play_data(qtk_auout_t *auout,char *data,int bytes)
{
	qtk_auout_msg_t *msg;

	if(bytes <= 0) {
		return;
	}

	msg = qtk_auout_pop_msg(auout);
	msg->type = QTK_AUOUT_MSG_DATA;
	wtk_strbuf_push(msg->buf,data,bytes);
	wtk_blockqueue_push(&auout->input_q,&msg->q_n);
}

static void qtk_auout_play_end_callback(wtk_sem_t *sem)
{
	wtk_sem_release(sem,1);
}

void qtk_auout_play_end(qtk_auout_t *auout,int syn)
{
	qtk_auout_msg_t *msg;
	wtk_sem_t sem;

	msg = qtk_auout_pop_msg(auout);
	msg->type = QTK_AUOUT_MSG_END;

	if(syn) {
		wtk_sem_init(&sem,0);
		msg->msg_notify_ths = &sem;
		msg->msg_notify_func = (qtk_auout_msg_notify_func)qtk_auout_play_end_callback;
		wtk_blockqueue_push(&auout->input_q,&msg->q_n);
		wtk_sem_acquire(&sem,-1);
		wtk_sem_clean(&sem);
	} else {
		wtk_blockqueue_push(&auout->input_q,&msg->q_n);
	}
}


int qtk_auout_play_file(qtk_auout_t *auout,char *fn,int syn)
{
	wtk_riff_t *riff = NULL;
	char *buf;
	int sample_rate,channel,bytes_per_sample;
	int step;
	int ret;

	wtk_log_log(auout->log,"auout play file %s",fn);
	riff = wtk_riff_new();
	ret = wtk_riff_open(riff,fn);
	if(ret != 0) {
		wtk_log_warn(auout->log,"play fn %s not exist",fn);
		goto end;
	}

	sample_rate = riff->fmt.sample_rate;
	channel     = riff->fmt.channels;
	bytes_per_sample = riff->fmt.bit_per_sample / 8;
	qtk_auout_play_start(auout,sample_rate,channel,bytes_per_sample);

	step = auout->buf_time * sample_rate * channel * bytes_per_sample / 1000;
	wtk_debug("======================> step = %d\n",step);
	buf = (char*)wtk_malloc(step);
	while(1) {
		ret = wtk_riff_read(riff,buf,step);
		if(ret <= 0) {
			break;
		}
		qtk_auout_play_data(auout,buf,ret);
	}
	qtk_auout_play_end(auout,syn);
	wtk_free(buf);

	ret = 0;
end:
	if(riff) {
		wtk_riff_close(riff);
		wtk_riff_delete(riff);
	}
	return ret;
}


// static void qtk_auout_on_mp3dec(qtk_auout_t *auout,wtk_mp3dec_state_t state,char *data,int bytes)
// {
// //	wtk_debug("======================>state = %d\n",state);
// 	switch(state) {
// 	case WTK_MP3DEC_START:
// 		qtk_auout_play_start(auout,16000,1,2);
// 		break;
// 	case WTK_MP3DEC_DATA:
// 		if(bytes > 0) {
// 			qtk_auout_play_data(auout,data,bytes);
// 		}
// 		break;
// 	case WTK_MP3DEC_END:
// 		qtk_auout_play_end(auout,0);
// 		break;
// 	case WTK_MP3DEC_ERR:
// 		qtk_auout_play_end(auout,0);
// 		break;
// 	}
// }

int qtk_auout_play_mp3(qtk_auout_t *auout,char *fn)
{
	// int ret;

	// if(!auout->cfg->use_mp3dec) {
	// 	return -1;
	// }

	// ret = wtk_lock_lock(&auout->lock);
	// if(ret != 0) {
	// 	return -1;
	// }
	// if(auout->mp3dec_active) {
	// 	wtk_mp3dec_set_stop_hint(auout->mp3dec->dec);
	// 	wtk_mp3dec2_reset_file(auout->mp3dec);
	// 	qtk_auout_play_end(auout,0);
	// }

	// wtk_mp3dec2_feed_file(auout->mp3dec,fn);
	// auout->mp3dec_active = 1;
	// wtk_lock_unlock(&auout->lock);
	return 0;
}

static void qtk_auout_stop_play_callback(wtk_sem_t *sem)
{
	wtk_sem_release(sem,1);
}

void qtk_auout_stop_play(qtk_auout_t *auout)
{
	qtk_auout_msg_t *msg;
	wtk_sem_t sem;
	// int ret;

	// wtk_log_log(auout->log,"stop play run = %d mp3dec_active = %d\n",auout->run,auout->mp3dec_active);

	// if(auout->cfg->use_mp3dec) {
	// 	ret = wtk_lock_lock(&auout->lock);
	// 	if(ret != 0) {
	// 		return;
	// 	}
	// 	if(auout->mp3dec_active) {
	// 		wtk_mp3dec_set_stop_hint(auout->mp3dec->dec);
	// 		wtk_mp3dec2_reset_file(auout->mp3dec);
	// 		auout->mp3dec_active = 0;
	// 	}
	// 	wtk_lock_unlock(&auout->lock);
	// }

	if(!auout->run) {
		return;
	}

	auout->stop_hint = 1;
	msg = qtk_auout_pop_msg(auout);
	msg->type = QTK_AUOUT_MSG_STOP;
	wtk_sem_init(&sem,0);
	msg->msg_notify_func = (qtk_auout_msg_notify_func)qtk_auout_stop_play_callback;
	msg->msg_notify_ths = &sem;
	wtk_blockqueue_push_front(&auout->input_q,&msg->q_n);
	wtk_sem_acquire(&sem,-1);
	auout->stop_hint = 0;
}

float qtk_auout_set_volume(qtk_auout_t *auout,float volume)
{
	if(volume > auout->cfg->volume_shift_max) {
		auout->volume_shift = auout->cfg->volume_shift_max;
	} else if (volume < auout->cfg->volume_shift_min) {
		auout->volume_shift = auout->cfg->volume_shift_min;
	} else {
		auout->volume_shift = volume;
	}

	return auout->volume_shift;
}

float qtk_auout_inc_volume(qtk_auout_t *auout)
{
	float volume;

	volume = auout->volume_shift + auout->cfg->volume_shift_step;
	if(volume > auout->cfg->volume_shift_max) {
		volume = auout->cfg->volume_shift_max;
	}
	auout->volume_shift = volume;

	return auout->volume_shift;
}

float qtk_auout_dec_volume(qtk_auout_t *auout)
{
	float volume;

	volume = auout->volume_shift - auout->cfg->volume_shift_step;
	if(volume < auout->cfg->volume_shift_min) {
		volume = auout->cfg->volume_shift_min;
	}
	auout->volume_shift = volume;

	return auout->volume_shift;
}

float qtk_auout_set_pitch(qtk_auout_t *auout,float pitch)
{
	if(pitch > auout->cfg->pitch_shift_max) {
		auout->pitch_shift = auout->cfg->pitch_shift_max;
	} else if (pitch < auout->cfg->pitch_shift_min) {
		auout->pitch_shift = auout->cfg->pitch_shift_min;
	} else {
		auout->pitch_shift = pitch;
	}

	return auout->pitch_shift;
}

float qtk_auout_inc_pitch(qtk_auout_t *auout)
{
	float pitch;

	pitch = auout->pitch_shift + auout->cfg->pitch_shift_step;
	if(pitch > auout->cfg->pitch_shift_max) {
		pitch = auout->cfg->pitch_shift_max;
	}
	auout->pitch_shift = pitch;

	return auout->pitch_shift;
}

float qtk_auout_dec_pitch(qtk_auout_t *auout)
{
	float pitch;

	pitch = auout->pitch_shift - auout->cfg->pitch_shift_step;
	if(pitch < auout->cfg->pitch_shift_min) {
		pitch = auout->cfg->pitch_shift_min;
	}
	auout->pitch_shift = pitch;

	return auout->pitch_shift;
}
