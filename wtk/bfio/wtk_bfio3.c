#include "wtk_bfio3.h" 

void wtk_bfio3_on_sp_stft2(wtk_bfio3_t *bfio3,wtk_stft2_msg_t *msg,int pos,int is_end);
void wtk_bfio3_on_stft2(wtk_bfio3_t *bfio3,wtk_stft2_msg_t *msg,int pos,int is_end);

void  wtk_bfio3_on_aec(wtk_bfio3_t *bfio3,wtk_stft2_msg_t *msg,int pos,int is_end);

void wtk_bfio3_on_wbf(wtk_bfio3_t *bfio3,wtk_complex_t *wbf_out,int idx,int is_end);
void wtk_bfio3_on_wake(wtk_bfio3_wake_t *wake,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len);

static void wtk_bfio3_on_qform(wtk_bfio3_t *bfio3, wtk_complex_t *qf_out,
                              int is_end);
void wtk_bfio3_on_vad(wtk_bfio3_t *bfio3,wtk_vad2_cmd_t cmd,short *data,int len);

int wtk_bfio3_run_wbf(wtk_bfio3_t *bfio3, wtk_thread_t *t);
// int wtk_bfio3_run_kwake(wtk_bfio3_t *bfio3, wtk_thread_t *t);
int wtk_bfio3_run_qform(wtk_bfio3_t *bfio3, wtk_thread_t *t);


wtk_bfio3_msg_t *wtk_bfio3_msg_new(wtk_bfio3_t *bfio3){
    wtk_bfio3_msg_t *msg;

    msg = (wtk_bfio3_msg_t *)wtk_malloc(sizeof(wtk_bfio3_msg_t));
    msg->pos = 0;
    msg->is_end = 0;
    return msg;
}

void wtk_bfio3_msg_delete(wtk_bfio3_msg_t *msg){
    wtk_free(msg);
}

wtk_bfio3_msg_t *wtk_bfio3_pop_msg(wtk_bfio3_t *bfio3){
    return (wtk_bfio3_msg_t *)wtk_lockhoard_pop(&(bfio3->msg_lockhoard));
}

void wtk_bfio3_push_msg(wtk_bfio3_t *bfio3, wtk_bfio3_msg_t *msg){
    wtk_lockhoard_push(&(bfio3->msg_lockhoard), msg);
}

wtk_bfio3_wake_t *wtk_bfio3_wake_new(wtk_bfio3_t *bfio3, wtk_bfio3_cfg_t *cfg)
{
    wtk_bfio3_wake_t *wake;

    wake=(wtk_bfio3_wake_t *)wtk_malloc(sizeof(wtk_bfio3_wake_t));
    wake->hook=bfio3;

    wake->vwake=wtk_kvadwake_new(&(cfg->vwake));
    wtk_kvadwake_set_notify(wake->vwake, wake, (wtk_kvadwake_notify_f)wtk_bfio3_on_wake);

    return wake;
}

void wtk_bfio3_wake_reset(wtk_bfio3_wake_t *wake)
{
    wake->waked=0;
    wake->wake_fs=0;
    wake->wake_fe=0;

    wtk_kvadwake_reset(wake->vwake);
}

void wtk_bfio3_wake_start(wtk_bfio3_wake_t *wake, int theta)
{
    wake->theta=theta;
    wtk_kvadwake_start(wake->vwake);
}

void wtk_bfio3_wake_delete(wtk_bfio3_wake_t *wake)
{
    wtk_kvadwake_delete(wake->vwake);
    wtk_free(wake);
}

wtk_bfio3_t* wtk_bfio3_new(wtk_bfio3_cfg_t *cfg)
{
    wtk_bfio3_t *bfio3;
    int i;
    int wbf_cnt;

    bfio3=(wtk_bfio3_t *)wtk_malloc(sizeof(wtk_bfio3_t));
    bfio3->cfg=cfg;
    bfio3->ths=NULL;
    bfio3->notify=NULL;

    bfio3->channel=cfg->stft2.channel;

    wtk_queue_init(&(bfio3->mspstft_q));
    wtk_queue_init(&(bfio3->stft2_q));

    bfio3->sp_stft2=NULL;
    bfio3->stft2=NULL;
    bfio3->aec=NULL;
    if(cfg->use_aec)
    {
        bfio3->sp_stft2=wtk_stft2_new(&(cfg->sp_stft2));
        wtk_stft2_set_notify(bfio3->sp_stft2,bfio3,(wtk_stft2_notify_f)wtk_bfio3_on_sp_stft2);

        bfio3->stft2=wtk_stft2_new(&(cfg->stft2));
        wtk_stft2_set_notify(bfio3->stft2,bfio3,(wtk_stft2_notify_f)wtk_bfio3_on_stft2);
        bfio3->aec=wtk_aec_new2(&(cfg->aec), bfio3->stft2);
        wtk_aec_set_notify2(bfio3->aec, bfio3, (wtk_aec_notify_f2)wtk_bfio3_on_aec);
    }else
    {
        bfio3->stft2=wtk_stft2_new(&(cfg->stft2));
        wtk_stft2_set_notify(bfio3->stft2,bfio3,(wtk_stft2_notify_f)wtk_bfio3_on_aec);
    }
    
    bfio3->wbf=NULL;
    bfio3->wbf2=NULL;

    if(cfg->use_wbf2)
    {
        bfio3->wbf2=wtk_wbf2_new2(&(cfg->wbf2), bfio3->stft2);
        wtk_wbf2_set_notify2(bfio3->wbf2, bfio3, (wtk_wbf2_notify_f2)wtk_bfio3_on_wbf);
    }else
    {
        bfio3->wbf=wtk_wbf_new2(&(cfg->wbf), bfio3->stft2);    
        wtk_wbf_set_notify2(bfio3->wbf, bfio3, (wtk_wbf_notify_f2)wtk_bfio3_on_wbf);
    }
    
    wbf_cnt=bfio3->wbf? bfio3->wbf->cfg->wbf_cnt:bfio3->wbf2->cfg->wbf2_cnt;
    bfio3->wake_cnt = bfio3->cfg->use_raw_audio ? wbf_cnt + 1 : wbf_cnt;
    bfio3->wake = (wtk_bfio3_wake_t **)wtk_malloc(sizeof(wtk_bfio3_wake_t *) *
                                                bfio3->wake_cnt);
    for (i = 0; i < bfio3->wake_cnt; ++i) {
        bfio3->wake[i]=wtk_bfio3_wake_new(bfio3, cfg);  // new多个wake,由bfio3 use_raw_audio 和 wbf_cnt或wbf2_cnt控制
    }

    bfio3->ssl=wtk_ssl_new2(&(cfg->ssl), bfio3->stft2);

    bfio3->qform=NULL;
    bfio3->qform2=NULL;

    if(cfg->use_qform2)
    {
        bfio3->qform2=wtk_qform2_new2(&(cfg->qform2),bfio3->stft2);
    }else
    {
        bfio3->qform=wtk_qform9_new2(&(cfg->qform9),bfio3->stft2);
        wtk_qform9_set_notify2(bfio3->qform, bfio3, (wtk_qform9_notify_f2)wtk_bfio3_on_qform);   
    }

    bfio3->input=NULL;
    if(cfg->use_preemph)
    {
        bfio3->memX=(float *)wtk_malloc(sizeof(float)*(wbf_cnt+1));
        if(cfg->use_aec)
        {
            bfio3->input=wtk_strbufs_new(bfio3->channel+1);
            bfio3->notch_mem=wtk_float_new_p2(bfio3->channel+1,2);
            bfio3->memD=(float *)wtk_malloc(sizeof(float)*(bfio3->channel+1));
        }else
        {
            bfio3->input=wtk_strbufs_new(bfio3->channel);
            bfio3->notch_mem=wtk_float_new_p2(bfio3->channel,2);
            bfio3->memD=(float *)wtk_malloc(sizeof(float)*(bfio3->channel));
        }
    }

    bfio3->vad=NULL;
    if(cfg->use_asr)
    {
        bfio3->vad=wtk_vad2_new(cfg->vad);
        wtk_vad2_set_notify(bfio3->vad, bfio3, (wtk_vad2_notify_f)wtk_bfio3_on_vad);
        bfio3->vad->use_vad_start=cfg->use_vad_start;
    }
    
    bfio3->decoder = cfg->use_offline_asr ? qtk_decoder_wrapper_new(&cfg->decoder) : NULL;

    if(bfio3->cfg->use_thread){
        bfio3->wbf_run = 0;
        // bfio3->kwake_run = 0;
        bfio3->qform_run = 0;
        if(bfio3->cfg->use_thread){
            wtk_lockhoard_init(&(bfio3->msg_lockhoard), offsetof(wtk_bfio3_msg_t, hoard_n), 10,
                (wtk_new_handler_t)wtk_bfio3_msg_new,
                (wtk_delete_handler_t)wtk_bfio3_msg_delete,
                bfio3);
    }
    }
    wtk_bfio3_reset(bfio3);

    return bfio3;
}

void wtk_bfio3_delete(wtk_bfio3_t *bfio3)
{
    int i;

    for (i = 0; i < bfio3->wake_cnt; ++i) {
        wtk_bfio3_wake_delete(bfio3->wake[i]);
    }
    wtk_free(bfio3->wake);

    if(bfio3->cfg->use_thread){  //// 

        bfio3->wbf_run = 0;
        wtk_blockqueue_wake(&bfio3->wbf_queue);
        wtk_thread_join(&bfio3->wbf_t);
        wtk_thread_clean(&bfio3->wbf_t);
        wtk_blockqueue_clean(&bfio3->wbf_queue);

        // bfio3->kwake_run = 0;
        // wtk_blockqueue_wake(&bfio3->kwake_queue);
        // wtk_thread_join(&bfio3->kwake_t);
        // wtk_thread_clean(&bfio3->kwake_t);
        // wtk_blockqueue_clean(&bfio3->kwake_queue);

        bfio3->qform_run = 0;
        wtk_blockqueue_wake(&bfio3->qform_queue);
        wtk_thread_join(&bfio3->qform_t);
        wtk_thread_clean(&bfio3->qform_t);
        wtk_blockqueue_clean(&bfio3->qform_queue);

		wtk_sem_clean(&(bfio3->aec_sem));
		wtk_sem_clean(&(bfio3->kwake_sem));

        wtk_lockhoard_clean(&bfio3->msg_lockhoard);
    }

    if(bfio3->wbf)
    {
        wtk_wbf_delete2(bfio3->wbf);
    }else
    {
        wtk_wbf2_delete2(bfio3->wbf2);   
    }
    
    wtk_ssl_delete2(bfio3->ssl);
    if(bfio3->qform)
    {
        wtk_qform9_delete2(bfio3->qform);
    }else
    {
        wtk_qform2_delete2(bfio3->qform2);
    }

    if(bfio3->vad)
    {
        wtk_vad2_delete(bfio3->vad);
    }

    if(bfio3->aec)
    {
        wtk_aec_delete2(bfio3->aec);
        wtk_stft2_delete(bfio3->sp_stft2);
    }
    wtk_stft2_delete(bfio3->stft2);

    if(bfio3->input)
    {
        if(bfio3->cfg->use_aec)
        {
            wtk_strbufs_delete(bfio3->input, bfio3->channel+1);
            wtk_float_delete_p2(bfio3->notch_mem,  bfio3->channel+1);
        }else
        {
            wtk_strbufs_delete(bfio3->input, bfio3->channel);
            wtk_float_delete_p2(bfio3->notch_mem,  bfio3->channel);
        }
        wtk_free(bfio3->memD);
        wtk_free(bfio3->memX);
    }

    if (bfio3->decoder) {
        qtk_decoder_wrapper_delete(bfio3->decoder);
    }

    wtk_free(bfio3);
}

void wtk_bfio3_reset(wtk_bfio3_t *bfio3)
{
    int i;
    wtk_queue_t *stft2_q=&(bfio3->stft2_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *msg2;
    int wbf_cnt=bfio3->wbf? bfio3->wbf->cfg->wbf_cnt:bfio3->wbf2->cfg->wbf2_cnt;
    bfio3->asr = 0;
    if(bfio3->input)
    {
        memset(bfio3->memX,0,(wbf_cnt+1)*sizeof(float));
        if(bfio3->aec)
        {
            for(i=0;i<bfio3->channel+1;++i)
            {
                memset(bfio3->notch_mem[i],0,2*sizeof(float));
            }
            memset(bfio3->memD,0,(bfio3->channel+1)*sizeof(float));

            if(bfio3->input)
            {
                wtk_strbufs_reset(bfio3->input,  bfio3->channel+1);
            }
        }else
        {
            for(i=0;i<bfio3->channel;++i)
            {
                memset(bfio3->notch_mem[i],0,2*sizeof(float));
            }
            memset(bfio3->memD,0,bfio3->channel*sizeof(float));

            if(bfio3->input)
            {
                wtk_strbufs_reset(bfio3->input,  bfio3->channel);
            }
        }   
    }

    wtk_queue_init(&(bfio3->mspstft_q));


    if(bfio3->cfg->use_thread && bfio3->wbf_run == 1){  //// 

        bfio3->wbf_run = 0;
        wtk_blockqueue_wake(&bfio3->wbf_queue);
        wtk_thread_join(&bfio3->wbf_t);
        wtk_thread_clean(&bfio3->wbf_t);
        wtk_blockqueue_clean(&bfio3->wbf_queue);

        // bfio3->kwake_run = 0;
        // wtk_blockqueue_wake(&bfio3->kwake_queue);
        // wtk_thread_join(&bfio3->kwake_t);
        // wtk_thread_clean(&bfio3->kwake_t);
        // wtk_blockqueue_clean(&bfio3->kwake_queue);

        bfio3->qform_run = 0;
        wtk_blockqueue_wake(&bfio3->qform_queue);
        wtk_thread_join(&bfio3->qform_t);
        wtk_thread_clean(&bfio3->qform_t);
        wtk_blockqueue_clean(&bfio3->qform_queue);

		wtk_sem_clean(&(bfio3->aec_sem));
		wtk_sem_clean(&(bfio3->kwake_sem));
    }

    if(bfio3->aec)
    {
        wtk_aec_reset2(bfio3->aec);
        wtk_stft2_reset(bfio3->sp_stft2);
    }

    while(stft2_q->length>0)
    {
        qn=wtk_queue_pop(stft2_q);
        msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
        wtk_stft2_push_msg(bfio3->stft2, msg2);
    }
    wtk_queue_init(stft2_q);
    wtk_stft2_reset(bfio3->stft2);

    bfio3->end_pos=0;
    bfio3->is_end=0;

    bfio3->waked=0;
    bfio3->wake_theta=0;
    bfio3->wake_fs=bfio3->wake_fe=0;
    bfio3->wake_prob=0;

    bfio3->vad_output=0;

    for (i = 0; i < bfio3->wake_cnt; ++i) {
        wtk_bfio3_wake_reset(bfio3->wake[i]);
    }

    if(bfio3->wbf)
    {
        wtk_wbf_reset2(bfio3->wbf);
    }else
    {
        wtk_wbf2_reset2(bfio3->wbf2);
    }
    
    wtk_ssl_reset2(bfio3->ssl);
    if(bfio3->qform)
    {
        wtk_qform9_reset2(bfio3->qform);
    }else
    {
        wtk_qform2_reset2(bfio3->qform2);
    }
    
    if(bfio3->vad)
    {
        wtk_vad2_reset(bfio3->vad);
    }
    
    if (bfio3->decoder) {
        qtk_decoder_wrapper_reset(bfio3->decoder);
    }

    bfio3->state = 0;//0 idle, 1 norm
    bfio3->idle_frame = 0.0;
    bfio3->idle_trigger_frame = 60000;
}

void wtk_bfio3_set_notify(wtk_bfio3_t *bfio3,void *ths,wtk_bfio3_notify_f notify)
{
    bfio3->ths=ths;
    bfio3->notify=notify;
}

void wtk_bfio3_start(wtk_bfio3_t *bfio3)
{
    int i, wbf_cnt;
    int *theta;

    if (bfio3->wbf) {
        wbf_cnt = bfio3->wbf->cfg->wbf_cnt;
        theta = bfio3->wbf->theta;
    } else {
        wbf_cnt = bfio3->wbf2->cfg->wbf2_cnt;
        theta = bfio3->wbf2->cfg->theta;
    }

    if(bfio3->cfg->use_thread){  //// 
        wtk_sem_init(&(bfio3->aec_sem),0);
        wtk_sem_init(&(bfio3->kwake_sem),0);
        bfio3->wbf_run = 1;
        wtk_blockqueue_init(&(bfio3->wbf_queue));
        wtk_thread_init(&(bfio3->wbf_t), (thread_route_handler)wtk_bfio3_run_wbf, bfio3);
        wtk_thread_start(&(bfio3->wbf_t));

        // bfio3->kwake_run = 1;
        // wtk_blockqueue_init(&(bfio3->kwake_queue));
        // wtk_thread_init(&(bfio3->kwake_t), (thread_route_handler)wtk_bfio3_run_kwake, bfio3);
        // wtk_thread_start(&(bfio3->kwake_t));

        bfio3->qform_run = 1;
        wtk_blockqueue_init(&(bfio3->qform_queue));
        wtk_thread_init(&(bfio3->qform_t), (thread_route_handler)wtk_bfio3_run_qform, bfio3);
        wtk_thread_start(&(bfio3->qform_t));
    }

    if(bfio3->wbf)
    {
        wtk_wbf_start2(bfio3->wbf);
    }else
    {
        wtk_wbf2_start(bfio3->wbf2);
    }
    
    for(i=0; i<wbf_cnt; ++i)
    {
        wtk_bfio3_wake_start(bfio3->wake[i], theta[i]);
    }
    if (bfio3->cfg->use_raw_audio) {
        wtk_bfio3_wake_start(bfio3->wake[wbf_cnt], -1);
    }
    if(bfio3->qform)
    {
        wtk_qform9_start2(bfio3->qform, 0, 0);
    }else
    {
        wtk_qform2_start2(bfio3->qform2, 0, 0);
    }
    
}

void wtk_bfio3_set_waked(wtk_bfio3_t *bfio3, float wake_fs, float wake_fe, int bftheta, float wake_prob, int need_notify, int need_ssl, wtk_bfio3_wake_t *wake)
{
    float ns, ne;
    int rate=bfio3->cfg->rate;
    // int i, 
    int cancel;
    int min_idx;
    // float min_thsub, fp;
    float conf;
    wtk_queue_t *stft2_q=&(bfio3->stft2_q);
    wtk_stft2_msg_t *msg;
    wtk_queue_node_t *qn;
    // int theta_range=bfio3->wbf?bfio3->wbf->theta_range:bfio3->wbf2->theta_range;

    bfio3->wake_fs=wake_fs;
    bfio3->wake_fe=wake_fe;
    bfio3->wake_prob=wake_prob;

    if(bfio3->cfg->use_trick)
    {
    	if(bfio3->state == 0)
    	{
    		conf = bfio3->cfg->idle_conf;
    	}else
    	{
    		conf = bfio3->cfg->norm_conf;
    	}
		if(wake_prob >= conf)
		{
    		bfio3->state = 1;
			bfio3->idle_frame = 0;
		}else
		{
			if(bfio3->idle_frame > bfio3->idle_trigger_frame)
			{
				bfio3->state = 0;
			}
			return;
		}
    }

    if(need_notify && bfio3->notify)
    {
        bfio3->notify(bfio3->ths, WTK_BFIO3_WAKE, NULL ,0);
    }

    ns=(wake_fs+bfio3->cfg->wake_ssl_fs)*rate;
    ne=(wake_fe+bfio3->cfg->wake_ssl_fe)*rate;
    // wtk_debug("\n%f======================== %f\n",bfio3->cfg->wake_ssl_fs,bfio3->cfg->wake_ssl_fe);
    // wtk_debug("\n%f======================== %f\n",ns,ne);
    msg=(wtk_stft2_msg_t *)data_offset2(stft2_q->pop, wtk_stft2_msg_t, q_n);
    if (bfio3->cfg->debug) {
        wtk_debug("smsg_s =%.0f wake_ssl_ns=%.0f wake_ssl_ne=%.0f [%f %f]\n",
                  msg->s, ns, ne, wake_fs + bfio3->cfg->wake_ssl_fs,
                  wake_fe + bfio3->cfg->wake_ssl_fe);
    }
    
    if(need_ssl)
    {
        wtk_ssl_reset2(bfio3->ssl);
        for(qn=stft2_q->pop; qn; qn=qn->next)
        {
            msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
            if(msg->s >= ns)
            {
                // printf("%f %f %f\n",msg->s,ns,ne);
                wtk_ssl_feed_stft2msg(bfio3->ssl, msg, 0);
            }
            if(msg->s >= ne)
            {
                break;
            }
        }
        wtk_ssl_feed_stft2msg(bfio3->ssl, NULL, 1);
        // wtk_ssl_print(bfio3->ssl);
    }
    min_idx=-1;

    if (bfio3->ssl->nbest >= 1) {
        min_idx = 0;
    }

    if (bfio3->cfg->use_asr && need_ssl) {
        if (bfio3->waked == 1) {
            wtk_vad2_feed(bfio3->vad, NULL, 0, 1);
            wtk_vad2_reset(bfio3->vad);
        }
        bfio3->waked = 1;

        wtk_vad_set_margin(bfio3->vad->vad, bfio3->cfg->vad_left_margin,
                           bfio3->cfg->vad_right_margin);
        wtk_vad2_start(bfio3->vad);
    }

    if(min_idx>=0)
    {
        if (abs(bfio3->wake_theta - bfio3->ssl->nbest_extp[min_idx].theta) <=
            15) {  // 与上次定位角度小于等于15度不更新qform
            bfio3->wake_theta=bfio3->ssl->nbest_extp[min_idx].theta;
            if (bfio3->cfg->debug) {
                wtk_debug("wake_theta=bfio3_theta=%d started\n",
                          bfio3->wake_theta);
            }
            bfio3->waked=1;
            if(need_notify && bfio3->notify)
            {
            	if(wake->vwake->asr_res)
            	{
            		//printf("%.*s\n",wake->vwake->asr_res->pos,wake->vwake->asr_res->data);
                    bfio3->notify(bfio3->ths, WTK_BFIO3_WAKE_RES, (short *)(wake->vwake->asr_res->data), wake->vwake->asr_res->pos);
            	}
                bfio3->notify(bfio3->ths, WTK_BFIO3_WAKE_SSL, NULL ,0);
            }

        } else {  // 与上次定位大于15度重新做qform
            bfio3->wake_theta=bfio3->ssl->nbest_extp[min_idx].theta;
            if (bfio3->cfg->debug) {
                wtk_debug("wake_theta=%d\n", bfio3->wake_theta);
            }
            if(need_notify && bfio3->notify)
            {
            	if(wake->vwake->asr_res)
            	{
            		//printf("%.*s\n",wake->vwake->asr_res->pos,wake->vwake->asr_res->data);
                    bfio3->notify(bfio3->ths, WTK_BFIO3_WAKE_RES, (short *)(wake->vwake->asr_res->data), wake->vwake->asr_res->pos);
            	}
                bfio3->notify(bfio3->ths, WTK_BFIO3_WAKE_SSL, NULL ,0);
            }

            if(bfio3->cfg->use_asr)
            {
                if(bfio3->qform)
                {                
                    wtk_qform9_feed_smsg(bfio3->qform,NULL,0,1);
                    wtk_qform9_reset2(bfio3->qform);
                }else
                {                
                    wtk_qform2_feed_smsg(bfio3->qform2,NULL,0,1);
                    wtk_qform2_reset2(bfio3->qform2);
                }
                if(bfio3->cfg->use_thread){
                    wtk_sem_release(&(bfio3->kwake_sem),1);
                }
                if(bfio3->qform)
                {
                    wtk_qform9_start2(bfio3->qform,bfio3->wake_theta,0);
                }else
                {
                    wtk_qform2_start2(bfio3->qform2,bfio3->wake_theta,0);
                }

                if (need_notify) {
                    ne = wake_fe * rate;
                    for (qn = stft2_q->pop; qn; qn = qn->next) {
                        msg = (wtk_stft2_msg_t *)data_offset2(
                            qn, wtk_stft2_msg_t, q_n);
                        if (msg->s >= ne) {
                            break;
                        }
                    }

                    if (msg->s >= ne) {
                        cancel = bfio3->vad_output - msg->s;
                        if (bfio3->cfg->debug) {
                            wtk_debug("out %.0f, msg->s %.0f cancel %d\n",
                                      bfio3->vad_output, msg->s, cancel);
                        }
                        if (cancel > 0) {
                            if (bfio3->notify) {
                                bfio3->notify(bfio3->ths, WTK_BFIO3_VAD_CANCEL,
                                             NULL, cancel);
                            }
                            bfio3->vad_output -= cancel;
                        }

                    }
                }
            }
        }
    }else
    {
        if (bfio3->cfg->debug) {
            wtk_debug("[%f %f] waked ssl nbest has no theta == bftheta\n",
                      wake_fs, wake_fe);
        }
    }

    if (bfio3->cfg->use_asr && need_ssl) {

        ne = wake_fe * rate;
        for (qn = stft2_q->pop; qn; qn = qn->next) {
            msg = (wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
            if (msg->s >= ne) {
                break;
            }
        }

        if (msg->s >= ne) {
            cancel = bfio3->vad_output - msg->s;
            if (bfio3->cfg->debug) {
                wtk_debug("out %.0f, msg->s %.0f cancel %d\n", bfio3->vad_output,
                          msg->s, cancel);
            }
            if (cancel > 0) {
                if (bfio3->notify) {
                    bfio3->notify(bfio3->ths, WTK_BFIO3_VAD_CANCEL, NULL, cancel);
                }
                bfio3->vad_output -= cancel;
            }

            for (; qn; qn = qn->next) {
                msg = (wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                if (bfio3->qform) {
                    wtk_qform9_feed_smsg(bfio3->qform, msg, 0, 0);
                } else {
                    wtk_qform2_feed_smsg(bfio3->qform2, msg, 0, 0);
                    wtk_bfio3_on_qform(bfio3, bfio3->qform2->out, 0);
                }
                if(bfio3->cfg->use_thread){
                    wtk_sem_release(&(bfio3->kwake_sem),1);
                }
            }
        }
    }
}

static void _bfio3_waked_post(wtk_bfio3_t *bfio3) {

    float max_wake_prob = -50;
    int max_idx = -1;
    wtk_bfio3_wake_t **wake = bfio3->wake;
    int i;

    if(bfio3->cfg->use_thread){
        wtk_sem_release(&(bfio3->kwake_sem),1);
        wtk_sem_acquire(&(bfio3->aec_sem),-1);
    }
    for (i = 0; i < bfio3->wake_cnt; ++i) {  // 挑选置信度最高的唤醒
        if (wake[i]->waked == 1) {
            if (bfio3->cfg->debug) {
                wtk_debug("wake[%d] bftheta=%d wake_prob=%f wake_ts=[%f %f]\n",
                          i, wake[i]->theta, wake[i]->wake_prob,
                          wake[i]->wake_fs, wake[i]->wake_fe);
            }
            if (wake[i]->wake_prob > max_wake_prob) {
                max_idx = i;
                max_wake_prob = wake[i]->wake_prob;
            }
        }
    }

    if (max_idx >= 0) {
        if (fabs(wake[max_idx]->wake_fe - bfio3->wake_fe) < 1) { // 判断本次唤醒位置跟上次位置差距，如果小于1则不回调
            if (wake[max_idx]->wake_prob > bfio3->wake_prob) {
                if (bfio3->cfg->debug) {
                    wtk_debug("change waked [%f %f] to [%f %f ], wake[%d] "
                              "bftheta=%d wake_prob=%f  lst_wakeprob=%f\n",
                              bfio3->wake_fs, bfio3->wake_fe,
                              wake[max_idx]->wake_fs, wake[max_idx]->wake_fe,
                              max_idx, wake[max_idx]->theta,
                              wake[max_idx]->wake_prob, bfio3->wake_prob);
                }
                wtk_bfio3_set_waked(bfio3, wake[max_idx]->wake_fs,
                                   wake[max_idx]->wake_fe, wake[max_idx]->theta,
                                   wake[max_idx]->wake_prob, 0, 1,
                                   wake[max_idx]);
            } else {
                if (bfio3->cfg->debug) {
                    wtk_debug("waked over [%f %f] pop [%f %f]\n", bfio3->wake_fs,
                              bfio3->wake_fe, wake[max_idx]->wake_fs,
                              wake[max_idx]->wake_fe);
                }
            }
        } else {  // 与上次唤醒大于1就回调唤醒消息
            wtk_bfio3_set_waked(bfio3, wake[max_idx]->wake_fs,
                               wake[max_idx]->wake_fe, wake[max_idx]->theta,
                               wake[max_idx]->wake_prob, 1, 1, wake[max_idx]);
        }
    }

}

void wtk_bfio3_on_wbf(wtk_bfio3_t *bfio3,wtk_complex_t *wbf_out,int idx,int is_end)
{
    float *data=bfio3->stft2->output;
    short *pv=NULL;
    int k=0;
    int i;

    if(wbf_out)
    {
        k=wtk_stft2_output_ifft(bfio3->stft2,wbf_out,data,bfio3->wbf?bfio3->wbf->bf[idx]->pad:bfio3->wbf2->qform2[idx]->pad,bfio3->end_pos,bfio3->is_end);
        if(bfio3->cfg->use_preemph)
        {
            bfio3->memX[idx]=wtk_preemph_asis2(data,k,bfio3->memX[idx]);
        }

        pv=(short *)data;
        for(i=0;i<k;++i)
        {
            pv[i] = QTK_SSAT16f(data[i]);
        }
    }
#ifdef USE_LOG
	static wtk_wavfile_t *log = NULL;

    if(bfio3->wake[idx]->theta==90)
	{
		if(!log)
		{
			log=wtk_wavfile_new(16000);
			wtk_wavfile_open2(log,"wbf");
			log->max_pend=0;
		}
        if(k>0)
        {
		    wtk_wavfile_write(log,(char *)pv,k<<1);
        }
		if(is_end)
		{
			wtk_debug("============ close ============\n");
			wtk_wavfile_close(log);
			wtk_wavfile_delete(log);
			log=NULL;
		}
	}
#endif

#ifdef USE_LOG_BFIO

    int wbf_cnt=bfio3->wbf? bfio3->wbf->cfg->wbf_cnt:bfio3->wbf2->cfg->wbf2_cnt;

int j;
	static wtk_wavfile_t *log[8];
    for(j=0;j < wbf_cnt;++j){
        if(idx==j)
        {
        	if(!log[j])
        	{
        		log[j]=wtk_wavfile_new(16000);
        		wtk_wavfile_open2(log[j],"wbf/wbf");
        		log[j]->max_pend=0;
        	}
            if(k>0)
            {
        	    wtk_wavfile_write(log[j],(char *)pv,k<<1);
            }
        	if(is_end)
        	{
        		wtk_debug("============ close ============\n");
        		wtk_wavfile_close(log[j]);
        		wtk_wavfile_delete(log[j]);
        		log[j]=NULL;
        	}
        }
    }

#endif

    wtk_kvadwake_feed(bfio3->wake[idx]->vwake, pv, k, is_end);
    if(bfio3->cfg->use_trick && idx==0)
    {
    	bfio3->idle_frame += k/640.0;
    }
    if(bfio3->cfg->use_thread){
        static int count = -1;
        static int count1 = 0;
        ++count;
        ++count1;
        if(count > 0){
            if(count1 == 4){
                wtk_sem_release(&(bfio3->aec_sem),1);
                count1 = 0;
            }
        }
    }
}

void wtk_bfio3_on_wake(wtk_bfio3_wake_t *wake,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len)
{
    if(cmd==WTK_KVADWAKE_WAKE)
    {
        wake->waked=1;
        wake->wake_fs=fs;  // 唤醒左边界
        wake->wake_fe=fe;  // 唤醒右边界
        wake->wake_prob=wtk_kvadwake_get_conf(wake->vwake);  // 唤醒置信度
    }
}

static void wtk_bfio3_on_qform(wtk_bfio3_t *bfio3, wtk_complex_t *qf_out,
                              int is_end) {
    float *data=bfio3->stft2->output;
    short *pv=NULL;
    int k=0;
    int i;
    int wbf_cnt=bfio3->wbf? bfio3->wbf->cfg->wbf_cnt:bfio3->wbf2->cfg->wbf2_cnt;
    float *pad = bfio3->qform ? bfio3->qform->bf->pad : bfio3->qform2->pad;


    if(qf_out)
    {
        k = wtk_stft2_output_ifft(bfio3->stft2, qf_out, data, pad, bfio3->end_pos,
                                  bfio3->is_end);
        if(bfio3->cfg->use_preemph)
        {
            bfio3->memX[wbf_cnt]=wtk_preemph_asis2(data,k,bfio3->memX[wbf_cnt]);
        }

        pv=(short *)data;
        for(i=0;i<k;++i)
        {
            pv[i] = QTK_SSAT16f(data[i]);
        }
    }

    if(bfio3->waked==1)
    {
        wtk_vad2_feed(bfio3->vad, (char *)pv, k<<1, is_end);
    }else
    {
        bfio3->vad_output+=k;
        if(bfio3->notify)
        {
            bfio3->notify(bfio3->ths, WTK_BFIO3_VAD_DATA, pv, k);
        }
    }
    if(bfio3->cfg->use_thread){
        static int count = -1;
        ++count;
        if(count > 0){
            wtk_sem_acquire(&(bfio3->kwake_sem),-1);
        }
    }
}

void wtk_bfio3_on_vad(wtk_bfio3_t *bfio3,wtk_vad2_cmd_t cmd,short *data,int len)

{
    if (bfio3->cfg->use_offline_asr) {
        wtk_string_t rec_res;
        switch(cmd) {
        case WTK_VAD2_START:
            bfio3->vad_ts = bfio3->vad_output / bfio3->cfg->rate;
            qtk_decoder_wrapper_start(bfio3->decoder);
            bfio3->asr = 1;
            break;
        case WTK_VAD2_DATA:
            bfio3->vad_output += len;
            if (bfio3->asr == 1) {
                qtk_decoder_wrapper_feed(bfio3->decoder, (char *)data, len << 1, 0);
            }
            if (bfio3->notify) {
                bfio3->notify(bfio3->ths, WTK_BFIO3_VAD_DATA, data, len);
            }
            break;
        case WTK_VAD2_END:
            bfio3->vad_te = bfio3->vad_output / bfio3->cfg->rate;
            qtk_decoder_wrapper_feed(bfio3->decoder, NULL, 0, 1);
            qtk_decoder_wrapper_get_result(bfio3->decoder, &rec_res);
            if(bfio3->notify) {
                bfio3->notify(bfio3->ths, WTK_BFIO3_ASR_RES, (short *)(rec_res.data), rec_res.len);
            }
            qtk_decoder_wrapper_reset(bfio3->decoder);
            bfio3->asr = 0;
            break;
        case WTK_VAD2_CANCEL:
            bfio3->vad_output -= len;
            break;
        }
        return ;
    }

	switch(cmd)
    {
    case WTK_VAD2_START:
        bfio3->vad_ts=bfio3->vad_output/bfio3->cfg->rate;
        if(bfio3->notify)
        {
            bfio3->notify(bfio3->ths, WTK_BFIO3_VAD_START, NULL, 0);
        }
        // sil=0;
        break;
    case WTK_VAD2_DATA:
        bfio3->vad_output+=len;
        // if(sil==0)
        // {
        if(bfio3->notify)
        {
            bfio3->notify(bfio3->ths, WTK_BFIO3_VAD_DATA, data, len);
        }
        // }
        break;
    case WTK_VAD2_END:
        bfio3->vad_te=bfio3->vad_output/bfio3->cfg->rate;
        if(bfio3->notify)
        {
            bfio3->notify(bfio3->ths, WTK_BFIO3_VAD_END, NULL, len);
        }
        // sil=1;
        break;
    case WTK_VAD2_CANCEL:
        bfio3->vad_output-=len;
        if(bfio3->notify)
        {
            bfio3->notify(bfio3->ths, WTK_BFIO3_VAD_CANCEL, NULL, len);
        }
        break;
    }

}

void wtk_bfio3_on_aec(wtk_bfio3_t *bfio3,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int i;
    wtk_bfio3_wake_t **wake=bfio3->wake;
    wtk_queue_t *stft2_q=&(bfio3->stft2_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *msg2;
    int wbf_cnt=bfio3->wbf? bfio3->wbf->cfg->wbf_cnt:bfio3->wbf2->cfg->wbf2_cnt;

    wtk_bfio3_msg_t *bf3_msg, *bf3_msg1;

    if(is_end)
    {
        bfio3->end_pos=pos;
        bfio3->is_end=1;
    }
    if(msg)
    {
        wtk_queue_push(stft2_q, &(msg->q_n));
        if(stft2_q->length > bfio3->cfg->stft2_hist)
        {
            qn=wtk_queue_pop(stft2_q);
            msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
            wtk_stft2_push_msg(bfio3->stft2, msg2);
        }
    }
    if(bfio3->cfg->use_asr)  //// 
    {
        if(bfio3->cfg->use_thread){
            bf3_msg = wtk_bfio3_pop_msg(bfio3);
            bf3_msg->pos = pos;
            bf3_msg->is_end = is_end;
            bf3_msg->stft2_msg = msg;
            wtk_blockqueue_push(&(bfio3->qform_queue), &(bf3_msg->q_n));
        }else{
            if(bfio3->qform)
            {
                wtk_qform9_feed_smsg(bfio3->qform, msg, pos, is_end);   
            }else
            {
                wtk_qform2_feed_smsg(bfio3->qform2, msg, pos ,is_end);
                wtk_bfio3_on_qform(bfio3, msg ? bfio3->qform2->out : NULL, is_end);
            } 
        }

    }

    for(i=0; i<wbf_cnt; ++i)
    {
        wake[i]->waked=0;
    }
    if(bfio3->cfg->use_thread){
        bf3_msg1 = wtk_bfio3_pop_msg(bfio3);
        bf3_msg1->pos = pos;
        bf3_msg1->is_end = is_end;
        bf3_msg1->stft2_msg = msg;
        wtk_blockqueue_push(&(bfio3->wbf_queue),&(bf3_msg1->q_n));
    }else{
        if(bfio3->wbf)
        {
            wtk_wbf_feed_smsg(bfio3->wbf, msg, pos, is_end);
        }else
        {
            wtk_wbf2_feed_smsg(bfio3->wbf2, msg, pos, is_end);
        }
    }

    _bfio3_waked_post(bfio3);
}


void wtk_bfio3_on_stft2(wtk_bfio3_t *bfio3,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_queue_t *mspstft_q=&(bfio3->mspstft_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *sp_msg;

    qn=wtk_queue_pop(mspstft_q);
    if(qn)
    {
        sp_msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
    }else
    {
        sp_msg=NULL;
    }

    wtk_aec_feed_stftmsg(bfio3->aec, msg, sp_msg, pos, is_end);

    if(sp_msg)
    {
        wtk_stft2_push_msg(bfio3->sp_stft2, sp_msg);
    }
}

void wtk_bfio3_on_sp_stft2(wtk_bfio3_t *bfio3,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    if(msg)
    {
        wtk_queue_push(&(bfio3->mspstft_q),  &(msg->q_n));
    }
}

void wtk_bfio3_feed_dc(wtk_bfio3_t *bfio3,short **data,int len,int is_end)
{   
    int i,j;
    int channel=bfio3->channel;
    float fv;
    float *fp[512],*fp2[1];
    wtk_strbuf_t **input=bfio3->input;

    for(i=0;i<channel;++i)  // 预加重
    {
        wtk_strbuf_reset(input[i]);
        for(j=0;j<len;++j)
        {
            fv=data[i][j];
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp[i]=(float *)(input[i]->data);
        
        wtk_preemph_dc(fp[i], bfio3->notch_mem[i], len);
        bfio3->memD[i]=wtk_preemph_asis(fp[i], len, bfio3->memD[i]);
    }

    if(bfio3->aec)
    {
        wtk_strbuf_reset(input[i]);  // aec的预加重，fp2[0]
        for(j=0;j<len;++j)
        {
            fv=data[i][j];
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp2[0]=(float *)(input[i]->data);
        
        wtk_preemph_dc(fp2[0], bfio3->notch_mem[i], len);
        bfio3->memD[i]=wtk_preemph_asis(fp2[0], len, bfio3->memD[i]);

        wtk_stft2_feed_float(bfio3->sp_stft2, fp2, len, is_end);
    }
    wtk_stft2_feed_float(bfio3->stft2, fp, len, is_end);
}


void wtk_bfio3_feed(wtk_bfio3_t *bfio3,short **data,int len,int is_end)
{
    short *pv[1];
    int channel=bfio3->channel;

    if(bfio3->input)
    {
        wtk_bfio3_feed_dc(bfio3,data,len,is_end);
    }else
    {
        if(bfio3->aec)
        {
			if(data)
			{
				pv[0]=data[channel];
				wtk_stft2_feed2(bfio3->sp_stft2, pv, len, is_end);
			}else
			{
				wtk_stft2_feed2(bfio3->sp_stft2, NULL, 0, is_end);
			}
        }
        wtk_stft2_feed2(bfio3->stft2, data, len, is_end);
    }

    if (bfio3->cfg->use_raw_audio) {
        wtk_bfio3_wake_t *wake = bfio3->wake[bfio3->wake_cnt - 1];
        wake->waked = 0;
        if (is_end) {
            wtk_kvadwake_feed(wake->vwake, NULL, 0, 1);
        } else {
            wtk_kvadwake_feed(wake->vwake, data[0], len, 0);
        }
        _bfio3_waked_post(bfio3);
    }
}

int wtk_bfio3_run_wbf(wtk_bfio3_t *bfio3, wtk_thread_t *t){
    wtk_queue_node_t *qn;
	wtk_bfio3_msg_t *msg;

	while(bfio3->wbf_run)
	{
		qn=wtk_blockqueue_pop(&(bfio3->wbf_queue),-1,NULL);
		if(!qn){continue;}
		msg=data_offset2(qn,wtk_bfio3_msg_t,q_n);
        if(bfio3->wbf)
        {
            wtk_wbf_feed_smsg(bfio3->wbf, msg->stft2_msg, msg->pos, msg->is_end);
        }else
        {
            wtk_wbf2_feed_smsg(bfio3->wbf2, msg->stft2_msg, msg->pos, msg->is_end);
        }
	}
	return 0;
}

// int wtk_bfio3_run_kwake(wtk_bfio3_t *bfio3, wtk_thread_t *t);

int wtk_bfio3_run_qform(wtk_bfio3_t *bfio3, wtk_thread_t *t){
    wtk_queue_node_t *qn;
    wtk_bfio3_msg_t *msg;

    while(bfio3->qform_run){
        qn = wtk_blockqueue_pop(&(bfio3->qform_queue), -1, NULL);
        if(!qn){continue;}
        msg = data_offset2(qn, wtk_bfio3_msg_t, q_n);
        if(bfio3->qform)
        {
            wtk_qform9_feed_smsg(bfio3->qform, msg->stft2_msg, msg->pos, msg->is_end);   
        }else
        {
            wtk_qform2_feed_smsg(bfio3->qform2, msg->stft2_msg, msg->pos, msg->is_end);
            wtk_bfio3_on_qform(bfio3, msg ? bfio3->qform2->out : NULL, msg->is_end);
        }
    }
    return 0;
}
