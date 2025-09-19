#include "wtk_consist.h"
#include "wtk/core/wtk_wavfile.h"

wtk_consist_msg_t* wtk_consist_msg_new();
void wtk_consist_msg_delete(wtk_consist_msg_t *m);
wtk_consist_msg_node_t* wtk_consist_msg_node_new(wtk_consist_msg_t *m);
void wtk_consist_msg_node_delete(wtk_consist_msg_node_t *msg);
wtk_consist_msg_node_t* wtk_consist_msg_pop_node(wtk_consist_msg_t *m);
void wtk_consist_msg_push_node(wtk_consist_msg_t *m,wtk_consist_msg_node_t *msg);

int wtk_consist_spk_run(wtk_consist_t *con, wtk_thread_t *thread);
int wtk_consist_mic_run(wtk_consist_t *con, wtk_thread_t *thread);

void wtk_consist_set_cpu(wtk_consist_t *con, int cpunum);
int wtk_consist_short_abs_max(short *pv,int len);
int wtk_consist_short_abs_rate(short *pv,int len);

wtk_consist_t* wtk_consist_new(wtk_consist_cfg_t *cfg)
{
    wtk_consist_t *con;
    con=(wtk_consist_t *)wtk_malloc(sizeof(wtk_consist_t));
    con->notify = NULL;
    con->ths = NULL;
    con->cfg=cfg;
    // con->ssl=wtk_ssl2_new(&(cfg->ssl));
    con->channel=con->cfg->channel;
    con->use_mic = 1;

    wtk_debug("channel=%d\n",con->channel);

    con->mic=wtk_strbufs_new(con->channel);
	con->sp=wtk_strbufs_new(cfg->spchannel);
    con->corr = (float *)wtk_malloc(sizeof(float) * 10);
    con->energy = (float *)wtk_malloc(sizeof(float) * 10);
    con->errchn = (int *)wtk_malloc(sizeof(int) *(con->cfg->channel + con->cfg->spchannel));
    con->errtype = (int **)wtk_malloc(sizeof(int*) *(con->cfg->channel + con->cfg->spchannel));
    int i;
    for(i=0;i<(con->cfg->channel+con->cfg->spchannel);++i)
    {
        con->errtype[i]=(int *)wtk_malloc(10*sizeof(int));
    }
    memset(con->errchn, 0, (con->cfg->channel + con->cfg->spchannel)*sizeof(int));
    wtk_sem_init(&(con->spk_sem), 0);
    wtk_sem_init(&(con->mic_sem), 0);

    con->msg = wtk_consist_msg_new();
    wtk_thread_init(&(con->spk_t), (thread_route_handler)wtk_consist_spk_run, (void *)con);
    wtk_blockqueue_init(&(con->spk_q));
    con->spk_run = 1;
    wtk_thread_start(&(con->spk_t));

    wtk_thread_init(&(con->mic_t), (thread_route_handler)wtk_consist_mic_run, (void *)con);
    wtk_blockqueue_init(&(con->mic_q));
    con->mic_run = 1;
    wtk_thread_start(&(con->mic_t));

    wtk_consist_reset(con);
    return con;
}

void wtk_consist_delete(wtk_consist_t *consist)
{
    consist->spk_run=0;
    wtk_blockqueue_wake(&(consist->spk_q));
    wtk_thread_join(&(consist->spk_t));
    wtk_blockqueue_clean(&(consist->spk_q));

    consist->mic_run=0;
    wtk_blockqueue_wake(&(consist->mic_q));
    wtk_thread_join(&(consist->mic_t));
    wtk_blockqueue_clean(&(consist->mic_q));
    
    if(consist->msg)
    {
        wtk_consist_msg_delete(consist->msg);
    }

    wtk_sem_clean(&(consist->spk_sem));
    wtk_sem_clean(&(consist->mic_sem));

    int i;
    for(i=0;i<(consist->cfg->channel+consist->cfg->spchannel);++i)
    {
        wtk_free(consist->errtype[i]);
    }
    wtk_free(consist->errtype);

    wtk_strbufs_delete(consist->mic,consist->channel);
    wtk_strbufs_delete(consist->sp, consist->cfg->spchannel);
    wtk_free(consist->corr);
    wtk_free(consist->energy);
    // wtk_ssl2_delete(consist->ssl);
    wtk_free(consist->errchn);
    wtk_free(consist);
}

void wtk_consist_reset(wtk_consist_t *consist)
{
    wtk_strbufs_reset(consist->mic,consist->channel);
    wtk_strbufs_reset(consist->sp, consist->cfg->spchannel);
    memset(consist->errchn, 0, (consist->cfg->channel + consist->cfg->spchannel)*sizeof(int));
    // wtk_ssl2_reset(consist->ssl);
    consist->consist=1;
    int i;
    for(i=0;i<(consist->cfg->channel+consist->cfg->spchannel);++i)
    {
        memset(consist->errtype[i], 0, 10*sizeof(int));
    }

}

void wtk_consist_set_notify(wtk_consist_t *consist, void *ths, wtk_consist_notify_f notify)
{
    consist->ths = ths;
    consist->notify = notify;
}

static float wtk_consist_cov(float *a,int len,float *aver)
{
    int i,tlen=0;
    float sum,cov,tmp;

    sum=0;
    for(i=0;i<len;++i)
    {
        if(i>0)
        {
            tmp=sum/(i-tlen);
            if(fabsf(a[i]-tmp) > 5000.0)
            {
                tlen++;
            }else{
                sum+=a[i];
            }
        }else{
            sum+=a[i];
        }
    }
    tmp=sum/(len-tlen);

    sum=0;
    tlen=0;
    for(i=0;i<len;++i)
    {
        if(fabsf(a[i]-tmp) > 1000.0)
        {
            tlen++;
        }else{
            sum+=a[i];
        }
    }
    sum/=(len-tlen);
    *aver=sum;

    cov=0;
    for(i=0;i<len;++i)
    {
        cov+=(a[i]-sum)*(a[i]-sum);
    }

    return sqrtf(cov);
}

static float wtk_consist_cov22(wtk_consist_t *con,float *a,int len,float *aver)
{
    int i,tlen=0;
    float sum,cov,tmp;

    sum=0;
    for(i=0;i<len;++i)
    {
        if(con->errchn[i])
        {
            tlen++;
            continue;
        }
        if(i>0)
        {
            tmp=sum/(i-tlen);
            if(fabsf(a[i]-tmp) > 5000.0)
            {
                tlen++;
            }else{
                sum+=a[i];
            }
        }else{
            sum+=a[i];
        }
    }
    if(len == tlen)
    {
        tlen=0;
    }
    tmp=sum/(len-tlen);

    sum=0;
    tlen=0;
    for(i=0;i<len;++i)
    {
        if(con->errchn[i])
        {
            tlen++;
            continue;
        }
        if(fabsf(a[i]-tmp) > 1000.0)
        {
            tlen++;
        }else{
            sum+=a[i];
        }
    }
    if(len == tlen)
    {
        tlen=0;
    }
    sum/=(len-tlen);
    *aver=sum;

    cov=0;
    for(i=0;i<len;++i)
    {
        cov+=(a[i]-sum)*(a[i]-sum);
    }

    return sqrtf(cov);
}

void wtk_consist_feed(wtk_consist_t *con,short *data,int len,int is_end)
{
	int i,j;
	short *pv[10];
	int channel=con->channel;
	wtk_strbuf_t **mic=con->mic;
	wtk_strbuf_t **sp=con->sp;
    float max[10];
    float energy[10];
    float corr[10];
    int b;
    float aver1,aver3,er1,er3;

	for(i=0;i<len;++i)
	{
		for(j=0;j<channel;++j)
		{
			// if(data[0]>32000)
			// {
			// 	data[0]=32000;
			// }else if(data[0]<-32000)
			// {
			// 	data[0]=-32000;
			// }
			wtk_strbuf_push(mic[j],(char *)data,2);
            ++data;
		}
		wtk_strbuf_push(sp[0],(char *)data,2);
        ++data;
        data+=3;
	}
    if(is_end)
    {

#ifdef DEBUG_WAV
	static wtk_wavfile_t *log=NULL;


    for(i=0;i<channel;++i)
    {
        pv[i]=(short *)(mic[i]->data);
    }
    pv[i]=(short *)(sp[0]->data);

	if(!log)
	{
		log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(log,con->channel+1);
		wtk_wavfile_open2(log,"con");
	}
	if(len>0)
	{
		wtk_wavfile_write_mc(log,pv,len);
	}
	if(is_end)
	{
		wtk_wavfile_close(log);
		wtk_wavfile_delete(log);
		log=NULL;
	}
#endif

        wtk_strbuf_t** buf;
        int nx;   
        buf=wtk_riff_read_channel(con->cfg->playfn,&nx);
        // wtk_debug("ess len %d sp len %d\n",buf[0]->pos,sp->pos);
        if(sp[0]->pos>buf[0]->pos)
        {
            wtk_strbuf_push(buf[0],NULL,sp[0]->pos-buf[0]->pos);
        }
        // wtk_debug("ess len %d sp len %d\n",buf[0]->pos,sp->pos);
        // nx=wtk_rfft_xcorr2(sp[0]->data,sp[0]->pos,buf[0]->data,buf[0]->pos);
        // // printf("%d\n",nx);
        // if(nx<0)
        // {
        //     wtk_debug("sp is not 'ess0-8k' wav\n");
        //     nx=0;
        // }
        // if(nx+24000>mic[0]->pos/2)
        // {
        //     nx=-24000;
        // }
        // for(i=0;i<channel;++i)
        // {
        //     pv[i]=(short *)(mic[i]->data)+nx+24000;
        // }
        // wtk_ssl2_feed(con->ssl,pv,min(5*16000,mic[0]->pos/2-(nx+24000)),1);
        // // wtk_ssl2_print(con->ssl);
        // b=0;
        // if(wtk_ssl2_has_theta(con->ssl,0,70) || wtk_ssl2_has_theta(con->ssl,180,70))
        // {
        //     b=1;
        // }
        // if(b==0)
        // {
        //     if(con->notify)
        //     {
        //         con->notify(con->ths, WTK_CONSIST_MICERR_ALIGN, 1);
        //     }
        //     printf("mic consist error\n");
        //     wtk_ssl2_print(con->ssl);
        // }
        // printf("THETA %d\n",b);

        for(i=0;i<channel;++i)
        {
            corr[i]=wtk_rfft_xcorr2(mic[i]->data,mic[i]->pos,buf[0]->data,buf[0]->pos);
            max[i]=wtk_short_abs_max((short *)(mic[i]->data),mic[i]->pos/2);
            energy[i]=wtk_short_energy((short *)(mic[i]->data),mic[i]->pos/2);
            printf("%f %f %f\n",corr[i],max[i],energy[i]);
            if(max[i]>=32767)
            {
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_MICERR_MAX, i+1);
                }
                printf("channel %d break sound\n",i+1);
            }
        }

        printf("%f %f %f\n",wtk_consist_cov(energy,2,&aver3),wtk_consist_cov(energy,3,&aver3),wtk_consist_cov(energy,4,&aver3));


        er1=wtk_consist_cov(corr,channel,&aver1);
        er3=wtk_consist_cov(energy,channel,&aver3);
        printf("%f  %f // %f  %f\n",er1,er3,aver1,aver3);
        if(er1>200  || er3>600)
        {
            b=0;

            {
                for(i=0;i<channel;++i)
                {
                    if(fabs(corr[i]-aver1)>180)
                    {
                        if(con->notify)
                        {
                            con->notify(con->ths, WTK_CONSIST_MICERR_CORR, i+1);
                        }
                        printf("channel %d corr error\n",i+1);
                    }
                }

                for(i=0;i<channel;++i)
                {
                    if(fabs(energy[i]-aver3)>500)
                    {
                        if(con->notify)
                        {
                            con->notify(con->ths, WTK_CONSIST_MICERR_ENERGY, i+1);
                        }
                        printf("channel %d energy error\n",i+1);
                    }
                }
            }
        }

        con->consist=b;

        wtk_strbufs_delete(buf,1);
    }
}

int wtk_consist_is_eq(wtk_consist_t *con)
{
    int i,tmpidx;
    float s1,s2;
#ifdef USE_303
    for(i=0;i<con->channel;i+=2)
    {
        s1 = wtk_short_abs_sum((short *)(con->mic[i]->data), con->mic[i]->pos >> 1);
        tmpidx = (con->channel>>1)+i;
        s2 = wtk_short_abs_sum((short *)(con->mic[i+1]->data),con->mic[i+1]->pos >> 1);
        wtk_debug("equal channel=[%d %d] s1=%f s2=%f s1-s2=%f\n",i , i+1, s1, s2,fabsf(s1-s2));
        if(fabsf(s1 - s2) < con->cfg->eq_offset)
        {
            if(con->notify)
            {
                con->notify(con->ths, WTK_CONSIST_MICERR_ALIGN, i+1);
                con->notify(con->ths, WTK_CONSIST_MICERR_ALIGN, i+1+1);
            }
        }
    }
#else
    for(i=0;i<(con->channel>>1);++i)
    {
        s1 = wtk_short_abs_sum((short *)(con->mic[i]->data), con->mic[i]->pos >> 1);
        tmpidx = (con->channel>>1)+i;
        s2 = wtk_short_abs_sum((short *)(con->mic[tmpidx]->data),con->mic[tmpidx]->pos >> 1);
        // wtk_debug("tmpidx=%d %d s1=%f s2=%f %f\n",tmpidx, i, s1, s2,fabsf(s1-s2));
        if(fabsf(s1 - s2) < con->cfg->eq_offset)
        {
            con->errtype[i][WTK_CONSIST_MICERR_ALIGN]=1;
            con->errtype[tmpidx][WTK_CONSIST_MICERR_ALIGN]=1;

            if(con->notify)
            {
                con->notify(con->ths, WTK_CONSIST_MICERR_ALIGN, i+1);
                con->notify(con->ths, WTK_CONSIST_MICERR_ALIGN, tmpidx+1);
            }
        }
    }
#endif
    return 0;
}

int wtk_consist_is_nil(wtk_consist_t *con, short *data, int len)
{
    int i;
    int count=0;
    for(i=0;i<len;++i)
    {
        if(data[i] == 0)
        {
            count++;
        }
    }
    if(count == len)
    {
        return 1;
    }
    return 0;
}

void wtk_consist_feed2(wtk_consist_t *con,short *data,int len,int is_end)
{
	int i,j,k;
	short *pv[10];
	int channel=con->channel;
	wtk_strbuf_t **mic=con->mic;
	wtk_strbuf_t **sp=con->sp;
    float corr[10];
    float energy[10];
    float max[10];
    int b,ret;
    float aver1=0.0f,aver3=0.0f,er1=0.0f,er3=0.0f;

	for(i=0;i<len;++i)
	{
		for(j=0;j<channel;++j)
		{
			wtk_strbuf_push(mic[j],(char *)data,2);
            ++data;
		}
        for(k=0;k<con->cfg->spchannel;++k)
        {
            wtk_strbuf_push(sp[k],(char *)data,2);
            ++data;
        }
	}
    if(is_end && mic[0]->pos == 0)
    {
        for(j=0;j<channel;++j)
		{
            if(con->notify)
            {
                printf("channel %d micerr nil\n",j+1);
                con->notify(con->ths, WTK_CONSIST_MICERR_NIL, j+1);
            }
      	}

        for(k=0;k<con->cfg->spchannel;++k)
        {
            printf("channel %d spkerr nil\n",k+1);
            if(con->notify)
            {
                con->notify(con->ths, WTK_CONSIST_SPKERR_NIL, k+1);
            }
        }
    }else if(is_end && mic[0]->pos > 0)
    {

#ifdef DEBUG_WAV
	static wtk_wavfile_t *log=NULL;


    for(i=0;i<channel;++i)
    {
        pv[i]=(short *)(mic[i]->data);
    }
    pv[i]=(short *)(sp[0]->data);

	if(!log)
	{
		log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(log,con->channel+1);
		wtk_wavfile_open2(log,"con");
	}
	if(len>0)
	{
		wtk_wavfile_write_mc(log,pv,len);
	}
	if(is_end)
	{
		wtk_wavfile_close(log);
		wtk_wavfile_delete(log);
		log=NULL;
	}
#endif

        if(con->cfg->use_equal)
        {
            wtk_consist_is_eq(con);
        }

		for(j=0;j<channel;++j)
		{
            ret = wtk_consist_is_nil(con, (short *)(mic[j]->data), mic[j]->pos>>1);
            if(ret)
            {
                con->errchn[j]=1;
                con->errtype[j][WTK_CONSIST_MICERR_NIL]=1;
                if(con->notify)
                {
                    printf("channel %d micerr nil\n",j+1);
                    con->notify(con->ths, WTK_CONSIST_MICERR_NIL, j+1);
                }
            }
      	}

        for(k=0;k<con->cfg->spchannel;++k)
        {
            ret = wtk_consist_is_nil(con, (short *)(sp[k]->data), sp[k]->pos>>1);
            if(ret)
            {
                con->errtype[channel+k][WTK_CONSIST_MICERR_NIL]=1;
                con->errchn[channel+k]=1;
                printf("channel %d spkerr nil\n",k+1);
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_SPKERR_NIL, k+1);
                }
            }
        }
        wtk_strbuf_t** buf;
        int nx;
        wtk_debug("ppppppppppppppppppplay fn=%s\n",con->cfg->playfn);
        buf=wtk_riff_read_channel(con->cfg->playfn,&nx);
        // wtk_debug("ess len %d sp len %d\n",buf[0]->pos,sp[0]->pos);
        if(sp[0]->pos>buf[0]->pos)
        {
            wtk_strbuf_push(buf[0],NULL,sp[0]->pos-buf[0]->pos);
        }
#if 0
        // wtk_debug("ess len %d sp len %d mic len %d\n",buf[0]->pos,sp[0]->pos,mic[0]->pos);
        for(i=0;i<con->cfg->spchannel;++i)
        {
            nx=wtk_rfft_xcorr2(sp[i]->data,sp[i]->pos,buf[0]->data,buf[0]->pos);
            // printf("%d\n",nx);
            if(nx<0)
            {
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_SPKERR_ALIGN, i+1);
                }
                // wtk_debug("sp is not 'ess0-8k' wav\n");
                nx=0;
            }
        }
        nx=wtk_rfft_xcorr2(mic[0]->data,mic[0]->pos,buf[0]->data,buf[0]->pos);
        // printf("%d\n",nx);
        if(nx<0)
        {
            // wtk_debug("mic is not 'ess0-8k' wav\n");
            nx=0;
        }
        if(nx+24000>mic[0]->pos/2)
        {
            nx=-24000;
        }
        for(i=0;i<channel;++i)
        {
            pv[i]=(short *)(mic[i]->data)+nx+24000;
        }
        wtk_ssl2_feed(con->ssl,pv,min(5*16000,mic[0]->pos/2-(nx+24000)),1);
        // wtk_ssl2_print(con->ssl);
        b=0;
        if(wtk_ssl2_has_theta(con->ssl,0,70) || wtk_ssl2_has_theta(con->ssl,180,70))
        {
            b=1;
        }
        if(b==0)
        {
            // if(con->notify)
            // {
            //     con->notify(con->ths, WTK_CONSIST_MICERR_ALIGN, 1);
            // }
            // printf("mic consist error\n");
            // wtk_ssl2_print(con->ssl);
        }
        // printf("THETA %d\n",b);
#endif
        int tc,tc2;
        tc=wtk_rfft_xcorr2(mic[0]->data,mic[0]->pos, mic[channel-1]->data, mic[channel-1]->pos);
        if(abs(tc) > 1)
        {
            tc=wtk_rfft_xcorr2(mic[0]->data,mic[0]->pos, mic[channel - 2]->data, mic[channel-2]->pos);
            tc2=wtk_rfft_xcorr2(mic[channel-1]->data,mic[channel-1]->pos, mic[1]->data, mic[1]->pos);
            if(abs(tc) > 1 || abs(tc2) > 1)
            {
                con->use_mic = 0;
            }
        }

        wtk_consist_msg_node_t *msg_node,*msg_node2;
        msg_node = wtk_consist_msg_pop_node(con->msg);
        wtk_strbuf_push(msg_node->buf, buf[0]->data, buf[0]->pos);
        wtk_blockqueue_push(&(con->spk_q), &(msg_node->qn));

        msg_node2 = wtk_consist_msg_pop_node(con->msg);
        wtk_strbuf_push(msg_node2->buf, buf[0]->data, buf[0]->pos);
        wtk_blockqueue_push(&(con->mic_q), &(msg_node2->qn));


        for(i=(channel>>1);i<channel;++i)
        {

            if(con->cfg->use_xcorr)
            {
                if(con->use_mic)
                {
                    corr[i]=wtk_rfft_xcorr2(mic[i]->data,mic[i]->pos, mic[0]->data, mic[0]->pos);
                }else{
                    corr[i]=wtk_rfft_xcorr2(mic[i]->data,mic[i]->pos, buf[0]->data, buf[0]->pos);
                }
            }
            max[i]=wtk_consist_short_abs_max((short *)(mic[i]->data),mic[i]->pos>>1);
            energy[i]=wtk_short_abs_mean((short *)(mic[i]->data),mic[i]->pos>>1);
            int rt = wtk_consist_short_abs_rate((short *)(mic[i]->data),mic[i]->pos>>1);
            // wtk_debug("rt=%d\n",rt);
            if(rt)
            {
                con->errchn[i]=1;
                con->errtype[i][WTK_CONSIST_MICERR_ENERGY]=1;
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_MICERR_ENERGY, i+1);
                }
                printf("==========>>>channel %d energy error\n",i+1);
            }
            printf("mic: channel=%d corr=%f max=%f energy=%f\n",i+1,corr[i],max[i],energy[i]);
            if(max[i]>=32767)
            {
                con->errtype[i][WTK_CONSIST_MICERR_MAX]=1;
                con->errchn[i]=1;
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_MICERR_MAX, i+1);
                }
                printf("channel %d break sound\n",i+1);
            }
            if(energy[i] < con->cfg->mic_energy_min)
            {
                con->errchn[i]=1;
                if(energy[i] < con->cfg->nil_er)
                {
	                printf("channel %d energy min >> nil_er=%f\n",i+1,energy[i]);
                    if(con->errtype[i][WTK_CONSIST_MICERR_NIL] == 0)
                    {
                        if(con->notify)
                        {
                            con->notify(con->ths, WTK_CONSIST_MICERR_NIL, i+1);
                        }
                    }
                }
            }
        }
        wtk_sem_acquire(&(con->mic_sem), -1);
        for(i=0;i<(channel>>1);++i)
        {
            if(con->cfg->use_xcorr)
            {
                corr[i]=con->corr[i];
            }
            energy[i]=con->energy[i];
        }

        // printf("%f %f %f\n",wtk_consist_cov(energy,2,&aver3),wtk_consist_cov(energy,3,&aver3),wtk_consist_cov(energy,4,&aver3));
        if(con->cfg->use_xcorr)
        {
            er1=wtk_consist_cov22(con, corr,channel,&aver1);
        }
        er3=wtk_consist_cov22(con, energy,channel,&aver3);
        printf("mic: corr_er=%f  energy_er=%f // corr_aver=%f  energy_aver=%f\n",er1,er3,aver1,aver3);
        if(er1>con->cfg->mic_corr_er || er3>con->cfg->mic_energy_er)
        {
            b=0;
            if(con->cfg->use_xcorr)
            {
                for(i=0;i<channel;++i)
                {
                    if(fabsf(corr[i]-aver1)>con->cfg->mic_corr_aver)
                    {
                        if(con->notify)
                        {
                            con->notify(con->ths, WTK_CONSIST_MICERR_CORR, i+1);
                        }
                        printf("channel %d corr error\n",i+1);
                    }
                }
            }

            for(i=0;i<channel;++i)
            {
                if(fabsf(energy[i]-aver3)>con->cfg->mic_energy_aver)
                {
                    if(con->notify)
                    {
                        con->notify(con->ths, WTK_CONSIST_MICERR_ENERGY, i+1);
                    }
                    printf("channel %d energy error\n",i+1);
                }
            }
        }

        con->consist=b;

        wtk_strbufs_delete(buf,1);
        wtk_sem_acquire(&(con->spk_sem), -1);
    }
}

int wtk_consist_spkconsist(wtk_consist_t *con, char *data, int len)
{
    int i;
	wtk_strbuf_t **sp=con->sp;
    float corr[10];
    float energy[10];
    float max[10];
    int b=1,ret=0;
    float aver1=0.0f,aver3=0.0f,er1=0.0f,er3=0.0f;

    for(i=0;i<con->cfg->spchannel;++i)
    {
        if(con->cfg->use_xcorr)
        {
            corr[i]=wtk_rfft_xcorr2(sp[i]->data,sp[i]->pos, data, len);
        }
        max[i]=wtk_consist_short_abs_max((short *)(sp[i]->data),sp[i]->pos>>1);
        energy[i]=wtk_short_abs_mean((short *)(sp[i]->data),sp[i]->pos>>1);
        printf("spk: channel=%d corr=%f max=%f energy=%f\n", i+1, corr[i], max[i], energy[i]);
        if(max[i]>=32767.0f)
        {
            con->errchn[con->cfg->channel+i]=1;
            if(con->notify)
            {
                con->notify(con->ths, WTK_CONSIST_SPKERR_MAX, i+1);
            }
            printf("spchannel %d break sound\n",i+1);
        }
        if(energy[i] < con->cfg->nil_er)
        {
            con->errchn[con->cfg->channel+i]=1;
            if(con->notify)
            {
                con->notify(con->ths, WTK_CONSIST_SPKERR_NIL, i+1);
            }
        }
    }

    // printf("%f %f %f\n",wtk_consist_cov(energy,2,&aver3),wtk_consist_cov(energy,3,&aver3),wtk_consist_cov(energy,4,&aver3));

    if(con->cfg->use_xcorr)
    {
        er1=wtk_consist_cov(corr,con->cfg->spchannel,&aver1);
    }
    er3=wtk_consist_cov(energy,con->cfg->spchannel,&aver3);
    printf("spk: corr_er=%f  energy_er=%f // corr_aver=%f  energy_aver=%f\n",er1,er3,aver1,aver3);
    if(er1>con->cfg->spk_corr_er  || er3>con->cfg->spk_energy_er)
    {
        b=0;
        if(con->cfg->use_xcorr)
        {
            for(i=0;i<con->cfg->spchannel;++i)
            {
                if(fabsf(corr[i]-aver1)>con->cfg->spk_corr_aver)
                {
                    if(con->notify)
                    {
                        con->notify(con->ths, WTK_CONSIST_SPKERR_CORR, i+1);
                    }
                    printf("spchannel %d corr error\n",i+1);
                }
            }
        }

        for(i=0;i<con->cfg->spchannel;++i)
        {
            if(fabsf(energy[i]-aver3)>con->cfg->spk_energy_aver)
            {
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_SPKERR_ENERGY, i+1);
                }
                printf("spchannel %d energy error\n",i+1);
            }
        }
    }
    con->consist = b;
    return ret;
}

int wtk_consist_spk_run(wtk_consist_t *con, wtk_thread_t *thread)
{
    wtk_consist_msg_node_t *msg_node;
    wtk_queue_node_t *qn;

    wtk_consist_set_cpu(con, 1);

    while(con->spk_run){
		qn= wtk_blockqueue_pop(&(con->spk_q),-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,wtk_consist_msg_node_t,qn);

        wtk_consist_spkconsist(con, msg_node->buf->data, msg_node->buf->pos);

		wtk_consist_msg_push_node(con->msg, msg_node);
        wtk_sem_release(&(con->spk_sem), 1);
	}

    return 0;
}

void wtk_consist_micconsist(wtk_consist_t *con, char *data, int len)
{
    int i;
	wtk_strbuf_t **mic=con->mic;
    int channel=con->channel;
    float max[10];
    int b=1,ret=0;

    for(i=0;i<(channel>>1);++i)
    {
        if(con->use_mic)
        {
            con->corr[i]=wtk_rfft_xcorr2(mic[i]->data,mic[i]->pos, mic[(channel>>1)]->data, mic[(channel>>1)]->pos);
        }else{
            con->corr[i]=wtk_rfft_xcorr2(mic[i]->data,mic[i]->pos, data, len);
        }
        max[i]=wtk_consist_short_abs_max((short *)(mic[i]->data),mic[i]->pos>>1);
        con->energy[i]=wtk_short_abs_mean((short *)(mic[i]->data),mic[i]->pos>>1);
        int rt = wtk_consist_short_abs_rate((short *)(mic[i]->data),mic[i]->pos>>1);
        // wtk_debug("rt=%d\n",rt);
        if(rt)
        {
            con->errchn[i]=1;
            if(con->notify)
            {
                con->notify(con->ths, WTK_CONSIST_MICERR_ENERGY, i+1);
            }
            printf("channel %d energy error\n",i+1);
        }
        printf("mic: channel=%d corr=%f max=%f energy=%f\n",i+1,con->corr[i],max[i],con->energy[i]);
        if(max[i]>=32767)
        {
            con->errchn[i]=1;
            if(con->notify)
            {
                con->notify(con->ths, WTK_CONSIST_MICERR_MAX, i+1);
            }
            printf("channel %d break sound\n",i+1);
        }
        if(con->energy[i] < con->cfg->mic_energy_min)
        {
            con->errchn[i]=1;
            if(con->energy[i] < con->cfg->nil_er)
            {
	            printf("channel %d energy min >> nil_er=%f\n",i+1,con->energy[i]);
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_MICERR_NIL, i+1);
                }
            }
        }
    }
    wtk_sem_release(&(con->mic_sem), 1);

}

int wtk_consist_mic_run(wtk_consist_t *con, wtk_thread_t *thread)
{
    wtk_consist_msg_node_t *msg_node;
    wtk_queue_node_t *qn;

    wtk_consist_set_cpu(con, 0);

    while(con->mic_run){
		qn= wtk_blockqueue_pop(&(con->mic_q),-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,wtk_consist_msg_node_t,qn);

        wtk_consist_micconsist(con, msg_node->buf->data, msg_node->buf->pos);

		wtk_consist_msg_push_node(con->msg, msg_node);
	}

    return 0;
}

wtk_consist_msg_t* wtk_consist_msg_new()
{
	wtk_consist_msg_t *m;

	m=wtk_malloc(sizeof(*m));
	wtk_lockhoard_init(&(m->msg_hoard),offsetof(wtk_consist_msg_node_t,hoard_on),30,
		(wtk_new_handler_t)wtk_consist_msg_node_new,
		(wtk_delete_handler_t)wtk_consist_msg_node_delete, 
		m);

	return m;
}

void wtk_consist_msg_delete(wtk_consist_msg_t *m)
{
	wtk_lockhoard_clean(&(m->msg_hoard));
	wtk_free(m);
}

wtk_consist_msg_node_t* wtk_consist_msg_node_new(wtk_consist_msg_t *m)
{
	wtk_consist_msg_node_t *msg;

	msg=(wtk_consist_msg_node_t*)wtk_malloc(sizeof(wtk_consist_msg_node_t));
	msg->buf=wtk_strbuf_new(6400,1.0);
	msg->type = -1;


	return msg;
}

void wtk_consist_msg_node_delete(wtk_consist_msg_node_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
}

wtk_consist_msg_node_t* wtk_consist_msg_pop_node(wtk_consist_msg_t *m)
{
	return  (wtk_consist_msg_node_t*)wtk_lockhoard_pop(&(m->msg_hoard));
}

void wtk_consist_msg_push_node(wtk_consist_msg_t *m,wtk_consist_msg_node_t *msg)
{
	msg->type = -1;
	wtk_strbuf_reset(msg->buf);
	wtk_lockhoard_push(&(m->msg_hoard),msg);
}

int wtk_consist_short_abs_max(short *pv,int len)
{
	int max=abs(pv[0]);
	int i;
	int m;
    int count=0;

	for(i=1;i<len;++i)
	{
		m=abs(pv[i]);
		if(m>=max)
		{
            max=m;
            if(max >= 32767)
            {
			    count++;
            }
		}
	}

    if(count >20)
    {
        return max;
    }else{
        if(max >= 32767)
        {
            return 32766;
        }else{
            return max;
        }
    }
}

int wtk_consist_short_abs_rate(short *pv,int len)
{
	int i;
	int m1,m2,m3,m4;
    float f1,f2,f3,f4;
    int count=0;
    int mcount=0;
    float ff=0.8f;
    // long ccount=0;
    // float max=0.0f;

	for(i=2;i<len-2;++i)
	{
        // ccount++;
        if(abs(pv[i]) >= 16384)
        {
            m1=abs(abs(pv[i]) - abs(pv[i-2]));
            m2=abs(abs(pv[i]) - abs(pv[i-1]));
            m3=abs(abs(pv[i]) - abs(pv[i+1]));
            m4=abs(abs(pv[i]) - abs(pv[i+2]));
            f1 = (m1*1.0f/abs(pv[i]));
            f2 = (m2*1.0f/abs(pv[i]));
            f3 = (m3*1.0f/abs(pv[i]));
            f4 = (m4*1.0f/abs(pv[i]));
            // if(f1 > max)
            // {
            //     max = f1;
            // }
            // if(f2 > max)
            // {
            //     max = f2;
            // }

            // if(f3 > max)
            // {
            //     max = f3;
            // }
            // if(f4 > max)
            // {
            //     max = f4;
            // }
            // printf("%f ==>%f %f %f %f [%d %d %d %d] %d %d %d %d %d\n",ccount/16.0 ,m1*1.0f/abs(pv[i]), m2*1.0f/abs(pv[i]), m3*1.0f/abs(pv[i]), m4*1.0f/abs(pv[i]),
            //      m1, m2, m3, m4, pv[i-2],pv[i-1], pv[i], pv[i+1], pv[i+2]);
            // if((m1*1.0f/abs(pv[i]))>=0.85f && (m2*1.0f/abs(pv[i])) >= 0.85f && (m3*1.0f/abs(pv[i])) >= 0.85f && (m4*1.0f/abs(pv[i])) >= 0.85f)
            // // if((m2*1.0f/abs(pv[i])) >= 0.9f && (m3*1.0f/abs(pv[i])) >= 0.9f)
            // {
            //     count++;
            // }
            mcount=0;
            if((m1*1.0f/abs(pv[i]))>=ff)
            {
                mcount++;
            }
            if((m2*1.0f/abs(pv[i]))>=ff)
            {
                mcount++;
            }
            if((m3*1.0f/abs(pv[i]))>=ff)
            {
                mcount++;
            }
            if((m4*1.0f/abs(pv[i]))>=ff)
            {
                mcount++;
            }
            if(mcount >=3)
            {
                count++;
            }
        }
	}
    // wtk_debug("==============================>>>max ==> %f %d  %d\n",max, count, ++channelccc);

    if(count >= 3)
    {
        return 1;
    }else{
        return 0;
    }
}

void wtk_consist_set_cpu(wtk_consist_t *con, int cpunum)
{
#ifdef USE_R328
	cpu_set_t cpuset;
	int ret;
	int num=0;

	num = sysconf(_SC_NPROCESSORS_CONF);
	CPU_ZERO(&cpuset);
	// __CPU_ZERO_S(0, &cpuset);
	CPU_SET(cpunum, &cpuset);
	ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
	{
        wtk_debug("pthread_setaffinity_np error %d!\n",ret);
	}
	ret = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
	{
        wtk_debug("pthread_getaffinity_np error %d!\n",ret);
	}

    printf("Set returned by pthread_getaffinity_np() contained:\n");
	int j;
    for (j = 0; j < 2; j++)
	{
        if (CPU_ISSET(j, &cpuset))
		{
            printf("    CPU %d\n", j);
		}
	}
#endif
}

void wtk_consist_feed3(wtk_consist_t *con,short *data,int len,int is_end)
{
	int i,j,k;
	short *pv[10];
	int channel=con->channel;
	wtk_strbuf_t **mic=con->mic;
	wtk_strbuf_t **sp=con->sp;
    float corr[10];
    float energy[10];
    float max[10];
    int b,ret;
    float aver1=0.0f,aver3=0.0f,er1=0.0f,er3=0.0f;

	for(i=0;i<len;++i)
	{
		for(j=0;j<channel;++j)
		{
			wtk_strbuf_push(mic[j],(char *)data,2);
            ++data;
		}
        for(k=0;k<con->cfg->spchannel;++k)
        {
            wtk_strbuf_push(sp[k],(char *)data,2);
            ++data;
        }
	}
    if(is_end)
    {
        wtk_consist_is_eq(con);

		for(j=0;j<channel;++j)
		{
            ret = wtk_consist_is_nil(con, (short *)(mic[j]->data), mic[j]->pos>>1);
            if(ret)
            {
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_MICERR_NIL, j+1);
                }
            }
      	}

        for(k=0;k<con->cfg->spchannel;++k)
        {
            ret = wtk_consist_is_nil(con, (short *)(sp[k]->data), sp[k]->pos>>1);
            if(ret)
            {
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_SPKERR_NIL, k+1);
                }
            }
        }

        for(i=0;i<channel;++i)
        {
            if(con->cfg->use_xcorr)
            {
                if(i!=(channel-1))
                {
                    corr[i]=wtk_rfft_xcorr2(mic[i]->data,mic[i]->pos,mic[i+1]->data,mic[i+1]->pos);
                }else{
                    corr[i]=wtk_rfft_xcorr2(mic[i]->data,mic[i]->pos,mic[channel>>1]->data,mic[channel >>1]->pos);
                }
            }
            max[i]=wtk_consist_short_abs_max((short *)(mic[i]->data),mic[i]->pos>>1);
            energy[i]=wtk_short_abs_mean((short *)(mic[i]->data),mic[i]->pos>>1);
            int rt = wtk_consist_short_abs_rate((short *)(mic[i]->data),mic[i]->pos>>1);
            // wtk_debug("rt=%d\n",rt);
            if(rt)
            {
                    if(con->notify)
                    {
                        con->notify(con->ths, WTK_CONSIST_MICERR_ENERGY, i+1);
                    }
                    printf("channel %d energy error\n",i+1);
            }
            wtk_debug("[%d] %f %f %f\n",i,corr[i],max[i],energy[i]);
            if(max[i]>=32767)
            {
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_MICERR_MAX, i+1);
                }
                printf("channel %d break sound\n",i+1);
                return;
            }
            if(energy[i] < 100.0f)
            {
                if(con->notify)
                {
                    con->notify(con->ths, WTK_CONSIST_MICERR_NIL, i+1);
                }
            }
        }

        // printf("%f %f %f\n",wtk_consist_cov(energy,2,&aver3),wtk_consist_cov(energy,3,&aver3),wtk_consist_cov(energy,4,&aver3));
        if(con->cfg->use_xcorr)
        {
            er1=wtk_consist_cov(corr,channel,&aver1);
        }
        er3=wtk_consist_cov(energy,channel,&aver3);
        wtk_debug("%f  %f // %f  %f\n",er1,er3,aver1,aver3);
        if(er1>200.0f || er3>600.0f)
        {
            b=0;
            if(con->cfg->use_xcorr)
            {
                for(i=0;i<channel;++i)
                {
                    if(fabsf(corr[i]-aver1)>180.0f)
                    {
                        if(con->notify)
                        {
                            con->notify(con->ths, WTK_CONSIST_MICERR_CORR, i+1);
                        }
                        printf("channel %d corr error\n",i+1);
                    }
                }
            }

            for(i=0;i<channel;++i)
            {
                if(fabsf(energy[i]-aver3)>500.0f)
                {
                    if(con->notify)
                    {
                        con->notify(con->ths, WTK_CONSIST_MICERR_ENERGY, i+1);
                    }
                    printf("channel %d energy error\n",i+1);
                }
            }
        }

        con->consist=b;
    }
}
