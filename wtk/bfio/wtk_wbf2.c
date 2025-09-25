#include "wtk_wbf2.h" 
void wtk_wbf2_on_stft2(wtk_wbf2_t *wbf2,wtk_stft2_msg_t *msg,int pos,int is_end);

wtk_wbf2_t* wtk_wbf2_new(wtk_wbf2_cfg_t *cfg)
{
    wtk_wbf2_t *wbf2;
    int i;

    wbf2=(wtk_wbf2_t *)wtk_malloc(sizeof(wtk_wbf2_t));
    wbf2->cfg=cfg;
    wbf2->notify=NULL;
    wbf2->notify2=NULL;
    wbf2->ths=NULL;

    wbf2->stft2=wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(wbf2->stft2, wbf2, (wtk_stft2_notify_f)wtk_wbf2_on_stft2);
    wbf2->nbin=wbf2->stft2->nbin;
    wbf2->channel=cfg->stft2.channel;

    if(cfg->use_line)
    {
        wbf2->theta_step=floor(180.0/(cfg->wbf2_cnt-1));   
    }else
    {
        wbf2->theta_step=floor(359.0/cfg->wbf2_cnt)+1;   
    }
    wbf2->theta_range=wbf2->theta_step/2;

    wbf2->qform2=NULL;
    wbf2->qform3=NULL;
    if(cfg->use_qform3)
    {
        wbf2->qform3=(wtk_qform3_t **)wtk_malloc(sizeof(wtk_qform3_t *)*cfg->wbf2_cnt);
        for(i=0; i<cfg->wbf2_cnt; ++i)
        {
            wbf2->qform3[i]=wtk_qform3_new2(&(cfg->qform3), wbf2->stft2);
        }
    }else
    {
        wbf2->qform2=(wtk_qform2_t **)wtk_malloc(sizeof(wtk_qform2_t *)*cfg->wbf2_cnt);
        for(i=0; i<cfg->wbf2_cnt; ++i)
        {
            wbf2->qform2[i]=wtk_qform2_new2(&(cfg->qform2), wbf2->stft2);
        }
    }

    wtk_wbf2_reset(wbf2);

    return wbf2;
}

void wtk_wbf2_delete(wtk_wbf2_t *wbf2)
{
    int i;

    wtk_stft2_delete(wbf2->stft2);
    if(wbf2->qform2)
    {
        for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
        {
            wtk_qform2_delete2(wbf2->qform2[i]);
        }
        wtk_free(wbf2->qform2);
    }else
    {
        for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
        {
            wtk_qform3_delete2(wbf2->qform3[i]);
        }
        wtk_free(wbf2->qform3);
    }


    wtk_free(wbf2);
}

void wtk_wbf2_reset(wtk_wbf2_t *wbf2)
{
    int i;

    wbf2->end_pos=0;
    wtk_stft2_reset(wbf2->stft2);
    if(wbf2->qform2)
    {
        for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
        {
            wtk_qform2_reset2(wbf2->qform2[i]);
        }
    }else
    {
        for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
        {
            wtk_qform3_reset2(wbf2->qform3[i]);
        }
    }
}

void wtk_wbf2_set_notify(wtk_wbf2_t *wbf2,void *ths,wtk_wbf2_notify_f notify)
{
    wbf2->ths=ths;
    wbf2->notify=notify;
}

void wtk_wbf2_start(wtk_wbf2_t *wbf2)
{
    int i;
    if(wbf2->qform2)
    {
        for (i = 0; i < wbf2->cfg->wbf2_cnt; i++) {
            wtk_qform2_start2(wbf2->qform2[i], wbf2->cfg->theta[i], 0);
        }
    }else
    {
        for (i = 0; i < wbf2->cfg->wbf2_cnt; i++) {
            wtk_qform3_start2(wbf2->qform3[i], wbf2->cfg->theta[i], 0);
        }
    }
}

void wtk_wbf2_feed(wtk_wbf2_t *wbf2,short **data,int len,int is_end)
{
    wtk_stft2_feed2(wbf2->stft2, data, len, is_end);
}

void wtk_wbf2_notify_data(wtk_wbf2_t *wbf2,int idx,float *data,int len,int is_end)
{
    short *pv=(short *)data;
    int i;

    for(i=0;i<len;++i)
    {
        if(fabs(data[i])<32000.0)
        {
            pv[i]=data[i];
        }else
        {
            if(data[i]>0)
            {
                pv[i]=32000;
            }else
            {
                pv[i]=-32000;
            }
        }
    }

    if(wbf2->notify)
    {
        wbf2->notify(wbf2->ths,pv,len,idx,is_end);
    }
}

void wtk_wbf2_on_stft2(wtk_wbf2_t *wbf2,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int i;
    int wbf2_cnt=wbf2->cfg->wbf2_cnt;
    int len;
    wtk_qform2_t *qform2;
    wtk_qform3_t *qform3;

    if(is_end)
    {
        wbf2->end_pos=pos;
    }

    if(wbf2->qform2)
    {
        for(i=0; i<wbf2_cnt; ++i)
        {
            qform2=wbf2->qform2[i];
            wtk_qform2_feed_smsg(qform2, msg, pos, is_end);
            len=wtk_stft2_output_ifft(wbf2->stft2, qform2->out, qform2->fout, qform2->pad, wbf2->end_pos, is_end);
            if(wbf2->notify)
            {
                wtk_wbf2_notify_data(wbf2, i, qform2->fout, len, is_end);
            }
        }
    }else
    {
        for(i=0; i<wbf2_cnt; ++i)
        {
            qform3=wbf2->qform3[i];
            wtk_qform3_feed_smsg(qform3, msg, pos, is_end);
            len=wtk_stft2_output_ifft(wbf2->stft2, qform3->out, qform3->fout, qform3->pad, wbf2->end_pos, is_end);
            if(wbf2->notify)
            {
                wtk_wbf2_notify_data(wbf2, i, qform3->fout, len, is_end);
            }
        }
    }

    wtk_stft2_push_msg(wbf2->stft2, msg);
}

wtk_wbf2_t* wtk_wbf2_new2(wtk_wbf2_cfg_t *cfg, wtk_stft2_t *stft2)
{
    wtk_wbf2_t *wbf2;
    int i;

    wbf2=(wtk_wbf2_t *)wtk_malloc(sizeof(wtk_wbf2_t));
    wbf2->cfg=cfg;
    wbf2->notify=NULL;
    wbf2->notify2=NULL;
    wbf2->ths=NULL;

    wbf2->stft2=stft2;
    wbf2->nbin=stft2->nbin;
    wbf2->channel=stft2->cfg->channel;

    if(cfg->use_line)
    {
        wbf2->theta_step=floor(180.0/(cfg->wbf2_cnt-1));   
    }else
    {
        wbf2->theta_step=floor(359.0/cfg->wbf2_cnt)+1;   
    }
    wbf2->theta_range=wbf2->theta_step/2;

    wbf2->qform2=NULL;
    wbf2->qform3=NULL;
    if(cfg->use_qform3)
    {
        wbf2->qform3=(wtk_qform3_t **)wtk_malloc(sizeof(wtk_qform3_t *)*cfg->wbf2_cnt);
        for(i=0; i<cfg->wbf2_cnt; ++i)
        {
            wbf2->qform3[i]=wtk_qform3_new2(&(cfg->qform3), wbf2->stft2);
        }
    }else
    {
        wbf2->qform2=(wtk_qform2_t **)wtk_malloc(sizeof(wtk_qform2_t *)*cfg->wbf2_cnt);
        for(i=0; i<cfg->wbf2_cnt; ++i)
        {
            wbf2->qform2[i]=wtk_qform2_new2(&(cfg->qform2), wbf2->stft2);
        }
    }

    wtk_wbf2_reset2(wbf2);

    return wbf2;
}

void wtk_wbf2_delete2(wtk_wbf2_t *wbf2)
{
    int i;

    if(wbf2->qform2)
    {
        for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
        {
            wtk_qform2_delete2(wbf2->qform2[i]);
        }
        wtk_free(wbf2->qform2);
    }else
    {
        for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
        {
            wtk_qform3_delete2(wbf2->qform3[i]);
        }
        wtk_free(wbf2->qform3);
    }

    wtk_free(wbf2);
}

void wtk_wbf2_reset2(wtk_wbf2_t *wbf2)
{
    int i;

    wbf2->end_pos=0;

    if(wbf2->qform2)
    {
        for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
        {
            wtk_qform2_reset2(wbf2->qform2[i]);
        }
    }else
    {
        for(i=0;i<wbf2->cfg->wbf2_cnt;++i)
        {
            wtk_qform3_reset2(wbf2->qform3[i]);
        }
    }
}

void wtk_wbf2_set_notify2(wtk_wbf2_t *wbf2,void *ths,wtk_wbf2_notify_f2 notify)
{
    wbf2->ths=ths;
    wbf2->notify2=notify;
}

void wtk_wbf2_feed_smsg(wtk_wbf2_t *wbf2,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int i;
    int wbf2_cnt=wbf2->cfg->wbf2_cnt;
    wtk_qform2_t *qform2;
    wtk_qform3_t *qform3;

    if(is_end)
    {
        wbf2->end_pos=pos;
    }

    if(wbf2->qform2)
    {
        for(i=0; i<wbf2_cnt; ++i)
        {
            qform2=wbf2->qform2[i];
            wtk_qform2_feed_smsg(qform2, msg, pos, is_end);
            if(wbf2->notify2)
            {
                wbf2->notify2(wbf2->ths, qform2->out, i, is_end);
            }
        }
    }else
    {
        for(i=0; i<wbf2_cnt; ++i)
        {
            qform3=wbf2->qform3[i];
            wtk_qform3_feed_smsg(qform3, msg, pos, is_end);
            if(wbf2->notify2)
            {
                wbf2->notify2(wbf2->ths, qform3->out, i, is_end);
            }
        }
    }
}

