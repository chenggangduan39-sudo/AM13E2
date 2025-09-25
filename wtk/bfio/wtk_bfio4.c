#include "wtk_bfio4.h" 

void wtk_bfio4_on_wake(wtk_bfio4_wake_t *wake, int res, float prob, int start, int end);
void wtk_bfio4_on_asr(wtk_bfio4_asr_t *asr, int res, float prob, int start, int end);
void wtk_bfio4_on_denoise(wtk_bfio4_t *bfio4, short *data, int len, int is_end);

int wtk_bfio4_run_wake(wtk_bfio4_t *bfio4,wtk_thread_t *t);
int wtk_bfio4_run_asr(wtk_bfio4_t *bfio4,wtk_thread_t *t);

wtk_bfio4_msg_t *wtk_bfio4_msg_new(wtk_bfio4_t *bfio4)
{
	wtk_bfio4_msg_t *msg;

	msg=(wtk_bfio4_msg_t *)wtk_malloc(sizeof(wtk_bfio4_msg_t));
	msg->pv=(short *)wtk_malloc(bfio4->cfg->min_pvlen*sizeof(short));
	msg->len=0;
	
	return msg;
}

void wtk_bfio4_msg_delete(wtk_bfio4_msg_t *msg)
{
	wtk_free(msg->pv);
	wtk_free(msg);
}

wtk_bfio4_msg_t* wtk_bfio4_pop_msg(wtk_bfio4_t *bfio4)
{
	return  (wtk_bfio4_msg_t*)wtk_lockhoard_pop(&(bfio4->msg_lockhoard));
}

void wtk_bfio4_push_msg(wtk_bfio4_t *bfio4,wtk_bfio4_msg_t *msg)
{
	wtk_lockhoard_push(&(bfio4->msg_lockhoard),msg);
}

//////////////////////////////////////////////////////////////////////////////////
wtk_bfio4_wake_t *wtk_bfio4_wake_new(wtk_bfio4_t *bfio4, wtk_bfio4_cfg_t *cfg, wtk_bfio4_model_t model)
{
    wtk_bfio4_wake_t *wake;

    wake=(wtk_bfio4_wake_t *)wtk_malloc(sizeof(wtk_bfio4_wake_t));
    wake->hook=bfio4;
    wake->wake_model=model;

    wake->img=qtk_img_rec_new(cfg->img);
    qtk_img_rec_set_notify(wake->img,(qtk_img_rec_notify_f)wtk_bfio4_on_wake,wake);

    return wake;
}

void wtk_bfio4_wake_reset(wtk_bfio4_wake_t *wake)
{
    wake->waked=0;
    wake->wake_fs=0;
    wake->wake_fe=0;

    qtk_img_rec_reset2(wake->img);
}

void wtk_bfio4_wake_start(wtk_bfio4_wake_t *wake)
{
    qtk_img_rec_start(wake->img);
}

void wtk_bfio4_wake_delete(wtk_bfio4_wake_t *wake)
{
    qtk_img_rec_delete(wake->img);
    wtk_free(wake);
}
//////////////////////////////////////////////////////////////////////////////////
wtk_bfio4_asr_t *wtk_bfio4_asr_new(wtk_bfio4_t *bfio4, wtk_bfio4_cfg_t *cfg, wtk_bfio4_model_t model)
{
    wtk_bfio4_asr_t *asr;

    asr=(wtk_bfio4_asr_t *)wtk_malloc(sizeof(wtk_bfio4_asr_t));
    asr->hook=bfio4;
    asr->asr_model=model;

    asr->img2=qtk_img2_rec_new(cfg->img2);
    qtk_img2_rec_set_notify(asr->img2,(qtk_img2_rec_notify_f)wtk_bfio4_on_asr,asr);

    return asr;
}

void wtk_bfio4_asr_reset(wtk_bfio4_asr_t *asr)
{
    asr->asr_id=0;
    asr->asr_fs=0;
    asr->asr_fe=0;

    qtk_img2_rec_reset2(asr->img2);
}

void wtk_bfio4_asr_start(wtk_bfio4_asr_t *asr)
{
    qtk_img2_rec_start(asr->img2);
}

void wtk_bfio4_asr_delete(wtk_bfio4_asr_t *asr)
{
    qtk_img2_rec_delete(asr->img2);
    wtk_free(asr);
}
//////////////////////////////////////////////////////////////////////////////////

wtk_bfio4_t* wtk_bfio4_new(wtk_bfio4_cfg_t *cfg)
{
    wtk_bfio4_t *bfio4;

    bfio4=(wtk_bfio4_t *)wtk_malloc(sizeof(wtk_bfio4_t));
    bfio4->cfg=cfg;
    bfio4->ths=NULL;
    bfio4->notify=NULL;

    bfio4->denoise=NULL;
    bfio4->wake=NULL;
    bfio4->asr=NULL;
    bfio4->wake_buf=NULL;
    bfio4->asr_buf=NULL;
    bfio4->wake_cnt = 0;
    bfio4->asr_cnt = 0;
    
    if(cfg->use_denoise_wake || cfg->use_denoise_asr){
        bfio4->denoise=wtk_gainnet_denoise_new(&(cfg->denoise));
        wtk_gainnet_denoise_set_notify(bfio4->denoise, bfio4, (wtk_gainnet_denoise_notify_f)wtk_bfio4_on_denoise);
        if(cfg->use_denoise_wake){
            bfio4->wake_cnt += 1;
        }
        if(cfg->use_denoise_asr){
            bfio4->asr_cnt += 1;
        }
    }

    if(cfg->use_wake){
        bfio4->wake_cnt += 1;
    }
    if(cfg->use_asr){
        bfio4->asr_cnt += 1;
    }

    if(bfio4->wake_cnt == 0 && bfio4->asr_cnt == 0){
        wtk_debug("No wake or asr mode selected!");
        bfio4 = NULL;
        goto end;
    }

    if(bfio4->wake_cnt > 0){
        bfio4->wake = (wtk_bfio4_wake_t **)wtk_malloc(sizeof(wtk_bfio4_wake_t *) * bfio4->wake_cnt);
        if(bfio4->wake_cnt == 1){
            bfio4->wake[0]=wtk_bfio4_wake_new(bfio4, cfg, WTK_BFIO4_MODEL_WAKE);
            if(bfio4->denoise){
                qtk_img_thresh_set_cfg(bfio4->wake[0]->img, cfg->img_denoise, 0);
            }
        }else{
            bfio4->wake[0]=wtk_bfio4_wake_new(bfio4, cfg, WTK_BFIO4_MODEL_WAKE);
            bfio4->wake[1]=wtk_bfio4_wake_new(bfio4, cfg, WTK_BFIO4_MODEL_DENOISE);
            qtk_img_thresh_set_cfg(bfio4->wake[1]->img, cfg->img_denoise, 1);
            bfio4->wake_buf=wtk_strbuf_new(1024,1);
        }
    }

    if(bfio4->asr_cnt > 0){
        bfio4->asr = (wtk_bfio4_asr_t **)wtk_malloc(sizeof(wtk_bfio4_asr_t *) * bfio4->asr_cnt);
        if(bfio4->asr_cnt == 1){
            bfio4->asr[0]=wtk_bfio4_asr_new(bfio4, cfg, WTK_BFIO4_MODEL_ASR);
            if(bfio4->denoise){
#if 0
                qtk_img2_thresh_set_cfg(bfio4->asr[0]->img2, cfg->img2_denoise.av_prob0, cfg->img2_denoise.maxx0, cfg->img2_denoise.avx0, cfg->img2_denoise.max_prob0,\
                    cfg->img2_denoise.av_prob1, cfg->img2_denoise.maxx1, cfg->img2_denoise.avx1, cfg->img2_denoise.max_prob1);
#endif
            }
        }else{
            bfio4->asr[0]=wtk_bfio4_asr_new(bfio4, cfg, WTK_BFIO4_MODEL_ASR);
            bfio4->asr[1]=wtk_bfio4_asr_new(bfio4, cfg, WTK_BFIO4_MODEL_DENOISE);
#if 0
            qtk_img2_thresh_set_cfg(bfio4->asr[1]->img2, cfg->img2_denoise.av_prob0, cfg->img2_denoise.maxx0, cfg->img2_denoise.avx0, cfg->img2_denoise.max_prob0,\
                cfg->img2_denoise.av_prob1, cfg->img2_denoise.maxx1, cfg->img2_denoise.avx1, cfg->img2_denoise.max_prob1);
            bfio4->asr_buf=wtk_strbuf_new(1024,1);
#endif
        }
    }
    bfio4->run=0;
    if(cfg->use_thread){
        wtk_lockhoard_init(&(bfio4->msg_lockhoard),offsetof(wtk_bfio4_msg_t,hoard_n),10,
                (wtk_new_handler_t)wtk_bfio4_msg_new,
				(wtk_delete_handler_t)wtk_bfio4_msg_delete,
                bfio4);
    }

    wtk_bfio4_reset(bfio4);
end:
    return bfio4;
}

void wtk_bfio4_delete(wtk_bfio4_t *bfio4)
{
    if(bfio4->run==1)
	{
		bfio4->run=0;
        wtk_blockqueue_wake(&(bfio4->wake_q));
        wtk_thread_join(&(bfio4->thread[0]));
        wtk_thread_clean(&(bfio4->thread[0]));
        wtk_blockqueue_clean(&(bfio4->wake_q));

        wtk_blockqueue_wake(&(bfio4->asr_q));
        wtk_thread_join(&(bfio4->thread[1]));
        wtk_thread_clean(&(bfio4->thread[1]));
        wtk_blockqueue_clean(&(bfio4->asr_q));

		wtk_sem_clean(&(bfio4->end_sem));
		wtk_sem_clean(&(bfio4->end_sem2));
	}
	if(bfio4->cfg->use_thread)
	{
		wtk_lockhoard_clean(&(bfio4->msg_lockhoard));
	}

    if(bfio4->wake_cnt == 1){
        wtk_bfio4_wake_delete(bfio4->wake[0]);
    }else if(bfio4->wake_cnt == 2){
        wtk_bfio4_wake_delete(bfio4->wake[0]);
        wtk_bfio4_wake_delete(bfio4->wake[1]);
    }
    wtk_free(bfio4->wake);
    if(bfio4->asr_cnt == 1){
        wtk_bfio4_asr_delete(bfio4->asr[0]);
    }else if(bfio4->asr_cnt == 2){
        wtk_bfio4_asr_delete(bfio4->asr[0]);
        wtk_bfio4_asr_delete(bfio4->asr[1]);
    }
    wtk_free(bfio4->asr);

    if(bfio4->denoise){
        wtk_gainnet_denoise_delete(bfio4->denoise);
    }
    if(bfio4->wake_buf){
        wtk_strbuf_delete(bfio4->wake_buf);
    }
    if(bfio4->asr_buf){
        wtk_strbuf_delete(bfio4->asr_buf);
    }
    wtk_free(bfio4);
}

void wtk_bfio4_reset(wtk_bfio4_t *bfio4)
{
    if(bfio4->run==1)
	{
		bfio4->run=0;
        wtk_blockqueue_wake(&(bfio4->wake_q));
        wtk_thread_join(&(bfio4->thread[0]));
        wtk_thread_clean(&(bfio4->thread[0]));
        wtk_blockqueue_clean(&(bfio4->wake_q));

        wtk_blockqueue_wake(&(bfio4->asr_q));
        wtk_thread_join(&(bfio4->thread[1]));
        wtk_thread_clean(&(bfio4->thread[1]));
        wtk_blockqueue_clean(&(bfio4->asr_q));

		wtk_sem_clean(&(bfio4->end_sem));
		wtk_sem_clean(&(bfio4->end_sem2));
	}

    bfio4->is_end=0;

    bfio4->waked=0;
    bfio4->wake_fs=bfio4->wake_fe=-1;
    bfio4->wake_prob=0;
    bfio4->asr_fs=bfio4->asr_fe=-1;
    bfio4->asr_prob=0;
    bfio4->asr_id=0;

    if(bfio4->wake_cnt == 1){
        wtk_bfio4_wake_reset(bfio4->wake[0]);
    }else if(bfio4->wake_cnt == 2){
        wtk_bfio4_wake_reset(bfio4->wake[0]);
        wtk_bfio4_wake_reset(bfio4->wake[1]);
    }
    if(bfio4->asr_cnt == 1){
        wtk_bfio4_asr_reset(bfio4->asr[0]);
    }else if(bfio4->asr_cnt == 2){
        wtk_bfio4_asr_reset(bfio4->asr[0]);
        wtk_bfio4_asr_reset(bfio4->asr[1]);
    }
    if(bfio4->denoise){
        wtk_gainnet_denoise_reset(bfio4->denoise);
    }
    if(bfio4->wake_buf){
        wtk_strbuf_reset(bfio4->wake_buf);
    }
    if(bfio4->asr_buf){
        wtk_strbuf_reset(bfio4->asr_buf);
    }
}

void wtk_bfio4_set_notify(wtk_bfio4_t *bfio4,void *ths,wtk_bfio4_notify_f notify)
{
    bfio4->ths=ths;
    bfio4->notify=notify;
}

void wtk_bfio4_start(wtk_bfio4_t *bfio4)
{
    if(bfio4->wake_cnt == 1){
        wtk_bfio4_wake_start(bfio4->wake[0]);
    }else if(bfio4->wake_cnt == 2){
        wtk_bfio4_wake_start(bfio4->wake[0]);
        wtk_bfio4_wake_start(bfio4->wake[1]);
    }
    if(bfio4->asr_cnt == 1){
        wtk_bfio4_asr_start(bfio4->asr[0]);
    }else if(bfio4->asr_cnt == 2){
        wtk_bfio4_asr_start(bfio4->asr[0]);
        wtk_bfio4_asr_start(bfio4->asr[1]);
    }
	if(bfio4->cfg->use_thread)
	{
		if(bfio4->run==0)
		{
			wtk_sem_init(&(bfio4->end_sem),0);
			wtk_sem_init(&(bfio4->end_sem2),0);
			bfio4->run=1;

			wtk_blockqueue_init(&(bfio4->wake_q));
			wtk_blockqueue_init(&(bfio4->asr_q));

			wtk_thread_init(&(bfio4->thread[0]),(thread_route_handler)wtk_bfio4_run_wake,bfio4);
			wtk_thread_init(&(bfio4->thread[1]),(thread_route_handler)wtk_bfio4_run_asr,bfio4);
			wtk_thread_start(&(bfio4->thread[0]));
			wtk_thread_start(&(bfio4->thread[1]));		
		}
	}
}

void wtk_bfio4_set_waked(wtk_bfio4_t *bfio4, float wake_fs, float wake_fe, float wake_prob, int need_notify)
{
    int i;
    bfio4->wake_fs=wake_fs;
    bfio4->wake_fe=wake_fe;
    bfio4->wake_prob=wake_prob;
    if(need_notify && bfio4->notify)
    {
        bfio4->notify(bfio4->ths, WTK_BFIO4_WAKE, 0);
    }
    for (i = 0; i < bfio4->wake_cnt; ++i) {
        bfio4->wake[i]->wake_prob=0;
        bfio4->wake[i]->waked=0;
    }
}

void wtk_bfio4_set_asred(wtk_bfio4_t *bfio4, float asr_fs, float asr_fe, float asr_prob, int id)
{
    int i;
    bfio4->asr_fs=asr_fs;
    bfio4->asr_fe=asr_fe;
    bfio4->asr_prob=asr_prob;
    bfio4->asr_id=id;
    if(bfio4->notify)
    {
        bfio4->notify(bfio4->ths, WTK_BFIO4_ASR, id);
    }
    for (i = 0; i < bfio4->asr_cnt; ++i) {
        bfio4->asr[i]->asr_id=0;
        bfio4->asr[i]->asr_prob=0;
    }
}

static void _bfio4_waked_post(wtk_bfio4_t *bfio4){
    float max_wake_prob = -50;
    int max_idx = -1;
    wtk_bfio4_wake_t **wake = bfio4->wake;
    int i;

    for (i = 0; i < bfio4->wake_cnt; ++i) {  // 挑选置信度最高的唤醒
        if (wake[i]->waked == 1) {
            if (bfio4->cfg->debug) {
                wtk_debug("wake[%d] wake_prob=%f wake_ts=[%f %f]\n",
                          i, wake[i]->wake_prob,
                          wake[i]->wake_fs, wake[i]->wake_fe);
            }
            if (wake[i]->wake_prob > max_wake_prob) {
                max_idx = i;
                max_wake_prob = wake[i]->wake_prob;
            }
        }
    }

    if (max_idx >= 0) {
        if (fabs(wake[max_idx]->wake_fe - bfio4->wake_fe) < 1) { // 判断本次唤醒位置跟上次位置差距，如果小于1则不回调，但需要重新定位
            if (wake[max_idx]->wake_prob > bfio4->wake_prob) {
                if (bfio4->cfg->debug) {
                    wtk_debug("change waked [%f %f] to [%f %f ], wake[%d] "
                              "wake_prob=%f  lst_wakeprob=%f\n",
                              bfio4->wake_fs, bfio4->wake_fe,
                              wake[max_idx]->wake_fs, wake[max_idx]->wake_fe,
                              max_idx, wake[max_idx]->wake_prob, bfio4->wake_prob);
                }
                wtk_bfio4_set_waked(bfio4,wake[max_idx]->wake_fs, wake[max_idx]->wake_fe,wake[max_idx]->wake_prob,0);
            } else {
                if (bfio4->cfg->debug) {
                    wtk_debug("waked over [%f %f] pop [%f %f]\n", bfio4->wake_fs,
                              bfio4->wake_fe, wake[max_idx]->wake_fs,
                              wake[max_idx]->wake_fe);
                }
            }
        } else {  // 与上次唤醒大于1就回调唤醒消息
            wtk_bfio4_set_waked(bfio4,wake[max_idx]->wake_fs, wake[max_idx]->wake_fe,wake[max_idx]->wake_prob,1);
        }
    }
}

static void _bfio4_asred_post(wtk_bfio4_t *bfio4){
    float max_asr_prob = -50;
    int max_idx = -1;
    wtk_bfio4_asr_t **asr = bfio4->asr;
    int i;

    for (i = 0; i < bfio4->asr_cnt; ++i) {
        if(asr[i]->asr_id > 0){
            if(asr[i]->asr_prob > max_asr_prob){
                max_asr_prob = asr[i]->asr_prob;
                max_idx = i;
            }
        }
    }

    if (max_idx >= 0) {
        // wtk_debug("%f %f\n", asr[max_idx]->asr_fe, bfio4->asr_fe);
        if (fabs(asr[max_idx]->asr_fe - bfio4->asr_fe) >= 1) {
            // wtk_debug("asr[max_idx]->asr_id=%d\n", asr[max_idx]->asr_id);
            wtk_bfio4_set_asred(bfio4,asr[max_idx]->asr_fs, asr[max_idx]->asr_fe,asr[max_idx]->asr_prob,asr[max_idx]->asr_id);
        }
    }
}

void wtk_bfio4_on_wake(wtk_bfio4_wake_t *wake, int res, float prob, int start, int end)
{
    wake->waked=1;
    wake->wake_fs=start*0.08;  // 唤醒左边界
    wake->wake_fe=end*0.08;  // 唤醒右边界
    wake->wake_prob=prob;  // 唤醒置信度
    //wtk_debug("wake_prob=%f wake_ts=[%f %f]\n", wake->wake_prob,wake->wake_fs, wake->wake_fe);
	qtk_img_rec_reset2(wake->img);
}

void wtk_bfio4_on_asr(wtk_bfio4_asr_t *asr, int res, float prob, int start, int end)
{
    asr->asr_id=res;
    asr->asr_fs=start*0.08;  // 唤醒左边界
    asr->asr_fe=end*0.08;  // 唤醒右边界
    asr->asr_prob=prob;  // 唤醒置信度
    // wtk_debug("asr->asr_prob=%f [asr_fs=%f asr_ts=%f], asr->asr_id=[%d] %d\n", asr->asr_prob,asr->asr_fs, asr->asr_fe, asr->asr_id, res);
    qtk_img2_rec_reset2(asr->img2);
}

int wtk_bfio4_run_wake(wtk_bfio4_t *bfio4,wtk_thread_t *t){
    wtk_queue_node_t *qn;
	wtk_bfio4_msg_t *msg;
    wtk_strbuf_t *wake_buf=bfio4->wake_buf;

	while(bfio4->run)
	{
		qn=wtk_blockqueue_pop(&(bfio4->wake_q),-1,NULL);
		if(!qn){continue;}
		msg=data_offset2(qn,wtk_bfio4_msg_t,q_n);
		switch(msg->type)
		{
		case WTK_BFIO4_MSG_WAKE:
            bfio4->wake[0]->waked=0;
            qtk_img_rec_feed(bfio4->wake[0]->img, (char *)msg->pv,msg->len<<1, 0);
            _bfio4_waked_post(bfio4);
			wtk_sem_release(&(bfio4->end_sem),1);
			break;
		case WTK_BFIO4_MSG_WAKE_DENOISE:
            bfio4->wake[0]->waked=0;
            bfio4->wake[1]->waked=0;
            qtk_img_rec_feed(bfio4->wake[0]->img, wake_buf->data, min(msg->len<<1, wake_buf->pos), 0);
            qtk_img_rec_feed(bfio4->wake[1]->img, (char *)msg->pv,msg->len<<1, 0);
            wtk_strbuf_pop(wake_buf, NULL, min(msg->len<<1, wake_buf->pos));
            _bfio4_waked_post(bfio4);
			wtk_sem_release(&(bfio4->end_sem),1);
			break;
		case WTK_BFIO4_MSG_END:
            bfio4->wake[0]->waked=0;
            qtk_img_rec_feed(bfio4->wake[0]->img, NULL, 0, 1);
            _bfio4_waked_post(bfio4);
			wtk_sem_release(&(bfio4->end_sem),1);
			break;
		default:
			break;
		}

		wtk_bfio4_push_msg(bfio4, msg);
	}
	return 0;
}

int wtk_bfio4_run_asr(wtk_bfio4_t *bfio4,wtk_thread_t *t){
    wtk_queue_node_t *qn;
	wtk_bfio4_msg_t *msg;
    wtk_strbuf_t *asr_buf=bfio4->asr_buf;

	while(bfio4->run)
	{
		qn=wtk_blockqueue_pop(&(bfio4->asr_q),-1,NULL);
		if(!qn){continue;}
		msg=data_offset2(qn,wtk_bfio4_msg_t,q_n);
		switch(msg->type)
		{
		case WTK_BFIO4_MSG_ASR:
            bfio4->asr[0]->asr_id=0;
            qtk_img2_rec_feed(bfio4->asr[0]->img2, (char *)msg->pv,msg->len<<1, 0);
            _bfio4_asred_post(bfio4);
			wtk_sem_release(&(bfio4->end_sem2),1);
			break;
		case WTK_BFIO4_MSG_ASR_DENOISE:
            bfio4->asr[0]->asr_id=0;
            bfio4->asr[1]->asr_id=0;
            qtk_img2_rec_feed(bfio4->asr[0]->img2, asr_buf->data, min(msg->len<<1, asr_buf->pos), 0);
            qtk_img2_rec_feed(bfio4->asr[1]->img2, (char *)msg->pv,msg->len<<1, 0);
            wtk_strbuf_pop(asr_buf, NULL, min(msg->len<<1, asr_buf->pos));
            _bfio4_asred_post(bfio4);
			wtk_sem_release(&(bfio4->end_sem2),1);
			break;
		case WTK_BFIO4_MSG_END:
            bfio4->asr[0]->asr_id=0;
            qtk_img2_rec_feed(bfio4->asr[0]->img2, NULL, 0, 1);
            _bfio4_asred_post(bfio4);
			wtk_sem_release(&(bfio4->end_sem2),1);
			break;
		default:
			break;
		}

		wtk_bfio4_push_msg(bfio4, msg);
	}
	return 0;
}

void wtk_bfio4_on_denoise(wtk_bfio4_t *bfio4, short *data, int len, int is_end)
{
	wtk_bfio4_msg_t *msg, *msg2;
#ifdef USE_LOG_BFIO4

    static wtk_wavfile_t *denoise_log;
    if(!denoise_log)
    {
        denoise_log=wtk_wavfile_new(16000);
        wtk_wavfile_open(denoise_log,"wbf/wbf.1.wav");
        denoise_log->max_pend=0;
    }
    if(len>0)
    {
        wtk_wavfile_write(denoise_log,(char *)data,len<<1);
    }
    if(is_end)
    {
        wtk_debug("============ close ============\n");
        wtk_wavfile_close(denoise_log);
        wtk_wavfile_delete(denoise_log);
        denoise_log=NULL;
    }
#endif
    if(bfio4->cfg->use_thread && bfio4->wake && bfio4->asr){
        if(len>0){
            msg=wtk_bfio4_pop_msg(bfio4);
            memcpy(msg->pv, data, sizeof(short)*len);
            msg->len=len;
            msg->type=WTK_BFIO4_MSG_WAKE_DENOISE;
            wtk_blockqueue_push(&(bfio4->wake_q), &(msg->q_n));

            msg2=wtk_bfio4_pop_msg(bfio4);
            memcpy(msg2->pv, data,sizeof(short)*len);
            msg2->len=len;
            msg2->type=WTK_BFIO4_MSG_ASR_DENOISE;
            wtk_blockqueue_push(&(bfio4->asr_q), &(msg2->q_n));
            wtk_sem_acquire(&(bfio4->end_sem),-1);
			wtk_sem_acquire(&(bfio4->end_sem2),-1);
        }
        if(is_end){
            msg=wtk_bfio4_pop_msg(bfio4);
            msg->len=0;
            msg->type=WTK_BFIO4_MSG_END;
            wtk_blockqueue_push(&(bfio4->wake_q), &(msg->q_n));

            msg2=wtk_bfio4_pop_msg(bfio4);
            msg2->len=0;
            msg2->type=WTK_BFIO4_MSG_END;
            wtk_blockqueue_push(&(bfio4->asr_q), &(msg2->q_n));
            wtk_sem_acquire(&(bfio4->end_sem),-1);
			wtk_sem_acquire(&(bfio4->end_sem2),-1);
        }
    }else{
        if(bfio4->wake){

            if(bfio4->cfg->use_wake && bfio4->cfg->use_denoise_wake){
                bfio4->wake[0]->waked=0;
                if(is_end){
                    qtk_img_rec_feed(bfio4->wake[0]->img, NULL, 0, 1);
                }else{
                    qtk_img_rec_feed(bfio4->wake[0]->img, bfio4->wake_buf->data, len<<1, 0);
                    wtk_strbuf_pop(bfio4->wake_buf, NULL, len<<1);
                }
                bfio4->wake[1]->waked=0;
                if(is_end){
                    qtk_img_rec_feed(bfio4->wake[1]->img, NULL, 0, 1);
                }else{
                    qtk_img_rec_feed(bfio4->wake[1]->img, (char *)data, len<<1, 0);
                }
            }else{
                bfio4->wake[0]->waked=0;
                if(is_end){
                    qtk_img_rec_feed(bfio4->wake[0]->img, NULL, 0, 1);
                }else{
                    qtk_img_rec_feed(bfio4->wake[0]->img, (char *)data, len<<1, 0);
                }
            }
            _bfio4_waked_post(bfio4);
        }
        if(bfio4->asr){
            if(bfio4->cfg->use_asr && bfio4->cfg->use_denoise_asr){
                bfio4->asr[0]->asr_id=0;
                if(is_end){
                    qtk_img2_rec_feed(bfio4->asr[0]->img2, NULL, 0, 1);
                }else{
                    qtk_img2_rec_feed(bfio4->asr[0]->img2, bfio4->asr_buf->data, len<<1, 0);
                    wtk_strbuf_pop(bfio4->asr_buf, NULL, len<<1);
                }
                bfio4->asr[1]->asr_id=0;
                if(is_end){
                    qtk_img2_rec_feed(bfio4->asr[1]->img2, NULL, 0, 1);
                }else{
                    qtk_img2_rec_feed(bfio4->asr[1]->img2, (char *)data, len<<1, 0);
                }
            }else{
                bfio4->asr[0]->asr_id=0;
                if(is_end){
                    qtk_img2_rec_feed(bfio4->asr[0]->img2, NULL, 0, 1);
                }else{
                    qtk_img2_rec_feed(bfio4->asr[0]->img2, (char *)data, len<<1, 0);
                }
            }
            _bfio4_asred_post(bfio4);
        }
    }
}

void wtk_bfio4_feed(wtk_bfio4_t *bfio4,short *data,int len,int is_end)
{
    wtk_gainnet_denoise_t *denoise=bfio4->denoise;
	wtk_bfio4_msg_t *msg, *msg2;

#ifdef USE_LOG_BFIO4
	    static wtk_wavfile_t *raw_log;
        if(!raw_log)
        {
            raw_log=wtk_wavfile_new(16000);
            wtk_wavfile_open(raw_log,"wbf/wbf.0.wav");
            raw_log->max_pend=0;
        }
        if(len>0)
        {
            wtk_wavfile_write(raw_log,(char *)data,len<<1);
        }
        if(is_end)
        {
            wtk_debug("============ close ============\n");
            wtk_wavfile_close(raw_log);
            wtk_wavfile_delete(raw_log);
            raw_log=NULL;
        }
#endif
    if(bfio4->cfg->use_thread && bfio4->wake && bfio4->asr){
        if(bfio4->denoise){
            wtk_strbuf_push(bfio4->wake_buf, (char *)data, len<<1);
            wtk_strbuf_push(bfio4->asr_buf, (char *)data, len<<1);
            wtk_gainnet_denoise_feed(denoise, data, len, is_end);
        }else{
            if(len>0){
                msg=wtk_bfio4_pop_msg(bfio4);
                memcpy(msg->pv, data, sizeof(short)*len);
                msg->len=len;
                msg->type=WTK_BFIO4_MSG_WAKE;
                wtk_blockqueue_push(&(bfio4->wake_q), &(msg->q_n));

                msg2=wtk_bfio4_pop_msg(bfio4);
                memcpy(msg2->pv, data, sizeof(short)*len);
                msg2->len=len;
                msg2->type=WTK_BFIO4_MSG_ASR;
                wtk_blockqueue_push(&(bfio4->asr_q), &(msg2->q_n));
            }
            if(is_end){
                msg=wtk_bfio4_pop_msg(bfio4);
                msg->len=0;
                msg->type=WTK_BFIO4_MSG_END;
                wtk_blockqueue_push(&(bfio4->wake_q), &(msg->q_n));

                msg2=wtk_bfio4_pop_msg(bfio4);
                msg2->len=0;
                msg2->type=WTK_BFIO4_MSG_END;
                wtk_blockqueue_push(&(bfio4->asr_q), &(msg2->q_n));
			    wtk_sem_acquire(&(bfio4->end_sem),-1);
			    wtk_sem_acquire(&(bfio4->end_sem2),-1);
            }
        }
    }else{
        if(bfio4->denoise){
            if(bfio4->cfg->use_wake && bfio4->cfg->use_denoise_wake){
                wtk_strbuf_push(bfio4->wake_buf, (char *)data, len<<1);
            }
            if(bfio4->cfg->use_asr && bfio4->cfg->use_denoise_asr){
                wtk_strbuf_push(bfio4->asr_buf, (char *)data, len<<1);
            }
            wtk_gainnet_denoise_feed(denoise, data, len, is_end);
        }else{
            if(bfio4->wake){
                wtk_bfio4_wake_t *wake = bfio4->wake[0];
                wake->waked = 0;
                if (is_end) {
                    qtk_img_rec_feed(wake->img, NULL, 0, 1);
                } else {
                    qtk_img_rec_feed(wake->img, (char *)data, len<<1, 0);
                }
                _bfio4_waked_post(bfio4);
            }
            if(bfio4->asr){
                wtk_bfio4_asr_t *asr = bfio4->asr[0];
                asr->asr_id = 0;
                if (is_end) {
                    qtk_img2_rec_feed(asr->img2, NULL, 0, 1);
                } else {
                    qtk_img2_rec_feed(asr->img2, (char *)data, len<<1, 0);
                }
                _bfio4_asred_post(bfio4);
            }

        }
    }
}
