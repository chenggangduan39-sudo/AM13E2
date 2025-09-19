#include "qtk_econsist.h" 
#include "sdk/engine/comm/qtk_engine_hdr.h"

void qtk_econsist_init(qtk_econsist_t *e)
{
	qtk_engine_param_init(&e->param);

	e->session = NULL;

	e->cfg = NULL;
	e->cs   = NULL;

	e->notify     = NULL;
	e->notify_ths = NULL;

	e->thread   = NULL;
	e->callback = NULL;

	e->swav = NULL;

}
FILE *ss;
FILE *infile[20];

int qtk_econsist_on_start(qtk_econsist_t *e);
int qtk_econsist_on_feed(qtk_econsist_t *e,char *data,int len);
int qtk_econsist_on_end(qtk_econsist_t *e);
void qtk_econsist_on_reset(qtk_econsist_t *e);
void qtk_econsist_on_set_notify(qtk_econsist_t *e,void *notify_ths,qtk_engine_notify_f notify_f);
int qtk_econsist_on_set(qtk_econsist_t *e,char *data,int bytes);
void qtk_econsist_on_msg(qtk_econsist_t *bfio, wtk_consist_micerror_type_t errtype, int channel);

qtk_econsist_t* qtk_econsist_new(qtk_session_t *session,wtk_local_cfg_t *params)
{
	qtk_econsist_t *e;
	int buf_size;
	int ret;
	int i;

	e=(qtk_econsist_t*)wtk_malloc(sizeof(qtk_econsist_t));
	qtk_econsist_init(e);
	e->session = session;

	qtk_engine_param_set_session(&e->param,e->session);
	ret = qtk_engine_param_feed(&e->param,params);
	if(ret != 0) {
		wtk_log_warn0(e->session->log,"params als failed.");
		goto end;
	}

	if(e->param.use_bin) {
		e->cfg = wtk_consist_cfg_new_bin(e->param.cfg);
	} else {
		e->cfg = wtk_consist_cfg_new(e->param.cfg);
	}
	if(!e->cfg) {
		wtk_log_warn0(e->session->log,"cfg new failed.");
		_qtk_error(e->session,_QTK_CFG_NEW_FAILED);
		ret = -1;
		goto end;
	}
	if(e->param.playfn)
	{
		e->cfg->playfn = e->param.playfn;
	}
	e->cfg->eq_offset=e->param.eq_offset;
	e->cfg->nil_er=e->param.nil_er;
	e->cfg->mic_corr_er=e->param.mic_corr_er;
	e->cfg->mic_corr_aver=e->param.mic_corr_aver;
	e->cfg->mic_energy_er=e->param.mic_energy_er;
	e->cfg->mic_energy_aver=e->param.mic_energy_aver;
	e->cfg->spk_corr_er=e->param.spk_corr_er;
	e->cfg->spk_corr_aver=e->param.spk_corr_aver;
	e->cfg->spk_energy_er=e->param.spk_energy_er;
	e->cfg->spk_energy_aver=e->param.spk_energy_aver;
	e->cfg->use_xcorr=e->param.use_xcorr;
	e->cfg->use_equal=e->param.use_equal;
	wtk_debug("##########################>>>>>>>>>>>.1\n");
	printf("##########################>>>>>>>>>>>.1\n");
	e->cs =wtk_consist_new(e->cfg);
	if(!e->cs){
		wtk_log_err0(e->session->log, "bfio new failed.");
		ret = -1;
		goto end;
	}
	wtk_consist_set_notify(e->cs, e, (wtk_consist_notify_f)qtk_econsist_on_msg);
	e->channel =  e->cs->channel + e->cs->cfg->spchannel;
	wtk_debug(">>>>channel = %d\n", e->channel);
	e->buffer = (short **)wtk_malloc(sizeof(short *)*e->channel);
	for(i = 0; i < e->channel; i++){
		e->buffer[i] = (short *)wtk_malloc(sizeof(short *)*QTK_BFIO_FEED_STEP*e->channel);
	}

	if(e->param.use_rearrange)
	{
		e->tmpbuf = wtk_strbuf_new(10240, 1.0f);
		e->rbuf = wtk_strbuf_new(10240, 1.0f);
		e->is_pcm_start = 1;
	}

	if(e->param.use_thread) {
		buf_size = (e->channel) * 2 * 16 * e->param.winStep;

		e->callback = qtk_engine_callback_new();
		e->callback->start_f      = (qtk_engine_thread_start_f)      qtk_econsist_on_start;
		e->callback->data_f       = (qtk_engine_thread_data_f)       qtk_econsist_on_feed;
		e->callback->end_f        = (qtk_engine_thread_end_f)        qtk_econsist_on_end;
		e->callback->reset_f      = (qtk_engine_thread_reset_f)      qtk_econsist_on_reset;
		e->callback->set_notify_f = (qtk_engine_thread_set_notify_f) qtk_econsist_on_set_notify;
		e->callback->set_f        = (qtk_engine_thread_set_f)        qtk_econsist_on_set;
		e->callback->ths          = e;

		e->thread = qtk_engine_thread_new(
				e->callback,
				e->session->log,
				"econsist",
				buf_size,
				20,
				1,
				e->param.syn
				);
	}

	ret = 0;
end:
	wtk_log_log(e->session->log,"ret = %d",ret);
	if(ret != 0) {
		qtk_econsist_delete(e);
		e = NULL;
	}
	return e;
}

int qtk_econsist_delete(qtk_econsist_t *e)
{
	wtk_debug("qtk_econsist_delete=================>>>>>>>>>>>>>>>>>>>>>%d\n",__LINE__);
	if(e->thread) {
		qtk_engine_thread_delete(e->thread,1);
	}
	if(e->callback) {
		qtk_engine_callback_delete(e->callback);
	}
	if(e->param.use_rearrange)
	{
		wtk_strbuf_delete(e->tmpbuf);
		wtk_strbuf_delete(e->rbuf);
	}
	if(e->cs)
	{
		wtk_consist_delete(e->cs);
		e->cs = NULL;
	}
	int i;
	for(i = 0; i < e->channel; i++){
		wtk_free(e->buffer[i]);
	}
	wtk_free(e->buffer);

	if(e->cfg)
	{
		if(e->param.use_bin)
		{
			wtk_consist_cfg_delete_bin(e->cfg);
		}else{
			wtk_consist_cfg_delete(e->cfg);
		}
		e->cfg = NULL;
	}
	qtk_engine_param_clean(&e->param);
	wtk_debug("qtk_econsist_delete=================>>>>>>>>>>>>>>>>>>>>>%d\n",__LINE__);

	wtk_free(e);
	return 0;
}


int qtk_econsist_on_start(qtk_econsist_t *e)
{
	if(e->param.use_logwav && e->param.consist_fn)
	{
		e->swav = wtk_wavfile_new(16000);
		wtk_wavfile_open(e->swav,e->param.consist_fn);
		wtk_wavfile_set_channel(e->swav, e->cfg->channel+e->cfg->spchannel);
	}
	// wtk_consist_start(e->cs);
	return 0;
}

void qtk_econsist_buffer_rearrangement(qtk_econsist_t *e,wtk_strbuf_t *buf,char *data, int len)
{
    int dlen,i,j;
	int channel=e->channel;
    
    wtk_strbuf_push(e->tmpbuf, data, len);
    dlen = e->tmpbuf->pos/(channel*sizeof(uint32_t));
    for(i=0;i<dlen;++i)
    {
		for(j=0;j<channel;++j)
		{
        	wtk_strbuf_push(buf, e->tmpbuf->data+(i*channel*sizeof(uint32_t))+(sizeof(uint32_t)*j+2), 2);
			if(e->param.use_inputpcm && infile[j])
			{
				fwrite(e->tmpbuf->data+(i*channel*sizeof(uint32_t))+(sizeof(uint32_t)*j+2), 2, 1, infile[j]);
			}
		}
		// wtk_strbuf_push(buf, e->tmpbuf->data+(i*6*sizeof(uint32_t))+2, 2);
        // wtk_strbuf_push(buf, e->tmpbuf->data+(i*6*sizeof(uint32_t))+6, 2);
        // wtk_strbuf_push(buf, e->tmpbuf->data+(i*6*sizeof(uint32_t))+10, 2);
        // wtk_strbuf_push(buf, e->tmpbuf->data+(i*6*sizeof(uint32_t))+14, 2);
        // wtk_strbuf_push(buf, e->tmpbuf->data+(i*6*sizeof(uint32_t))+18, 2);
        // wtk_strbuf_push(buf, e->tmpbuf->data+(i*6*sizeof(uint32_t))+22, 2);
    }
    wtk_strbuf_pop(e->tmpbuf, NULL, i*channel*sizeof(uint32_t));
}

void qtk_econsist_data_to_buffer(qtk_econsist_t *e,wtk_strbuf_t *buf,char *data, int len)
{
	int channel=e->channel;
    int dlen=len/(channel*sizeof(uint32_t));
    int i,j;

    wtk_strbuf_reset(buf);
    if(e->is_pcm_start)
    {
        char *tmpdata=data;
        char tc;
        int co=0;
        int start_len=0;
        for(i=0;i<len;++i)
        {
            for(j=0;j<channel;++j)
            {
                tc=tmpdata[(j*sizeof(uint32_t))];
                if(tc == (j+1))
                {
                    co++;
                }
            }
            if(co==channel)
            {
				//wtk_debug("===============>>>>>>>>>>>>start_len=%d\n",start_len);
                e->is_pcm_start=0;
                break;
            }
            co=0;
            tmpdata+=1;
            start_len++;
        }
        qtk_econsist_buffer_rearrangement(e, buf,data+start_len, len-start_len);
    }else{
        qtk_econsist_buffer_rearrangement(e, buf, data, len);
    }
}

int qtk_econsist_on_feed(qtk_econsist_t *e,char *data,int bytes)
{
#if 0
	int i, j;
	short *pv = NULL;
	int len;

	if(bytes > 0){
		pv = (short *)data;
		len = bytes/(e->channel * sizeof(short));
		for(i = 0; i < len; ++i){
			for(j = 0; j < e->channel; ++j){
				e->buffer[j][i] = pv[i * e->channel + j];
			}
		}
		// wtk_debug(">>>>len %d\n", len);
		wtk_consist_feed(e->cs, e->buffer, len, 0);
	}
#else
	
	if(e->param.use_rearrange)
	{
		qtk_econsist_data_to_buffer(e, e->rbuf, data, bytes);
		if(e->param.use_inputpcm)
		{
			if(ss)
			{
				fwrite(e->rbuf->data, e->rbuf->pos, 1, ss);
				fflush(ss);
			}
		}
		if(e->param.use_logwav)
		{
			if(e->swav)
			{
				wtk_wavfile_write(e->swav, e->rbuf->data, e->rbuf->pos);
			}
		}
		wtk_consist_feed2(e->cs, (short *)(e->rbuf->data), (e->rbuf->pos>>1)/e->channel, 0);
	}else{
		if(e->param.use_inputpcm)
		{
			int nx=0,i;
			char *st=data;
			while(nx<bytes)
			{
				if((nx+(2*e->channel)) > bytes)
				{
					break;
				}
				for(i=0;i<e->channel;++i)
				{
					if(infile[i])
					{
						fwrite(st+nx, 2, 1, infile[i]);
						fflush(infile[i]);
					}
					nx+=2;
				}
			}
			if(ss)
			{
				fwrite(data, bytes, 1, ss);
				fflush(ss);
			}
		}
		if(e->param.use_logwav)
		{
			
			if(e->swav)
			{
				wtk_wavfile_write(e->swav, data, bytes);
			}
		}
		wtk_consist_feed2(e->cs, (short *)data, (bytes>>1)/e->channel, 0);
	}

#endif

	return 0;
}

int qtk_econsist_on_end(qtk_econsist_t *e)
{
	wtk_debug("qtk_econsist_on_end=================>>>>>>>>>>>>>>>>>>>>>0\n");
	wtk_consist_feed2(e->cs, NULL, 0, 1);
	wtk_debug("qtk_econsist_on_end=================>>>>>>>>>>>>>>>>>>>>>1\n");
	e->feedend = 1;
	return 0;
}

void qtk_econsist_on_reset(qtk_econsist_t *e)
{
	if(e->feedend == 0){
		qtk_econsist_on_end(e);
	}
	if(e->param.use_logwav)
	{
		if(e->swav)
		{
			wtk_wavfile_delete(e->swav);
		}
	}
	wtk_consist_reset(e->cs);
}

void qtk_econsist_on_set_notify(qtk_econsist_t *e,void *notify_ths,qtk_engine_notify_f notify_f)
{
	e->notify_ths = notify_ths;
	e->notify     = notify_f;
}

int qtk_econsist_on_set(qtk_econsist_t *e,char *data,int bytes)
{
	return 0;
}

int qtk_econsist_start(qtk_econsist_t *e)
{
	int ret;

	if(e->param.use_inputpcm)
	{
		char fname[32]={0};
		snprintf(fname,32,"%s/input.pcm",e->param.inputpcm_fn);
		ss=fopen(fname,"wb+");
		if(!ss)
		{
			wtk_debug("=====================================+>>>>>>>>>>>>>input.pcm open faild\n");
		}
		int i;
		for(i=0;i<e->channel;++i)
		{
			snprintf(fname,32,"%s/input-%d.pcm",e->param.inputpcm_fn,i+1);
			infile[i]=fopen(fname,"wb+");
			if(!infile[i])
			{
				wtk_debug("=====================================+>>>>>>>>>>>>>input-x.pcm open faild\n");
			}
		}
	}

	if(e->param.use_thread){
		qtk_engine_thread_start(e->thread);
	}else{
		qtk_econsist_on_start(e);
	}
	ret = 0;
	return ret;
}

int qtk_econsist_feed(qtk_econsist_t *e,char *data,int bytes,int is_end)
{
	if(e->param.use_thread)
	{
		qtk_engine_thread_feed(e->thread,data,bytes,is_end);
	}else
	{
		if(bytes > 0) {
			qtk_econsist_on_feed(e,data,bytes);
		}
		if(is_end) {
			qtk_econsist_on_end(e);
		}
	}
	return 0;
}

int qtk_econsist_reset(qtk_econsist_t *e)
{
	if(e->param.use_thread){
		qtk_engine_thread_reset(e->thread);
	}else{
		qtk_econsist_on_reset(e);
	}

	if(e->param.use_inputpcm)
	{
		if(ss)
		{
			fclose(ss);
			ss=NULL;
		}
		int i;
		for(i=0;i<e->channel;++i)
		{
			if(infile[i])
			{
				fclose(infile[i]);
			}
			infile[i]=NULL;
		}
	}
	return 0;
}

int qtk_econsist_cancel(qtk_econsist_t *e)
{
	if(e->param.use_thread) {
		qtk_engine_thread_cancel(e->thread);
	}
	return 0;
}

void qtk_econsist_set_notify(qtk_econsist_t *e,void *ths,qtk_engine_notify_f notify_f)
{
	if(e->param.use_thread) {
		qtk_engine_thread_set_notify(e->thread,ths,notify_f);
	} else {
		qtk_econsist_on_set_notify(e,ths,notify_f);
	}
}

int qtk_econsist_set(qtk_econsist_t *e,char *data,int bytes)
{
	int ret;

	if(e->param.use_thread) {
		qtk_engine_thread_set(e->thread,data,bytes);
	} else {
		ret = qtk_econsist_on_set(e,data,bytes);
	}
	return ret;
}

void qtk_econsist_on_msg(qtk_econsist_t *bfio, wtk_consist_micerror_type_t errtype, int channel)
{
	qtk_var_t var;

	switch (errtype)
	{
	case WTK_CONSIST_MICERR_NIL:
		wtk_debug("=========WTK_CONSIST_MICERR_NIL==================+>>>>%d\n",channel);
		var.type=QTK_CONSIST_MICERR_NIL;
		break;
	case WTK_CONSIST_MICERR_ALIGN:
		var.type=QTK_CONSIST_MICERR_ALIGN;
		wtk_debug("=========WTK_CONSIST_MICERR_ALIGN==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_MAX:
		var.type=QTK_CONSIST_MICERR_MAX;
		wtk_debug("=========WTK_CONSIST_MICERR_MAX==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_CORR:
		var.type=QTK_CONSIST_MICERR_CORR;
		wtk_debug("=========WTK_CONSIST_MICERR_CORR==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_ENERGY:
		var.type=QTK_CONSIST_MICERR_ENERGY;
		wtk_debug("=========WTK_CONSIST_MICERR_ENERGY==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_NIL:
		var.type=QTK_CONSIST_SPKERR_NIL;
		wtk_debug("=========WTK_CONSIST_SPKERR_NIL==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_ALIGN:
		var.type=QTK_CONSIST_SPKERR_ALIGN;
		wtk_debug("=========WTK_CONSIST_SPKERR_ALIGN==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_MAX:
		var.type=QTK_CONSIST_SPKERR_MAX;
		wtk_debug("=========WTK_CONSIST_SPKERR_MAX==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_CORR:
		var.type=QTK_CONSIST_SPKERR_CORR;
		wtk_debug("=========WTK_CONSIST_SPKERR_CORR==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_ENERGY:
		var.type=QTK_CONSIST_SPKERR_ENERGY;
		wtk_debug("=========WTK_CONSIST_SPKERR_ENERGY==================+>>>>%d\n",channel);
		break;
	default:
		break;
	}
	var.v.i = channel;
	if(bfio->notify){
		bfio->notify(bfio->notify_ths, &var);
	}
}
