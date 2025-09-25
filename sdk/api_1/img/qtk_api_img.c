#include "qtk_api_img.h"

void qtk_api_img_img_cb(qtk_api_img_t *qw, int res, int start, int end);
void qtk_api_img_asr_run(qtk_api_img_t *qw, wtk_thread_t *t);

void qtk_api_img_init(qtk_api_img_t *qw)
{
	qw->cfg = NULL;
	qw->session = NULL;
	qw->img = NULL;
	qw->ths = NULL;
	qw->notify = NULL;
	qw->wav_asr = NULL;
}

qtk_api_img_t *qtk_api_img_new(qtk_api_img_cfg_t *cfg, qtk_session_t *session)
{
	qtk_api_img_t *qw;
	int ret;
    qtk_img_thresh_cfg_t normal_thresh;

	qw = (qtk_api_img_t *)wtk_malloc(sizeof(*qw));
	qtk_api_img_init(qw);
	qw->cfg = cfg;
	qw->session = session;
	qw->img = qtk_img_rec_new(cfg->img);
	if (!qw->img) {
		wtk_debug("img new failed.\n");
		ret = -1; goto end;
	}
	normal_thresh.av_prob0 = 0.0;
	normal_thresh.avx0 = -0.5;
	normal_thresh.maxx0 = -2.0;
	normal_thresh.max_prob0 = 3.0;
	normal_thresh.av_prob1 = 4.0;
	normal_thresh.avx1 = -0.5;
	normal_thresh.maxx1 = -2.0;
	normal_thresh.max_prob1 = 6.0;
	normal_thresh.speech_dur = 2;

	qtk_img_thresh_set_cfg(qw->img, normal_thresh, 0);
	qtk_img_rec_set_notify(qw->img, (qtk_img_rec_notify_f)qtk_api_img_img_cb, qw);

	wtk_queue_init(&qw->vad_q);
	qw->vad = wtk_vad_new(cfg->vad, &qw->vad_q);

	if (qw->cfg->log_wav) {
		qw->wav_asr = wtk_wavfile_new(16000);
		qw->wav_asr->max_pend = 0;
	}

	ret = 0;
end:
	if (ret != 0) {
		qtk_api_img_delete(qw);
		qw = NULL;
	}
	return qw;
}

void qtk_api_img_delete(qtk_api_img_t *qw)
{
	if (qw->img) {
		qtk_img_rec_delete(qw->img);
	}
	if(qw->vad) {
		wtk_vad_delete(qw->vad);
	}
	if (qw->wav_asr) {
		wtk_wavfile_delete(qw->wav_asr);
	}
	wtk_free(qw);
}

int qtk_api_img_start(qtk_api_img_t *qw)
{
	qw->sil    = 1;
	wtk_vad_start(qw->vad);
	return 0;
}

int qtk_api_img_reset(qtk_api_img_t *qw)
{
	wtk_vad_reset(qw->vad);
	qtk_img_rec_reset(qw->img);
	return 0;
}

int qtk_api_img_feed(qtk_api_img_t *qw, char *data, int bytes, int is_end)
{
	wtk_queue_t *vad_q = &(qw->vad_q);
	wtk_queue_node_t *qn;
	qtk_msg_node_t *node;	
	wtk_vframe_t *f;

	wtk_vad_feed(qw->vad,data,bytes,is_end);
	while(1)
	{
		qn = wtk_queue_pop(vad_q);
		if(!qn) {
			break;
		}
		f = data_offset2(qn,wtk_vframe_t,q_n);

		if(qw->sil) {
			if(f->state == wtk_vframe_speech) {
				qtk_img_rec_start(qw->img);
				qw->sil = 0;
				if(qw->notify) {
					qw->notify(qw->ths, QTK_API_IMG_TYPE_SPEECH_START, NULL, 0);
				}
			}
		} else {
			if(f->state == wtk_vframe_sil) {
				if(qw->notify) {
					qw->notify(qw->ths, QTK_API_IMG_TYPE_SPEECH_END, NULL, 0);
				}
				qw->sil = 1;
					qtk_img_rec_feed(qw->img, NULL, 0, 1);
					qtk_img_rec_reset(qw->img);
			} else {
					qtk_img_rec_feed(qw->img, (char*)f->wav_data,f->frame_step<<1, 0);
			}
		}
		wtk_vad_push_vframe(qw->vad,f);
	}
	if(is_end && !qw->sil) {
		if(qw->notify) {
			qw->notify(qw->ths, QTK_API_IMG_TYPE_SPEECH_END, NULL, 0);
		}
			qtk_img_rec_feed(qw->img, NULL, 0, 1);
			qtk_img_rec_reset(qw->img);
	}
	return 0;
}


void qtk_api_img_set_notify(qtk_api_img_t *qw, void *ths, qtk_api_img_notify_f notify)
{
	qw->ths = ths;
	qw->notify = notify;
}

void qtk_api_img_img_cb(qtk_api_img_t *qw, int res, int start, int end)
{
	if (!qw->notify) {
		wtk_debug("notify function not set\n");
		return ;
	}
	switch (res) {
		case 1:
			// printf("%f %f\n",start*0.08,end*0.08);
			qw->notify(qw->ths, QTK_API_IMG_TYPE_WAKE, NULL, 0);
			break;
		default:
			break;
	}
}

