#include "qtk_out_engine.h"

#define QTK_BFIO_MSG_BUFSIZE 49152

int ainlen=0;
int aoulen=0;

double qtk_time_get_ms()
{
    struct timeval tv;
    double ret;
    int err;

    err=gettimeofday(&tv,0);
    if(err==0)
    {
        ret=tv.tv_sec*1000.0+tv.tv_usec*1.0/1000;
        //ret maybe is NAN
        if(ret!=ret)
        {
            printf("NAN(%.0f,sec=%.d,usec=%.d).\n",ret,(int)tv.tv_sec,(int)tv.tv_usec);
            ret=0;
        }
    }else
    {
        perror(__FUNCTION__);
        ret=0;
    }
    return ret;
}

static void player_start(qtk_out_engine_t *oe)
{
    int ret=-1;
    if(oe->is_engine_new && (oe->is_engine_start==0))
    {
        wtk_strbuf_reset(oe->output);
        wtk_strbuf_reset(oe->input);
		qtk_engine_start(oe->engine);
		oe->is_engine_start=1;
    }

    if(1){
        oe->pf=fopen("./play.pcm","wb+");
        if(!oe->pf)
        {
            wtk_debug("pppppppppppppppppppppppffffffffffffffffffff open faild");
        }

        oe->sf=fopen("./mul.pcm","wb+");
        if(!oe->sf)
        {
                wtk_debug("sssssssssssssssssssssssffffffffffffffffffff open faild");
        }
        oe->echof=fopen("./echo.pcm","wb+");
        if(!oe->echof)
        {
                wtk_debug("eeeeeeeeeeeeeeeeeeeeeeeefffffffffffffffff open faild");
        }
        oe->outf=fopen("./out.pcm","wb+");
        if(!oe->outf)
        {
                wtk_debug("outf fffffffffffffffff open faild");
        }
        oe->out2_f=fopen("./out2.pcm","wb+");
        if(!oe->out2_f)
        {
                wtk_debug("out2_f fffffffffffffffff open faild");
        }
        oe->out3_f=fopen("./out3.pcm","wb+");
        if(!oe->out3_f)
        {
                wtk_debug("out2_f fffffffffffffffff open faild");
        }
    }


    oe->is_push_zero=0;
}

static void player_stop(qtk_out_engine_t *oe)
{
	int ret;
    wtk_debug("############################player stop\n");

    if(oe->is_engine_new && oe->is_engine_start)
    {
        oe->is_engine_start=0;
        qtk_engine_feed(oe->engine, NULL, 0, QTK_FEED_END);
        qtk_engine_reset(oe->engine);
        wtk_strbuf_reset(oe->output);
        wtk_strbuf_reset(oe->input);
    }

    // wtk_debug("pcm_close card:device(hw:%d:%d)\n", oe->qtk_outstream->dev->out_card, oe->qtk_outstream->dev->out_device);
    // if(oe->qtk_outstream->pcm)
    // {
    //     pcm_close(oe->qtk_outstream->pcm);
    //     oe->qtk_outstream->pcm = NULL;
    // }

    if(oe->sf)
    {
        fclose(oe->sf);
    }
    if(oe->echof)
    {
        fclose(oe->echof);
    }
    if(oe->outf)
    {
        fclose(oe->outf);
    }
    if(oe->out2_f)
    {
        fclose(oe->out2_f);
    }
    if(oe->out3_f)
    {
        fclose(oe->out3_f);
    }
    if(oe->pf)
    {
        fclose(oe->pf);
    }

    if(oe->outresample)
    {
        speex_resampler_destroy(oe->outresample);
        oe->outresample = NULL;
    }

    if(oe->outresample2)
    {
        speex_resampler_destroy(oe->outresample2);
        oe->outresample2 = NULL;
    }

    if(oe->is_resample)
    {
        wtk_debug("#################################>>>>resample reset.\n");
        oe->is_resample=0;
        wtk_resample_close(oe->sample);
    }
    oe->is_push_zero=0;
}



static void enine_on_errcode(qtk_session_t *session, int errcode,char *errstr)
{
    (void)session;
    (void)errcode;
    (void)errstr;
//     wtk_debug("====> errcode :%d errstr: %s",errcode,errstr);
}

void qtk_out_engine_notify_on_data(void *e,qtk_var_t *var)
{
    qtk_out_engine_t *oe=(qtk_out_engine_t *)e;

    int inlen=0;
    int outlen=inlen;
    char *ss=NULL;
    char *tmpdata=NULL;
    int nx=0,dlen=0;
    double tm;

    switch(var->type)
    {
    case QTK_SPEECH_DATA_PCM:
		// wtk_debug("===================>>>>>>>>>>>>>>>>>>>>>>>>>555555555555555555555555 var=%d\n",var->v.str.len);
        aoulen+=(var->v.str.len<<1);
		if(oe->echof)
		{
			fwrite(var->v.str.data, var->v.str.len, 1, oe->echof);
		}
		if(oe->is_resample)
		{
            tm = qtk_time_get_ms();
			wtk_resample_feed(oe->sample, var->v.str.data, var->v.str.len, 0);
            wtk_debug("====================>>>>>>>>>>>>>>>>>>>>>resample feed time=%f\n",qtk_time_get_ms() - tm);
		}else{
            if(oe->mode == 2)
            {
                // wtk_strbuf_reset(oe->output);
                // wtk_strbuf_push(oe->output, var->v.str.data, var->v.str.len);
                char *ss=var->v.str.data;
                int vlen=var->v.str.len;
                int nx=0;
                while(nx < vlen)
                {

                    wtk_strbuf_push(oe->output, ss + nx, 2);
                    wtk_strbuf_push(oe->output, ss + nx, 2);
                    nx+=2;
                }
            }
		}
		break;
    default:
        break;
	}
}

void qtk_out_reample_on_data(void *e, char *data, int len)
{
    qtk_out_engine_t *oe=(qtk_out_engine_t *)e;
    
    if(oe->mode == 2)
    {
        // wtk_strbuf_reset(oe->output);
        // wtk_strbuf_push(oe->output, data, len);
        // wtk_debug("###################################>>>>>>>>>>>>>resample on len=%d\n",len);
        char *ss=data;
        int vlen=len;
        int nx=0;

        while(nx < vlen)
        {
            wtk_strbuf_push(oe->output, ss + nx, 2);
            wtk_strbuf_push(oe->output, ss + nx, 2);
            nx+=2;
        }
        if(oe->out2_f)
        {
            fwrite(data, len, 1, oe->out2_f);
        }
    }
}

void qtk_out_engine_init(qtk_out_engine_t *oe)
{
	oe->session=NULL;
	oe->engine=NULL;
	oe->sample=NULL;
	oe->outresample=NULL;
	oe->outresamplebuf=NULL;
	oe->outrslen=20480;

	oe->player_type=QTK_ENGINE_PLAYER_START;
	oe->player_run=-1;
	oe->is_engine_new=0;
	oe->is_engine_start=0;
	oe->is_resample=0;
	oe->is_player_resample=1;
    oe->is_player_run=0;
    oe->is_engine_run = 1;
	oe->pchannel=1;
	oe->sample_rate=16000;
	oe->qtk_byte_written=0;
    oe->mode=0;
	oe->sf=NULL;
	oe->echof=NULL;
	oe->outf=NULL;
    oe->out2_f=NULL;
    oe->out3_f=NULL;
}


 void qtk_out_engine_init_func()
{

}

void qtk_out_engine_close_func()
{

}

 qtk_out_engine_t *qtk_out_engine_new()
{
    int ret=-1;
	qtk_out_engine_t *oe;

	oe=(qtk_out_engine_t *)malloc(sizeof(qtk_out_engine_t));

	qtk_out_engine_init(oe);

    const char *params = "appid=f4b7c86a-ef57-11e6-8aa7-00e04c12c2c7;secretkey=0063909a-ef58-11e6-8a33-00e04c12c2c7;"
                        "cache_path=./qvoice;log_wav=0;use_timer=1;use_cldhub=0;";
    const char *engine_params = "role=gainnetbf;cfg=res-gainnetbf/engine.cfg;syn=1;";//gainnetbf

    oe->session = qtk_session_init((char *)params,QTK_WARN,NULL,(qtk_errcode_handler)enine_on_errcode);
    if(!oe->session) {
        wtk_debug("session init failed.");
        goto end;
    }
    oe->engine = qtk_engine_new(oe->session, (char *)engine_params);
    if(!oe->engine)
    {
        wtk_debug("engine new faild.");
        goto end;
    }
    qtk_engine_set_notify(oe->engine, oe, (qtk_engine_notify_f)qtk_out_engine_notify_on_data);

    ret=wtk_blockqueue_init(&(oe->bfio_queue));
    if(ret!=0){goto end;}
    oe->sample = wtk_resample_new(64);
    wtk_resample_set_notify(oe->sample, oe, (wtk_resample_notify_f)qtk_out_reample_on_data);
    oe->outresamplebuf=(char *)malloc(oe->outrslen);
    oe->outresamplebuf2=(char *)malloc(oe->outrslen);
	oe->input = wtk_strbuf_new(1024, 1.0);
    oe->output = wtk_strbuf_new(1024, 1.0);
    oe->tmpbuf = wtk_strbuf_new(1024, 1.0);
    // ret=pthread_attr_init(&thread_attr);
    // if(ret!=0){goto end;}
    // pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
    // thread_param.sched_priority = sched_get_priority_max(SCHED_RR);
    // pthread_attr_setschedparam(&thread_attr, &thread_param);
    oe->mode=2;
    oe->hal_channel_mask=0;

    wtk_debug("engine new ok.\n");
    ret = 0;
    oe->is_engine_new = 1;
end:
    return oe;
}

void qtk_out_engine_delete(qtk_out_engine_t *oe)
{
    if(oe->is_engine_new)
    {

        if(oe->outresamplebuf)
        {
            free(oe->outresamplebuf);
        }
        if(oe->outresamplebuf2)
        {
            free(oe->outresamplebuf2);
        }
        if(oe->sample)
        {
            wtk_resample_delete(oe->sample);
        }
		if(oe->input)
		{
			wtk_strbuf_delete(oe->input);
		}
        if(oe->output)
        {
            wtk_strbuf_delete(oe->output);
        }
        if(oe->tmpbuf)
        {
            wtk_strbuf_delete(oe->tmpbuf);
        }
        if(oe->engine)
        {
            qtk_engine_reset(oe->engine);
            qtk_engine_delete(oe->engine);
        }
        if(oe->session)
        {
            qtk_session_exit(oe->session);
        }
    }
}

void qtk_out_engine_resample_init(qtk_out_engine_t *oe)
{
    if(oe->mode == 2)
    {
        oe->pchannel = 2;
        oe->sample_rate = 48000;
        oe->outresample = speex_resampler_init(oe->pchannel, oe->sample_rate, 16000, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
        if(oe->is_resample == 0 && (oe->sample_rate != 16000))
        {
            wtk_debug("=========================>>>>>>>>>>>>>>>>>%d\n",oe->sample_rate);
            wtk_resample_start(oe->sample, 16000, oe->sample_rate);
            oe->outresample2 = speex_resampler_init(oe->pchannel, 16000, oe->sample_rate, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
            oe->is_resample = 1;
        }
    }
}

void qtk_out_engine_start(qtk_out_engine_t *oe)
{
    if(oe->mode == 2)
    {
        player_start(oe);
    }
    wtk_debug("==================>>>>>>>>>>>>>qtk_out_engine_start\n");
}

void qtk_out_engine_reset(qtk_out_engine_t *oe)
{
    if(oe->mode == 2)
    {
        player_stop(oe);
    }
    wtk_debug("==================>>>>>>>>>>>>>qtk_out_engine_reset\n");
}

void qtk_out_engine_set(qtk_out_engine_t *oe, int is_engine)
{
    oe->is_engine_run = is_engine;
}

int qtk_out_engine_feedz(qtk_out_engine_t *oe, char *input, int len)
{
    wtk_strbuf_push(oe->output, input, len);
    return 0;
}

int qtk_out_engine_pop(qtk_out_engine_t *oe, int len)
{
    wtk_strbuf_pop(oe->output, NULL, len);
    return 0;
}

int qtk_out_engine_feed2(qtk_out_engine_t *oe, char *input, int len)
{
    int ilen=1536;
    if(oe->pf)
    {
        fwrite(input, len, 1, oe->pf);
    }
    if(oe->is_push_zero == 0)
    {
        char zdata[6144]={0};
        oe->is_push_zero=1;
        // wtk_debug("====================>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
            wtk_strbuf_push(oe->output, zdata, 6144);
    }

    wtk_strbuf_push(oe->input, input, len);
    ilen=oe->input->pos/192*192;
    // wtk_debug("==================>>>>>>>>>>>>>>>>>>>>mmmmmmmmmmmode=%d len=%d %d %d\n",oe->mode,len, oe->input->pos, ilen);
    if(oe->mode == 2 && oe->input->pos >= ilen)
    {
        int inlen=(ilen >> 1)/oe->pchannel;
        int outlen=inlen;
        char *ss=NULL;
        char *tmpdata=oe->input->data;
        int nx=0,dlen=0;
        // wtk_debug("==================>>>>>>>>>>>>>>>>>>>>>>>len=%d\n",len);

        if(oe->outresample)
        {
            if(ilen*(oe->sample_rate/16000.0) > oe->outrslen)
            {
                oe->outrslen = ilen*(oe->sample_rate/16000.0);
                free(oe->outresamplebuf);
                oe->outresamplebuf = (char *)malloc(oe->outrslen);
                memset(oe->outresamplebuf, 0, oe->outrslen);
            }else{
                memset(oe->outresamplebuf, 0, oe->outrslen);
            }
            speex_resampler_process_interleaved_int(oe->outresample,
                                                (spx_int16_t *)(tmpdata),(spx_uint32_t *)&inlen,
                                                (spx_int16_t *)(oe->outresamplebuf),(spx_uint32_t *)&outlen);
            ss=oe->outresamplebuf;
            dlen=(outlen<<1)*oe->pchannel;
            //wtk_debug("==================>>>>>>>>>>>>>>>>>>>>>>>%d/%d/%d",len,outlen,dlen);
        }else
        {
            ss=tmpdata;
            dlen=ilen;
        }
        
        // while(nx<dlen)
        // {
        //     wtk_strbuf_push(oe->input, ss+nx, 2);
        //     nx+=(2*oe->pchannel);
        // }
        ainlen+=dlen;
        wtk_debug("==================>>>>>>>>>>>>>>>input len=%d\n",dlen);
        if(dlen > 0)
        {
            if(oe->sf)//mul.pcm
            {
                fwrite(ss, dlen, 1, oe->sf);
            }
            double tm;
            tm = qtk_time_get_ms();
            // if(oe->is_push_zero == 0)
            // {
            //     char zdata[32*16]={0};
            //     oe->is_push_zero=1;
            //     // wtk_debug("====================>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
            //     qtk_engine_feed(oe->engine,zdata, 32*16, 0);
            // }
            qtk_engine_feed(oe->engine, ss, dlen, 0);
            wtk_debug("============>>>>>>>engine runing time %f\n",qtk_time_get_ms() - tm);
            // wtk_strbuf_reset(oe->input);
            wtk_strbuf_pop(oe->input, NULL, ilen);
            wtk_debug("===============>>>>>>>>>>>>>>>>>>>>input pos=%d output pos=%d   ainlen[%d] - aoulen[%d]=%d\n",oe->input->pos, oe->output->pos,ainlen,aoulen,ainlen-aoulen);
        }
        
    }
    return 0;
}

int qtk_out_engine_feed(qtk_out_engine_t *oe, char *data, int len)
{

    return 0;
}


