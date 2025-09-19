#include "wtk_qenvelope.h"


wtk_qenvelope_msg_t * wtk_qenvelope_msg_new(wtk_qenvelope_t *qenvl)
{
    wtk_qenvelope_msg_t *msg;

    msg=(wtk_qenvelope_msg_t *)wtk_malloc(sizeof(wtk_qenvelope_msg_t));
    msg->hook=NULL;

    return msg;
}

void wtk_qenvelope_msg_delete(wtk_qenvelope_t *qenvl, wtk_qenvelope_msg_t *msg)
{
    wtk_free(msg);
}

wtk_qenvelope_msg_t *wtk_qenvelope_pop_msg(wtk_qenvelope_t *qenvl)
{
    return (wtk_qenvelope_msg_t *)wtk_hoard_pop(&(qenvl->msg_hoard));
}

void wtk_qenvelope_push_msg(wtk_qenvelope_t *qenvl, wtk_qenvelope_msg_t *msg)
{
    wtk_hoard_push(&(qenvl->msg_hoard),msg);
}

wtk_qenvelope_t* wtk_qenvelope_new(wtk_qenvelope_cfg_t *cfg)
{
    wtk_qenvelope_t *qenvl;

    qenvl=(wtk_qenvelope_t *)wtk_malloc(sizeof(wtk_qenvelope_t));
    qenvl->cfg=cfg;
    qenvl->notify=NULL;
    qenvl->ths=NULL;

    qenvl->last_specm=cfg->min_speccrest;
    wtk_queue_init(&(qenvl->qenvelope_q));
    wtk_queue_init(&(qenvl->qenvelope_q2));
    qenvl->crest = -1;
    qenvl->trough[0]=-1;
    qenvl->trough[1]=-1;
    qenvl->last_specm2=0;
    qenvl->has_crest=qenvl->has_crest2=qenvl->has_troughl=qenvl->has_troughr=0;
    qenvl->nframe=0;

    wtk_hoard_init2(&(qenvl->msg_hoard),offsetof(wtk_qenvelope_msg_t,hoard_n),10,
        (wtk_new_handler_t)wtk_qenvelope_msg_new,
        (wtk_delete_handler2_t)wtk_qenvelope_msg_delete,
        qenvl);

    qenvl->qenvl_fn=NULL;
    if(cfg->debug)
    {
        char buf[1024];
        if(cfg->idx==0){
    	    sprintf(buf,"qenvl.dat");
        }else{
    	    sprintf(buf,"qenvl%d.dat", cfg->idx);
        }
        qenvl->qenvl_fn=fopen(buf,"w");
    }
    qenvl->idx=cfg->idx;

    wtk_qenvelope_reset(qenvl);

    return qenvl;
}

void wtk_qenvelope_delete(wtk_qenvelope_t *qenvl)
{
	wtk_hoard_clean(&(qenvl->msg_hoard));

    if(qenvl->qenvl_fn)
    {
        fclose(qenvl->qenvl_fn);
    }

    wtk_free(qenvl);
}

void wtk_qenvelope_reset(wtk_qenvelope_t *qenvl)
{
    wtk_qenvelope_msg_t *qemsg;
    wtk_queue_node_t *qn;

    while(qenvl->qenvelope_q.length>0)
    {
        qn=wtk_queue_pop(&(qenvl->qenvelope_q));
        if(!qn){break;}
        qemsg=(wtk_qenvelope_msg_t *)data_offset(qn,wtk_qenvelope_msg_t,q_n);
        wtk_qenvelope_push_msg(qenvl, qemsg);
    }
    while(qenvl->qenvelope_q2.length>0)
    {
        qn=wtk_queue_pop(&(qenvl->qenvelope_q2));
        if(!qn){break;}
        qemsg=(wtk_qenvelope_msg_t *)data_offset(qn,wtk_qenvelope_msg_t,q_n);
        wtk_qenvelope_push_msg(qenvl, qemsg);
    }
    qenvl->nframe=0;
    wtk_queue_init(&(qenvl->qenvelope_q));
    wtk_queue_init(&(qenvl->qenvelope_q2));

    qenvl->last_specm=qenvl->cfg->min_speccrest;
    
    qenvl->crest = -1;
    qenvl->trough[0]=-1;
    qenvl->trough[1]=-1;
    qenvl->has_crest=qenvl->has_crest2=qenvl->has_troughl=qenvl->has_troughr=0;
    qenvl->last_specm2=0;

    qenvl->envelope_start=0;
    qenvl->envelope2_start=0;
    qenvl->right_nf=0;
}

void wtk_qenvelope_set_notify(wtk_qenvelope_t *qenvelope,void *ths,wtk_qenvelope_notify_f notify)
{
    qenvelope->notify=notify;
    qenvelope->ths=ths;
}




void wtk_qenvelope_flush_envelope2_crest_trough(wtk_qenvelope_t *qenvl, int is_end)
{
    wtk_queue_t *qenvelope_q2=&(qenvl->qenvelope_q2);
    wtk_queue_node_t *qn;
    wtk_qenvelope_msg_t *qemsg;
    int rl;
    float envelope_thresh=qenvl->cfg->envelope_thresh;
    float right_min_thresh=qenvl->cfg->right_min_thresh;
    int max_rise_nf=qenvl->cfg->max_rise_nf;

    // wtk_debug("qenvelope_q2 length=%d %d %d %d \n",qenvelope_q2->length,qenvl->qenvelope_q.length,qenvl->has_crest,qenvl->has_crest2);
    if(qenvl->has_crest==0 && qenvl->has_crest2==0)
    {
        while(qenvelope_q2->length>0)
        {
            qn=wtk_queue_pop(qenvelope_q2);
            qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);

            if(qemsg->specsum < envelope_thresh)
            {
                --qenvl->right_nf;
                if(qenvl->right_nf<=0 || qemsg->specsum<right_min_thresh)
                {
                    if(qenvl->notify)
                    {
                        qenvl->notify(qenvl->ths, qemsg, WTK_QENVELOPE_TROUGH, 0, qenvl->idx);
                    }
                    qenvl->right_nf=0;
                }else
                {
                    if(qenvl->notify)
                    {
                        qenvl->notify(qenvl->ths, qemsg, WTK_QENVELOPE_FLAT,  0, qenvl->idx);
                    }
                }
                wtk_qenvelope_push_msg(qenvl, qemsg);
            }else
            {
                wtk_queue_push_front(qenvelope_q2,qn);
                break;
            }
        }
        if(qenvelope_q2->length>max_rise_nf)
        {
            while(qenvelope_q2->length>0)
            {
                qn=wtk_queue_pop(qenvelope_q2);
                qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);

                if(qenvl->notify)
                {
                    qenvl->notify(qenvl->ths, qemsg, WTK_QENVELOPE_TROUGH,  0, qenvl->idx);
                }
                wtk_qenvelope_push_msg(qenvl, qemsg);
            }
        }
    }else if(qenvl->has_crest2==1)
    {
        while(qenvelope_q2->length>0)
        {
            qn=wtk_queue_pop(qenvelope_q2);
            qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
            // wtk_debug("%f %f\n",qemsg->specsum,qemsg->smsg->s*1.0/16000);

            if(qemsg->specsum >= envelope_thresh)
            {
                qenvl->right_nf=qenvl->cfg->right_nf;
                if(qenvl->notify)
                {
                    qenvl->notify(qenvl->ths, qemsg, WTK_QENVELOPE_CREST, 0, qenvl->idx);
                }
            }else
            {
                --qenvl->right_nf;
                if(qenvl->right_nf<=0 || qemsg->specsum<right_min_thresh)
                {
                    if(qenvl->notify)
                    {
                        qenvl->notify(qenvl->ths, qemsg, WTK_QENVELOPE_TROUGH, 0, qenvl->idx);
                    }
                    qenvl->right_nf=0;
                }else
                {
                    if(qenvl->notify)
                    {
                        qenvl->notify(qenvl->ths, qemsg, WTK_QENVELOPE_CREST, 0, qenvl->idx);
                    }
                }
            }
            wtk_qenvelope_push_msg(qenvl, qemsg);
        }
    }else if(qenvl->has_crest==1 && qenvl->has_crest2==0)
    {
        // wtk_debug("qenvelope_q2 length=%d\n",qenvelope_q2->length);
        rl=qenvl->has_troughr==0?0:1;
        while(qenvelope_q2->length>rl)
        {
            qn=wtk_queue_pop(qenvelope_q2);
            qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);

            --qenvl->right_nf;
            if(qenvl->right_nf<=0 || qemsg->specsum<right_min_thresh)
            {
                if(qenvl->notify)
                {
                    qenvl->notify(qenvl->ths, qemsg, WTK_QENVELOPE_TROUGH, 0, qenvl->idx);
                }
                qenvl->right_nf=0;
            }else
            {
                if(qenvl->notify)
                {
                    qenvl->notify(qenvl->ths, qemsg, WTK_QENVELOPE_FLAT, 0, qenvl->idx);
                }
            }
            wtk_qenvelope_push_msg(qenvl, qemsg);
        }   
        if(is_end && rl==1)
        {
            qn=wtk_queue_pop(qenvelope_q2);
            qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
            --qenvl->right_nf;
            if(qenvl->notify)
            {
                qenvl->notify(qenvl->ths, qemsg, (qenvl->right_nf<=0 || qemsg->specsum<right_min_thresh)?WTK_QENVELOPE_TROUGH:WTK_QENVELOPE_FLAT, 0, qenvl->idx);
            }
            wtk_qenvelope_push_msg(qenvl, qemsg);
        }
    }
    if (is_end && qenvl->notify) {
        while(qenvelope_q2->length>0){
            qn=wtk_queue_pop(qenvelope_q2);
            qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
            if(qenvl->notify){
                qenvl->notify(qenvl->ths, qemsg, (qenvl->right_nf<=0 || qemsg->specsum<right_min_thresh)?WTK_QENVELOPE_TROUGH:WTK_QENVELOPE_FLAT, 0, qenvl->idx);
            }
            wtk_qenvelope_push_msg(qenvl, qemsg);
        }
        qenvl->notify(qenvl->ths, NULL, 0, 1, qenvl->idx);
    }
}


void wtk_qenvelope_flush_envelope2(wtk_qenvelope_t *qenvl,wtk_qenvelope_msg_t *qemsg, int is_end)
{
    wtk_queue_t *qenvolope_q2=&(qenvl->qenvelope_q2);
    float min_speccrest=qenvl->cfg->min_speccrest;
    
    if(qemsg)
    {
        wtk_queue_push(qenvolope_q2, &(qemsg->q_n));

        if(qenvl->envelope2_start==0)
        {
            qenvl->envelope2_start=1;
            qenvl->last_specm2=qemsg->specsum;

            qenvl->trough[0]=qenvl->last_specm2;
            qenvl->has_troughl=1;
        }else
        {
            if(qenvl->has_crest == 0)
            {
                if(qemsg->specsum>qenvl->last_specm2 && qenvl->has_troughl == 0)
                {
                    qenvl->trough[0]=qenvl->last_specm2;
                    qenvl->has_troughl=1;
                }else if(qemsg->specsum<=qenvl->last_specm2)
                {
                    qenvl->crest=qenvl->last_specm2;
                    qenvl->has_crest=1;
                }else if(qemsg->specsum>min_speccrest)
                {
                    if(qenvl->has_crest2==0)
                    {
                        qenvl->crest=qemsg->specsum;
                    }else
                    {
                        if(qemsg->specsum>qenvl->crest)
                        {
                            qenvl->crest=qemsg->specsum;
                        }
                    }
                    qenvl->has_crest2=1;
                }
            }else
            {
                if(qemsg->specsum>qenvl->last_specm2 && qenvl->has_troughr == 0)
                {
                    qenvl->trough[1]=qenvl->last_specm2;
                    qenvl->has_troughr=1;

                    qenvl->trough[0]=qenvl->last_specm2;
                    qenvl->has_troughl=1;
                }
            }
            qenvl->last_specm2=qemsg->specsum;
        }
    }
    wtk_qenvelope_flush_envelope2_crest_trough(qenvl, is_end);
    if(qenvl->has_troughr==1)
    {
        qenvl->has_crest=0;
        qenvl->has_crest2=0;
        qenvl->has_troughr=0;
    }
}

void wtk_qenvelope_feed(wtk_qenvelope_t *qenvl,float specsum,void *hook,int is_end)
{
    wtk_qenvelope_msg_t *qemsg;
    wtk_queue_t *qenvolope_q=&(qenvl->qenvelope_q);
    wtk_queue_node_t *qn;
    int i;
    int envelope_nf=qenvl->cfg->envelope_nf;
    int flush_nf;
    float specm=0, specstep;

    if(hook)
    {
        qemsg=wtk_qenvelope_pop_msg(qenvl);
        qemsg->specsum=specsum;
        qemsg->hook=hook;
        wtk_queue_push(qenvolope_q,&(qemsg->q_n));
    }
    if(is_end)
    {
        if(qenvl->cfg->use_envelope2)
        {
            while(qenvolope_q->length>0)
            {
                qn=wtk_queue_pop(qenvolope_q);
                qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
                ++qenvl->nframe;

                wtk_qenvelope_flush_envelope2(qenvl, qemsg, 0);
            }
            wtk_qenvelope_flush_envelope2(qenvl, NULL, 1);
        }else
        {
            while(qenvolope_q->length>0)
            {
                qn=wtk_queue_pop(qenvolope_q);
                qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
                ++qenvl->nframe;

                if(qenvl->notify)
                {
                    qenvl->notify(qenvl->ths, qemsg, qemsg->specsum<qenvl->cfg->envelope_thresh?WTK_QENVELOPE_TROUGH:WTK_QENVELOPE_CREST, 0, qenvl->idx);
                }
                wtk_qenvelope_push_msg(qenvl, qemsg);
            }
            if(qenvl->notify)
            {
                qenvl->notify(qenvl->ths, NULL, 0, 1, qenvl->idx);
            }
        }

        return;
    }

    if(qenvl->envelope_start==0)
    {
        if(qenvolope_q->length==envelope_nf)  //// 
        {
            qenvl->envelope_start=1;   
            for(i=0; i<envelope_nf; ++i)
            {
                qn=wtk_queue_peek(qenvolope_q, i);
                qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
                if(i==0)
                {
                    specm=qemsg->specsum;
                }else
                {
                    specm=max(specm, qemsg->specsum);
                }
            }
            flush_nf=envelope_nf-qenvl->cfg->left_nf;
            specstep=(specm-qenvl->last_specm)/flush_nf;
            // wtk_debug("%f %f %f %d\n", specstep, specm, qenvl->last_specm, flush_nf);
            for(i=0; i<flush_nf; ++i)
            {
                qn=wtk_queue_pop(qenvolope_q);
                qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
                qemsg->specsum=qenvl->last_specm+specstep*(i+1);
                ++qenvl->nframe;
                if(qenvl->qenvl_fn)
                {
                    fprintf(qenvl->qenvl_fn,"%.0f %f\n",qenvl->nframe,qemsg->specsum);
                }
                if(qenvl->cfg->use_envelope2)
                {
                    wtk_qenvelope_flush_envelope2(qenvl, qemsg, 0);
                }else
                {
                    if(qenvl->notify)
                    {
                        qenvl->notify(qenvl->ths, qemsg, qemsg->specsum<qenvl->cfg->envelope_thresh?WTK_QENVELOPE_TROUGH:WTK_QENVELOPE_CREST, 0, qenvl->idx);
                    }
                    wtk_qenvelope_push_msg(qenvl, qemsg);
                }
            }
            qenvl->last_specm=specm;
        }
    }else
    {
        if(qenvolope_q->length==envelope_nf+qenvl->cfg->left_nf)
        {   
            for(i=qenvl->cfg->left_nf; i<envelope_nf+qenvl->cfg->left_nf; ++i)
            {
                qn=wtk_queue_peek(qenvolope_q, i);
                qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
                if(i==qenvl->cfg->left_nf)
                {
                    specm=qemsg->specsum;
                }else
                {
                    specm=max(specm, qemsg->specsum);
                }
            }
            specstep=(specm-qenvl->last_specm)/envelope_nf;
            for(i=0; i<envelope_nf; ++i)
            {
                qn=wtk_queue_pop(qenvolope_q);
                qemsg=(wtk_qenvelope_msg_t *)data_offset2(qn, wtk_qenvelope_msg_t, q_n);
                qemsg->specsum=qenvl->last_specm+specstep*(i+1);
                ++qenvl->nframe;
                if(qenvl->qenvl_fn)
                {
                    fprintf(qenvl->qenvl_fn,"%.0f %f\n",qenvl->nframe,qemsg->specsum);
                }
                if(qenvl->cfg->use_envelope2)
                {
                    wtk_qenvelope_flush_envelope2(qenvl, qemsg, 0);
                }else
                {
                    if(qenvl->notify)
                    {
                        qenvl->notify(qenvl->ths, qemsg, qemsg->specsum<qenvl->cfg->envelope_thresh?WTK_QENVELOPE_TROUGH:WTK_QENVELOPE_CREST, 0, qenvl->idx);
                    }
                    wtk_qenvelope_push_msg(qenvl, qemsg);
                }
            }
            qenvl->last_specm=specm;
        }
    }
}
