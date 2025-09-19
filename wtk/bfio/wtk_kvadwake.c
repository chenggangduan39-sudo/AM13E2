#include "wtk_kvadwake.h"
void wtk_kvadwake_on_vad2(wtk_kvadwake_t *vwake,wtk_vad2_cmd_t cmd,short *data,int len);
void wtk_kvadwake_img_notify(wtk_kvadwake_t *vwake, int res, int start, int end);

wtk_kvadwake_t *wtk_kvadwake_new(wtk_kvadwake_cfg_t *cfg)
{
    wtk_kvadwake_t *vwake;

    vwake=(wtk_kvadwake_t *)wtk_malloc(sizeof(wtk_kvadwake_t));
    vwake->cfg=cfg;
    vwake->ths=NULL;
    vwake->notify=NULL;
    vwake->ths2=NULL;
    vwake->notify2=NULL;

    vwake->vad2=NULL;
    if(cfg->use_vad)
    {
        vwake->vad2=wtk_vad2_new(&(cfg->vad));
        wtk_vad2_set_notify(vwake->vad2, vwake, (wtk_vad2_notify_f)wtk_kvadwake_on_vad2);
        vwake->vad2->use_vad_start=cfg->use_vad_start;
    }

    vwake->wdec=NULL;
    vwake->kwdec2=NULL;
    vwake->kwake=NULL;
    vwake->img=NULL;
    vwake->decoder=NULL;
    vwake->asr_res=NULL;
    vwake->wav_buf=NULL;
    vwake->wav_pos=0;
    if(cfg->use_wdec)
    {
        vwake->wdec=wtk_wdec_new(&(cfg->wdec));
    }else if(cfg->use_kwdec2)
    {
        vwake->kwdec2=wtk_kwdec2_new(&(cfg->kwdec2));
    }else if(cfg->use_img)
    {
    	vwake->img=qtk_img_rec_new(&(cfg->img));
        qtk_img_rec_set_notify(vwake->img,(qtk_img_rec_notify_f)wtk_kvadwake_img_notify,vwake);
    	if(cfg->use_ivad)
    	{
    		vwake->wav_buf=wtk_strbuf_new(1024,1);
    	}
    	if(cfg->use_kdec)
		{
			vwake->decoder=qtk_decoder_wrapper_new(&(cfg->decoder));
			vwake->asr_res = wtk_strbuf_new(1024,1);
		}
    }else if(cfg->use_kdec)
    {
    	vwake->decoder=qtk_decoder_wrapper_new(&(cfg->decoder));
        vwake->asr_res = wtk_strbuf_new(1024,1);
    }else
    {
        vwake->kwake=wtk_kwake_new(&(cfg->kwake));
    }

    vwake->idx=0;
    wtk_kvadwake_reset(vwake);

    return vwake;
}

void wtk_kvadwake_delete(wtk_kvadwake_t *vwake)
{
    if(vwake->vad2)
    {
        wtk_vad2_delete(vwake->vad2);
    }

    if (vwake->kwake) {

        wtk_kwake_delete(vwake->kwake);
    }
    if(vwake->kwdec2)
    {
        wtk_kwdec2_delete(vwake->kwdec2);
    }
    if(vwake->wdec)
    {
        wtk_wdec_delete(vwake->wdec);
    }

    if(vwake->img)
    {
    	qtk_img_rec_delete(vwake->img);
    	if(vwake->wav_buf)
    	{
    		wtk_strbuf_delete(vwake->wav_buf);
    	}
    }
    if(vwake->decoder)
    {
    	qtk_decoder_wrapper_delete(vwake->decoder);
        wtk_strbuf_delete(vwake->asr_res);
    }
    wtk_free(vwake);
}

void wtk_kvadwake_reset(wtk_kvadwake_t *vwake)
{
    vwake->sil=1;
    vwake->cnt=0;
    vwake->start_wake_cnt=0;
    vwake->vad_len=0;

    if(vwake->vad2)
    {
        wtk_vad2_reset(vwake->vad2);
    }

    if(vwake->kwake)
    {
        wtk_kwake_reset(vwake->kwake);
    }
    if(vwake->kwdec2)
    {
        wtk_kwdec2_reset(vwake->kwdec2);
    }

    if(vwake->wdec)
    {
        wtk_wdec_reset(vwake->wdec);
    }

    if(vwake->img)
    {
        qtk_img_rec_reset(vwake->img);
        if(vwake->wav_buf)
        {
    		wtk_strbuf_reset(vwake->wav_buf);
        }
    }

    if(vwake->decoder)
    {
        wtk_strbuf_reset(vwake->asr_res);
    }
}

void wtk_kvadwake_start(wtk_kvadwake_t *vwake)
{
    if(vwake->vad2)
    {
        wtk_vad_set_margin(vwake->vad2->vad, vwake->cfg->vad_left_margin, vwake->cfg->vad_right_margin);
        wtk_vad2_start(vwake->vad2);
    }else
    {
        if(vwake->kwake)
        {
            wtk_kwake_start(vwake->kwake);
        }else if(vwake->wdec)
        {
            wtk_wdec_start(vwake->wdec, NULL);
        }else if(vwake->kwdec2)
        {
            wtk_kwdec2_start(vwake->kwdec2);
        }else if(vwake->decoder)
        {
        	//qtk_decoder_wrapper_start(vwake->decoder);
        }else
        {
        	qtk_img_rec_start(vwake->img);
        }
    }
}

void wtk_kvadwake_on_vad2(wtk_kvadwake_t *vwake,wtk_vad2_cmd_t cmd,short *data,int len)
{
	wtk_kwake_t *kwake=vwake->kwake;
    wtk_wdec_t *wdec=vwake->wdec;
    wtk_kwdec2_t *kwdec2 = vwake->kwdec2;
    qtk_img_rec_t *img=vwake->img;
    qtk_decoder_wrapper_t *dec=vwake->decoder;
    int rate=vwake->cfg->rate;
	float ff,fs,fe;
    int ret = 0;
    wtk_string_t rec_res;

	switch(cmd)
    {
    case WTK_VAD2_START:
        if(kwake)
        {
            wtk_kwake_start(kwake);
        }else if(wdec)
        {
            wtk_wdec_start(wdec, NULL);
        }else if(kwdec2)
        {
            wtk_kwdec2_start(kwdec2);
        }else if(dec)
        {
        	qtk_decoder_wrapper_start(dec);
        }else
        {
        	qtk_img_rec_start(img);
        }
        
        vwake->start_wake_cnt=vwake->cnt;

        // wtk_debug("spech start = %f s\n",vwake->start_wake_cnt*1.0/rate);
        vwake->sil=0;
        vwake->vad_len=0;
        if(vwake->notify)
        {
            vwake->notify(vwake->ths, WTK_KVADWAKE_VAD_START, 0, 0, NULL, 0);
        }

        break;
    case WTK_VAD2_DATA:
        vwake->cnt+=len;
        vwake->vad_len+=len;
        if(vwake->sil==0)
        {
            if(kwake)
            {
                ret=wtk_kwake_feed(kwake,data,len,0);
            }else if(wdec)
            {
                ret=wtk_wdec_feed(wdec,(char *)data, len<<1,0);
            }else if(kwdec2)
            {
                ret=wtk_kwdec2_feed2(kwdec2,data, len,0);
            }else if(dec)
            {
            	qtk_decoder_wrapper_feed(dec,(char *)data, len<<1,0);
            }else
            {
            	ret=qtk_img_rec_feed(img,(char *)data, len<<1,0);
            }
            if(ret==1)
		    {
                if(kwake)
                {
                    wtk_kwake_get_wake_time(kwake,&fs,&fe);
                }else if(wdec)
                {
                    wtk_wdec_get_final_time(wdec,&fs,&fe);
                }else if(kwdec2)
                {
                    wtk_kwdec2_get_wake_time(kwdec2,&fs,&fe);
                }else if(dec)
                {
                	//TODO never be here
                	qtk_decoder_wrapper_get_time(dec,&fs,&fe);
                }else
                {
                	qtk_img_rec_get_time(img,&fs,&fe);
                }

                ff=vwake->start_wake_cnt*1.0/rate;
                // wtk_debug("wake=[%f,%f] pos=%f\n",fs,fe,ff);
                fs+=ff;
                fe+=ff+vwake->cfg->wake_right_fe;
                if(vwake->notify)
                {
                    // wtk_debug("xxxxxxx\n");
                    vwake->notify(vwake->ths, WTK_KVADWAKE_WAKE, fs, fe, NULL, 0);
                }
                if(kwake)
                {
                    // wtk_kwake_print(kwake);
                    wtk_kwake_feed(kwake,NULL,0,1);
                    wtk_kwake_reset(kwake);
                    wtk_kwake_start(kwake);
                }else if(kwdec2)
                {
                    wtk_kwdec2_feed2(kwdec2,0,0,1);
                    wtk_kwdec2_reset(kwdec2);
                    wtk_kwdec2_start(kwdec2);
                }else if(wdec)
                {
                    // wtk_wdec_feed(wdec,NULL,0,1);
                    // wtk_wdec_reset(wdec);
                    wtk_wdec_reset2(wdec);
                    wtk_wdec_start(wdec, NULL);
                }else if(dec)
                {
                	qtk_decoder_wrapper_reset(dec);
                	qtk_decoder_wrapper_start(dec);
                }else
                {
                	qtk_img_rec_reset(img);
                	qtk_img_rec_start(img);
                }
                
                vwake->start_wake_cnt=vwake->cnt;
            }
        }

        if(vwake->cfg->max_vad_len > 0 && vwake->vad_len>vwake->cfg->max_vad_len)
        {
            if(kwake)
            {
                // wtk_kwake_print(kwake);
                wtk_kwake_feed(kwake,NULL,0,1);
                wtk_kwake_reset(kwake);
                wtk_kwake_start(kwake);
            }else if(kwdec2)
            {
                wtk_kwdec2_feed2(kwdec2,0,0,1);
                wtk_kwdec2_reset(kwdec2);
                wtk_kwdec2_start(kwdec2);
            }else if(wdec)
            {
                // wtk_wdec_feed(wdec,NULL,0,1);
                // wtk_wdec_reset(wdec);
                wtk_wdec_reset2(wdec);
                wtk_wdec_start(wdec, NULL);
            }else if(dec)
            {
                qtk_decoder_wrapper_reset(dec);
                qtk_decoder_wrapper_start(dec);
            }else
            {
                qtk_img_rec_reset(img);
                qtk_img_rec_start(img);
            }
            vwake->vad_len=0;
        }

        if(vwake->notify)
        {
            vwake->notify(vwake->ths, WTK_KVADWAKE_VAD_DATA, 0, 0, data, len);
        }
        
        break;
    case WTK_VAD2_END:
        if(kwake)
        {
            ret=wtk_kwake_feed(kwake,NULL,0,1);
        }else if(kwdec2)
        {
            ret=wtk_kwdec2_feed2(kwdec2,0,0,1);
        }else if(wdec)
        {
            // ret=wtk_wdec_feed(wdec,NULL,0,1);
            ret = wdec->found;
        }else if(dec)
        {
        	qtk_decoder_wrapper_feed(dec,NULL,0,1);
            ret = qtk_decoder_wrapper_get_result2(dec, &rec_res);
            wtk_strbuf_reset(vwake->asr_res);
        }else
        {
            ret=qtk_img_rec_feed(img,NULL,0,1);
        }
        if(ret==1)
        {
            if(kwake)
            {
                wtk_kwake_get_wake_time(kwake,&fs,&fe);
            }else if(kwdec2)
            {
                wtk_kwdec2_get_wake_time(kwdec2,&fs,&fe);
            }else if(wdec)
            {
                wtk_wdec_get_final_time(wdec,&fs,&fe);
            }else if(dec)
            {
            	wtk_strbuf_push(vwake->asr_res,rec_res.data,rec_res.len);
            	qtk_decoder_wrapper_get_time(dec,&fs,&fe);
            }else
            {
            	qtk_img_rec_get_time(img,&fs,&fe);
            }
            ff=vwake->start_wake_cnt*1.0/rate;
            if(vwake->cfg->debug)
            {
            	wtk_debug("wake=[%f,%f] pos=%f\n",fs,fe,ff);
            }
            fs+=ff;
            fe+=ff+vwake->cfg->wake_right_fe;
            if(vwake->notify)
            {
                vwake->notify(vwake->ths, WTK_KVADWAKE_WAKE, fs, fe, NULL, 0);
            }

            // wtk_kwake_print(kwake);
        }
        // wtk_debug("spech end = %f s\n",vwake->cnt*1.0/rate);
        if(kwake)
        {
            // wtk_kwake_print(kwake);
            wtk_kwake_reset(kwake);
        }else if(kwdec2)
        {
            wtk_kwdec2_reset(kwdec2);
        }else if(wdec)
        {
            // wtk_wdec_reset(wdec);
            wtk_wdec_reset2(wdec);
        }else if(dec)
        {
        	qtk_decoder_wrapper_reset(dec);
        }else
        {
        	qtk_img_rec_reset(img);
        }

        vwake->sil=1;
        if(vwake->notify)
        {
            vwake->notify(vwake->ths, WTK_KVADWAKE_VAD_END, 0, 0, NULL, 0);
        }

        break;
    case WTK_VAD2_CANCEL:
        vwake->cnt-=len;
        if(vwake->notify)
        {
            vwake->notify(vwake->ths, WTK_KVADWAKE_VAD_CANCEL, 0, 0, NULL, len);
        }
        break;
    }
}

void wtk_kvadwake_img_notify(wtk_kvadwake_t *vwake, int res, int start, int end){
    float fs, fe;
    float scale=vwake->img->cfg->f_dur;
    fs = start*scale;
    fe = end*scale;
    if(res==1){
        vwake->notify(vwake->ths, WTK_KVADWAKE_WAKE, fs, fe, NULL, 0);
    }else if(res==0){
        vwake->notify2(vwake->ths2, WTK_KVADWAKE_WAKE2, fs, fe, NULL, 0);
    }
}


static int count = 0;
void wtk_kvadwake_feed(wtk_kvadwake_t *vwake, short *data, int len,int is_end)
{
    int ret;
    float fs,fe,ff;
    // float fs,fe;
    int rate=vwake->cfg->rate;
    int nlen;
	wtk_kwake_t *kwake=vwake->kwake;
    wtk_wdec_t *wdec=vwake->wdec;
    wtk_kwdec2_t *kwdec2 = vwake->kwdec2;
    qtk_img_rec_t *img=vwake->img;
    qtk_decoder_wrapper_t *dec=vwake->decoder;
    wtk_string_t rec_res;
    int wdec_ret=-1;

    if(vwake->vad2)
    {
        // wtk_vad2_feed(vwake->vad2, (char *)data, len<<1, is_end);
        if(!is_end){
            wtk_vad2_feed(vwake->vad2, (char *)data, len<<1, is_end);
        }
    }else
    {
        vwake->cnt+=len;
        if(kwake)
        {
            ret=wtk_kwake_feed(kwake, data, len, is_end);
        }else if(kwdec2)
        {
            wtk_debug("kwdec2 don't support without vad now");
            exit(0);//to do
        }else if(wdec)
        {
            ++count;
            ret=wtk_wdec_feed(wdec, (char *)data, len<<1, is_end);
            if(count==300){
                wdec_ret = wtk_wdec_reset_check(wdec);
                count=0;
            }
        }else
        {
        	if(vwake->cfg->use_ivad)
        	{
        		nlen = len<<1;
        		if(vwake->wav_buf->pos + nlen > 64000)
        		{
        			vwake->wav_pos+=vwake->wav_buf->pos + nlen - 64000;
        			wtk_strbuf_pop(vwake->wav_buf,NULL,(vwake->wav_buf->pos + nlen - 64000));
        			//wtk_debug("%d\n",vwake->wav_pos);
        		}
        		wtk_strbuf_push(vwake->wav_buf,(char *)data,len<<1);
        	}
        	ret=qtk_img_rec_feed(img, (char *)data, len<<1, is_end);
        }
        if(ret==1)
        {
            // printf("count=%d\n", count);
            if(kwake)
            {
                wtk_kwake_get_wake_time(kwake,&fs,&fe);
            }else if(wdec)
            {
                wtk_wdec_get_final_time(wdec,&fs,&fe);
            }else if(img)
            {
            	qtk_img_rec_get_time(img,&fs,&fe);
                // wtk_debug("%f %f\n", fs, fe);
            	if(vwake->cfg->use_ivad)
            	{
            		//int e=vwake->wav_buf->pos;
 //           		int xx = (fs-0.5)*32000-vwake->wav_pos;
 //           		char *s = vwake->wav_buf->data + xx;
 //           		int e = (fe - fs + 0.5)*32000;
            		//wtk_debug("%f %f %d %d\n",fs*32000,fe*32000,vwake->wav_pos,vwake->wav_buf->pos);
            		//wtk_debug("%d %d\n",xx,e);
            		//s += (fs / vwake->img->parm->cfg->frame_dur - vwake->wav_pos);
            		wtk_strbuf_reset(vwake->asr_res);
            		char *s2 = vwake->wav_buf->data + 5*3200;
            		int e2 = (fe+0.1)*32000 - vwake->wav_pos - 5*3200;
//                	wtk_wavfile_t *log=NULL;
//                	char *wname;
//					if(!log)
//					{
//						log=wtk_wavfile_new(16000);
//						wname=wtk_strbuf_to_str(vwake->name);
//						wtk_wavfile_open2(log,wname);
//						log->max_pend=0;
//					}
//					//wtk_wavfile_write(log,vwake->wav_buf->data+2*3200, vwake->wav_buf->pos - 0.4*32000);
//					wtk_wavfile_write(log,s2, e2);
//					wtk_wavfile_close(log);
//					wtk_wavfile_delete(log);
//					wtk_free(wname);
//					log=NULL;

            		qtk_decoder_wrapper_start(vwake->decoder);
                	qtk_decoder_wrapper_feed(vwake->decoder, s2, e2, 1);
                	//qtk_decoder_wrapper_feed(vwake->decoder, vwake->wav_buf->data+2*3200, vwake->wav_buf->pos - 0.4*32000, 1);
                    ret = qtk_decoder_wrapper_get_result2(vwake->decoder, &rec_res);
                    //qtk_decoder_wrapper_reset(vwake->decoder);


                    if(ret)
                    {
                    	wtk_strbuf_push(vwake->asr_res,rec_res.data,rec_res.len);
                    }
            		vwake->wav_pos = 0;
            		wtk_strbuf_reset(vwake->wav_buf);
            	}
            }
            if(!img){
                ff=vwake->start_wake_cnt*1.0/rate;
    //            if (vwake->cfg->debug) {
    //            	if(vwake->img)
    //            	{
    //            		qtk_img_rec_show(vwake->img,vwake->img->wake_id);
    //            	}
    //            }
                fs+=ff;
                fe+=ff+vwake->cfg->wake_right_fe;
                if(vwake->notify)
                {
                    vwake->notify(vwake->ths, WTK_KVADWAKE_WAKE, fs, fe, NULL, 0);
                }

                if(kwake)
                {
                    // wtk_kwake_print(kwake);
                    wtk_kwake_feed(kwake,NULL,0,1);
                    wtk_kwake_reset(kwake);
                    wtk_kwake_start(kwake);
                }else if(kwdec2)
                {
                    wtk_kwdec2_feed2(kwdec2,0,0,1);
                    wtk_kwdec2_reset(kwdec2);
                    wtk_kwdec2_start(kwdec2);
                }else if(wdec)
                {
                    // wtk_wdec_feed(wdec,NULL,0,1);
                    // wtk_wdec_reset(wdec);
                    wtk_wdec_reset2(wdec);
                    wtk_wdec_start(wdec, NULL);
                }else if(dec)
                {
                    qtk_decoder_wrapper_reset(dec);
                    qtk_decoder_wrapper_start(dec);
                }

            }
            vwake->start_wake_cnt=vwake->cnt;
        }

        if(wdec_ret==1){
            // wtk_wdec_feed(wdec,NULL,0,1);
            // wtk_wdec_reset(wdec);
            wtk_wdec_reset2(wdec);
            wtk_wdec_start(wdec, NULL);
        }

        if(vwake->notify)
        {
            vwake->notify(vwake->ths, WTK_KVADWAKE_VAD_DATA, 0, 0, data, len);
        }
    }
    
}

void wtk_kvadwake_set_notify(wtk_kvadwake_t *vwake, void *ths, wtk_kvadwake_notify_f notify)
{
    vwake->ths=ths;
    vwake->notify=notify;
}

void wtk_kvadwake_set_notify2(wtk_kvadwake_t *vwake, void *ths, wtk_kvadwake_notify_f2 notify)
{
    vwake->ths2=ths;
    vwake->notify2=notify;
}

float wtk_kvadwake_get_conf(wtk_kvadwake_t *vwake)
{
    if(vwake->kwake)
    {
        return wtk_kwake_get_conf(vwake->kwake);
    }else if(vwake->kwdec2)
    {
        return wtk_kwdec2_get_conf(vwake->kwdec2);
    }else if(vwake->wdec)
    {
        return wtk_wdec_get_conf(vwake->wdec);
    }else if(vwake->decoder)
    {
    	return qtk_decoder_wrapper_get_conf(vwake->decoder);
    }else
    {
    	return qtk_img_rec_get_conf(vwake->img);
    }
}

void wtk_kvadwake_set_idx(wtk_kvadwake_t *vwake, int idx)
{
    vwake->idx=idx;
    if(vwake->cfg->use_img){
        if(idx==0){ // 0表示非模型降噪通道
            qtk_img_thresh_set_cfg(vwake->img, vwake->cfg->img_no_mod, 0);  // 0表示非回声情况
            qtk_img_thresh_set_cfg(vwake->img, vwake->cfg->img_no_mod_echo, 1);  // 1表示回声情况
        }else if(idx==1){  // 1表示模型降噪通道
            qtk_img_thresh_set_cfg(vwake->img, vwake->cfg->img_mod, 0);  // 0表示非回声情况
            qtk_img_thresh_set_cfg(vwake->img, vwake->cfg->img_mod_echo, 1);  // 1表示回声情况
        }
    }
}

void wtk_kvadwake_get_sp_sil(wtk_kvadwake_t *vwake, int sp_sil)
{
    if(vwake->cfg->use_img){
        qtk_img_rec_get_sp_sil(vwake->img, sp_sil);
    }
}


int wtk_kvadwake_set_words(wtk_kvadwake_t *vwake,char *words,int len)
{
    int ret = -1;

    if (!vwake->cfg->use_kwdec2) {
        goto end;
    }
    ret=wtk_kwdec2_set_words(vwake->kwdec2, words, len);
end:
    return ret;
}
