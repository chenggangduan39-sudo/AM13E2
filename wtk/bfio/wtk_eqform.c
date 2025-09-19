#include "wtk_eqform.h"

void wtk_eqform_on_aec(wtk_eqform_t *eq,short **data,int len,int is_end);
void wtk_eqform_on_qform(wtk_eqform_t *eq,short *data,int len);

wtk_eqform_t* wtk_eqform_new(wtk_eqform_cfg_t *cfg)
{
    wtk_eqform_t *eq;
    
    eq=(wtk_eqform_t *)wtk_malloc(sizeof(wtk_eqform_t));
    eq->cfg=cfg;

    eq->qform9=NULL;
    eq->qform11=NULL;
    eq->qform3=NULL;
    if(cfg->use_qform9){

        eq->channel=eq->cfg->qform9.stft2.channel+eq->cfg->aec.spchannel;

        eq->qform9=wtk_qform9_new(&(cfg->qform9));
        wtk_qform9_set_notify(eq->qform9,eq,(wtk_qform9_notify_f)wtk_eqform_on_qform);
    }else if(cfg->use_qform11){
        eq->channel=eq->cfg->qform11.stft2.channel+eq->cfg->aec.spchannel;

        eq->qform11=wtk_qform11_new(&(cfg->qform11));
        wtk_qform11_set_notify(eq->qform11,eq,(wtk_qform11_notify_f)wtk_eqform_on_qform);
    }else if(cfg->use_qform3){
        eq->channel=eq->cfg->qform3.stft2.channel+eq->cfg->aec.spchannel;

        eq->qform3=wtk_qform3_new(&(cfg->qform3));
        wtk_qform3_set_notify(eq->qform3,eq,(wtk_qform3_notify_f)wtk_eqform_on_qform);
    }

    eq->buf=NULL;
    eq->aec=NULL;
    if(!cfg->use_post_form && cfg->use_aec)
    {
        eq->buf=wtk_strbufs_new(1+cfg->aec.spchannel);
        eq->aec=wtk_aec_new(&(cfg->aec));
        wtk_aec_set_notify(eq->aec,eq,(wtk_aec_notify_f)wtk_eqform_on_aec);
    }else if(cfg->use_aec)
    {
        eq->aec=wtk_aec_new(&(cfg->aec));
        wtk_aec_set_notify(eq->aec,eq,(wtk_aec_notify_f)wtk_eqform_on_aec);
    }

    eq->obuf=NULL;
    if(cfg->use_enrcheck)
    {
        eq->obuf=wtk_strbuf_new(cfg->enrcheck_hist*sizeof(short), 2);
    }

    eq->ths=NULL;
    eq->notify=NULL;
    eq->en_ths=NULL;
    eq->en_notify=NULL;

    wtk_eqform_reset(eq);

    return eq;
}

void wtk_eqform_delete(wtk_eqform_t *eqform)
{
    if(eqform->buf)
    {
        wtk_strbufs_delete(eqform->buf,1+eqform->cfg->aec.spchannel);
    }
    if(eqform->aec)
    {
        wtk_aec_delete(eqform->aec);
    }
    if(eqform->qform9)
    {
        wtk_qform9_delete(eqform->qform9);
    }
    if(eqform->qform11)
    {
        wtk_qform11_delete(eqform->qform11);
    }
    if(eqform->qform3)
    {
        wtk_qform3_delete(eqform->qform3);
    }
    if(eqform->obuf)
    {
        wtk_strbuf_delete(eqform->obuf);
    }
    wtk_free(eqform);
}

void wtk_eqform_reset(wtk_eqform_t *eqform)
{
    if(eqform->aec)
    {
        wtk_aec_reset(eqform->aec);
    }
    if(eqform->qform9)
    {
        wtk_qform9_reset(eqform->qform9);
    }
    if(eqform->qform11){
        wtk_qform11_reset(eqform->qform11);
    }
    if(eqform->qform3)
    {
        wtk_qform3_reset(eqform->qform3);
    }
    if(eqform->obuf)
    {
        wtk_strbuf_reset(eqform->obuf);
    }
}


void wtk_eqform_notify_data(wtk_eqform_t *eq,short *data,int len)
{
    wtk_strbuf_t *obuf=eq->obuf;
    int pop, i, len2;
    short *pv;
    float sum;

    if(obuf)
    {
        wtk_strbuf_push(obuf, (char *)data, len*sizeof(short));
        pop=obuf->pos-eq->cfg->enrcheck_hist*sizeof(short);
        if(pop>=0)
        {
            pv=(short *)obuf->data;
            len2=obuf->pos/sizeof(short);
            sum=0;
            for(i=0; i<len2; ++i)
            {
                sum+=pv[i]*pv[i];
            }
            sum/=len2;
            if(sum>eq->cfg->enr_thresh)
            {
                if(eq->en_notify)
                {
                    eq->en_notify(eq->en_ths, eq->cfg->enr_thresh, sum, 1);
                }
            }
            wtk_strbuf_pop(obuf, NULL, pop);
        }
    }
    if(eq->notify)
    {
        eq->notify(eq->ths, data, len);
    }
}

void wtk_eqform_on_qform(wtk_eqform_t *eq,short *data,int len)
{
    if(eq->cfg->use_post_form || !eq->cfg->use_aec)
    {
        wtk_eqform_notify_data(eq, data, len);
    }else
    {
        wtk_strbuf_push(eq->buf[0],(char *)data,len*sizeof(short));
    }
}

void wtk_eqform_set_notify(wtk_eqform_t *eqform,void *ths,wtk_eqform_notify_f notify)
{
    eqform->ths=ths;
    eqform->notify=notify;
}

void wtk_eqform_set_enrcheck_notify(wtk_eqform_t *eqform,void *ths,wtk_eqform_notify_enrcheck_f notify)
{
    eqform->en_ths=ths;
    eqform->en_notify=notify;
}

void wtk_eqform_start(wtk_eqform_t *eqform,float theta,float phi)
{
    if(eqform->qform9){
        wtk_qform9_start(eqform->qform9,theta,phi);
    }else if(eqform->qform11){
        wtk_qform11_start(eqform->qform11,theta,phi);
    }else if(eqform->qform3){
        wtk_qform3_start(eqform->qform3,theta,phi);
    }
}


void wtk_eqform_on_aec(wtk_eqform_t *eq,short **pv,int len,int is_end)
{
    if(eq->cfg->use_post_form)
    {
        if(eq->qform9){
            wtk_qform9_feed(eq->qform9,pv,len,is_end);
        }else if(eq->qform11){
            wtk_qform11_feed(eq->qform11,pv,len,is_end);
        }else if(eq->qform3){
            wtk_qform3_feed(eq->qform3,pv,len,is_end);
        }
    }else
    {
        if(eq->notify)
        {
            wtk_eqform_notify_data(eq, pv[0], len);
        }
    }
}

void wtk_eqform_feed(wtk_eqform_t *eqform,short **data,int len,int is_end)
{
    short *pv[10];
    int i;
    int spchannel=eqform->cfg->aec.spchannel;
    int nmicchannel=eqform->channel-spchannel;

    if(eqform->cfg->use_aec)
    {
        if(eqform->cfg->use_post_form)
        {
            wtk_aec_feed(eqform->aec,data,len,is_end);
        }else
        {
            wtk_strbuf_t **buf=eqform->buf;
            if(data){
                for(i=0; i<spchannel; ++i)
                {
                    wtk_strbuf_push(buf[i+1],(char *)(data[nmicchannel]),len*sizeof(short));
                }
            }
            if(eqform->qform9){
                wtk_qform9_feed(eqform->qform9,data,len,is_end);
            }else if(eqform->qform11){
                wtk_qform11_feed(eqform->qform11,data,len,is_end);
            }else if(eqform->qform3){
                wtk_qform3_feed(eqform->qform3,data,len,is_end);
            }
            
            if(buf[0]->pos>0)
            {
                pv[0]=(short *)buf[0]->data;
                for(i=0; i<spchannel; ++i)
                {
                    pv[i+1]=(short *)buf[i+1]->data;
                }
                len=buf[0]->pos/sizeof(short);
                wtk_aec_feed(eqform->aec,pv,len,0);
                for(i=0; i<spchannel; ++i)
                {
                    wtk_strbuf_pop(buf[i+1],NULL,buf[0]->pos);
                }
                wtk_strbuf_reset(buf[0]);
            }
            if(is_end)
            {
                wtk_aec_feed(eqform->aec,NULL,0,1);
            } 
        }
    }else
    {
        if(eqform->qform9){
            wtk_qform9_feed(eqform->qform9,data,len,is_end);
        }else if(eqform->qform11){
            wtk_qform11_feed(eqform->qform11,data,len,is_end);
        }else if(eqform->qform3){
            wtk_qform3_feed(eqform->qform3,data,len,is_end);
        }
    }
}