#include "qtk_mulsv.h"

void qtk_mulsv_img_notify(qtk_mulsv_t *m, int res, int start, int end);
int qtk_mulsv_run(void *data,wtk_thread_t *t);
void qtk_mulsv_cache_q_clean(qtk_mulsv_t *m);
void qtk_mulsv_pop_cache_q(qtk_mulsv_t *m);
void qtk_mulsv_restore_pcm(qtk_mulsv_t *m,char *src,int len,wtk_strbuf_t *dst,int channel);
void qtk_mulsv_waked1st_pcm(qtk_mulsv_t *m,float fs,float fe);
wtk_strbuf_t* qtk_mulsv_convert_pcm(qtk_mulsv_t *m,wtk_strbuf_t *src,int channel);
void qtk_mulsv_save_pcm_do(qtk_mulsv_t *m,char *fn,char *data,int len,int channel);
void qtk_mulsv_save_pcm1st(qtk_mulsv_t *m);
void qtk_mulsv_bfio_notify(qtk_mulsv_t *m, wtk_bfio_cmd_t cmd, short *data, int len);
void qtk_mulsv_save_pcm(qtk_mulsv_t *m,char *sub_dir,char *data,int len,int channel,int index);
void qtk_mulsv_set_vprint_thresh(qtk_mulsv_t *m,char *data,int len);
void qtk_mulsv_set_notify_bias(qtk_mulsv_t *m, char *data, int len);
int qtk_mulsv_vsprint_feed(qtk_mulsv_t *m);
wtk_strbuf_t* qtk_mulsv_pad(qtk_mulsv_t *m,char *src,int src_len,int pad_len,int channel);
int qtk_mulsv_cache_q_len(qtk_mulsv_t *m);
void qtk_mulsv_aec_notify(qtk_mulsv_t *m, short **pv, int len, int is_end);
#define CACHE_2ND_LEN 320000
qtk_mulsv_t* qtk_mulsv_new(qtk_mulsv_cfg_t *cfg)
{
    qtk_mulsv_t *m = NULL;
    int channel = 0;

    m = wtk_calloc(1, sizeof(*m));
    if (m == 0) {
        goto end;}
	m->cfg=cfg;

	m->svprint=wtk_svprint_new(&cfg->svprint);
	if(m->svprint==0){goto end;}

        m->bfio=wtk_bfio_new(&(cfg->bfio));
	if(m->bfio==0){goto end;}
	wtk_bfio_set_notify(m->bfio, m, (wtk_bfio_notify_f)qtk_mulsv_bfio_notify);

        if (cfg->use_1st) {
            channel = cfg->wake1st_channel;
            m->img = qtk_img_rec_new(&(cfg->img));
            if (m->img == 0) {
                goto end;
            }
            qtk_img_thresh_set_cfg(m->img, cfg->img1st, 0);
            qtk_img_rec_set_notify(
                m->img, (qtk_img_rec_notify_f)qtk_mulsv_img_notify, m);
            if (cfg->use_aec) {
                m->aec = wtk_aec_new(&(cfg->aec));
                if (m->aec == 0) {
                    goto end;
                }
                wtk_aec_set_notify(m->aec, m,
                                   (wtk_aec_notify_f)qtk_mulsv_aec_notify);
                channel = cfg->wake1st_channel + m->aec->cfg->spchannel;
            }
            m->wkd1st_pcm = wtk_strbuf_new(cfg->wkd_tm * channel * 32, 0);
            if (m->wkd1st_pcm == 0) {
                goto end;
            }
        } else {
            channel = cfg->wake2nd_channel;
        }

        m->data_ptr = wtk_calloc(1, sizeof(char *) * channel);
        if (m->data_ptr == 0) {
            goto end;
        }

        m->msg_pool=qtk_lockmsg_new(2048);
	if(m->msg_pool==0){goto end;}
	
	m->wkd2nd_pcm=wtk_strbuf_new(cfg->vprint_feed_len,0);
	if(m->wkd2nd_pcm==0){goto end;}

	m->name=wtk_strbuf_new(64,0);
	if(m->name==0){goto end;}

        m->total_log = cfg->use_total_log;
        m->eval = 1;
        m->main_thread_eval = 1;
        m->pop_len = 0;
        m->total_time_len = 0;
        m->cancel_time_len = 0;
        wtk_queue_init(&(m->cache_q));
        wtk_blockqueue_init(&(m->q));

        wtk_thread_init(&(m->t), qtk_mulsv_run, m);
        wtk_thread_start(&(m->t));

end:
	return m;
}

void qtk_mulsv_delete(qtk_mulsv_t *m)
{
	qtk_lockmsg_node_t *node;

	node=qtk_lockmsg_pop_node(m->msg_pool);
	node->data_type=QTK_MULSV_MSG_END;
	wtk_blockqueue_push(&(m->q),&(node->qn));
	wtk_thread_join(&(m->t));
	wtk_thread_clean(&(m->t));

	if(m->img)
	{
		qtk_img_rec_delete(m->img);
	}

	if(m->svprint)
	{
		wtk_svprint_delete(m->svprint);
	}

	if(m->bfio)
	{
		wtk_bfio_delete(m->bfio);
	}

	if(m->aec)
	{
		wtk_aec_delete(m->aec);
	}

	if(m->data_ptr)
	{
		wtk_free(m->data_ptr);
	}

	if(m->msg_pool)
	{
		qtk_lockmsg_delete(m->msg_pool);
	}

	if(m->wkd1st_pcm)
	{
		wtk_strbuf_delete(m->wkd1st_pcm);
	}

	if(m->wkd2nd_pcm)
	{
		wtk_strbuf_delete(m->wkd2nd_pcm);
	}

	if(m->name)
	{
		wtk_strbuf_delete(m->name);
	}

	wtk_blockqueue_clean(&(m->q));
	wtk_free(m);
}

int qtk_mulsv_start(qtk_mulsv_t *m)
{
	qtk_lockmsg_node_t *node;

	node=qtk_lockmsg_pop_node(m->msg_pool);
	node->data_type=QTK_MULSV_MSG_START;
	wtk_blockqueue_push(&(m->q),&(node->qn));

	return 0;
}

void qtk_mulsv_reset(qtk_mulsv_t *m)
{
	qtk_lockmsg_node_t *node;

	node=qtk_lockmsg_pop_node(m->msg_pool);
	node->data_type=QTK_MULSV_MSG_RESET;
	wtk_blockqueue_push(&(m->q),&(node->qn));
}

void qtk_mulsv_set_notify(qtk_mulsv_t *m,void *ths,qtk_mulsv_notify_f notify)
{
	m->notify=notify;
	m->notify_ths=ths;
}

int qtk_mulsv_feed(qtk_mulsv_t *m,char *data,int len,int is_end)
{
	qtk_mulsv_cfg_t *cfg=m->cfg;
	qtk_lockmsg_node_t *node;
	int ret;

	if(len>0)
	{
		node=qtk_lockmsg_pop_node(m->msg_pool);
		if(cfg->use_1st && m->main_thread_eval)
		{
                    if (cfg->use_aec) {
                        qtk_mulsv_restore_pcm(m, data, len, node->buf,
                                              cfg->wake1st_channel +
                                                  m->aec->cfg->spchannel);
                    } else {
                        qtk_mulsv_restore_pcm(m, data, len, node->buf,
                                              cfg->wake1st_channel);
                    }
                        node->data_type=QTK_MULSV_MSG_1ST_PCM;
		}else
		{
			//need restore pcm first
			qtk_mulsv_restore_pcm(m,data,len,node->buf,cfg->wake2nd_channel);
			if(cfg->use_2nd_pad_sp)
			{
				wtk_strbuf_push(node->buf,0,len/cfg->wake2nd_channel);
			}
			node->data_type=QTK_MULSV_MSG_2ND_PCM;
		}
		wtk_blockqueue_push(&(m->q),&(node->qn));
	}

	if(is_end)
	{
		if(cfg->use_1st && m->main_thread_eval)
		{
			node=qtk_lockmsg_pop_node(m->msg_pool);
			node->data_type=QTK_MULSV_MSG_1ST_PCM;
			wtk_blockqueue_push(&(m->q),&(node->qn));
		}

		node=qtk_lockmsg_pop_node(m->msg_pool);
		node->data_type=QTK_MULSV_MSG_2ND_PCM;
		wtk_blockqueue_push(&(m->q),&(node->qn));
	}

        ret = 0;
        return ret;
}

int qtk_mulsv_feed2(qtk_mulsv_t *m,char *data,int len,int data_type)
{
    qtk_lockmsg_node_t *node;

    switch (data_type) {
    case QTK_MULSV_MSG_ENROLL_START:
        m->main_thread_eval = 0;
        break;
    case QTK_MULSV_MSG_ENROLL_END:
        m->main_thread_eval = 1;
        break;
    case QTK_MULSV_MSG_VPRINT_THRESH:
        break;
    case QTK_MULSV_MSG_NOTIFY_BIAS:
        break;
    default:
        break;
    }

    node = qtk_lockmsg_pop_node(m->msg_pool);
    node->data_type = data_type;
    wtk_strbuf_push(node->buf, data, len);
    wtk_blockqueue_push(&(m->q), &(node->qn));

    return 0;
}

int qtk_mulsv_run(void *data,wtk_thread_t *t)
{
	qtk_mulsv_t *m=data;
        qtk_lockmsg_node_t *node = NULL;
        wtk_queue_node_t *qn = NULL;
        qtk_mulsv_cfg_t *cfg=m->cfg;
        int v_cnt = 0, aec_channel = 0, bfio_channel = 0, tmp_len = 0, len = 0,
            i = 0;
        short **pcm = NULL, *ptr = NULL;
        short **aec_pcm = NULL;
	wtk_string_t v;

        if (m->cfg->use_aec) {
            aec_channel = cfg->wake1st_channel + m->aec->cfg->spchannel;
            aec_pcm = wtk_malloc(sizeof(short *) * aec_channel);
        }
        bfio_channel=cfg->wake2nd_channel;

        if(cfg->use_2nd_pad_sp)
	{
            bfio_channel = cfg->wake2nd_channel + m->bfio->sp_channel;
        }
	pcm=wtk_malloc(sizeof(short*)*bfio_channel);
	m->run=1;
	while(m->run)
	{
		qn=wtk_blockqueue_pop(&(m->q),-1,NULL);
		if(!qn){break;}
		node = data_offset2(qn, qtk_lockmsg_node_t, qn);
		switch(node->data_type)
		{
		case QTK_MULSV_MSG_START:
                    if (cfg->use_1st) {
                        qtk_img_rec_start(m->img);
                    }
#ifdef USE_2ND_OFFLINE
                    if (!cfg->use_1st) {
#endif
                        wtk_bfio_start(m->bfio);
#ifdef USE_2ND_OFFLINE
                    }
#endif
                    break;
		case QTK_MULSV_MSG_RESET:
                    if (cfg->use_1st) {
                        if (m->aec) {
                            wtk_aec_reset(m->aec);
                        }
                        qtk_img_rec_reset(m->img);
                        if (m->total_log) {
                            printf("一级唤醒:%d\n", m->cnt_1st);
                        }
                    }
                    if (m->total_log) {
                        printf("二级唤醒:%d\n", m->cnt_2nd);
                        printf("声纹唤醒:%d\n", m->cnt_vprint);
                    }
#ifdef USE_2ND_OFFLINE
                    if (!cfg->use_1st) {
#endif
                        wtk_bfio_reset(m->bfio);
#ifdef USE_2ND_OFFLINE
                    }
#endif
                    m->pop_len = 0;
                    m->total_time_len = 0;
                    m->cancel_time_len = 0;
                    m->in_tm = 0;
                    m->cnt_1st = 0;
                    m->cnt_2nd = 0;
                    m->cnt_vprint = 0;
                    break;
                case QTK_MULSV_MSG_END:
			m->run=0;
			break;
		case QTK_MULSV_MSG_1ST_PCM:
			//feed 1st wakeup
                        if (cfg->use_aec) {
                            if (node->buf->pos == 0) {
                                wtk_aec_feed(m->aec, NULL, 0, 1);
                            } else {
                                len = node->buf->pos /
                                      (sizeof(short) * aec_channel);
                                ptr = (short *)node->buf->data;
                                for (i = 0; i < aec_channel; ++i) {
                                    aec_pcm[i] = ptr;
                                    ptr += len;
                                }
                                wtk_aec_feed(m->aec, aec_pcm, len, 0);
                            }
                        } else {
                            if (node->buf->pos == 0) {
                                // printf("1st feed end... %f\n",m->in_tm);
                                qtk_img_rec_feed(m->img, 0, 0, 1);
                            } else {
                                len = node->buf->pos / cfg->wake1st_channel;
                                // m->in_tm+=len/(1000*32.0f);	//change bytes
                                // to ms time
                                m->in_tm += len;
                                wtk_queue_push(&(m->cache_q), &(node->qn));
                                qtk_mulsv_pop_cache_q(m);
                                qtk_img_rec_feed(m->img, node->buf->data, len,
                                                 0);
                                node = 0;
                            }
                        }
                        break;
		case QTK_MULSV_MSG_2ND_PCM:
#ifdef USE_2ND_OFFLINE
                    if (cfg->use_1st) {
                        wtk_bfio_start(m->bfio);
                        if (node->buf->pos == 0) {
                            // printf("2nd feed end...\n");
                            wtk_bfio_feed(m->bfio, 0, 0, 1);
                        } else {
                            len =
                                node->buf->pos / (sizeof(short) * bfio_channel);
                            ptr = (short *)node->buf->data;
                            for (i = 0; i < bfio_channel; ++i) {
                                pcm[i] = ptr;
                                ptr += len;
                            }
                            wtk_bfio_feed(m->bfio, pcm, len, 0);
                            wtk_bfio_feed(m->bfio, 0, 0, 1);
                        }
                        wtk_strbuf_reset(m->wkd2nd_pcm);
                        m->pop_len = 0;
                        wtk_bfio_reset(m->bfio);
                    } else {
#endif
                        if (node->buf->pos == 0) {
                            // printf("2nd feed end...\n");
                            wtk_bfio_feed(m->bfio, 0, 0, 1);
                        } else {
                            len =
                                node->buf->pos / (sizeof(short) * bfio_channel);
                            ptr = (short *)node->buf->data;
                            for (i = 0; i < bfio_channel; ++i) {
                                pcm[i] = ptr;
                                ptr += len;
                            }
                            wtk_bfio_feed(m->bfio, pcm, len, 0);
                            if (!cfg->use_1st && cfg->use_ori_enroll) {
                                m->total_time_len += len * sizeof(short);
                                wtk_strbuf_push(m->wkd2nd_pcm, (char *)pcm[0],
                                                len * sizeof(short));
                                if (m->wkd2nd_pcm->pos >= CACHE_2ND_LEN) {
                                    tmp_len =
                                        m->wkd2nd_pcm->pos - CACHE_2ND_LEN;
                                    wtk_strbuf_pop(m->wkd2nd_pcm, NULL,
                                                   tmp_len);
                                    m->pop_len += tmp_len;
                                }
                            }
                        }
#ifdef USE_2ND_OFFLINE
                    }
#endif
                    break;
		case QTK_MULSV_MSG_ENROLL_START:
			// printf("enroll start............\n");
			m->eval=0;
			wtk_strbuf_reset(m->name);
			wtk_strbuf_push(m->name,node->buf->data,node->buf->pos);
			break;
		case QTK_MULSV_MSG_ENROLL_END:
			// printf("enroll end..........\n");
			v.data = m->name->data;
                        v.len = m->name->pos;
                        //#ifdef USE_MULSV_VPRINT_LOG
                        v_cnt =
                            wtk_net3_xvector_compute_get_spk_cnt(m->svprint->x);
                        if (v_cnt) {
                            printf("%.*s注册成功\n", v.len, v.data);
                        }
                        //#endif
                        wtk_svprint_enroll2file(m->svprint, &v);
                        wtk_svprint_reload(m->svprint);
                        m->eval = 1;
                        break;
		case QTK_MULSV_MSG_VPRINT_THRESH:
			qtk_mulsv_set_vprint_thresh(m,node->buf->data,node->buf->pos);
			break;
                case QTK_MULSV_MSG_NOTIFY_BIAS:
                    qtk_mulsv_set_notify_bias(m, node->buf->data,
                                              node->buf->pos);
                    break;
                default:
			break;
		}

		if(node)
		{
			qtk_lockmsg_push_node(m->msg_pool,node);
		}
	}

	qtk_mulsv_cache_q_clean(m);
        if (aec_pcm) {
            wtk_free(aec_pcm);
        }
        wtk_free(pcm);
	return 0;
}

void qtk_mulsv_aec_notify(qtk_mulsv_t *m, short **pv, int len, int is_end) {
    qtk_lockmsg_node_t *node;
    qtk_mulsv_cfg_t *cfg = m->cfg;
    char *pcm;

    if (0 == len) {
        // printf("1st feed end... %f\n",m->in_tm);
        qtk_img_rec_feed(m->img, NULL, 0, 1);
    } else {
        node = qtk_lockmsg_pop_node(m->msg_pool);
        m->in_tm += len << 1;
        int i;
	for (i = 0; i < cfg->wake1st_channel; i++) {
            wtk_strbuf_push(node->buf, (char *)pv[i], len << 1);
        }
        node->data_type = QTK_MULSV_MSG_1ST_PCM;
        wtk_queue_push(&(m->cache_q), &(node->qn));
        qtk_mulsv_pop_cache_q(m);
        pcm = (char *)pv[0];
        qtk_img_rec_feed(m->img, pcm, len << 1, 0);
    }
}

void qtk_mulsv_bfio_notify(qtk_mulsv_t *m, wtk_bfio_cmd_t cmd, short *data,
                           int len) {
    int
#ifdef USE_MULSV_WAKEUP2ND_LOG
#ifndef USE_BUF_TIME
        fs,
        f_len,
#endif
#endif
        tmp_len;
    static int
#ifdef USE_MULSV_WAKEUP2ND_LOG
        index = 0,
#endif
        max_buf_len = 0;
#ifdef USE_MULSV_WAKEUP2ND_LOG
#ifndef USE_BUF_TIME
    wtk_bfio_t *bfio = m->bfio;
#endif
#endif
    qtk_mulsv_cfg_t *cfg = m->cfg;
    max_buf_len = cfg->wkd_tm * 32;

    // printf("bfio cmd %d\n",cmd);
    switch (cmd) {
    case WTK_BFIO_VAD_START:
#ifdef USE_BUF_TIME
        if (!cfg->use_ori_enroll || cfg->use_1st) {
            wtk_strbuf_reset(m->wkd2nd_pcm);
        }
#endif
        break;
    case WTK_BFIO_VAD_DATA:
        if (!cfg->use_ori_enroll || cfg->use_1st) {
#ifdef USE_BUF_TIME
            m->total_time_len += len * sizeof(short);
#endif
            wtk_strbuf_push(m->wkd2nd_pcm, (char *)data, len * sizeof(short));
            if (m->wkd2nd_pcm->pos >= max_buf_len) {
                tmp_len = m->wkd2nd_pcm->pos - max_buf_len;
                wtk_strbuf_pop(m->wkd2nd_pcm, NULL, tmp_len);
                m->pop_len += tmp_len;
            }
        }
        break;
    case WTK_BFIO_VAD_END:
        break;
    case WTK_BFIO_WAKE:
        m->cnt_2nd++;
#ifdef USE_MULSV_WAKEUP2ND_LOG
#ifdef USE_BUF_TIME
        qtk_mulsv_save_pcm(m, "./tmp2nd/", m->wkd2nd_pcm->data,
                           m->wkd2nd_pcm->pos, 1, index++);
#else
        if (m->cfg->use_1st) {
            fs = (int)floor((bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                             m->cfg->vprint_test_loff) *
                            bfio->cfg->rate);
            f_len = (int)floor(((bfio->wake_fe + bfio->cfg->wake_ssl_fe) -
                                (bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                                 m->cfg->vprint_test_loff)) *
                               bfio->cfg->rate);
            if ((bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                 m->cfg->vprint_enroll_loff) < 0) {
                fs = 0;
                f_len = (int)floor(((bfio->wake_fe + bfio->cfg->wake_ssl_fe) *
                                    (bfio->cfg->rate)));
            }
        } else {
            fs = (int)floor((bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                             m->cfg->vprint_enroll_loff) *
                            bfio->cfg->rate);
            f_len = (int)floor(((bfio->wake_fe + bfio->cfg->wake_ssl_fe) -
                                (bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                                 m->cfg->vprint_enroll_loff)) *
                               bfio->cfg->rate);
            if ((bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                 m->cfg->vprint_enroll_loff) < 0) {
                fs = 0;
                f_len = (int)floor(((bfio->wake_fe + bfio->cfg->wake_ssl_fe) *
                                    (bfio->cfg->rate)));
            }
        }

        if (m->cfg->use_padding) {
            int bytes;
            bytes = cfg->vprint_feed_len - f_len * 2;
            if(bytes>0) {
                wtk_strbuf_t *btmp;
                btmp = wtk_strbuf_new(cfg->vprint_feed_len, 0);
                wtk_strbuf_push(btmp, m->wkd2nd_pcm->data + fs * 2 - m->pop_len,
                                bytes);
                wtk_strbuf_push(btmp, m->wkd2nd_pcm->data + fs * 2 - m->pop_len,
                                f_len * 2);
                qtk_mulsv_save_pcm(m,"./tmp2nd/",btmp->data,btmp->pos,1,index++);
                wtk_strbuf_delete(btmp);
            }else {
                qtk_mulsv_save_pcm(m, "./tmp2nd/",
                                   m->wkd2nd_pcm->data + fs * 2 - m->pop_len,
                                   cfg->vprint_feed_len, 1, index++);
            }
        } else {
            qtk_mulsv_save_pcm(m, "./tmp2nd/",
                               m->wkd2nd_pcm->data + fs * 2 - m->pop_len,
                               f_len * 2, 1, index++);
        }
#endif
#endif
        qtk_mulsv_vsprint_feed(m);
        break;
    case WTK_BFIO_WAKE_SSL:
        break;
    case WTK_BFIO_VAD_CANCEL:
#ifdef USE_BUF_TIME
        m->cancel_time_len += len * sizeof(short);
#endif
#ifndef USE_BUF_TIME
        if (!cfg->use_ori_enroll || cfg->use_1st) {
            if (m->wkd2nd_pcm) {
                m->wkd2nd_pcm->pos -= len * sizeof(short);
                m->wkd2nd_pcm->pos = max(m->wkd2nd_pcm->pos, 0);
            }
        }
#endif
        break;
    case WTK_BFIO_ASR_CANCEL:
        break;
    case WTK_BFIO_ASR_RES:
        break;
    case WTK_BFIO_WAKE_RES:
        break;
    case WTK_BFIO_SIL_END:
        break;
    case WTK_BFIO_SPEECH_END:
        break;
    case WTK_BFIO_WAKE2:
        break;
    default:
        break;
    }
}

void qtk_mulsv_img_notify(qtk_mulsv_t *m, int res, int start, int end)
{
    double fs, fe;
    qtk_mulsv_cfg_t *cfg = m->cfg;
    // qtk_lockmsg_node_t *node;

    fs = (double)(start * cfg->frame_samp);
    fe = (double)(end * cfg->frame_samp);
    if (m->cfg->use_oppo_log) {
        printf("%.3f %.3f one\n", fs, fe);
    }
    // printf("wakeup %f %f %f\n",fs,fe,m->in_tm);
    // vwake->notify(vwake->ths, WTK_KVADWAKE_WAKE, fs, fe, NULL, 0);
    qtk_mulsv_waked1st_pcm(m, fs, fe);
    m->cnt_1st++;
#ifdef USE_MULSV_WAKEUP1ST_LOG
    qtk_mulsv_save_pcm1st(m);
#endif
}

int qtk_mulsv_vsprint_feed(qtk_mulsv_t *m)
{
    int
#ifndef USE_BUF_TIME
        bytes = 0,
        fs = 0, len_2nd = 0,
#endif
        cnt = 0, ret = 0, i = 0;
    float sv_prob = 0.0, s = 0.0, e = 0.0;
    wtk_string_t *name;
    wtk_vecf_t *vec, *mean;
#ifndef USE_BUF_TIME
    short *data_2nd;
    qtk_mulsv_cfg_t *cfg = m->cfg;
    wtk_bfio_t *bfio = m->bfio;
#endif

#ifndef USE_BUF_TIME
    if (m->cfg->use_1st) {
        fs = (int)floor((bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                         m->cfg->vprint_test_loff) *
                        bfio->cfg->rate);
        len_2nd = (int)floor(((bfio->wake_fe + bfio->cfg->wake_ssl_fe) -
                              (bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                               m->cfg->vprint_test_loff)) *
                             bfio->cfg->rate);
        if ((bfio->wake_fs + bfio->cfg->wake_ssl_fs +
             m->cfg->vprint_enroll_loff) < 0) {
            fs = 0;
            len_2nd = (int)floor(
                ((bfio->wake_fe + bfio->cfg->wake_ssl_fe) * (bfio->cfg->rate)));
        }
    } else {
        fs = (int)floor((bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                         m->cfg->vprint_enroll_loff) *
                        bfio->cfg->rate);
        len_2nd = (int)floor(((bfio->wake_fe + bfio->cfg->wake_ssl_fe) -
                              (bfio->wake_fs + bfio->cfg->wake_ssl_fs +
                               m->cfg->vprint_enroll_loff)) *
                             bfio->cfg->rate);
        if ((bfio->wake_fs + bfio->cfg->wake_ssl_fs +
             m->cfg->vprint_enroll_loff) < 0) {
            fs = 0;
            len_2nd = (int)floor(
                ((bfio->wake_fe + bfio->cfg->wake_ssl_fe) * (bfio->cfg->rate)));
        }
    }

    if (len_2nd <= 0) {
        return ret;
    }

    data_2nd = (short *)(m->wkd2nd_pcm->data + fs * 2 - m->pop_len);

    if (m->cfg->use_oppo_log) {
        if (m->cfg->use_1st) {
#ifndef USE_2ND_OFFLINE
            printf("%.3f %.3f send_vprint\n", (float)fs / 16000,
                   (float)(fs + len_2nd) / 16000);
#endif
        } else {
            printf("%.3f %.3f send_vprint\n", (float)fs / 16000,
                   (float)(fs + len_2nd) / 16000);
        }
    }
#else
    if (m->cfg->use_oppo_log) {
        if (m->cfg->use_1st) {
            printf("%.3f %.3f send_vprint\n",
                   (float)(m->total_time_len - m->cancel_time_len -
                           m->wkd2nd_pcm->pos) /
                       32000,
                   (float)(m->total_time_len - m->cancel_time_len) / 32000);

        } else {
            printf("%.3f %.3f send_vprint\n",
                   (float)(m->total_time_len - m->cancel_time_len -
                           m->wkd2nd_pcm->pos) /
                       32000,
                   (float)(m->total_time_len - m->cancel_time_len) / 32000);
        }
    }
#endif
    wtk_svprint_start(m->svprint);

#ifndef USE_BUF_TIME
    if (m->cfg->use_padding) {
        if (len_2nd<(cfg->vprint_feed_len)>> 1) {
            bytes = ((cfg->vprint_feed_len) >> 1) - len_2nd;
            wtk_svprint_feed(m->svprint, data_2nd, bytes, 0);
            wtk_svprint_feed(m->svprint, data_2nd, len_2nd, 0);
        } else {
            wtk_svprint_feed(m->svprint, data_2nd, (cfg->vprint_feed_len >> 1),
                             0);
        }
    } else {
        wtk_svprint_feed(m->svprint, data_2nd, len_2nd, 0);
    }
#else
    wtk_svprint_feed(m->svprint, (short *)m->wkd2nd_pcm->data,
                     (m->wkd2nd_pcm->pos >> 1), 0);
#endif
    wtk_svprint_feed(m->svprint, 0, 0, 1);

    if (m->eval) {
        name = wtk_svprint_eval(m->svprint, &sv_prob);
        if (name != NULL) {
            if (m->cfg->use_oppo_log) {
#ifdef USE_2ND_OFFLINE
                printf("%.3f %.3f ", (float)fs / 16000 + (float)m->st,
                       (float)(fs + len_2nd) / 16000 + (float)(m->st));
#endif
                printf("%.*s\n", name->len, name->data);
            }
#ifdef USE_MULSV_VPRINT_LOG
                printf("%.3f %.3f %.*s\n", s, e, name->len, name->data);
#endif
                if (m->total_log) {
                    printf("%.*s唤醒 %.3f %.3f\n", name->len, name->data, s, e);
                }
            m->cnt_vprint++;
            if (m->notify) {
                m->notify(m->notify_ths, sv_prob, name->data, name->len);
            }
        } else {
            if (m->cfg->use_oppo_log) {
#ifdef USE_2ND_OFFLINE
                printf("%.3f %.3f ", (float)fs / 16000 + (float)m->st,
                       (float)(fs + len_2nd) / 16000 + (float)(m->st));
#endif
                printf("nobody\n");
            }
#ifdef USE_MULSV_VPRINT_LOG
                printf("%.3f %.3f nobody\n", s, e);
#endif
            if (m->notify) {
                m->notify(m->notify_ths, sv_prob, 0, 0);
            }
        }
    } else {
        m->svprint->x->spk_cnt++;
        vec = wtk_svprint_compute_feat2(m->svprint, &cnt);
        mean = m->svprint->x->spk_mean;
        for (i = 0; i < mean->len; i++) {
            mean->p[i] += vec->p[i];
        }
    }
    wtk_svprint_reset(m->svprint);

    return ret;
}

void qtk_mulsv_set_vprint_thresh(qtk_mulsv_t *m,char *data,int len)
{
    m->svprint->cfg->score_thresh =
        wtk_float_clip(wtk_str_atof(data, len), -1, 1);
}

void qtk_mulsv_set_notify_bias(qtk_mulsv_t *m, char *data, int len) {
    m->img->cfg->notify_bias = wtk_str_atoi(data, len);
    if (m->img->cfg->notify_bias < 0) {
        m->img->cfg->notify_bias = 0;
    }
    if (m->img->cfg->notify_bias > 10) {
        m->img->cfg->notify_bias = 10;
    }
    printf("set bias = %d\n", m->img->cfg->notify_bias);
}

void qtk_mulsv_restore_pcm(qtk_mulsv_t *m,char *src,int len,wtk_strbuf_t *dst,int channel)
{
    // qtk_mulsv_cfg_t *cfg=m->cfg;
    int mono_len, i, j;
    char *ptr;

    if (dst->length < len) {
        wtk_strbuf_expand(dst, len);
	}

	mono_len=len/channel;
	ptr=dst->data;
	for(i=0;i<channel;++i)
	{
		m->data_ptr[i]=ptr;
		ptr+=mono_len;
	}

	ptr=src;
	for(i=0;i<mono_len;i+=2)
	{
		for(j=0;j<channel;++j)
		{
			*(m->data_ptr[j]+i)=*ptr++;
			*(m->data_ptr[j]+i+1)=*ptr++;
		}
	}

	dst->pos=len;
}

void qtk_mulsv_waked1st_pcm(qtk_mulsv_t *m, float fs, float fe) {
    qtk_mulsv_cfg_t *cfg = m->cfg;
    char
#ifdef USE_MULSV_WAKEUP1ST_LOG
        *dst_p_1st = NULL,
#endif
        *ptr = NULL, *dst_p = NULL;
    float right = cfg->wake1st_right_tm;
    int64_t len = 0, clen = 0, st = 0, ed = 0, bytes = 0, off = 0,
            target_bytes = (int64_t)(cfg->wkd_tm * 32);
    int state = 0, i = 0, pos = 0, pad = 0;
    wtk_queue_t *q = &(m->cache_q);
    wtk_queue_node_t *qn = NULL;
    qtk_lockmsg_node_t *node = NULL, *node2 = NULL;
    wtk_strbuf_t *buf = NULL, *buf2 = NULL;

    len = qtk_mulsv_cache_q_len(m);
    // bytes=((int)round(m->in_tm*1000))*32;
    bytes = m->in_tm;

    ed = ceil((fe + right) * 1000 * 32);
    if (ed > bytes || cfg->use_1st_notify_wav) {
        ed = bytes;
    }
    if (ed % sizeof(short) != 0) {
        ed = ed - (ed % sizeof(short));
    }
    bytes = bytes - len;
    st = ed - target_bytes;
    if (st < bytes) {
        st = bytes;
    }
    if (st < 0) {
        st = 0;
    }
    m->st = (double)st / 32000;
    m->ed = (double)ed / 32000;
    if (m->cfg->use_oppo_log) {
#ifndef USE_2ND_OFFLINE
        printf("%.3f %.3f send_two\n", m->st, m->ed);
#endif
    }
    if (ed - st < target_bytes) {
        pad = 1;
    }
#ifdef USE_MULSV_WAKEUP1ST_LOG
	wtk_strbuf_reset(m->wkd1st_pcm);
	m->wkd1st_pcm->pos=(ed-st)*cfg->wake1st_channel;
	// m->wkd1st_pcm->pos=target_bytes*cfg->wake1st_channel;
#endif
        node2=qtk_lockmsg_pop_node(m->msg_pool);
	if(cfg->use_2nd_pad_sp)
	{
		wtk_strbuf_expand(node2->buf,(ed-st)*(1+cfg->wake1st_channel));
		// wtk_strbuf_expand(node2->buf,target_bytes*(1+cfg->wake1st_channel));
	}else
	{
		wtk_strbuf_expand(node2->buf,(ed-st)*cfg->wake1st_channel);
		// wtk_strbuf_expand(node2->buf,target_bytes*cfg->wake1st_channel);
	}
	node2->buf->pos=(ed-st)*cfg->wake1st_channel;
	// node2->buf->pos=target_bytes*cfg->wake1st_channel;
	node2->data_type=QTK_MULSV_MSG_2ND_PCM;

	state=0;pos=0;
	for(qn=q->pop;qn;qn=qn->next)
	{
		node=data_offset2(qn, qtk_lockmsg_node_t, qn);
		len=node->buf->pos/cfg->wake1st_channel;

		switch(state)
		{
		case 0:
			// printf("bytes %d %d %d\n",bytes,len,st);
			if( (bytes+len) > st )
			{
				off=st-bytes;
				if(off<0){goto end;}
				clen=len-off;
				if(clen>ed-st)
				{
					clen=ed-st;
				}
				if(clen<=0){goto end;}

#ifdef USE_MULSV_WAKEUP1ST_LOG
                                dst_p_1st = m->wkd1st_pcm->data + pos;
#endif
                                dst_p = node2->buf->data + pos;
                                ptr=node->buf->data+off;
				for(i=0;i<cfg->wake1st_channel;++i)
				{
					memcpy(dst_p,ptr,clen);
                                        dst_p += (ed - st);
#ifdef USE_MULSV_WAKEUP1ST_LOG
                                        memcpy(dst_p_1st, ptr, clen);
                                        dst_p_1st += (ed - st);
#endif
                                        // dst_p+=target_bytes;
					ptr+=len;
				}
				pos+=clen;
				state=1;
				// printf("off %d %d %d\n",off,clen,pos);
			}
			break;
		case 1:
			if(bytes<ed)
			{
				clen=len<(ed-bytes)?len:(ed-bytes);
				if(clen<=0){goto end;}

#ifdef USE_MULSV_WAKEUP1ST_LOG
				// printf("%d %d %d\n",bytes,ed,clen);
                                dst_p_1st = m->wkd1st_pcm->data + pos;
#endif
                                dst_p = node2->buf->data + pos;
                                ptr=node->buf->data;
				for(i=0;i<cfg->wake1st_channel;++i)
				{
					memcpy(dst_p,ptr,clen);
					dst_p+=(ed-st);
#ifdef USE_MULSV_WAKEUP1ST_LOG
                                        memcpy(dst_p_1st, ptr, clen);
                                        dst_p_1st += (ed - st);
#endif
                                        ptr += len;
                                        // dst_p+=target_bytes;
				}
				pos+=clen;
			}else
			{
				state=2;
			}
			break;
		case 2:
			break;
		default:
			break;
		}

		if(state==2){break;}
		bytes+=node->buf->pos/cfg->wake1st_channel;
	}

	if(pad)
	{
		buf2=node2->buf;
		buf=qtk_mulsv_pad(m,buf2->data,ed-st,target_bytes-(ed-st),cfg->wake1st_channel);
		if(buf)
		{
			wtk_strbuf_delete(node2->buf);
			node2->buf=buf;
		}
	}

	if(node2)
	{
		if(cfg->use_2nd_pad_sp)
		{
			if(pad)
			{
				wtk_strbuf_push(node2->buf,0,target_bytes);
			}else
			{
				wtk_strbuf_push(node2->buf,0,ed-st);
			}
		}
		wtk_blockqueue_push_front(&(m->q),&(node2->qn));
	}

end:
	// exit(0);
	return;
}

wtk_strbuf_t* qtk_mulsv_pad(qtk_mulsv_t *m,char *src,int src_len,int pad_len,int channel)
{
	wtk_strbuf_t *buf;
	qtk_mulsv_cfg_t *cfg=m->cfg;
	int i;
	int dst_len=src_len+pad_len;
	int pad_bytes,bytes;
	int pad_end_pos=src_len/3;
	char *ptr,*dst_p,*pad_p,*pad_dst;
	int target_bytes=cfg->wkd_tm*32;

	if(cfg->use_2nd_pad_sp)
	{
		buf=wtk_strbuf_new(dst_len*(1+channel),0);
	}else
	{
		buf=wtk_strbuf_new(dst_len*channel,0);
	}
	if(buf==0){goto end;}
	if(pad_end_pos%sizeof(short)!=0)
	{
		pad_end_pos=pad_end_pos-(pad_end_pos%sizeof(short));
	}

	pad_dst=buf->data;
	pad_p=src;
	for(i=0;i<channel;++i)
	{
		pad_bytes=pad_len;
		dst_p=pad_dst;
		ptr=pad_p;
		while(pad_bytes>0)
		{
			bytes=min(pad_bytes,pad_end_pos);
			memcpy(dst_p,ptr,bytes);

			dst_p+=bytes;
			pad_bytes-=bytes;
		}
		memcpy(dst_p,ptr,src_len);

		pad_dst+=target_bytes;
		pad_p+=src_len;
	}
	buf->pos=dst_len*channel;

end:
	return buf;
}

int qtk_mulsv_cache_q_len(qtk_mulsv_t *m)
{
	int len;
	wtk_queue_t *q=&(m->cache_q);
	qtk_mulsv_cfg_t *cfg=m->cfg;
	wtk_queue_node_t *qn;
	qtk_lockmsg_node_t *node;

	len=0;
	for(qn=q->pop;qn;qn=qn->next)
	{
		node=data_offset2(qn, qtk_lockmsg_node_t, qn);
		len+=(node->buf->pos/cfg->wake1st_channel);
	}

	return len;
}

void qtk_mulsv_pop_cache_q(qtk_mulsv_t *m)
{
	int len,len2;
	float tm;
	wtk_queue_node_t *qn;
	qtk_lockmsg_node_t *node;
	qtk_mulsv_cfg_t *cfg=m->cfg;

	len=qtk_mulsv_cache_q_len(m);
	do{
		qn=m->cache_q.pop;
		if(qn==0){break;}
		node=data_offset2(qn, qtk_lockmsg_node_t, qn);
		len2=node->buf->pos/cfg->wake1st_channel;
		tm=(len-len2)/32.0;
		if(tm>cfg->cache_tm)
		{
			qn=wtk_queue_pop(&(m->cache_q));
			qtk_lockmsg_push_node(m->msg_pool,node);
			len=len-len2;
		}else
		{
			break;
		}
	}while(qn);
}


void qtk_mulsv_cache_q_clean(qtk_mulsv_t *m)
{
	wtk_queue_t *q=&(m->cache_q);
	wtk_queue_node_t *qn;
	qtk_lockmsg_node_t *node;

	do
	{
		qn=wtk_queue_pop(q);
		if(qn==0){break;}
		node=data_offset2(qn, qtk_lockmsg_node_t, qn);
		qtk_lockmsg_push_node(m->msg_pool,node);
	}while(qn);
}

wtk_strbuf_t* qtk_mulsv_convert_pcm(qtk_mulsv_t *m,wtk_strbuf_t *src,int channel)
{
	wtk_strbuf_t *b;
	char *ptr;
        int len, i, j;
        char **data;

	b=wtk_strbuf_new(src->pos,0.0);
	len=src->pos/channel;
	data=wtk_malloc(sizeof(char*)*channel);
	ptr=src->data;
	for(i=0;i<channel;++i)
	{
		data[i]=ptr;
		ptr+=len;
	}
	
	for(j=0;j<len;j+=2)
	{
		for(i=0;i<channel;++i)
		{
			wtk_strbuf_push(b,data[i]+j,2);
		}
	}

        wtk_free(data);
	return b;
}

void qtk_mulsv_save_pcm_do(qtk_mulsv_t *m,char *fn,char *data,int len,int channel)
{
    wtk_wavfile_t *w;

    w = wtk_wavfile_new(16000);
    wtk_wavfile_set_channel(w, channel);
    wtk_wavfile_open(w, fn);

    wtk_wavfile_write(w, data, len);

    wtk_wavfile_close(w);
    wtk_wavfile_delete(w);
}

void qtk_mulsv_save_pcm1st(qtk_mulsv_t *m)
{
	static int i=0;
	wtk_strbuf_t *buf;
	qtk_mulsv_cfg_t *cfg=m->cfg;

	buf=qtk_mulsv_convert_pcm(m,m->wkd1st_pcm,cfg->wake1st_channel);

	qtk_mulsv_save_pcm(m,"./tmp1st/",buf->data,buf->pos,cfg->wake1st_channel,i++);

	wtk_strbuf_delete(buf);
}

void qtk_mulsv_save_pcm(qtk_mulsv_t *m,char *sub_dir,char *data,int len,int channel,int index)
{
	// static int i=0;
        wtk_strbuf_t *name;

        name=wtk_strbuf_new(64,0);
	wtk_strbuf_push(name,sub_dir,strlen(sub_dir));
	wtk_strbuf_push_f(name,"%d",index);
	wtk_strbuf_push(name,".wav",sizeof(".wav"));

	qtk_mulsv_save_pcm_do(m,name->data,data,len,channel);

	wtk_strbuf_delete(name);
}

int qtk_mulsv_pad2(qtk_mulsv_t *m,char *dst,char *src,int src_len,int src_len_lda,int pad_len)
{
/** 			if(pad)
// 				{
// #ifdef USE_MULSV_WAKEUP1ST_LOG
// 					dst_p=m->wkd1st_pcm->data+pos;
// #else
// 					dst_p=node2->buf->data+pos;
// #endif
// 					ptr=node->buf->data+off;
// 					pad_bytes=target_bytes-(ed-st);
// 					qtk_mulsv_pad(m,dst_p,ptr,clen,len,pad_bytes);
// 					pos+=pad_bytes;
// 				}
*********/
	qtk_mulsv_cfg_t *cfg=m->cfg;
	int pad_bytes;
	int i;
	char *ptr,*dst_p,*pad_p,*pad_dst;
	int target_bytes=cfg->wkd_tm*32;
        int bytes2;

        pad_dst=dst;
	pad_p=src;
	for(i=0;i<cfg->wake1st_channel;++i)
	{
		// pad_bytes=target_bytes-real_len;
		pad_bytes=pad_len;
		dst_p=pad_dst;
		ptr=pad_p;
		while(pad_bytes>0)
		{
			bytes2=min(src_len,pad_bytes);
			memcpy(dst_p,ptr,bytes2);

			dst_p+=bytes2;
			pad_bytes-=bytes2;
		}
		
		pad_dst+=target_bytes;
		pad_p+=src_len_lda;
	}

	return 0;
}
