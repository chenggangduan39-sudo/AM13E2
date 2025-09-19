#include "qtk_wdec.h"

#define FEED_STEP (32*16*2)

// void qtk_wdec_on_wdec_wake(qtk_wdec_t *wdec,wtk_wdec_cmd_t cmd,float fs,float fe,short *data, int len);

qtk_wdec_t *qtk_wdec_new(qtk_wdec_cfg_t *cfg, qtk_session_t *session) 
{
    qtk_wdec_t *wdec;
    int ret=0;

    wdec = (qtk_wdec_t *)wtk_calloc(1,sizeof(*wdec));

    wdec->cfg = cfg;
    wdec->session = session;
    wdec->wdec=NULL;
    wtk_lock_init(&wdec->lock);
    if(wdec->cfg->use_wdec){
        if(wdec->cfg->wdec_cfg){
            wdec->wdec = wtk_wdec_new(wdec->cfg->wdec_cfg);
            if(!wdec->wdec){
                ret = -1;
                goto end;
            }
        }
    }else if(wdec->cfg->use_kwdec2){
        if(wdec->cfg->kwdec2_cfg){
            wdec->kwdec2 = wtk_kwdec2_new(wdec->cfg->kwdec2_cfg);
            if(!wdec->kwdec2){
                ret = -1;
                goto end;
            }
            if(wdec->cfg->words.len > 0){
                wtk_kwdec2_set_words(wdec->kwdec2, wdec->cfg->words.data, wdec->cfg->words.len);
                wtk_kwdec2_set_words_cfg(wdec->kwdec2, wdec->cfg->words.data, wdec->cfg->words.len);
                wtk_kwdec2_decwords_set(wdec->kwdec2);
            }
        }
    }
    if(wdec->cfg->use_vad){
        wtk_queue_init(&wdec->vad_q);
	    wdec->vad = wtk_vad_new(cfg->vad_cfg, &wdec->vad_q);
    }
end:
    if(ret !=0 ){
        qtk_wdec_delete(wdec);
        wdec=NULL;
        wtk_debug("wdec new fail\n");
    }else{
        wtk_debug("wdec new sucess\n");
    }
    return wdec;
}

void qtk_wdec_delete(qtk_wdec_t *wdec) 
{
    if(wdec->vad) {
		wtk_vad_delete(wdec->vad);
	}
    if(wdec->cfg->use_wdec){
        if(wdec->wdec){
            wtk_wdec_delete(wdec->wdec);
        }
    }else if(wdec->cfg->use_kwdec2){
        if(wdec->kwdec2){
            wtk_kwdec2_delete(wdec->kwdec2);
        }
    }
    wtk_lock_clean(&wdec->lock);
    
    wtk_debug("wdec delete sucess\n");
    wtk_free(wdec);
}

int qtk_wdec_start(qtk_wdec_t *wdec) 
{
    if(wdec->cfg->use_vad){
        wdec->sil = 1;
        wtk_vad_start(wdec->vad);
    }else{
        if(wdec->cfg->use_wdec){
            wtk_wdec_start(wdec->wdec,NULL);
        }else if(wdec->cfg->use_kwdec2){
            wtk_kwdec2_start(wdec->kwdec2);
        }
    }
    wtk_debug("wdec start sucess\n");
    return 0;
}

int qtk_wdec_reset(qtk_wdec_t *wdec) 
{
    if(wdec->cfg->use_vad){
        wtk_vad_reset(wdec->vad);
    }
    if(wdec->cfg->use_wdec){
        wtk_wdec_reset(wdec->wdec);
    }else if(wdec->cfg->use_kwdec2){
        wtk_kwdec2_reset(wdec->kwdec2);
    }
    wtk_debug("wdec reset sucess\n");
    return 0;
}

void qtk_wdec_get_wake(qtk_wdec_t *wdec)
{
    if(wdec->cfg->use_wdec){
        if(wdec->wdec->found){
            float fs,fe;
            wtk_lock_lock(&wdec->lock);
            
            wtk_wdec_get_final_time(wdec->wdec,&fs,&fe);
            wdec->wake_prob=wtk_wdec_get_conf(wdec->wdec);
            wtk_debug("wake_prob=[%f] [%f,%f]\n",wdec->wake_prob,fs,fe);
            qtk_var_t evar;
            evar.type = QTK_AEC_WAKE;
            evar.v.fi.theta=wdec->wake_prob;
            if (wdec->enotify) {
                wdec->enotify(wdec->eths, &evar);
            }
            wtk_wdec_feed(wdec->wdec, NULL, 0 , 1);
            wtk_wdec_reset(wdec->wdec);
            wtk_wdec_start(wdec->wdec,NULL);
            wtk_lock_unlock(&wdec->lock);
        }
    }else if(wdec->cfg->use_kwdec2){
        if(wdec->kwdec2->found){
            float fs,fe;
            wtk_kwdec2_get_wake_time(wdec->kwdec2, &fs, &fe);
            wtk_debug("wake-time[%f,%f]\n", fs, fe);
            qtk_var_t evar;
            evar.type = QTK_AEC_WAKE;
            evar.v.f=wdec->wake_prob;
            if (wdec->enotify) {
                wdec->enotify(wdec->eths, &evar);
            }
            wtk_kwdec2_feed2(wdec->kwdec2, NULL, 0, 1);
            wtk_kwdec2_reset(wdec->kwdec2);
            wtk_kwdec2_start(wdec->kwdec2);
        }
    }
}

int qtk_wdec_on_feed(qtk_wdec_t *wdec, char *data, int bytes , int is_end) 
{
    int ret;
    qtk_var_t evar;

    if(wdec->cfg->use_wdec){
        ret = wtk_wdec_feed(wdec->wdec, data, bytes , is_end);
    }else if(wdec->cfg->use_kwdec2){
        ret = wtk_kwdec2_feed2(wdec->kwdec2, (short *)data, bytes>>1, is_end);
    }
    qtk_wdec_get_wake(wdec);
    return ret;
}

int qtk_wdec_on_feed2(qtk_wdec_t *wdec, char *data, int bytes , int is_end)
{
	wtk_queue_t *vad_q = &(wdec->vad_q);
	wtk_queue_node_t *qn;
	qtk_msg_node_t *node;	
	wtk_vframe_t *f;

	wtk_vad_feed(wdec->vad,data,bytes,is_end);
	while(1){
		qn = wtk_queue_pop(vad_q);
		if(!qn) {
			break;
		}
		f = data_offset2(qn,wtk_vframe_t,q_n);

		if(wdec->sil) {
			if(f->state == wtk_vframe_speech) {
                if(wdec->cfg->use_wdec){
                    wtk_wdec_start(wdec->wdec,NULL);
                }else if(wdec->cfg->use_kwdec2){
                    wtk_kwdec2_start(wdec->kwdec2);
                }
				wdec->sil = 0;
				if(wdec->enotify) {
                    qtk_var_t evar;
                    evar.type = QTK_SPEECH_START;
                    wdec->enotify(wdec->eths, &evar);
				}
			}
		} else {
			if(f->state == wtk_vframe_sil) {
				if(wdec->enotify) {
                    qtk_var_t evar;
                    evar.type = QTK_SPEECH_END;
                    wdec->enotify(wdec->eths, &evar);
				}
				wdec->sil = 1;
                if(wdec->cfg->use_wdec){
                    wtk_wdec_feed(wdec->wdec, NULL, 0 , 1);
                    qtk_wdec_get_wake(wdec);
                    wtk_wdec_reset(wdec->wdec);
                }else if(wdec->cfg->use_kwdec2){
                    wtk_kwdec2_feed2(wdec->kwdec2, NULL, 0, 1);
                    qtk_wdec_get_wake(wdec);
                    wtk_kwdec2_reset(wdec->kwdec2);
                }
			} else {
                if(wdec->cfg->use_wdec){
                    wtk_wdec_feed(wdec->wdec, (char*)f->wav_data, f->frame_step<<1 , 0);
                }else if(wdec->cfg->use_kwdec2){
                    wtk_kwdec2_feed2(wdec->kwdec2, f->wav_data, f->frame_step, 0);
                }
                qtk_wdec_get_wake(wdec);
			}
		}
		wtk_vad_push_vframe(wdec->vad,f);
	}
	if(is_end && !wdec->sil) {
		if(wdec->enotify) {
            qtk_var_t evar;
            evar.type = QTK_SPEECH_END;
            wdec->enotify(wdec->eths, &evar);
		}
        if(wdec->cfg->use_wdec){
            wtk_wdec_feed(wdec->wdec, NULL, 0 , 1);
            qtk_wdec_get_wake(wdec);
            wtk_wdec_reset(wdec->wdec);
        }else if(wdec->cfg->use_kwdec2){
            wtk_kwdec2_feed2(wdec->kwdec2, NULL, 0, 1);
            qtk_wdec_get_wake(wdec);
            wtk_kwdec2_reset(wdec->kwdec2);
        }
	}
}

int qtk_wdec_feed(qtk_wdec_t *wdec, char *data, int bytes, int is_end) 
{
	int pos = 0;
	int step = 0;
	int flen;
	//一次最多传200毫秒的数据
    step = FEED_STEP;
    // step = FEED_STEP * wdec->channel;
	
	while(pos < bytes){
		flen = min(step, bytes - pos);
        if(wdec->cfg->use_vad){
            qtk_wdec_on_feed2(wdec, data + pos, flen, 0);
        }else{
		    qtk_wdec_on_feed(wdec, data + pos, flen, 0);
        }
		pos += flen;
	}
	if(is_end){
        if(wdec->cfg->use_vad){
            qtk_wdec_on_feed2(wdec, NULL, 0, 1);
        }else{
		    qtk_wdec_on_feed(wdec, NULL, 0, 1);
        }
	}
	return 0;
}

void qtk_wdec_set_notify_info(qtk_wdec_t *wdec, void *ths_info,qtk_wdec_notify_f notify_info) 
{
    wtk_debug("qtk_wdec_set_notify_info\n");
	wdec->ths_info = ths_info;
	wdec->notify_info = notify_info;
}

void qtk_wdec_set_notify(qtk_wdec_t *wdec, void *eths,qtk_engine_notify_f notify_f) 
{
    wtk_debug("qtk_wdec_set_notify\n");
	wdec->eths = eths;
	wdec->enotify = notify_f;
}

void qtk_wdec_set_wake_words(qtk_wdec_t *wdec,char *data,int bytes)
{
    if(wdec->cfg->use_wdec){
        wtk_wdec_set_words(wdec->wdec,data,bytes);
    }else if(wdec->cfg->use_kwdec2){
        wtk_kwdec2_set_words(wdec->kwdec2, data, bytes);
        wtk_kwdec2_set_words_cfg(wdec->kwdec2, data, bytes);
        wtk_kwdec2_decwords_set(wdec->kwdec2);
    }
}
// void qtk_wdec_on_wdec_wake(qtk_wdec_t *wdec,wtk_wdec_cmd_t cmd,float fs,float fe,short *data, int len)
// {
//     qtk_var_t evar;
//     memset(&evar,0,sizeof(evar));

//     switch (cmd) 
//     {
//     case WTK_wdec_WAKE:
//         // wake->waked=1;
//         wdec->wake_fs = fs;
//         wdec->wake_fe = fe;
//         wdec->wake_prob = wtk_wdec_get_conf(wdec->wdec);
//         wtk_debug("====>>wake_fs/fe [%f %f] wake_prob\n", fs, fe,wdec->wake_prob);
//         evar.type = QTK_AEC_WAKE;
//         if (wdec->enotify) 
//         {
//             wdec->enotify(wdec->eths, &evar);
//         }
//             break;
//     default:
//             break;
//     }
// }