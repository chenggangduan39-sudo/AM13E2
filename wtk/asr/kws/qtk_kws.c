#include "qtk_kws.h"
#define SV_WAV_LEN 64000

void qtk_kws_kxparm_notity(qtk_kws_t *kws,wtk_kfeat_t *feat);
void qtk_kws_wakenn_flush_notify(qtk_kws_t *kws,qtk_blas_matrix_t *feat);
void qtk_kws_on_vad2(qtk_kws_t *kws,wtk_vad2_cmd_t cmd,short *data,int len);
int qtk_kws_col = 64;
int kws_mdl_read(qtk_torchnn_t **kws,wtk_source_t *src)
{
    int ret = 0;
    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);

    *kws = qtk_torchnn_read(src,buf,0,qtk_kws_col,1);

    wtk_strbuf_delete(buf);
    return ret;
}

qtk_kws_t* qtk_kws_new(qtk_kws_cfg_t *cfg)
{
	qtk_kws_t *kws = (qtk_kws_t*)wtk_malloc(sizeof(qtk_kws_t));

    kws->cfg = cfg;
    if(cfg->use_wake1)
    {
        kws->parm = wtk_kxparm_new(&cfg->kxparm);
        wtk_kxparm_set_notify(kws->parm,kws,(wtk_kxparm_notify_f)qtk_kws_kxparm_notity);
    }else
    {
    	kws->parm = NULL;
    }
    kws->wake = NULL;
    kws->svprint = NULL;
    // kws->log_wav = wtk_wavfile_new(16000);
    kws->img = NULL;
    kws->vad = NULL;
    kws->vad2 = NULL;
    kws->notify = NULL;
    kws->notify_ths = NULL;
    kws->notify2 = NULL;
    kws->notify_ths2 = NULL;
    kws->vad_state = 0;
    kws->name = NULL;
	if(cfg->use_wake1)
	{
	    qtk_kws_col = kws->parm->cfg->parm.melbank.num_bins;
            wtk_source_loader_load(&cfg->sl, &(kws->wakenn),(wtk_source_load_handler_t)kws_mdl_read, cfg->wake_fn);
	    //kws->wake = wtk_kwake_new2(&cfg->kwake,kws->wakenn->output_col);
	    //qtk_torchnn_set_notify(kws->wakenn,kws,(qtk_torchnn_notify_f)qtk_kws_wakenn_flush_notify);
	}
	if(cfg->use_wake2)
		wtk_source_loader_load(&cfg->sl, &(kws->wakee2e_nn),(wtk_source_load_handler_t)kws_mdl_read, cfg->e2e_fn);
	if(cfg->use_svprint)
	{
		qtk_kws_col = 64;
		//wtk_source_loader_load(&cfg->sl, &(kws->svprint_nn),(wtk_source_load_handler_t)kws_mdl_read, cfg->svprint_fn);
	    kws->svprint = wtk_svprint_new(&cfg->svprint);
	    //kws->svprint->x->nn = kws->svprint_nn;
	    //wtk_svprint_set_nnet3_xvec_notify(kws->svprint);
		//wtk_source_loader_load(&cfg->sl, kws->svprint->x,(wtk_source_load_handler_t)xvector_pool_read, cfg->svprint.pool_fn);
	}
	if(cfg->use_img)
	{
		kws->img = qtk_img_rec_new(&cfg->img);
	}

	if(cfg->use_vad)
	{
		kws->vad = wtk_kvad_new(&cfg->vad);
	}

    if(cfg->use_vad2)
    {
        kws->vad2 = wtk_vad2_new(&(cfg->vad2));
        wtk_vad2_set_notify(kws->vad2, kws, (wtk_vad2_notify_f)qtk_kws_on_vad2);
    }

    kws->enroll = 0;
    kws->eval = 1;
    kws->index = 0;
    kws->user = wtk_strbuf_new(1024,1);
    if(kws->cfg->use_window_pad)
    {
    	kws->wav_buf = wtk_strbuf_new(1024,1);
    }
    if(kws->cfg->use_sliding_window)
    {
        kws->sliding_buf = wtk_strbuf_new(1024,1);
        kws->window_size = 32000 * 2;
        kws->shift_size = -1;
    }
    kws->sv_prob = 2.0;
    kws->process_size = 0;
    kws->acc_size = 0;
    kws->acc_size2 = 0;
    kws->has_res = 0;
    kws->bkg_db = 1000.0;
    kws->speech_db = -1000.0;
	return kws;
}

void qtk_kws_delete(qtk_kws_t *kws)
{
	if(kws->parm)
	{
		wtk_kxparm_delete(kws->parm);
	}
    wtk_strbuf_delete(kws->user);
    if(kws->cfg->use_window_pad)
    {
    	wtk_strbuf_delete(kws->wav_buf);
    }
    if(kws->cfg->use_sliding_window)
    {
        wtk_strbuf_delete(kws->sliding_buf);
    }
    //if(kws->wake)
    //{
    //	qtk_torchnn_delete(kws->wakenn);
    //	wtk_kwake_delete(kws->wake);
    //}
    if(kws->svprint)
    {
        //qtk_torchnn_delete(kws->svprint_nn);
        wtk_svprint_delete(kws->svprint);
    }
    if(kws->img)
    {
        qtk_img_rec_delete(kws->img);
    }
    if(kws->vad)
    {
        wtk_kvad_delete(kws->vad);
    }
    if(kws->vad2){
        wtk_vad2_delete(kws->vad2);
    }

    wtk_free(kws);
}

int qtk_kws_start(qtk_kws_t *kws)
{
	if(kws->enroll || kws->eval)
	{
		wtk_svprint_start(kws->svprint);
	}

	if(kws->wake && !kws->enroll)
	{
		if(kws->vad)
		{
			wtk_kvad_start(kws->vad);
		}
		wtk_kwake_start(kws->wake);
		wtk_kxparm_start(kws->parm);
	}
	return 0;
}
int wake=0;
int wake_cnt=0;
int qtk_kws_reset(qtk_kws_t *kws)
{
    if(kws->parm)
    {
        wtk_kxparm_reset(kws->parm);
    }
    kws->index = 0;
    if(kws->cfg->use_wake1)
    {
        wtk_kwake_reset(kws->wake);
        qtk_torchnn_reset(kws->wakenn);
        wake = 0;
    }
    if(kws->cfg->use_svprint)
    {
        //wtk_debug("reset\n");
        wtk_svprint_reset(kws->svprint);
        //qtk_torchnn_reset(kws->svprint_nn);
    }
    kws->vad_state = 0;
    if(kws->vad)
    {
        wtk_kvad_reset(kws->vad);
    }

    if(kws->vad2){
        wtk_vad2_reset(kws->vad2);
    }

    if(kws->cfg->use_window_pad)
    {
        wtk_strbuf_reset(kws->wav_buf);
    }
    if(kws->cfg->use_sliding_window)
    {
        wtk_strbuf_reset(kws->sliding_buf);
    }
    kws->name = NULL;
    kws->process_size = 0;
    kws->acc_size = 0;
    kws->acc_size2 = 0;
    kws->has_res = 0;
    kws->bkg_db = 1000.0;
    kws->speech_db = -1000.0;
	return 0;
}

int qtk_kws_reset2(qtk_kws_t *kws)
{
    kws->vad_state = 0;
    if(kws->parm)
    {
    	wtk_kxparm_reset(kws->parm);
    }
	kws->index = 0;
	if(kws->cfg->use_wake1)
	{
		wtk_kwake_reset(kws->wake);
		qtk_torchnn_reset(kws->wakenn);
		wake = 0;
	}
	if(kws->cfg->use_svprint)
	{
		wtk_svprint_reset(kws->svprint);
		//qtk_torchnn_reset(kws->svprint_nn);
	}
    kws->bkg_db = 1000.0;
    kws->speech_db = -1000.0;
	return 0;
}

void qtk_kws_kxparm_notity(qtk_kws_t *kws,wtk_kfeat_t *feat)
{
	qtk_blas_matrix_t *output;
	wtk_string_t *name;
	//memcpy(feat->v,ppen+p_idx*40,sizeof(float)*40);
	//p_idx++;
//	print_float(feat->v,40);
	output = qtk_torchnn_feed(kws->wakenn,feat->v,40,1,0);
	if(output)
	{
		//printf("idx %d:",indx++);
		//qtk_blas_matrix_print(output);
		//printf("\n");
		//wtk_kwake_on_featf(kws->wake,output->m, kws->index++);
		if(kws->wake->waked == 1 && !wake)
		{
			wake=1;
			//wtk_debug("wakedxxxxxxxxxx %d\n",wake_cnt++);
			if(kws->eval)
			{
				qtk_torchnn_flush(kws->svprint_nn);
				name = wtk_svprint_eval(kws->svprint, &kws->sv_prob);
				//wtk_debug("%f\n",kws->sv_prob);
				if(name != NULL)
				{
					if(kws->notify)
					{
						kws->notify(kws->notify_ths,3,name->data,name->len);
					}else
					{
						printf("%.*s\n",name->len,name->data);
					}
				}else
				{
					//wtk_debug("sv distance:%f\n",kws->sv_prob);
					kws->notify(kws->notify_ths,1,NULL,0);
				}
				qtk_kws_reset2(kws);
			}
		}
	}
}


void qtk_kws_wakenn_flush_notify(qtk_kws_t *kws,qtk_blas_matrix_t *feat)
{
	if(feat)
	{
		//printf("idx %d:",indx++);
		//qtk_blas_matrix_print(feat);
		//printf("\n");
		//wtk_kwake_on_featf(kws->wake,feat->m, kws->index++);
		if(kws->wake->waked == 1)
		{
			wtk_debug("wakedxxxxxxxxxx\n");
		}
	}
}

void qtk_kws_clean(qtk_kws_t *kws)
{
	if(kws->svprint)
	{
		wtk_svprint_clean(kws->svprint);
	}
}

void qtk_kws_reload(qtk_kws_t *kws)
{
	if(kws->svprint)
	{
		wtk_svprint_reload(kws->svprint);
	}
}

char* qtk_kws_get_fn(qtk_kws_t *kws)
{
	if(kws->svprint)
	{
		return kws->svprint->cfg->enroll_fn;
	}
	return NULL;
}

static short *qtk_kws_pad_data(short *data, int len) {
    short *to_data = NULL;
    short *s;
    int s3l = 16000 * 2; //最低3s
    int pad = s3l / len;
    int i, j = s3l % len;

    to_data = (short *)wtk_calloc(s3l, sizeof(short));
    s = to_data;
    for (i = 0; i < pad; ++i) {
        memcpy(s, data, len * sizeof(short));
        s = s + len;
    }
    memcpy(s, data, j * sizeof(short));

    return to_data;
}

float qtk_kws_wav_scale(char *data, int len)
{
	short *wav = (short*)data;
	int l = len>>1;
	int max = wtk_short_abs_max(wav,l);

	return 0.99/max;
}

void qtk_kws_feed_vprint(qtk_kws_t *kws,char *data,int bytes){
    wtk_vecf_t *mean,*vec;
    int cnt,i;
    wtk_svprint_feed(kws->svprint,
                    (short *)data,
                    bytes >> 1, 0);
    wtk_svprint_feed(kws->svprint, NULL, 0, 1);

    vec = wtk_svprint_compute_feat2(kws->svprint, &cnt);
    mean = kws->svprint->x->spk_mean;
    kws->svprint->x->spk_cnt++;
    for (i = 0; i < mean->len; i++) {
        mean->p[i] += vec->p[i];
    }
    wtk_svprint_eval_Nbest(kws->svprint, mean, vec, kws->svprint->x->spk_cnt,&kws->sv_prob);
    wtk_svprint_reset(kws->svprint);
    kws->has_res = 1;
}

float _get_db(short *val,int len){
    int i,cnt = 0;
    float db = 0.0;
    for(i = 0; i < len; i++){
        if(val[i] != 0){
            db += 20 * log10f(fabs(val[i]/32768.0));
            cnt++;
        }
    }
    return db/cnt;
}

int qtk_kws_feed(qtk_kws_t *kws,char *data,int bytes,int is_end)
{
    qtk_kws_cfg_t *cfg = kws->cfg;
    wtk_queue_t *q = &(kws->vad->output_q);
    wtk_vframe_t *vf = NULL;
    wtk_queue_node_t *qn;
    wtk_string_t *name;
    wtk_vecf_t *vec, *mean;
    int cnt, i, flag;
    float db;
    if (kws->enroll) // || kws->eval)
    {
        if (kws->svprint->enroll_available == 0) {
            return -1;
        }

        if (kws->vad) {
            wtk_kvad_feed(kws->vad, (short *)data, bytes >> 1, is_end);
            // wtk_debug("%d\n",kws->vad->output_q.length);
            while (1) {
                qn = wtk_queue_pop(q);
                if (!qn) {
                    break;
                }
                vf = data_offset2(qn, wtk_vframe_t, q_n);

                switch (vf->state) {
                case wtk_vframe_sil:
                    db = _get_db(vf->wav_data,vf->frame_step);
                    if(kws->bkg_db > db){
                        kws->bkg_db = db;
                    }
                    // if(kws->cfg->use_window_pad)
                    //{
                    //    wtk_strbuf_push(kws->wav_buf,(char*)vf->wav_data,vf->frame_step*sizeof(short));
                    //}
                    break;
                case wtk_vframe_speech:
                    db = _get_db(vf->wav_data,vf->frame_step);
                    if(kws->speech_db < db){
                        kws->speech_db = db;
                    }
                    if (!kws->cfg->use_window_pad) {
                        wtk_svprint_feed(kws->svprint, vf->wav_data,
                                         vf->frame_step, 0);
                    } else {
                        wtk_strbuf_push(kws->wav_buf, (char *)vf->wav_data,
                                        vf->frame_step * sizeof(short));
                    }
                    break;
                case wtk_vframe_speech_end:
                    if (kws->cfg->use_window_pad) {
                        wtk_strbuf_push(kws->wav_buf, (char *)vf->wav_data,
                                        vf->frame_step * sizeof(short));
                    }
                    // qtk_kws_reset2(kws);
                    break;
                default:
                    break;
                }
                wtk_kvad_push_frame(kws->vad, vf);
            }
        } else {
            wtk_svprint_feed(kws->svprint, (short *)data, (bytes) >> 1, 0);
        }
        // wtk_svprint_feed(kws->svprint,(short*)data,(bytes)>>1,is_end);
        if (is_end == 1) {
            if(kws->bkg_db - kws->speech_db >= -5){
                return -1;
            }
            if (kws->cfg->use_window_pad) {
                // float sc =
                // qtk_kws_wav_scale(kws->wav_buf->data,kws->wav_buf->pos);
                // kws->svprint->x->kxparm->parm->scale = sc;
                if (kws->wav_buf->pos < SV_WAV_LEN) {
                    return -1;
                } else {
                    if (kws->wav_buf->pos > SV_WAV_LEN * 2) {
                        cnt = kws->wav_buf->pos / SV_WAV_LEN;
                        char *s, *e;
                        int nx;

                        s = kws->wav_buf->data;
                        e = kws->wav_buf->data + kws->wav_buf->pos;

                        while (s < e) {
                            // wtk_wavfile_open2(kws->log_wav,"vad");
                            nx = min(SV_WAV_LEN, e - SV_WAV_LEN - s);
                            if (nx < SV_WAV_LEN) {
                                nx = e - s;
                                // wtk_wavfile_write(kws->log_wav,s,nx);
                                wtk_svprint_feed(kws->svprint, (short *)s,
                                                 nx >> 1, 0);
                                wtk_svprint_feed(kws->svprint, NULL, 0, 1);
                                s += nx;
                            } else {
                                // wtk_wavfile_write(kws->log_wav,s,SV_WAV_LEN);
                                wtk_svprint_feed(kws->svprint, (short *)s,
                                                 SV_WAV_LEN >> 1, 0);
                                wtk_svprint_feed(kws->svprint, NULL, 0, 1);
                                s += SV_WAV_LEN;
                            }
                            // wtk_wavfile_close(kws->log_wav);
                            kws->svprint->x->spk_cnt++;
                            vec = wtk_svprint_compute_feat2(kws->svprint, &cnt);
                            mean = kws->svprint->x->spk_mean;

                            for (i = 0; i < mean->len; i++) {
                                mean->p[i] += vec->p[i];
                            }
                            qtk_kws_reset2(kws);
                        }
                    } else {
                        wtk_svprint_feed(kws->svprint,
                                         (short *)kws->wav_buf->data,
                                         kws->wav_buf->pos >> 1, 0);
                        wtk_svprint_feed(kws->svprint, NULL, 0, 1);
                        kws->svprint->x->spk_cnt++;
                        vec = wtk_svprint_compute_feat2(kws->svprint, &cnt);
                        mean = kws->svprint->x->spk_mean;

                        for (i = 0; i < mean->len; i++) {
                            mean->p[i] += vec->p[i];
                        }
                    }
                }
            } else {
                wtk_svprint_feed(kws->svprint, NULL, 0, 1);
                kws->svprint->x->spk_cnt++;
                vec = wtk_svprint_compute_feat2(kws->svprint, &cnt);
                mean = kws->svprint->x->spk_mean;
                // wtk_debug("%d %d\n",mean->len,vec->len);
                // print_float(mean->p,mean->len);
                for (i = 0; i < mean->len; i++) {
                    mean->p[i] += vec->p[i];
                }
            }
            // print_float(mean->p,mean->len);
            // wtk_svprint_enroll2file(kws->svprint,&v);
        }

    } else {
        if (cfg->use_img) {
            qtk_img_rec_feed(kws->img, data, bytes, is_end);
        } else {
            if (kws->svprint->eval_available == 0) {
                return -1;
            }

            if (kws->vad) {
                wtk_kvad_feed(kws->vad, (short *)data, bytes >> 1, is_end);
                // wtk_debug("%d\n",kws->vad->output_q.length);
                while (1) {
                    flag = 0;
                    qn = wtk_queue_pop(q);
                    if (!qn) {
                        break;
                    }
                    vf = data_offset2(qn, wtk_vframe_t, q_n);
                    // wtk_debug("----------vf state-------- %d\n",vf->state);
                    switch (kws->vad_state) {
                    case 0:
                        if (vf->state != wtk_vframe_sil) {
                            if (kws->parm) {
                                wtk_kxparm_feed(kws->parm, vf->wav_data,
                                                vf->frame_step, 0);
                            }

                            if (kws->eval) {
                                if (!kws->cfg->use_window_pad) {
                                    wtk_svprint_feed(kws->svprint, vf->wav_data,
                                                     vf->frame_step, 0);
                                } else {
                                    flag = 1;
                                    wtk_strbuf_push(
                                        kws->wav_buf, (char *)vf->wav_data,
                                        vf->frame_step * sizeof(short));
                                }
                            }
                            kws->vad_state = 1;
                        }
                        break;
                    case 1:
                        if (vf->state == wtk_vframe_sil) {
                            kws->vad_state = 0;
                        } else {
                            if (kws->parm) {
                                wtk_kxparm_feed(kws->parm, vf->wav_data,
                                                vf->frame_step, 0);
                            }

                            if (kws->eval) {
                                if (!kws->cfg->use_window_pad) {
                                    wtk_svprint_feed(kws->svprint, vf->wav_data,
                                                     vf->frame_step, 0);
                                } else {
                                    flag = 1;
                                    wtk_strbuf_push(
                                        kws->wav_buf, (char *)vf->wav_data,
                                        vf->frame_step * sizeof(short));
                                }
                            }
                        }
                        break;

                    default:
                        break;
                    }
                    if(kws->cfg->section_secs > 0){
                        if(kws->shift_size == -1){
                            if(kws->wav_buf->pos >= 2 * 32000){
                                qtk_kws_feed_vprint(kws,kws->wav_buf->data,kws->wav_buf->pos);
                                wtk_strbuf_reset(kws->wav_buf);
                            }
                        }else{
                            if(kws->process_size == 0){
                                if(kws->wav_buf->pos >= 2 * 32000){
                                    qtk_kws_feed_vprint(kws,kws->wav_buf->data,kws->wav_buf->pos);
                                    kws->process_size = kws->wav_buf->pos;
                                }
                            }else{
                                if(flag == 1){
                                    kws->acc_size2 += vf->frame_step * sizeof(short);
                                }
                                if(kws->acc_size2 >= kws->shift_size){
                                    int len = kws->window_size;
                                    char *value = kws->wav_buf->data + (kws->wav_buf->pos - kws->window_size);
                                    qtk_kws_feed_vprint(kws,value,len);
                                    kws->process_size = kws->wav_buf->pos;
                                    kws->acc_size2 = 0;
                                }
                            }
                        }
                    }

                    kws->acc_size += vf->frame_step * sizeof(short);
                    if(kws->cfg->section_secs > 0 && kws->has_res == 1 && kws->acc_size >= kws->cfg->section_secs * 32000 - 0.1){
                        //wtk_debug("%d %f %f\n",kws->acc_size,kws->cfg->section_secs * 32000,kws->cfg->section_secs);
                        if (kws->notify2) {
                            kws->notify2(kws->notify_ths2, kws->svprint->res_cnt, kws->svprint->nbest_res,
                                        kws->svprint->score);
                        }
                        kws->acc_size = 0;
                    }
                    wtk_kvad_push_frame(kws->vad, vf);
                }

                if (is_end) //&& kws->parm)
                {
                    if (kws->cfg->use_window_pad) {
                        if(kws->shift_size == -1 || kws->process_size == 0){
                            if (kws->wav_buf->pos < 3200) {
                                if(kws->notify2){
                                    kws->notify2(kws->notify_ths2, kws->svprint->res_cnt, kws->svprint->nbest_res,
                                        kws->svprint->score);
                                }
                                kws->svprint->x->spk_cnt = 0;
                                wtk_vecf_zero(kws->svprint->x->spk_mean);
                                return 0;
                            }
                            if (kws->wav_buf->pos > 32000) {
                                // wtk_wavfile_open2(kws->log_wav,"vad");
                                // wtk_wavfile_write(kws->log_wav,kws->wav_buf->data,kws->wav_buf->pos);
                                // wtk_wavfile_close(kws->log_wav);
                                wtk_svprint_feed(kws->svprint,
                                                (short *)kws->wav_buf->data,
                                                kws->wav_buf->pos >> 1, 0);
                                wtk_svprint_feed(kws->svprint, NULL, 0, 1);
                            } else {
                                short *pad_wav;
                                pad_wav =
                                    qtk_kws_pad_data((short *)kws->wav_buf->data,
                                                    kws->wav_buf->pos >> 1);
                                // wtk_wavfile_open2(kws->log_wav,"vad");
                                // wtk_wavfile_write(kws->log_wav,(char*)pad_wav,64000);
                                // wtk_wavfile_close(kws->log_wav);
                                wtk_svprint_feed(kws->svprint, pad_wav, 32000, 0);
                                wtk_svprint_feed(kws->svprint, NULL, 0, 1);
                                wtk_free(pad_wav);
                            }
                        }else{
                            int len = kws->window_size;
                            char *value = kws->wav_buf->data + (kws->wav_buf->pos - kws->window_size);
                            qtk_kws_feed_vprint(kws,value,len);
                            kws->process_size = kws->wav_buf->pos;
                            kws->acc_size2 = 0;
                        }

                        if(kws->cfg->section_secs > 0){
                            if(kws->shift_size == -1 || kws->process_size == 0){
                                vec = wtk_svprint_compute_feat2(kws->svprint, &cnt);
                                mean = kws->svprint->x->spk_mean;
                                for (i = 0; i < mean->len; i++) {
                                    mean->p[i] += vec->p[i];
                                }
                                kws->svprint->x->spk_cnt++;
                                wtk_svprint_eval_Nbest(kws->svprint, mean, vec, kws->svprint->x->spk_cnt,&kws->sv_prob);
                            }
                            kws->svprint->x->spk_cnt = 0;
                            wtk_vecf_zero(kws->svprint->x->spk_mean);
                            if (kws->notify2) {
                                kws->notify2(kws->notify_ths2, kws->svprint->res_cnt, kws->svprint->nbest_res,
                                    kws->svprint->score);
                            }
                        }else{
                            name = wtk_svprint_eval(kws->svprint, &kws->sv_prob);
                            if (name != NULL) {
                                if (kws->notify) {
                                    kws->notify(kws->notify_ths, 3, name->data,
                                                name->len);
                                } else {
                                    printf("%.*s\n", name->len, name->data);
                                }
                            } else {
                                if(kws->notify) 
                                    kws->notify(kws->notify_ths, 3, NULL, 0);
                            }
                        }
                    } else {
                        wtk_svprint_feed(kws->svprint, NULL, 0, 1);
                        // qtk_torchnn_flush(kws->svprint_nn);
                        name = wtk_svprint_eval(kws->svprint, &kws->sv_prob);
                        // wtk_debug("%f\n",kws->sv_prob);
                        if (name != NULL) {
                            if (kws->notify) {
                                kws->notify(kws->notify_ths, 3, name->data,
                                            name->len);
                            } else {
                                printf("%.*s\n", name->len, name->data);
                            }
                        } else {
                            kws->notify(kws->notify_ths, 3, NULL, 0);
                        }
                    }
                }
            } else {
                if (kws->parm) {
                    wtk_kxparm_feed(kws->parm, (short *)data, bytes >> 1,
                                    is_end);
                }

                if(kws->eval)
                {
                    //wtk_debug("feed\n");
                    wtk_svprint_feed(kws->svprint,(short*)data,bytes>>1,is_end);
                    if(is_end)
                    {
                        //qtk_torchnn_flush(kws->svprint_nn);
                        name = wtk_svprint_eval(kws->svprint, &kws->sv_prob);
                        //wtk_debug("%f\n",kws->sv_prob);
                        if(name != NULL)
                        {
                            if(kws->notify)
                            {
                                kws->notify(kws->notify_ths,3,name->data,name->len);
                            }else
                            {
                                printf("%.*s\n",name->len,name->data);
                            }
                        } else {
                            kws->notify(kws->notify_ths, 3, NULL, 0);
                        }
                    }
                }

			}
        }
    }
    return 0;
}

void qtk_kws_on_vad2(qtk_kws_t *kws,wtk_vad2_cmd_t cmd,short *data,int len){
    switch (cmd)
    {
    case WTK_VAD2_START:
        qtk_kws_start(kws);
        break;
    case WTK_VAD2_DATA:
        qtk_kws_feed(kws,(char *)data,len<<1,0);
        break;
    case WTK_VAD2_END:
        qtk_kws_feed(kws,0,0,1);
        qtk_kws_reset(kws);
        break;
    case WTK_VAD2_CANCEL:
        break;
    default:
        break;
    }
}

void qtk_kws_feed2(qtk_kws_t *kws,char *data,int bytes,int is_end){
    wtk_vad2_feed(kws->vad2, data, bytes, is_end);
}

int qtk_kws_enroll(qtk_kws_t *kws,char *name,int len)
{
	if(len > 0)
	{
		kws->enroll = 1;
		wtk_strbuf_reset(kws->user);
		wtk_strbuf_push(kws->user,name,len);
	}else
	{
		kws->eval = 1;
	}

	return 0;
}

int qtk_kws_enroll_end(qtk_kws_t *kws) {
    int ret = 0;
    wtk_string_t v;

    if(!kws->svprint->enroll_available){
        return -1;
    }

    v.data = kws->user->data;
    v.len = kws->user->pos;
    ret = wtk_svprint_enroll2file(kws->svprint, &v);

    kws->enroll = 0;
    kws->eval = 1;
    wtk_svprint_reload(kws->svprint);
    return ret;
}

int qtk_kws_set_enroll_fn(qtk_kws_t *kws,char *fn, int len)
{
	if(kws->svprint)
	{
		kws->svprint->cfg->enroll_fn=fn;
		wtk_svprint_reload(kws->svprint);
	}
	return 0;
}

void qtk_kws_set_notify(qtk_kws_t * kws,qtk_kws_res_notify_f notify, void *ths)
{
	kws->notify = notify;
	kws->notify_ths = ths;
}

void qtk_kws_set_notify2(qtk_kws_t * kws,qtk_kws_res_notify2_f notify, void *ths)
{
	kws->notify2 = notify;
	kws->notify_ths2 = ths;
}

float qtk_kws_get_prob(qtk_kws_t *kws)
{
	return kws->sv_prob;
}

void qtk_kws_set_sv_thresh(qtk_kws_t *kws, float thresh)
{
	if(kws->svprint)
	{
		kws->svprint->cfg->score_thresh = thresh;
	}
}


void qtk_kws_set_max_spk(qtk_kws_t *kws, int max_spk){
    if(max_spk <= 60){
        kws->svprint->max_spk = max_spk;
    }
    if(kws->svprint->spk_cnt >= kws->svprint->max_spk){
        kws->svprint->enroll_available = 0;
    }
}

void qtk_kws_set_result_dur(qtk_kws_t *kws, float val){
	if(val <= 0.0){
		return;
	}
    kws->cfg->section_secs = val;
    if(val >= 2.0){
        kws->shift_size = -1;
    }else if(val >= 0.5){
        kws->shift_size = val * 32000;
    }else{
        kws->shift_size = 0.5 * 32000;
    }
}
