#include "qtk_sweetspot.h" 

qtk_sweetspot_gc_t *qtk_sweetspot_gc_new(){
    qtk_sweetspot_gc_t *gc = (qtk_sweetspot_gc_t*)wtk_malloc(sizeof(qtk_sweetspot_gc_t));

    gc->gain_set = (float*)wtk_calloc(20,sizeof(float));
    gc->gidx_result_in_truncation = (float*)wtk_calloc(20,sizeof(float));
    qtk_sweetspot_gc_reset(gc);
    int i,j = 0;
    float p;
    float factor = pow(10.0,2);
    for(i = -20; i < 20; i++){
        p = round(pow(gc->mu,i) * factor) / factor;
        if(p > 0 && p <= 1){
            gc->gain_set[j] = p;
            j++;
        }
    }
    gc->gain = gc->gain_set[0];
    gc->num_gain = j;

    return gc;
}


void qtk_sweetspot_gc_delete(qtk_sweetspot_gc_t *gc){
    wtk_free(gc->gain_set);
    wtk_free(gc->gidx_result_in_truncation);
    wtk_free(gc);
}

void qtk_sweetspot_gc_reset(qtk_sweetspot_gc_t *gc){
    gc->truncation_count = 0;
    gc->truncation_thre2Adjust_gain = 5;
    gc->mu = 0.9;
    gc->gidx = 0;
    gc->cold_delay = 200;
    gc->cold_delay_count_down = 0;
    memset(gc->gidx_result_in_truncation,0,sizeof(float)*20);
}

static int sp_check(float *p, int len){
    int i,ret = 0;
    float sum = 0.0,ff;
    for(i = 0; i < len; i++){
        ff = fabs(*p);
        if(ff > 0.98){
            sum += ff;
        }
        p++;
    }
    if(sum >= 10.0){
        ret = 1;
    }

    return ret;
}

void qtk_sweetspot_gc_run(qtk_sweetspot_gc_t *gc, float *frm, int len){
    if(sp_check(frm,len) == 1){
        gc->truncation_count += 1;
        if(gc->truncation_count > gc->truncation_thre2Adjust_gain){
            gc->truncation_count = 0;
            gc->gidx_result_in_truncation[gc->gidx] += 1;
            gc->gidx = min(gc->gidx + 1, gc->num_gain - 1);
            gc->gain = gc->gain_set[gc->gidx];
            gc->cold_delay_count_down = gc->cold_delay;
        }
    }else{
        if(gc->cold_delay_count_down > 0){
            gc->cold_delay_count_down -= 1;
        }else{
            int gidx = max(0, gc->gidx - 1);
            if(gc->gidx_result_in_truncation[gidx] <= 2){
                gc->gidx = gidx;
                gc->gain = gc->gain_set[gc->gidx];
                gc->cold_delay_count_down = gc->cold_delay;
            }
        }
    }
}

void qtk_sweetspot_gc_gain(qtk_sweetspot_gc_t *gc, float *frm, int len){
    int i;
    for(i = 0; i < len; i++){
        frm[i] *= gc->gain;
    }
}

qtk_sweetspot_t *qtk_sweetspot_new(qtk_sweetspot_cfg_t *cfg){
	qtk_sweetspot_t *sspot = wtk_malloc(sizeof(qtk_sweetspot_t));
	sspot->cfg = cfg;
	sspot->gain_factor = cfg->gain / cfg->speaker_distance;

	sspot->left = wtk_strbuf_new(256 * sizeof(float), 1);
	sspot->right = wtk_strbuf_new(256 * sizeof(float), 1);

	sspot->left_buff = (float *)wtk_malloc(cfg->max_N_delay * sizeof(float));
	sspot->right_buff = (float *)wtk_malloc(cfg->max_N_delay * sizeof(float));
	sspot->left_prev_half_win = (float *)wtk_malloc(sizeof(float) * cfg->hop_size);
	sspot->right_prev_half_win = (float *)wtk_malloc(sizeof(float) * cfg->hop_size);

	sspot->left_frame = (float *)wtk_malloc((cfg->hop_size * 2 + cfg->max_N_delay) * sizeof(float));
	sspot->right_frame = (float *)wtk_malloc((cfg->hop_size * 2 + cfg->max_N_delay) * sizeof(float));
	sspot->left_frame_delayed = (float *)wtk_malloc(cfg->hop_size * sizeof(float));
	sspot->right_frame_delayed = (float *)wtk_malloc(cfg->hop_size * sizeof(float));
	sspot->fs = 16000;
	sspot->output = (short **)wtk_malloc(sizeof(short*) * 2);
	sspot->output[0] = (short *)wtk_malloc(sizeof(short) * cfg->hop_size);
	sspot->output[1] = (short *)wtk_malloc(sizeof(short) * cfg->hop_size);
	sspot->notify = NULL;
	sspot->upval = NULL;

	qtk_ahs_limiter_init(&sspot->left_limitor, 0.002, 0.01, 0.01, 0.99, sspot->fs);
	qtk_ahs_limiter_init(&sspot->right_limitor, 0.002, 0.01, 0.01, 0.99, sspot->fs);
	sspot->gc = qtk_sweetspot_gc_new();
	qtk_sweetspot_reset(sspot);

	sspot->delay_left = 0;
	sspot->delay_right = 0;
	sspot->vol_left = 1.0;
	sspot->vol_right = 1.0;

	return sspot;
}

void qtk_sweetspot_delete(qtk_sweetspot_t *sspot){
	wtk_strbuf_delete(sspot->left);
	wtk_strbuf_delete(sspot->right);
	wtk_free(sspot->left_buff);
	wtk_free(sspot->right_buff);
	wtk_free(sspot->left_prev_half_win);
	wtk_free(sspot->right_prev_half_win);
	wtk_free(sspot->left_frame);
	wtk_free(sspot->right_frame);
	wtk_free(sspot->left_frame_delayed);
	wtk_free(sspot->right_frame_delayed);
	wtk_free(sspot->output[0]);
	wtk_free(sspot->output[1]);
	wtk_free(sspot->output);
	qtk_sweetspot_gc_delete(sspot->gc);
	wtk_free(sspot);
}

void qtk_sweetspot_reset(qtk_sweetspot_t *sspot){
	wtk_strbuf_reset(sspot->left);
	wtk_strbuf_reset(sspot->right);
	memset(sspot->left_buff,0,sizeof(float) * sspot->cfg->max_N_delay);
	memset(sspot->right_buff,0,sizeof(float) * sspot->cfg->max_N_delay);
	memset(sspot->left_prev_half_win,0,sizeof(float) * sspot->cfg->hop_size);
	memset(sspot->right_prev_half_win,0,sizeof(float) * sspot->cfg->hop_size);
}

static float db2gain(float in){
    return powf(10.0,in/10);
}

void qtk_sweetspot_update(qtk_sweetspot_t *sspot, float x, float y){
	float speakerDistance = sspot->cfg->speaker_distance/2;

	sspot->dr = sqrtf( powf( (double)speakerDistance - x, 2) + powf(y, 2));
	sspot->dl = sqrtf( powf( (double)speakerDistance + x, 2) + powf(y, 2));
	sspot->delay = 1000.0f * (sspot->dr - sspot->dl) / 34300.0;
	//wtk_debug("dr %f dl %f delay %f\n", sspot->dr, sspot->dl,sspot->delay);

	if(sspot->delay < 0.0f) {
		sspot->delay_left = 0.0f;
		sspot->delay_right = fabs(sspot->delay);
	} else if (sspot->delay == 0.0f) {
		sspot->delay_left = 0.0f;
		sspot->delay_right = 0.0f;
	} else {
		sspot->delay_left = fabs(sspot->delay);
		sspot->delay_right = 0.0f;
	}

	sspot->vol_left = 1.0 * sspot->dl/ max(sspot->dr, sspot->dl);
	sspot->vol_right = 1.0 * sspot->dr/ max(sspot->dr, sspot->dl);

	sspot->vol_left *= db2gain(3);
	sspot->vol_right *= db2gain(3);

	sspot->ndelay_left = sspot->delay_left * sspot->fs * 1.0f /1000;
	sspot->ndelay_right = sspot->delay_right * sspot->fs * 1.0f /1000;
	if(sspot->ndelay_left > sspot->cfg->max_N_delay || sspot->ndelay_right > sspot->cfg->max_N_delay){
		wtk_debug("error delay below max delay\n");
		exit(0);
	}
	// wtk_debug("ndelay_left %d\n", sspot->ndelay_left);
	// wtk_debug("ndelay_right %d\n", sspot->ndelay_right);
	// wtk_debug("vol_left %f\n", sspot->vol_left);
	// wtk_debug("vol_right %f\n", sspot->vol_right);
}

void qtk_sweetspot_set(qtk_sweetspot_t *sspot, float delay, float gain){
	sspot->vol_left = gain;
	sspot->vol_right = gain;
	sspot->ndelay_left = delay * sspot->fs;
	sspot->ndelay_right = delay * sspot->fs;
}

static void process_frame_(qtk_sweetspot_t *sspot, float *frame, float *buff, 
	float* frame_, float *frame_delayed, int delay, float gain, float *prev_half_win){
	qtk_sweetspot_cfg_t *cfg = sspot->cfg;
	int win_size = cfg->hop_size * 2;
	int i;
	float tmp_frame[512];

	memcpy(frame_, buff + cfg->max_N_delay - delay, sizeof(float) * delay);
	memcpy(frame_ + delay, frame, sizeof(float) * win_size);
	memcpy(buff,frame + win_size - cfg->max_N_delay, sizeof(float) * cfg->max_N_delay);
	for(i = 0; i < win_size; i++){
		tmp_frame[i] = frame_[i] * gain;
	}
	for(i = 0; i < cfg->hop_size; i++){
		frame_delayed[i] = (tmp_frame[i] * cfg->window[i] + prev_half_win[i] * cfg->window[i + cfg->hop_size]) / cfg->win_gain[i];
	}
	//print_float(frame_delayed,cfg->hop_size);
	memcpy(prev_half_win, tmp_frame + cfg->hop_size, sizeof(float) * cfg->hop_size);
}


void qtk_sweetspot_feed(qtk_sweetspot_t *sspot, short **data,int len,int is_end){
	int i;
	float fv;
	for(i=0;i<len;++i){
		fv = data[0][i] * 1.0 / 32768.0;
		wtk_strbuf_push_float(sspot->left,&fv,1);
		fv = data[1][i] * 1.0 / 32768.0;
		wtk_strbuf_push_float(sspot->right,&fv,1);
	}

	int wav_len = sspot->left->pos/sizeof(float);
	int fsize = sspot->cfg->hop_size * 2;
	while(wav_len > fsize){
		process_frame_(sspot, (float *)sspot->left->data, sspot->left_buff, sspot->left_frame,
		 sspot->left_frame_delayed, sspot->ndelay_left, sspot->vol_left, sspot->left_prev_half_win);
		qtk_ahs_limiter_process(&sspot->left_limitor, sspot->left_frame_delayed, sspot->cfg->hop_size);
		qtk_sweetspot_gc_gain(sspot->gc, sspot->left_frame_delayed, sspot->cfg->hop_size);

		process_frame_(sspot, (float *)sspot->right->data, sspot->right_buff, sspot->right_frame,
		 sspot->right_frame_delayed, sspot->ndelay_right, sspot->vol_right, sspot->right_prev_half_win);
		qtk_ahs_limiter_process(&sspot->right_limitor, sspot->right_frame_delayed, sspot->cfg->hop_size);
		qtk_sweetspot_gc_gain(sspot->gc, sspot->right_frame_delayed, sspot->cfg->hop_size);

		if(sspot->vol_left > sspot->vol_right){
			qtk_sweetspot_gc_run(sspot->gc, sspot->left_frame_delayed, sspot->cfg->hop_size);
		}else{
			qtk_sweetspot_gc_run(sspot->gc, sspot->right_frame_delayed, sspot->cfg->hop_size);
		}

		for(i = 0; i < sspot->cfg->hop_size; i++){
			sspot->output[0][i] = sspot->left_frame_delayed[i] * 32768.0;
			sspot->output[1][i] = sspot->right_frame_delayed[i] * 32768.0;
		}

		if(sspot->notify){
			sspot->notify(sspot->upval, sspot->output, sspot->cfg->hop_size);
		}

		wtk_strbuf_pop(sspot->left,NULL,sspot->cfg->hop_size * sizeof(float));
		wtk_strbuf_pop(sspot->right,NULL,sspot->cfg->hop_size * sizeof(float));
		wav_len = sspot->left->pos/sizeof(float);
	}
}

void qtk_sweetspot_set_notify(qtk_sweetspot_t *sspot, void *upval, qtk_sweetspot_notify_f notify){
    sspot->upval = upval;
    sspot->notify = notify;
}