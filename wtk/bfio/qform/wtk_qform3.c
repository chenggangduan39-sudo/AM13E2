#include "wtk_qform3.h"

void wtk_qform3_on_stft2(wtk_qform3_t *qform3, wtk_stft2_msg_t *msg, int pos,
                         int is_end);
void wtk_qform3_on_admm(wtk_qform3_t *qform3, wtk_stft2_msg_t *msg,
                        wtk_stft2_msg_t *err_msg, wtk_stft2_msg_t *lsty_msg,
                        int pos, int is_end);

wtk_qform3_t *wtk_qform3_new(wtk_qform3_cfg_t *cfg) {
    wtk_qform3_t *qform3;
    int i;

    qform3 = (wtk_qform3_t *)wtk_malloc(sizeof(wtk_qform3_t));
    qform3->cfg = cfg;
    qform3->ths = NULL;
    qform3->notify = NULL;

    qform3->stft2 = wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(qform3->stft2, qform3,
                         (wtk_stft2_notify_f)wtk_qform3_on_stft2);

    qform3->nbin = qform3->stft2->nbin;
    qform3->channel = qform3->stft2->cfg->channel;

    qform3->input = NULL;
    if (cfg->use_preemph) {
        qform3->input = wtk_strbufs_new(qform3->channel);
    }
    qform3->admm = NULL;
    if (cfg->use_admm2) {
        qform3->admm = wtk_admm2_new2(&(cfg->admm), qform3->stft2);
        wtk_admm2_set_notify2(qform3->admm, qform3,
                              (wtk_admm2_notify_f2)wtk_qform3_on_admm);
    }

    qform3->fft = wtk_complex_new_p2(2, qform3->nbin);
    qform3->ovec = wtk_complex_new_p3(2, qform3->nbin, qform3->channel);

    qform3->rls3 = NULL;
    qform3->nlms = NULL;
    if (!cfg->use_nlms) {
        qform3->rls3 = (wtk_rls3_t *)wtk_malloc(sizeof(wtk_rls3_t));
        wtk_rls3_init(qform3->rls3, &(qform3->cfg->rls3), qform3->nbin);
    } else {
        qform3->nlms =
            (wtk_nlms_t *)wtk_malloc(sizeof(wtk_nlms_t) * qform3->nbin);
        for (i = 0; i < qform3->nbin; ++i) {
            wtk_nlms_init(qform3->nlms + i, &(qform3->cfg->nlms));
        }
    }

    qform3->pad = (float *)wtk_malloc(sizeof(float) * qform3->stft2->cfg->win);
    qform3->fout = (float *)wtk_malloc(sizeof(float) * qform3->stft2->cfg->win);

    qform3->Yf = (float *)wtk_malloc(sizeof(float) * qform3->nbin);
    qform3->out =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * qform3->nbin);

    qform3->Se = NULL;
    qform3->Sd = NULL;
    qform3->Sed = NULL;
    qform3->qmmse = NULL;
    if (cfg->use_post) {
        qform3->Se = (float *)wtk_malloc(sizeof(float) * qform3->nbin);
        qform3->Sd = (float *)wtk_malloc(sizeof(float) * qform3->nbin);
        qform3->Sed =
            (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * qform3->nbin);
        qform3->qmmse = wtk_qmmse_new(&(cfg->qmmse));
    }
    qform3->aspec = NULL;
    qform3->cohv_fn=NULL;
    if(cfg->use_aspec){
        qform3->aspec = wtk_aspec_new(&(cfg->aspec), qform3->nbin, 3);
        if(cfg->debug)
        {
            qform3->cohv_fn=fopen("cohv.dat","w");
        }
    }
    qform3->cov = NULL;
    qform3->inv_cov = NULL;
    qform3->tmp = NULL;
    if(qform3->aspec && qform3->aspec->need_cov)
    {
        qform3->cov = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform3->channel*qform3->channel);
        qform3->tmp = (wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*qform3->channel*qform3->channel);
    }
    if(qform3->aspec && qform3->aspec->need_inv_cov)
    {
        qform3->inv_cov = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform3->channel*qform3->channel);
    }

    wtk_qform3_reset(qform3);

    return qform3;
}

void wtk_qform3_delete(wtk_qform3_t *qform3) {
    int nbin = qform3->nbin;
    int i;

    wtk_complex_delete_p2(qform3->fft, 2);
    wtk_complex_delete_p3(qform3->ovec, 2, qform3->nbin);

    if (qform3->input) {
        wtk_strbufs_delete(qform3->input, qform3->channel);
    }
    wtk_stft2_delete(qform3->stft2);

    if (qform3->admm) {
        wtk_admm2_delete2(qform3->admm);
    }

    if (qform3->rls3) {
        wtk_rls3_clean(qform3->rls3);
        wtk_free(qform3->rls3);
    } else if (qform3->nlms) {
        for (i = 0; i < nbin; ++i) {
            wtk_nlms_clean(qform3->nlms + i);
        }
        wtk_free(qform3->nlms);
    }

    wtk_free(qform3->Yf);
    wtk_free(qform3->out);
    if (qform3->qmmse) {
        wtk_free(qform3->Se);
        wtk_free(qform3->Sd);
        wtk_free(qform3->Sed);
        wtk_qmmse_delete(qform3->qmmse);
    }
    if(qform3->aspec)
    {
        wtk_aspec_delete(qform3->aspec);
    }
    if(qform3->cohv_fn)
    {
        fclose(qform3->cohv_fn);
    }
    if(qform3->cov)
    {
        wtk_free(qform3->cov);
    }
    if(qform3->inv_cov)
    {
        wtk_free(qform3->inv_cov);
    }
    if(qform3->tmp)
    {
        wtk_free(qform3->tmp);
    }

    wtk_free(qform3->fout);
    wtk_free(qform3->pad);
    wtk_free(qform3);
}

void wtk_qform3_reset(wtk_qform3_t *qform3) {
    int i;
    int channel = qform3->channel;
    int nbin = qform3->nbin;

    // wtk_bf_reset(qform3->bf);

    for (i = 0; i < channel; ++i) {
        memset(qform3->notch_mem[i], 0, 2 * sizeof(float));
    }
    memset(qform3->memD, 0, channel * sizeof(float));
    qform3->memX = 0;

    qform3->theta = qform3->phi = 0;
    wtk_stft2_reset(qform3->stft2);

    if (qform3->rls3) {
        wtk_rls3_reset(qform3->rls3, nbin);
    } else if (qform3->nlms) {
        for (i = 0; i < nbin; ++i) {
            wtk_nlms_reset(qform3->nlms + i);
        }
    }

    if (qform3->admm) {
        wtk_admm2_reset2(qform3->admm);
    }

    qform3->nframe = 0;
    qform3->end_pos = 0;

    if (qform3->qmmse) {
        memset(qform3->Se, 0, sizeof(float) * qform3->nbin);
        memset(qform3->Sd, 0, sizeof(float) * qform3->nbin);
        memset(qform3->Sed, 0, sizeof(wtk_complex_t) * qform3->nbin);

        wtk_qmmse_reset(qform3->qmmse);
    }

    if(qform3->aspec)
    {
        wtk_aspec_reset(qform3->aspec);
    }
    qform3->aspec_mask = 1.0;
    qform3->spec_cnt = qform3->cfg->spec_cnt;

    memset(qform3->fout, 0, sizeof(float) * qform3->stft2->cfg->win);
    memset(qform3->pad, 0, sizeof(float) * qform3->stft2->cfg->win);
}

void wtk_qform3_set_notify(wtk_qform3_t *qform3, void *ths,
                           wtk_qform3_notify_f notify) {
    qform3->ths = ths;
    qform3->notify = notify;
}

void wtk_qform3_notify_data(wtk_qform3_t *qform3, float *data, int len,
                            int is_end) {
    short *pv = (short *)data;
    int i;

    if (qform3->input) {
        qform3->memX = wtk_preemph_asis2(data, len, qform3->memX);
    }
    for (i = 0; i < len; ++i) {
        if (fabs(data[i]) < 32000.0) {
            pv[i] = data[i];
        } else {
            if (data[i] > 0) {
                pv[i] = 32000;
            } else {
                pv[i] = -32000;
            }
        }
    }
    qform3->notify(qform3->ths, pv, len, is_end);
}

void wtk_qform3_update_aspec2(wtk_qform3_t *qform3, wtk_aspec_t *aspec, wtk_complex_t *cov, 
                                                                                    wtk_complex_t *inv_cov, float cov_travg, int k, float *spec_k)
{
    int sang_num=aspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
    }
}

void wtk_qform3_aspec_update_cov(wtk_qform3_t *qform3, wtk_complex_t **infft)
{
    int i, j, k;
    wtk_complex_t *fft1, *fft2, *a, *b;
    float cov_travg;
    int ret;
    int nbin = qform3->nbin;
    int channel = qform3->channel;
    wtk_complex_t *cov=qform3->cov;
    wtk_complex_t *inv_cov=qform3->inv_cov;
    wtk_dcomplex_t *tmp=qform3->tmp;
    float spec_k[3]={0}, specsum=0;
    int spec_cnt=qform3->cfg->spec_cnt;
    float spec_thresh=qform3->cfg->spec_thresh;
    int reduce_cnt=qform3->cfg->reduce_cnt;
    float min_scale=qform3->cfg->min_scale;

    for(k=1;k<nbin-1;++k){
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        fft2=fft1=infft[k];
        for(i=0;i<channel;++i,++fft1){
            fft2=fft1;
            for(j=i;j<channel;++j,++fft2){
                a=cov+i*channel+j;
                if(i!=j)
                {
                    a->a=fft1->a*fft2->a+fft1->b*fft2->b;
                    a->b=-fft1->a*fft2->b+fft1->b*fft2->a;
                }else
                {
                    a->a=fft1->a*fft2->a+fft1->b*fft2->b;
                    a->b=0;
                }
            }
        }
        for(i=0;i<channel;++i)
        {
            for(j=i;j<channel;++j)
            {
                a=cov+i*channel+j;
                if(i!=j)
                {
                    b=cov+j*channel+i;
                    b->a=a->a;
                    b->b=-a->b;
                }
            }
        }
        if(inv_cov)
        {
            ret=wtk_complex_invx4(cov,tmp,channel,inv_cov,1);            
            if(ret!=0)
            {
                j=0;
                for(i=0;i<channel;++i)
                {
                    cov[j].a+=0.01;
                    j+=channel+1;
                }
                wtk_complex_invx4(cov,tmp,channel,inv_cov,1);
            }
        }
        cov_travg=0;
        if(qform3->aspec->need_cov_travg) 
        {
            for(i=0;i<channel;++i)
            {
                cov_travg+=cov[i*channel+i].a;
            }
            cov_travg/=channel;
        }

        wtk_qform3_update_aspec2(qform3, qform3->aspec, cov, inv_cov, cov_travg, k, spec_k);

        if(spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
        {
            specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
        }
    }
    if(qform3->cohv_fn)
    {
        fprintf(qform3->cohv_fn,"%.0f %f\n",qform3->nframe,specsum);
    }
    // printf("%f\n", specsum);
    if(specsum >= spec_thresh){
        qform3->spec_cnt = spec_cnt;
    }else{
        --qform3->spec_cnt;
    }
    // printf("%d\n", qform3->spec_cnt);
    if(qform3->spec_cnt<=0){
        qform3->aspec_mask = min_scale;
    }else if(qform3->spec_cnt<reduce_cnt){
        qform3->aspec_mask = ((1.0-min_scale)/reduce_cnt)*qform3->spec_cnt;
    }else{
        qform3->aspec_mask = 1.0;
    }
}

void wtk_qform3_update_aspec(wtk_qform3_t *qform3, wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, int k, float *spec_k)
{
    int sang_num=aspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, fft, fftabs2, 0, NULL, NULL, k ,n);
    }
}

void wtk_qform3_aspec_update(wtk_qform3_t *qform3, wtk_complex_t **infft)
{
    int i, k;
    wtk_complex_t *fft2;
    int nbin = qform3->nbin;
    int channel = qform3->channel;
    float fftabs2;
    float spec_k[3]={0}, specsum=0;
    int spec_cnt=qform3->cfg->spec_cnt;
    float spec_thresh=qform3->cfg->spec_thresh;
    int reduce_cnt=qform3->cfg->reduce_cnt;
    float min_scale=qform3->cfg->min_scale;

    for(k=1;k<nbin-1;++k){
        fftabs2=0;
        fft2=infft[k];
        for(i=0; i<channel; ++i,++fft2)
        {
            fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
        }
        wtk_qform3_update_aspec(qform3,qform3->aspec,infft,fftabs2,k,spec_k);
        if(spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
        {
            specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
        }
    }
    if(qform3->cohv_fn)
    {
        fprintf(qform3->cohv_fn,"%.0f %f\n",qform3->nframe,specsum);
    }
    // printf("%f\n", specsum);
    if(specsum >= spec_thresh){
        qform3->spec_cnt = spec_cnt;
    }else{
        --qform3->spec_cnt;
    }
    // printf("%d\n", qform3->spec_cnt);
    if(qform3->spec_cnt<=0){
        qform3->aspec_mask = min_scale;
    }else if(qform3->spec_cnt<reduce_cnt){
        qform3->aspec_mask = ((1.0-min_scale)/reduce_cnt)*qform3->spec_cnt;
    }else{
        qform3->aspec_mask = 1.0;
    }
}

void wtk_qform3_feed_smsg2(wtk_qform3_t *qform3, wtk_stft2_msg_t *msg, int pos,
                           int is_end) {
    wtk_complex_t ***ovec = qform3->ovec, **ovec2, *ovec3;
    wtk_complex_t **fft = qform3->fft, *fft1;
    int channel = qform3->channel;
    int nbin = qform3->nbin;
    int i, k, c;
    float ta, tb;
    wtk_complex_t **infft, *infft2;
    wtk_rls3_t *rls3 = qform3->rls3;
    wtk_nlms_t *nlms = qform3->nlms;
    float *Se = qform3->Se;
    float *Sd = qform3->Sd;
    wtk_complex_t *Sed = qform3->Sed;
    wtk_complex_t *out = qform3->out, *E, *Y;
    float *Yf = qform3->Yf, ef, yf, leak;
    float coh_alpha = qform3->cfg->coh_alpha;

    ++qform3->nframe;
    if (is_end) {
        qform3->end_pos = pos;
    }

    if (msg) {
        infft = msg->fft;
        for (c = 0; c < 2; ++c) {
            ovec2 = ovec[c];

            fft1 = fft[c];
            for (k = 0; k < nbin; ++k, ++fft1) {
                ovec3 = ovec2[k];
                ta = tb = 0;
                infft2 = infft[k];
                if(c==0){
                    for (i = 0; i < channel; ++i, ++infft2, ++ovec3) {
                        ta += ovec3->a * infft2->a + ovec3->b * infft2->b;
                        tb += ovec3->a * infft2->b - ovec3->b * infft2->a;
                    }
                }else{
                    for (i = 0; i < channel; ++i, ++infft2, ++ovec3) {
                        ta += ovec3->a * infft2->a - ovec3->b * infft2->b;
                        tb += ovec3->a * infft2->b + ovec3->b * infft2->a;
                    }
                }
                fft1->a = ta;
                fft1->b = tb;
            }
        }
        if(qform3->aspec){
            if(qform3->cov){
                wtk_qform3_aspec_update_cov(qform3, infft);
            }else{
                wtk_qform3_aspec_update(qform3, infft);
            }
        }

        if (qform3->rls3) {
            wtk_rls3_feed(rls3, fft[0], fft[1], nbin);
            for (i = 0; i < nbin; ++i) {
                out[i] = rls3->out[i];
                if (qform3->qmmse) {
                    E = rls3->out + i;
                    Y = rls3->lsty + i;
                    if (qform3->nframe <= 1) {
                        Se[i] += E->a * E->a + E->b * E->b;
                        yf = Y->a * Y->a + Y->b * Y->b;
                        Sd[i] += yf;
                        Sed[i].a += Y->a * E->a + Y->b * E->b;
                        Sed[i].b += -Y->a * E->b + Y->b * E->a;

                        leak = (Sed[i].a * Sed[i].a + Sed[i].b * Sed[i].b) /
                                (max(Se[i], Sd[i]) * Sd[i] + 1e-9);
                        /*leak=max(0.005, leak);*/
                        Yf[i] = leak * yf;
                    } else {
                        ef = E->a * E->a + E->b * E->b;
                        yf = Y->a * Y->a + Y->b * Y->b;

                        Se[i] = (1 - coh_alpha) * Se[i] + coh_alpha * ef;
                        Sd[i] = (1 - coh_alpha) * Sd[i] + coh_alpha * yf;
                        Sed[i].a = (1 - coh_alpha) * Sed[i].a +
                                    coh_alpha * (Y->a * E->a + Y->b * E->b);
                        Sed[i].b = (1 - coh_alpha) * Sed[i].b +
                                    coh_alpha * (-Y->a * E->b + Y->b * E->a);

                        leak = (Sed[i].a * Sed[i].a + Sed[i].b * Sed[i].b) /
                                (max(Se[i], Sd[i]) * Sd[i] + 1e-9);
                        /*leak=max(0.005, leak);*/
                        Yf[i] = leak * yf;
                    }
                }
            }
        } else if (qform3->nlms) {
            for (i = 0; i < nbin; ++i, ++nlms) {
                wtk_nlms_feed(nlms, &(fft[0][i]), &(fft[1][i]));
                out[i] = nlms->out[0];
                if (qform3->qmmse) {
                    Yf[i] = nlms->lsty_power[0];
                }
            }
        }

        if (qform3->qmmse) {
            wtk_qmmse_feed_echo_denoise(qform3->qmmse, out, Yf);
        }
        if(qform3->aspec){
            for(i=0;i<nbin;++i){
                out[i].a *= qform3->aspec_mask;
                out[i].b *= qform3->aspec_mask;
            }
        }
    }
}

void wtk_qform3_on_stft2(wtk_qform3_t *qform3, wtk_stft2_msg_t *msg, int pos,
                         int is_end) {
    int len;
    wtk_complex_t *out = qform3->out;
    if (qform3->admm) {
        wtk_admm2_feed_stftmsg(qform3->admm, msg, pos, is_end);
    } else {
        wtk_qform3_feed_smsg2(qform3, msg, pos, is_end);
    }
    len = wtk_stft2_output_ifft(qform3->stft2, out, qform3->fout, qform3->pad,
                                qform3->end_pos, is_end);
    if (qform3->notify) {
        wtk_qform3_notify_data(qform3, qform3->fout, len, is_end);
    }
    wtk_stft2_push_msg(qform3->stft2, msg);
}

void wtk_qform3_on_admm(wtk_qform3_t *qform3, wtk_stft2_msg_t *omsg,
                        wtk_stft2_msg_t *msg, wtk_stft2_msg_t *lmsg, int pos,
                        int is_end) {
    wtk_qform3_feed_smsg2(qform3, msg, pos, is_end);
}

void wtk_qform3_flush_ovec(wtk_qform3_t *qform3, wtk_complex_t *ovec,
                           float **mic_pos, float sv, int rate, float theta2,
                           float phi2, int k) {
    float x, y, z;
    float t;
    float *mic;
    int j;
    int channel = qform3->channel;
    int win = (qform3->nbin - 1) * 2;
    wtk_complex_t *ovec1;
    float theta, phi;
    float tdoa;

    phi = phi2 * PI / 180;
    theta = theta2 * PI / 180;
    x = cos(phi) * cos(theta);
    y = cos(phi) * sin(theta);
    z = sin(phi);

    ovec1 = ovec;
    t = 2 * PI * rate * 1.0 / win * k;
    for (j = 0; j < channel; ++j) {
        mic = mic_pos[j];
        tdoa = (mic[0] * x + mic[1] * y + mic[2] * z) / sv;
        ovec1[j].a = cos(t * tdoa);
        ovec1[j].b = sin(t * tdoa);
    }
}

void wtk_qform3_start_aspec(wtk_qform3_t *qform3, float theta, float phi, float theta_range)
{
    wtk_aspec_t *aspec=qform3->aspec;
    float theta2, theta3;

    if(qform3->cfg->use_line){
        if(theta==0 || theta==180)
        {
            theta2=theta+2*theta_range;
            if(theta2>180)
            {
                theta2=360-theta2;
            }

            aspec->start_ang_num=2;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2, 0, 1);
        }else
        {
            theta2=theta+2*theta_range;
            if(theta2>180)
            {
                if(theta+theta_range>=180)
                {
                    theta2=-1;
                }else
                {
                    theta2=180;   
                }
            }
            theta3=theta-2*theta_range;
            if(theta3<0)
            {
                if(theta-theta_range<=0)
                {
                    theta3=-1;
                }else
                {
                    theta3=0;
                }
            }
            if(theta2==-1 || theta3==-1)
            {
                aspec->start_ang_num=2;
                wtk_aspec_start(aspec, theta, 0, 0);
                wtk_aspec_start(aspec, theta2==-1?theta3:theta2, 0, 1);
            }else
            {
                aspec->start_ang_num=3;
                wtk_aspec_start(aspec, theta, 0, 0);
                wtk_aspec_start(aspec, theta2, 0, 1);
                wtk_aspec_start(aspec, theta3, 0, 2);   
            }
        }
    }else{
        theta2=theta+2*theta_range;
        if(theta2>=360)
        {
            theta2-=360;
        }
        theta3=theta-2*theta_range;
        if(theta3<0)
        {
            theta3+=360;
        }
        aspec->start_ang_num=3;
        wtk_aspec_start(aspec, theta, 0, 0);
        wtk_aspec_start(aspec, theta2, 0, 1);
        wtk_aspec_start(aspec, theta3, 0, 2);   
    }
}


void wtk_qform3_start(wtk_qform3_t *qform3, float theta, float phi) {
    wtk_complex_t ***ovec = qform3->ovec;
    int channel = qform3->channel;
    int nbin = qform3->nbin;
    int i, k;
    wtk_complex_t *ncov, *rk, *otmp;
    float f;
    wtk_dcomplex_t *tmp2;
    int ret;
    int theta_step =
        qform3->cfg->use_line ? 180 / (channel - 1) : 360 / (channel - 1);
    float theta2[channel-1];
    float **mic_pos=qform3->cfg->bf.mic_pos;
    float sv=qform3->cfg->bf.speed;
    int rate=qform3->cfg->rate;

    int max_th=qform3->cfg->use_line ? 180:359;
    int th;
    int j;
    float ls_eye=1e-3;
    wtk_complex_t *novec=NULL,*a,*b;
    wtk_complex_t *cov=NULL,*cov1,*cov2;
    wtk_complex_t *scov=NULL;
    float x = 1.0/sqrt(channel);
    int bf_min_th=max(0,theta-qform3->cfg->theta_range);
    int bf_max_th=min(max_th,theta+qform3->cfg->theta_range);

    ncov =
        (wtk_complex_t *)wtk_calloc(channel * channel, sizeof(wtk_complex_t));
    rk = (wtk_complex_t *)wtk_calloc(channel, sizeof(wtk_complex_t));
    tmp2 = (wtk_dcomplex_t *)wtk_calloc(channel * channel * 2,
                                        sizeof(wtk_dcomplex_t));

    if(qform3->cfg->use_ls_bf){
        novec=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
        cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel*channel);
        scov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*channel);
    }

    if(qform3->aspec){
        wtk_qform3_start_aspec(qform3, theta, phi, qform3->cfg->theta_range);
    }

    if (qform3->cfg->use_line) {
        for (i = 0; i < (channel - 1); ++i) {
            theta2[i] = theta + theta_step * (i + 1);
            if (theta2[i] > 180) {
                theta2[i] -= 180;
            }
            if (theta2[i] == theta) {
                if ((theta2[i] - 0) > (180 - theta2[i])) {
                    theta2[i] = 0;
                } else {
                    theta2[i] = 180;
                }
            }
        }
        wtk_qsort_float(theta2, channel-1);
        for (k = 0; k < nbin; ++k) {
            wtk_qform3_flush_ovec(qform3, ovec[0][k], mic_pos, sv, rate, theta, 0, k);
            memcpy(ncov + channel * (channel - 1), ovec[0][k],
                   sizeof(wtk_complex_t) * channel);
            rk[channel - 1].a = rk[channel - 1].b = 0;
            if(qform3->cfg->use_ls_bf){
                memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
                memset(scov,0,sizeof(wtk_complex_t)*channel);
                for(th=0;th<=max_th;++th){
                    wtk_qform3_flush_ovec(qform3, novec, mic_pos, sv, rate, th, 0, k);
                    for(i=0;i<channel;++i){
                        novec[i].a *= x;
                        novec[i].b *= x;
                    }
                    a=novec;
                    for(i=0;i<channel;++i,++a)
                    {
                        b=a;
                        for(j=i;j<channel;++j,++cov1,++b)
                        {
                            cov1=cov+i*channel+j;
                            if(i!=j)
                            {
                                cov1->a+=a->a*b->a+a->b*b->b;
                                cov1->b+=-a->a*b->b+a->b*b->a;
                                cov2=cov+j*channel+i;
                                cov2->a=cov1->a;
                                cov2->b=-cov1->b;
                            }else
                            {
                                cov1->a+=a->a*b->a+a->b*b->b;
                                cov1->b=0;
                            }
                        }
                    }
                }
                for(i=0,j=0;i<channel;++i)
                {
                    cov[j].a+=ls_eye;
                    j+=channel+1;
                }

                for(th=bf_min_th;th<=bf_max_th;++th){
                    wtk_qform3_flush_ovec(qform3, novec, mic_pos, sv, rate, th, 0, k);
                    for(i=0;i<channel;++i){
                        novec[i].a *= x;
                        // novec[i].b *= x;
                        scov[i].a += novec[i].a;
                        // scov[i].b += novec[i].b;
                    }
                }
                ret = wtk_complex_guass_elimination_p1(cov, scov, tmp2, channel, ovec[0][k]);
                if (ret != 0) {
                    wtk_debug("inv error\n");
                }
            }

            for (i = 0; i < (channel - 1); ++i) {
                wtk_qform3_flush_ovec(qform3, ncov + channel * i, mic_pos, sv, rate, theta2[i], 0, k);
                rk[i].a = 1;
                rk[i].b = 0;
            }
            for (i = 0; i < channel; ++i) {
                ncov[i * channel + i].a += 0.01;
            }

            ret = wtk_complex_guass_elimination_p1(ncov, rk, tmp2, channel,
                                                   ovec[1][k]);
            if (ret != 0) {
                wtk_debug("inv error\n");
            }
            f = 0;
            otmp = ovec[1][k];
            for (i = 0; i < channel; ++i, ++otmp) {
                f += otmp->a * otmp->a + otmp->b * otmp->b;
            }
            f = 1.0 / sqrtf(f);
            otmp = ovec[1][k];
            for (i = 0; i < channel; ++i, ++otmp) {
                otmp->a *= f;
                otmp->b *= f;
            }
        }
    } else {
        for (k = 0; k < nbin; ++k) {
            wtk_qform3_flush_ovec(qform3, ovec[0][k], mic_pos, sv, rate, theta, 0, k);
            memcpy(ncov + channel * (channel - 1), ovec[0][k],
                   sizeof(wtk_complex_t) * channel);
            rk[channel - 1].a = rk[channel - 1].b = 0;
            for (i = 0; i < (channel - 1); ++i) {
                wtk_qform3_flush_ovec(qform3, ncov + channel * i, mic_pos, sv, rate, theta + theta_step * (i + 1), 0, k);
                rk[i].a = 1;
                rk[i].b = 0;
            }
            for (i = 0; i < channel; ++i) {
                ncov[i * channel + i].a += 0.01;
            }
            ret = wtk_complex_guass_elimination_p1(ncov, rk, tmp2, channel,
                                                   ovec[1][k]);
            if (ret != 0) {
                wtk_debug("inv error\n");
            }
            f = 0;
            otmp = ovec[1][k];
            for (i = 0; i < channel; ++i, ++otmp) {
                f += otmp->a * otmp->a + otmp->b * otmp->b;
            }
            f = 1.0 / sqrtf(f);
            otmp = ovec[1][k];
            for (i = 0; i < channel; ++i, ++otmp) {
                otmp->a *= f;
                otmp->b *= f;
            }
        }
    }

    wtk_free(ncov);
    wtk_free(rk);
    wtk_free(tmp2);
    if(novec){
        wtk_free(novec);
    }
    if(cov){
        wtk_free(cov);
    }
    if(scov){
        wtk_free(scov);
    }
}

void wtk_qform3_feed2(wtk_qform3_t *qform3, short **data, int len, int is_end) {
    int i, j;
    int channel = qform3->channel;
    float fv;
    float *fp[10];
    wtk_strbuf_t **input = qform3->input;

    for (i = 0; i < channel; ++i) {
        wtk_strbuf_reset(input[i]);
        for (j = 0; j < len; ++j) {
            fv = data[i][j];
            wtk_strbuf_push(input[i], (char *)(&fv), sizeof(float));
        }
        fp[i] = (float *)(input[i]->data);

        wtk_preemph_dc(fp[i], qform3->notch_mem[i], len);

        qform3->memD[i] = wtk_preemph_asis(fp[i], len, qform3->memD[i]);
    }

    wtk_stft2_feed_float(qform3->stft2, fp, len, is_end);
}

void wtk_qform3_feed(wtk_qform3_t *qform3, short **data, int len, int is_end) {
#ifdef DEBUG_WAV
    static wtk_wavfile_t *mic_log = NULL;

    if (!mic_log) {
        mic_log = wtk_wavfile_new(16000);
        wtk_wavfile_set_channel(mic_log, qform3->bf->channel);
        wtk_wavfile_open2(mic_log, "qform3");
    }
    if (len > 0) {
        wtk_wavfile_write_mc(mic_log, data, len);
    }
    if (is_end && mic_log) {
        wtk_wavfile_close(mic_log);
        wtk_wavfile_delete(mic_log);
        mic_log = NULL;
    }
#endif
    if (qform3->input) {
        wtk_qform3_feed2(qform3, data, len, is_end);
    } else {
        wtk_stft2_feed2(qform3->stft2, data, len, is_end);
    }
}

wtk_qform3_t *wtk_qform3_new2(wtk_qform3_cfg_t *cfg, wtk_stft2_t *stft2) {
    wtk_qform3_t *qform3;
    int i;

    qform3 = (wtk_qform3_t *)wtk_malloc(sizeof(wtk_qform3_t));
    qform3->cfg = cfg;
    qform3->ths = NULL;
    qform3->notify = NULL;

    qform3->stft2 = stft2;

    qform3->nbin = qform3->stft2->nbin;
    qform3->channel = qform3->stft2->cfg->channel;

    qform3->input = NULL;

    qform3->admm = NULL;
    if (cfg->use_admm2) {
        qform3->admm = wtk_admm2_new2(&(cfg->admm), qform3->stft2);
        wtk_admm2_set_notify2(qform3->admm, qform3,
                              (wtk_admm2_notify_f2)wtk_qform3_on_admm);
    }

    qform3->rls3 = NULL;
    qform3->nlms = NULL;
    if (!cfg->use_nlms) {
        qform3->rls3 = (wtk_rls3_t *)wtk_malloc(sizeof(wtk_rls3_t));
        wtk_rls3_init(qform3->rls3, &(qform3->cfg->rls3), qform3->nbin);
    } else {
        qform3->nlms =
            (wtk_nlms_t *)wtk_malloc(sizeof(wtk_nlms_t) * qform3->nbin);
        for (i = 0; i < qform3->nbin; ++i) {
            wtk_nlms_init(qform3->nlms + i, &(qform3->cfg->nlms));
        }
    }
    qform3->fft = wtk_complex_new_p2(2, qform3->nbin);
    qform3->ovec = wtk_complex_new_p3(2, qform3->nbin, qform3->channel);

    qform3->pad = (float *)wtk_malloc(sizeof(float) * stft2->cfg->win);
    qform3->fout = (float *)wtk_malloc(sizeof(float) * stft2->cfg->win);

    qform3->Yf = (float *)wtk_malloc(sizeof(float) * qform3->nbin);
    qform3->out =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * qform3->nbin);

    qform3->Se = NULL;
    qform3->Sd = NULL;
    qform3->Sed = NULL;
    qform3->qmmse = NULL;
    if (cfg->use_post) {
        qform3->Se = (float *)wtk_malloc(sizeof(float) * qform3->nbin);
        qform3->Sd = (float *)wtk_malloc(sizeof(float) * qform3->nbin);
        qform3->Sed =
            (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * qform3->nbin);
        qform3->qmmse = wtk_qmmse_new(&(cfg->qmmse));
    }
    qform3->aspec = NULL;
    if(cfg->use_aspec){
        qform3->aspec = wtk_aspec_new(&(cfg->aspec), qform3->nbin, 3);
    }
    qform3->cov = NULL;
    qform3->inv_cov = NULL;
    qform3->tmp = NULL;
    if(qform3->aspec && qform3->aspec->need_cov)
    {
        qform3->cov = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform3->channel*qform3->channel);
        qform3->tmp = (wtk_dcomplex_t *)wtk_malloc(sizeof(wtk_dcomplex_t)*qform3->channel*qform3->channel);
    }
    if(qform3->aspec && qform3->aspec->need_inv_cov)
    {
        qform3->inv_cov = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform3->channel*qform3->channel);
    }

    wtk_qform3_reset2(qform3);

    return qform3;
}

void wtk_qform3_delete2(wtk_qform3_t *qform3) {
    int nbin = qform3->nbin;
    int i;

    wtk_complex_delete_p2(qform3->fft, 2);
    wtk_complex_delete_p3(qform3->ovec, 2, qform3->nbin);

    if (qform3->admm) {
        wtk_admm2_delete2(qform3->admm);
    }

    if (qform3->rls3) {
        wtk_rls3_clean(qform3->rls3);
        wtk_free(qform3->rls3);
    } else if (qform3->nlms) {
        for (i = 0; i < nbin; ++i) {
            wtk_nlms_clean(qform3->nlms + i);
        }
        wtk_free(qform3->nlms);
    }
    wtk_free(qform3->Yf);
    wtk_free(qform3->out);
    if (qform3->qmmse) {
        wtk_free(qform3->Se);
        wtk_free(qform3->Sd);
        wtk_free(qform3->Sed);
        wtk_qmmse_delete(qform3->qmmse);
    }
    if(qform3->aspec)
    {
        wtk_aspec_delete(qform3->aspec);
    }
    if(qform3->cov)
    {
        wtk_free(qform3->cov);
    }
    if(qform3->inv_cov)
    {
        wtk_free(qform3->inv_cov);
    }
    if(qform3->tmp)
    {
        wtk_free(qform3->tmp);
    }

    wtk_free(qform3->fout);
    wtk_free(qform3->pad);
    wtk_free(qform3);
}

void wtk_qform3_reset2(wtk_qform3_t *qform3) {
    int i;
    int nbin = qform3->nbin;

    qform3->theta = qform3->phi = 0;

    if (qform3->admm) {
        wtk_admm2_reset2(qform3->admm);
    }

    if (qform3->rls3) {
        wtk_rls3_reset(qform3->rls3, nbin);
    } else if (qform3->nlms) {
        for (i = 0; i < nbin; ++i) {
            wtk_nlms_reset(qform3->nlms + i);
        }
    }

    qform3->nframe = 0;
    qform3->end_pos = 0;

    if (qform3->qmmse) {
        memset(qform3->Se, 0, sizeof(float) * qform3->nbin);
        memset(qform3->Sd, 0, sizeof(float) * qform3->nbin);
        memset(qform3->Sed, 0, sizeof(wtk_complex_t) * qform3->nbin);

        wtk_qmmse_reset(qform3->qmmse);
    }
    if(qform3->aspec)
    {
        wtk_aspec_reset(qform3->aspec);
    }
    qform3->aspec_mask = 1.0;
    qform3->spec_cnt = qform3->cfg->spec_cnt;

    memset(qform3->fout, 0, sizeof(float) * qform3->stft2->cfg->win);
    memset(qform3->pad, 0, sizeof(float) * qform3->stft2->cfg->win);
}

void wtk_qform3_feed_smsg(wtk_qform3_t *qform3, wtk_stft2_msg_t *msg, int pos,
                          int is_end) {
    if (qform3->admm) {
        wtk_admm2_feed_stftmsg(qform3->admm, msg, pos, is_end);
    } else {
        wtk_qform3_feed_smsg2(qform3, msg, pos, is_end);
    }
}

void wtk_qform3_start2(wtk_qform3_t *qform3, float theta, float phi) {
    wtk_qform3_start(qform3, theta, phi);
}
