#include "qtk_audio.h"

static void qtk_audio_on_record(qtk_audio_t *a,char *data,int len);
static void qtk_audio_on_play(qtk_audio_t *a,qtk_auout_data_state_t state,char *data,int bytes);
void qtk_audio_on_err(qtk_audio_t *a,int is_err)
{
	if(a->ad) {
			qtk_audio_daemon_feed(a->ad,QTK_AUDIO_DAEMON_ERROR);
		}
}

static void qtk_audio_stop_on_daemon(qtk_audio_t *a)
{
//	wtk_debug("===================>stop audio     >3\n");
	if(a->rcd_flg) {
		qtk_auin_stop(a->auin);
	}
	if(a->ply_flg) {
		qtk_auout_stop2(a->auout);
	}
	if(!a->cfg->use_audio) {
		qtk_usb_stop(a->v.u);
	}
}

static void qtk_audio_start_on_daemon(qtk_audio_t *a)
{
//	wtk_debug("===================>restart audio  >2\n");
	if(!a->cfg->use_audio) {
		qtk_usb_start(a->v.u);
	}
	if(a->rcd_flg) {
		qtk_auin_start(a->auin);
	}
	if(a->ply_flg) {
		qtk_auout_start(a->auout);
	}
}

static void qtk_audio_reset_on_daemon(qtk_audio_t *a)
{
//	wtk_debug("===================>reset audio   >1\n");
	if(a->cfg->use_audio) {
		qtk_audio_daemon_reset_dev(a->ad);
	} else if (a->v.u) {
#ifdef USE_USB
		qtk_usb_send_cmd(a->v.u,0x82);
#endif
	}
}

static void qtk_audio_on_daemon(qtk_audio_t *a,qtk_audio_daemon_cmd_t cmd)
{
	if(a->daemon_notify) {
		a->daemon_notify(a->daemon_notify_ths,cmd);
	}

	switch(cmd) {
	case QTK_AUDIO_DAEMON_LEFT:
		wtk_log_log0(a->session->log,"==>left");
		_qtk_warning(a->session,_QTK_USB_LEFT);
		qtk_audio_stop_on_daemon(a);
		break;
	case QTK_AUDIO_DAEMON_ARRIVE:
		wtk_log_log0(a->session->log,"==>arrive");
		_qtk_warning(a->session,_QTK_USB_ARRIVED);
		wtk_msleep(a->cfg->daemon_wait_time);
		qtk_audio_start_on_daemon(a);
		break;
	case QTK_AUDIO_DAEMON_ERROR:
		wtk_log_log0(a->session->log,"==>reset");
		_qtk_warning(a->session,_QTK_DEV_RESET);
		if(a->cfg->use_audio) {
			qtk_audio_stop_on_daemon(a);
			qtk_audio_reset_on_daemon(a);
			wtk_msleep(a->cfg->daemon_wait_time);
			qtk_audio_start_on_daemon(a);
		} else {
			qtk_audio_reset_on_daemon(a);
		}
		break;
	}
}


static void qtk_audio_init(qtk_audio_t *a)
{
	a->ad = NULL;
	a->auin = NULL;
	a->auout = NULL;
	if(a->cfg->use_audio){
		a->v.audio.p = NULL;
		a->v.audio.r = NULL;
	}else{
		a->v.u = NULL;
	}
	a->atype = -1;
	a->ply_flg = 0;
	a->rcd_flg = 0;

	a->daemon_notify = NULL;
	a->daemon_notify_ths = NULL;
}

static void qtk_audio_init_auin(qtk_audio_t *a)
{
	a->auin = qtk_auin_new(&(a->cfg->auin),a->session->log);
	qtk_auin_set_process(a->auin,a,(qtk_auin_process_f)qtk_audio_on_record);

	if(a->cfg->use_audio) {
		wtk_debug("qtk_audio_init_auin================>>>>>>>>>>>>>>0000000000\n");
		a->v.audio.r = qtk_recorder_new(&a->cfg->recorder,a->session,a,(qtk_recorder_notify_f)qtk_audio_on_err);
		qtk_auin_set_callback(a->auin,a->v.audio.r,
				(qtk_auin_start_f)qtk_recorder_start,
				(qtk_auin_read_f)qtk_recorder_read,
				(qtk_auin_stop_f)qtk_recorder_stop,
				(qtk_auin_clean_f)qtk_recorder_clean
		);
		wtk_debug("qtk_audio_init_auin================>>>>>>>>>>>>>>ok\n");
		a->channel = a->cfg->recorder.channel - a->cfg->recorder.nskip;
	} else {
		qtk_auin_set_callback(a->auin,a->v.u,
				(qtk_auin_start_f)qtk_usb_record_start,
				(qtk_auin_read_f)qtk_usb_record_read,
				(qtk_auin_stop_f)qtk_usb_record_stop,
				NULL);
		a->channel = a->cfg->usb.channel - a->cfg->usb.nskip;
	}

	a->talking = 0;
	a->rcd_notify     = NULL;
	a->rcd_notify_ths = NULL;

	if(a->cfg->use_zero) {
		a->zero_wait_bytes = a->cfg->zero_wait_time * 32 * a->channel;
		a->zero_waited_bytes = 0;
	}
}

static void qtk_audio_init_auout(qtk_audio_t *a)
{
	a->auout = qtk_auout_new(&(a->cfg->auout),a->session->log);
	qtk_auout_set_notify(a->auout,a,(qtk_auout_data_notify_func)qtk_audio_on_play);
	if(a->cfg->use_audio) {
		a->v.audio.p = qtk_player_new(&a->cfg->player,a->session,a,(qtk_player_notify_f)qtk_audio_on_err);
		qtk_auout_set_callback(a->auout,a->v.audio.p,
				(qtk_auout_start_f)qtk_player_start,
				(qtk_auout_write_f)qtk_player_write,
				(qtk_auout_stop_f)qtk_player_stop,
				(qtk_auout_clean_f)qtk_player_clean);
		qtk_auout_set_bufTime(a->auout,a->cfg->player.buf_time);
	} else {
		qtk_auout_set_callback(a->auout,a->v.u,
				(qtk_auout_start_f)qtk_usb_play_start,
				(qtk_auout_write_f)qtk_usb_play_write,
				(qtk_auout_stop_f)qtk_usb_play_stop,
				NULL);
		qtk_auout_set_bufTime(a->auout,a->cfg->usb.buf_time);
	}
	a->model = NULL;
	a->talk_item = NULL;
	a->ply_notify = NULL;
	a->ply_notify_ths  = NULL;
}

qtk_audio_t* qtk_audio_new(qtk_audio_cfg_t *cfg,qtk_session_t *session)
{
	qtk_audio_t *a;
	int ret;

	a=(qtk_audio_t*)wtk_malloc(sizeof(qtk_audio_t));
	a->cfg = cfg;
	a->session = session;
	qtk_audio_init(a);

	wtk_log_log(session->log,"use_audio=%d use_daemon = %d",cfg->use_audio,cfg->use_daemon);
	if(cfg->use_audio) {
		a->atype = QTK_AUDIO_AUDIO;
	} else {
		a->v.u = qtk_usb_new(&(a->cfg->usb),a->session,a,(qtk_audio_notify_f)qtk_audio_on_err);
		if(!a->v.u) {
			ret = -1;
			wtk_log_log0(a->session->log,"usb new failed");
			goto end;
		}
		a->atype = QTK_AUDIO_USB;
	}

	if(cfg->use_daemon) {
		a->ad=qtk_audio_daemon_new(&a->cfg->daemon,a->session);
		if(!a->ad) {
			ret = -1;
			wtk_log_warn0(a->session->log,"daemon new failed");
			goto end;
		}
		qtk_audio_daemon_set_notify(a->ad,a,(qtk_audio_daemon_notify_func)qtk_audio_on_daemon);
		if(cfg->use_audio){
			qtk_audio_daemon_set_vid_pid(a->ad,cfg->recorder.vendor_id,cfg->recorder.product_id);
		}else{
			qtk_audio_daemon_set_vid_pid(a->ad,cfg->usb.vendor_id,cfg->usb.product_id);
		}
		qtk_audio_daemon_start(a->ad);

		cfg->auin.err_exit = 1;
		cfg->auout.err_exit = 1;
	} else {
		cfg->auin.err_exit = 0;
		cfg->auout.err_exit = 0;
	}
	qtk_audio_init_auin(a);
	qtk_audio_init_auout(a);
	ret = 0;
end:
	if(ret != 0)
	{
		qtk_audio_delete(a);
		a=NULL;
	}
	return a;
}

int qtk_audio_delete(qtk_audio_t *a)
{
	a->run = 0;
	if(a->auin)
	{
		qtk_auin_delete(a->auin);
	}
	if(a->auout)
	{
		qtk_auout_delete(a->auout);
	}
	if(a->ad){
		qtk_audio_daemon_stop(a->ad);
		qtk_audio_daemon_delete(a->ad);
	}
	switch(a->atype)
	{
	case QTK_AUDIO_AUDIO:
		if(a->v.audio.r)
		{
			qtk_recorder_delete(a->v.audio.r);
		}
		if(a->v.audio.p)
		{
			qtk_player_delete(a->v.audio.p);
		}
		break;
	case QTK_AUDIO_USB:
		if(a->v.u)
		{
			qtk_usb_delete(a->v.u);
			a->v.u = NULL;
		}
		break;
	default:
		break;
	}
	wtk_free(a);
	return 0;
}

void qtk_audio_set_model(qtk_audio_t *a,wtk_model_t *model)
{
	a->model = model;
	a->talk_item = wtk_model_get_item_s(model,"talk");
}

void qtk_audio_set_callback(qtk_audio_t *a,
		void *user_data,
		qtk_recorder_start_func recorder_start,
		qtk_recorder_read_func  recorder_read,
		qtk_recorder_stop_func  recorder_stop,
		qtk_recorder_clean_func recorder_clean,
		qtk_player_start_func   player_start,
		qtk_player_write_func   player_write,
		qtk_player_stop_func    player_stop,
		qtk_player_clean_func   player_clean
		)
{
	if(a->cfg->use_audio){
		if(a->cfg->use_sys_record){
			qtk_recorder_set_callback(a->v.audio.r,user_data,
					recorder_start,
					recorder_stop,
					recorder_read,
					recorder_clean
					);
		}
		if(a->cfg->use_sys_play){
			qtk_player_set_callback(a->v.audio.p,user_data,
					player_start,
					player_stop,
					player_write,
					player_clean
					);
		}
	}
}

void qtk_audio_set_daemon_notify(qtk_audio_t *a,void *daemon_notify_ths,qtk_audio_daemon_notify_f daemon_notify)
{
	a->daemon_notify = daemon_notify;
	a->daemon_notify_ths = daemon_notify_ths;
}

//recorder
void qtk_audio_rcder_set_notify(qtk_audio_t *a,void *rcd_notify_ths,qtk_audio_rcd_notify_f rcd_notify)
{
	a->rcd_notify     = rcd_notify;
	a->rcd_notify_ths = rcd_notify_ths;
}

static void qtk_audio_on_record(qtk_audio_t *a,char *data,int len)
{
	short *pv;
	int i,nx;
	int slen;

	if(!a->cfg->use_zero) {
		goto end;
	}

	if(a->talking) {
		goto end;
	}

	//wtk_debug("zero_waited_bytes = %d.\n",a->zero_waited_bytes);

	if(a->zero_waited_bytes > 0) {
		a->zero_waited_bytes -= len;
		goto end;
	}

	pv=(short*)data;
	slen=len>>1;
	nx=a->channel;

	for(i=nx-1;i<slen;i+=nx)
	{
		pv[i]=0;
		if(a->cfg->use_dc) {
			pv[i-1] = 0;
		}
	}
end:
	if(a->rcd_notify)
	{
		a->rcd_notify(a->rcd_notify_ths,data,len);
	}
}

int qtk_audio_rcder_start(qtk_audio_t *a)
{
	a->rcd_flg = 1;
	return qtk_auin_start(a->auin);
}

int qtk_audio_rcder_stop(qtk_audio_t *a)
{
	a->rcd_flg = 0;
	qtk_auin_stop(a->auin);
	return 0;
}

int qtk_audio_rcder_get_bufSize(qtk_audio_t *a)
{
	switch(a->atype)
	{
	case QTK_AUDIO_AUDIO:
		return a->v.audio.r->cfg->buf_size;
		break;
	case QTK_AUDIO_USB:
		return a->v.u->cfg->buf_bytes;
		break;
	}

	return 0;
}

int qtk_audio_rcder_get_bufTime(qtk_audio_t *a)
{
	if(a->cfg->use_audio) {
		return a->v.audio.r->cfg->buf_time;
	} else {
		return a->v.u->cfg->buf_time;
	}
}

int qtk_audio_rcder_get_channel(qtk_audio_t *a)
{
	return a->channel;
}

int qtk_audio_rcder_get_sampleRate(qtk_audio_t *a)
{
	if(a->cfg->use_audio) {
		return a->v.audio.r->sample_rate;
	} else {
		return a->v.u->cfg->sample_rate;
	}

	return 0;
}

int qtk_audio_rcder_get_bytes(qtk_audio_t *a)
{
	if(a->cfg->use_audio) {
		return a->v.audio.r->bytes_per_sample;
	} else {
		return a->v.u->cfg->bytes_per_sample;
	}

	return 0;
}

//player

static void qtk_audio_on_play(qtk_audio_t *a,qtk_auout_data_state_t state,char *data,int bytes)
{
	switch(state) {
	case QTK_AUOUT_DATA_START:
		a->talking = 1;
		if(a->talk_item) {
			wtk_model_item_set_i(a->talk_item,1);
		}
		break;
	case QTK_AUOUT_DATA_WRITE:
		break;
	case QTK_AUOUT_DATA_END:
		a->talking = 0;
		if(a->talk_item) {
			wtk_model_item_set_i(a->talk_item,1);
		}
		if(a->cfg->use_zero) {
			a->zero_waited_bytes = a->zero_wait_bytes;
		}
		break;
	}

	if(a->ply_notify) {
		a->ply_notify(a->ply_notify_ths,state,data,bytes);
	}
}

void qtk_audio_plyer_set_notify(qtk_audio_t *a,void *ply_notify_ths,qtk_audio_ply_notify_f ply_notify)
{
	a->ply_notify = ply_notify;
	a->ply_notify_ths = ply_notify_ths;
}

int qtk_audio_plyer_start(qtk_audio_t *a)
{
	a->ply_flg= 1;
	return qtk_auout_start(a->auout);
}

int qtk_audio_plyer_stop(qtk_audio_t *a)
{
	a->ply_flg = 0;
	qtk_auout_stop(a->auout);
	return 0;
}

void qtk_audio_plyer_play_start(qtk_audio_t *a,int sample_rate,int channel,int bytes_per_sample)
{
	qtk_auout_play_start(a->auout,sample_rate,channel,bytes_per_sample);
}

void qtk_audio_plyer_play_data(qtk_audio_t *a,char *data,int bytes)
{
//	qtk_auout_play_data(a->auout,data,bytes);
	qtk_auout_play_feed_data(a->auout,data,bytes);
}

void qtk_audio_plyer_play_end(qtk_audio_t *a,int syn)
{
	qtk_auout_play_end(a->auout,syn);
}

void qtk_audio_plyer_stop_play(qtk_audio_t *a)
{
	qtk_auout_stop_play(a->auout);
}

int qtk_audio_plyer_is_playing(qtk_audio_t *a)
{
	return a->talking;
}

int qtk_audio_plyer_play_file(qtk_audio_t *a,char *fn,int syn)
{
	return qtk_auout_play_file(a->auout,fn,syn);
}

int qtk_audio_plyer_play_mp3(qtk_audio_t *a,char *fn)
{
	return qtk_auout_play_mp3(a->auout,fn);
}

float qtk_audio_plyer_set_volume(qtk_audio_t *a,float volume)
{
	return qtk_auout_set_volume(a->auout,volume);
}

float qtk_audio_plyer_inc_volume(qtk_audio_t *a)
{
	return qtk_auout_inc_volume(a->auout);
}

float qtk_audio_plyer_dec_volume(qtk_audio_t *a)
{
	return qtk_auout_dec_volume(a->auout);
}

float qtk_audio_plyer_set_pitch(qtk_audio_t *a,float pitch)
{
	return qtk_auout_set_pitch(a->auout,pitch);
}

float qtk_audio_plyer_inc_pitch(qtk_audio_t *a)
{
	return qtk_auout_inc_pitch(a->auout);
}

float qtk_audio_plyer_dec_pitch(qtk_audio_t *a)
{
	return qtk_auout_dec_pitch(a->auout);
}
