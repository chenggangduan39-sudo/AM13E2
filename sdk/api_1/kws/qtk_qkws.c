#include "qtk_qkws.h"

void qtk_qkws_on_kws(qtk_qkws_t *qk, int res, char *name, int name_len);
void qtk_qkws_on_kws2(qtk_qkws_t *qk, int cnt, wtk_string_t **res, float *scores);
void qtk_qkws_on_vboxebf(qtk_qkws_t *qk, char *data, int len);
void qtk_qkws_on_cmask_pse(qtk_qkws_t *qk, short *output, int len);
void qtk_qkws_on_feat_pse(qtk_qkws_t *qk, float *output, int len);
void qtk_qkws_on_aec(qtk_qkws_t *qk, qtk_var_t *var);
int qtk_qkws_feed2(qtk_qkws_t *qk, char *data, int bytes, int is_end);

static void qtk_qkws_init(qtk_qkws_t *qk) {
    qk->cfg = NULL;
    qk->kws = NULL;
    qk->cmask_pse = NULL;
    qk->vad = NULL;
    qk->sc = NULL;
    qk->vboxebf = NULL;
    qk->aec = NULL;

    qk->session = NULL;
    qk->rlt_buf = NULL;
    qk->input_buf = NULL;
    qk->result_buf = NULL;
    qk->score_buf = NULL;
    qk->outbuf = NULL;
    qk->this = NULL;
    qk->efp = NULL;
    qk->enroll_path = NULL;
    qk->zdata = NULL;
    qk->is_start = 0;
}

qtk_qkws_t *qtk_qkws_new(qtk_qkws_cfg_t *cfg, qtk_session_t *session) {
    qtk_qkws_t *qk;
    int ret=-1;

    qk = (qtk_qkws_t *)wtk_malloc(sizeof(qtk_qkws_t));
    qtk_qkws_init(qk);

    qk->cfg = cfg;
    qk->session = session;

    if(qk->cfg->use_kws){
        qk->kws = qtk_kws_new(qk->cfg->kws_cfg);
#ifdef USE_KSD_EBNF
        qtk_kws_set_notify(qk->kws, (qtk_kws_res_notify_f)qtk_qkws_on_kws, qk);
#else
        qtk_kws_set_notify2(qk->kws, (qtk_kws_res_notify2_f)qtk_qkws_on_kws2, qk);
        qtk_kws_set_result_dur(qk->kws, qk->cfg->result_dur);
#endif
    }
    if(qk->cfg->use_sc){
        qk->sc = qtk_sond_cluster_new(qk->cfg->sc_cfg);
    }
    if(qk->cfg->use_cmask_pse){
        qk->cmask_pse = wtk_cmask_pse_new(qk->cfg->cmask_pse_cfg);
        wtk_cmask_pse_set_notify(qk->cmask_pse, qk, (wtk_cmask_pse_notify_f)qtk_qkws_on_cmask_pse);
        qk->is_start_enroll = 0;
        qk->enroll_path = NULL;
        qk->efp = NULL;
    }
    if(qk->cfg->use_vad){
        if (cfg->xvad) {
            qk->vad = wtk_vad_new(cfg->xvad, 0);
        } else {
            qk->vad = wtk_vad_new(&cfg->vad, 0);
        }
    }
    if(qk->cfg->use_vboxebf){
        qk->vboxebf = qtk_vboxebf_new(cfg->vbox_cfg);
        qtk_vboxebf_set_notify(qk->vboxebf, qk, (qtk_vboxebf_notify_f)qtk_qkws_on_vboxebf);
    }
    if(qk->cfg->use_aec){
        qk->aec = qtk_aec_new(cfg->aec_cfg);
        qtk_aec_set_notify2(qk->aec, qk, (qtk_engine_notify_f)qtk_qkws_on_aec);
    }
    qk->rlt_buf = wtk_strbuf_new(512, 1);
    qk->input_buf = wtk_strbuf_new(1024, 1);
    qk->result_buf = wtk_strbuf_new(1024, 1.0);
    qk->score_buf = wtk_strbuf_new(1024, 1.0);
    if(qk->cfg->use_add_zero){
        qk->outbuf = wtk_strbuf_new(1024*16, 1.0);
    }
    qk->zdata = (char *)wtk_malloc(1024*16);
    memset(qk->zdata, 0, 1024*16);
    qk->is_set_number = 0;

    wtk_debug("qtk_qkws_new==>>use_kws=%d use_sc=%d use_cmask_pse=%d\n",qk->cfg->use_kws,qk->cfg->use_sc,qk->cfg->use_cmask_pse);
    ret = 0;
end:
    if (ret != 0) {
        qtk_qkws_delete(qk);
        qk = NULL;
    }
    return qk;
}

void qtk_qkws_delete(qtk_qkws_t *qk)
{
    wtk_debug("qtk_qkws_delete==================>>>>>>>>>\n");
    if(qk->cfg->use_vboxebf){
        if(qk->vboxebf){
            qtk_vboxebf_delete(qk->vboxebf);
        }
    }
    if(qk->cfg->use_aec){
        if(qk->aec){
            qtk_aec_delete(qk->aec);
        }
    }
    if(qk->cfg->use_vad){
        if (qk->vad) {
            wtk_vad_delete(qk->vad);
        }
    }
    if(qk->cfg->use_kws){
        if(qk->kws){
            qtk_kws_delete(qk->kws);
        }
    }
    if(qk->cfg->use_sc){
        if(qk->sc){
            qtk_sond_cluster_delete(qk->sc);
        }
    }
    if(qk->cfg->use_cmask_pse){
        if(qk->cmask_pse){
            wtk_cmask_pse_delete(qk->cmask_pse);
        }
    }
    if (qk->rlt_buf) {
        wtk_strbuf_delete(qk->rlt_buf);
    }
    if (qk->input_buf) {
        wtk_strbuf_delete(qk->input_buf);
    }
    if(qk->result_buf){
        wtk_strbuf_delete(qk->result_buf);
    }
    if(qk->score_buf){
        wtk_strbuf_delete(qk->score_buf);
    }
    if(qk->outbuf){
        wtk_strbuf_delete(qk->outbuf);
    }
    if(qk->zdata){
        wtk_free(qk->zdata);
    }
    wtk_free(qk);
}

void qtk_qkws_set_notify(qtk_qkws_t *qk, void *ths, qtk_engine_notify_f notify)
{
	qk->this=ths;
	qk->notify=notify;
}

int qtk_qkws_start(qtk_qkws_t *qk)
{
    int ret=0;

    wtk_debug("qtk_qkws_start==================>>>>>>>>>%d is_start=%d\n",__LINE__,qk->is_start);
    wtk_strbuf_reset(qk->rlt_buf);
    if(qk->cfg->use_add_zero){
        wtk_strbuf_reset(qk->outbuf);
    }
    if(qk->is_start == 0){
        wtk_strbuf_reset(qk->input_buf);
        if(qk->cfg->use_kws){
            ret = qtk_kws_start(qk->kws);
            if(ret != 0){goto end;}
        }
        if(qk->cfg->use_sc){
            wtk_debug("qtk_qkws_start==================>>>>>>>>>%d eval=%d enroll=%d use_clu=%d\n",__LINE__,qk->sc->eval,qk->sc->enroll,qk->sc->cfg->use_clu);
            if(qk->sc->eval == 1 && qk->sc->enroll==0){
                if(qk->sc->cfg->use_clu==0){
                    ret = qtk_sond_cluster_prepare(qk->sc);
                    if(ret != 0){goto end;}
                    // qtk_sond_cluster_prepare2(qk->sc);
                }
                if(qk->is_set_number == 0){
                    qtk_sond_cluster_set_spk_nums(qk->sc, 1);
                }
            }
            ret = qtk_sond_cluster_start(qk->sc);
            if(ret != 0){goto end;}
        }
        if(qk->cfg->use_cmask_pse){
            if(qk->is_start_enroll){
                wtk_cmask_pse_new_vp(qk->cmask_pse);
                wtk_cmask_pse_set_notify2(qk->cmask_pse, qk, (wtk_cmask_pse_notify_f2)qtk_qkws_on_feat_pse);
            }else{
                // if(qk->enroll_path != NULL){
                //     wtk_debug("enroll path=%s\n",qk->enroll_path);
                //     FILE *fp;
                //     float *feat;
                //     fp=fopen(qk->enroll_path,"rb");
                //     if(fp==NULL){wtk_debug("open %s failed\n",qk->enroll_path);goto end;}
                //     feat=(float *)wtk_malloc(sizeof(float)*qk->cmask_pse->feat_len);
                //     fread(feat,sizeof(float), qk->cmask_pse->feat_len, fp);
                //     fclose(fp);
                //     wtk_debug("============>>>>>>>>>>>>>>>>>pse start<<<<<<<<<<<<<<<<<<<=========\n");
                //     wtk_debug("===================+>>>>>>>>>>>>>feat_len=%d feat=%f/%f/%f/%f\n",qk->cmask_pse->feat_len, feat[20], feat[200], feat[1000], feat[1500]);
                //     wtk_cmask_pse_start_vp_feat(qk->cmask_pse, feat, qk->cmask_pse->feat_len);
                //     wtk_free(feat);
                // }else{
                //     wtk_debug("enroll path is null\n");
                // }
                qtk_qkws_set_feat(qk, qk->enroll_path);
                wtk_cmask_pse_start(qk->cmask_pse);
            }
        }
        if(qk->cfg->use_vad){
            qk->sil = 1;
            qk->valid = 0;
            qk->cancel = 0;
            // wtk_vad_set_margin(qk->vad, left, right);
            wtk_vad_start(qk->vad);
        }
        if(qk->cfg->use_vboxebf){
            qtk_vboxebf_start(qk->vboxebf);
        }
        if(qk->cfg->use_aec){
            qtk_aec_start(qk->aec);
        }
        qk->is_start=1;
    }
    ret = 0;
end:
    return ret;
}

int qtk_qkws_reset(qtk_qkws_t *qk)
{
    int ret = 0;

    if(qk->is_start){
        if(qk->cfg->use_vboxebf){
            if(qk->cfg->use_cmask_pse){
                if(qk->is_start_enroll){
                    wtk_cmask_pse_feed_vp(qk->cmask_pse, NULL, 0, 1);
                }else{
                    wtk_cmask_pse_feed(qk->cmask_pse, NULL, 0, 1);
                }
            }
            if(qk->cfg->use_kws || qk->cfg->use_sc){
                qtk_qkws_feed2(qk, NULL, 0, 1);
            }
            qtk_vboxebf_reset(qk->vboxebf);
        }
        if(qk->cfg->use_aec){
            if(qk->is_start_enroll){
                wtk_cmask_pse_feed_vp(qk->cmask_pse, NULL, 0, 1);
            }else{
                wtk_cmask_pse_feed(qk->cmask_pse, NULL, 0, 1);
            }
            qtk_aec_reset(qk->aec);
        }

        if(qk->cfg->use_vad){
            wtk_vad_reset(qk->vad);
        }
        wtk_debug("qtk_qkws_reset==================>>>>>>>>>\n");
        if(qk->cfg->use_kws){
            ret = qtk_kws_reset(qk->kws);
        }
        if(qk->cfg->use_sc){
            ret = qtk_sond_cluster_reset(qk->sc);
        }
        if(qk->cfg->use_cmask_pse){
            if(qk->is_start_enroll){
                wtk_cmask_pse_reset_vp(qk->cmask_pse);
                wtk_cmask_pse_delete_vp(qk->cmask_pse);
            }else{
                wtk_cmask_pse_reset(qk->cmask_pse);
            }
        }
        qk->is_start=0;
    }
    return ret;
}

void qtk_qkws_cancel(qtk_qkws_t *qk)
{
    
}

/**
 * 2.仅注册使用的接口,每注册一个人,这两个接口就都需要调用一次
*/
int qtk_qkws_set_enroll(qtk_qkws_t *qk, char *name, int len, int is_end)
{
    wtk_debug("qtk_qkws_set_enroll==================>>>>>>>>>name=%s len=%d is_end=%d\n",name,len,is_end);
    int ret=-1;
    if(is_end < 1){
        /**
         * feed音频之前,指定注册说话人的id, 如果name不为空 声纹会切换到声纹注册模式,为空则是声纹识别模式
        */
        // printf("enroll==>name=[%s] len=%d\n",name, len);
        if(qk->cfg->use_kws){
            ret = qtk_kws_enroll(qk->kws, name, len);
        }
        if(qk->cfg->use_sc){
            ret = qtk_sond_cluster_enroll(qk->sc, name, len);
        }
        if(qk->cfg->use_cmask_pse){
            qk->is_start_enroll = 1;
        }
    }
    if(is_end==1){
        /**
         * feed音频结束之后,注册好的声纹保存到文件里,这个接口调用之后,声纹会切换到声纹识别模式
         */
        // printf("enroll_end==>\n");
        if(qk->cfg->use_kws){
            ret = qtk_kws_enroll_end(qk->kws);
        }
        if(qk->cfg->use_sc){
            ret = qtk_sond_cluster_enroll_end(qk->sc);
        }
        if(qk->cfg->use_cmask_pse){
            qk->is_start_enroll = 0;
        }
    }
    return ret;
}

/**
 * 设置注册人数的限制,超过人数的时候 注册送音频的返回值是 -1 qtk_kws_enroll_end也会返回-1
 */
void qtk_qkws_set_max_people(qtk_qkws_t *qk, int max_people)
{
    wtk_debug("qtk_qkws_set_max_people==================>>>>>>>>>max_people=%d\n",max_people);
    if(max_people > 0){
        if(qk->cfg->use_kws){
            qtk_kws_set_max_spk(qk->kws, max_people);
        }
    }
}

/**
 * 3.注册的声纹文件相关接口
 * 清除注册好的声纹,不会删除文件,调用后 原来注册的所有声纹都不会生效
 */
void qtk_qkws_clean(qtk_qkws_t *qk)
{
    wtk_debug("qtk_qkws_clean==================>>>>>>>>>\n");
    if(qk->cfg->use_kws){
        qtk_kws_clean(qk->kws);
    }
    if(qk->cfg->use_sc){
        qtk_sond_cluster_clean(qk->sc);
    }
}

/**
 * 注册声纹文件重新加载 重新读取并加载文件里的声纹
*/
void qtk_qkws_reload(qtk_qkws_t *qk)
{
    wtk_debug("qtk_qkws_reload==================>>>>>>>>>\n");
    if(qk->cfg->use_kws){
        qtk_kws_reload(qk->kws);
    }
    if(qk->cfg->use_sc){
        qtk_sond_cluster_reload(qk->sc);
    }
}

/**
 * 设置声纹文件   vp.bin.idx  vp.bin.data  fn传入"vp.bin",接口里面会调用qtk_kws_reload,不需要在外面重新加载
*/
int qtk_qkws_set_enroll_fn(qtk_qkws_t *qk, char *fn, int len)
{
    wtk_debug("qtk_qkws_set_enroll_fn==================>>>>>>>>>fn=%s len=%d\n",fn,len);
    if(qk->cfg->use_kws){
        return qtk_kws_set_enroll_fn(qk->kws, fn, len);
    }
    if(qk->cfg->use_sc){
        return qtk_sond_cluster_set_enroll_fn(qk->sc, fn, len);
    }
    if(qk->cfg->use_cmask_pse){
        qk->enroll_path = fn;
        wtk_debug("=================>>>>>>>>>>>is_start=%d\n",qk->is_start);
        if(qk->is_start){
            qtk_qkws_set_feat(qk, fn);
        }
        return 0;
    }
    return -1;
}

/**
 * 获取当前的声纹文件名
*/
char* qtk_qkws_get_fn(qtk_qkws_t *qk)
{
    if(qk->cfg->use_kws){
        return qtk_kws_get_fn(qk->kws);
    }
    if(qk->cfg->use_sc){
        return qtk_sond_cluster_get_fn(qk->sc);
    }
    if(qk->cfg->use_cmask_pse){
        return qk->enroll_path;
    }
    return NULL;
}

/**
 * 4.声纹识别结果判决相关
 * 声纹识别音频feed end时候,这个接口可以拿到声纹匹配到的最高的得分 
*/
float qtk_qkws_get_prob(qtk_qkws_t *qk)
{
    if(qk->cfg->use_kws){
        return qtk_kws_get_prob(qk->kws);
    }
    return 0.0f;
}

/**
 * 设置声纹匹配得分的阈值,0-1.0之间 越大越严格 对应enroll.cfg中的 score_thresh=0.3;
*/
void qtk_qkws_set_sv_thresh(qtk_qkws_t *qk, float thresh)
{
    wtk_debug("qtk_qkws_set_sv_thresh==================>>>>>>>>>thresh=%f\n",thresh);
    if(qk->cfg->use_kws){
        qtk_kws_set_sv_thresh(qk->kws, thresh);
    }
}

int qtk_qkws_set_vad_time(qtk_qkws_t *qk, float vs, float ve)
{
    static float vadtime=0.0f;
    if(qk->cfg->use_sc){
        vadtime+=(ve-vs);
        wtk_debug("qtk_qkws_set_vad_time=====================>>>>.st=%f et=%f vadtime=%f\n",vs,ve,vadtime);
        return qtk_sond_cluster_set_vad_time(qk->sc, vs, ve);
    }
    return -1;
}

int qtk_qkws_set_spk_nums(qtk_qkws_t *qk, int num)
{
    if(qk->cfg->use_sc){
        qk->is_set_number=1;
        wtk_debug("====================>>>>>>>>spk nums=%d\n",num);
        return qtk_sond_cluster_set_spk_nums(qk->sc, num);
    }
    return -1;
}

void qtk_qkws_set_result_dur(qtk_qkws_t *qk, int val)
{
#ifndef USE_KSD_EBNF
    if(qk->cfg->use_kws){
        qtk_kws_set_result_dur(qk->kws, val);
    }
#endif
}

int qtk_qkws_feed2(qtk_qkws_t *qk, char *data, int bytes, int is_end)
{
    int ret = 0;

    if(qk->cfg->use_vad){
        float eslen=0.0f;
        static float feedsum=0.0f;
        static int feed_bytes=0;
        static float start_time=0;
        static float end_time=0;
        static int vad_count=1;
        wtk_queue_t *vad_q = (qk->vad->output_queue);
        wtk_vframe_t *f;
        wtk_queue_node_t *qn;
        // wtk_json_parser_t *parser = NULL;
        wtk_json_item_t *result = NULL;
        // wtk_strbuf_t *buf = NULL;
        wtk_string_t v;
        qtk_var_t var;
        int ret;
        double t1, t2;

        // parser = wtk_json_parser_new();
        // buf = wtk_strbuf_new(256, 1);
        wtk_vad_feed(qk->vad, data, bytes, is_end);
        while (1) {
            qn = wtk_queue_pop(vad_q);
            if (!qn) {
                break;
            }
            f = data_offset2(qn, wtk_vframe_t, q_n);

            feed_bytes+=(f->frame_step << 1)/32;
            if (qk->cancel) {
                wtk_vad_push_vframe(qk->vad, f);
                continue;
            }

            if (qk->sil) {
                if (f->state == wtk_vframe_speech) {
                    qk->sil = 0;
                    start_time=feed_bytes/1000.0;
                    //speech start
                    // (char *)f->wav_data, f->frame_step << 1
                    // qtk_sond_cluster_feed(qk->sc, (char *)f->wav_data, f->frame_step << 1, 0);
                    wtk_strbuf_push(qk->rlt_buf, (char *)f->wav_data, f->frame_step << 1);
                }
            } else {
                if (f->state == wtk_vframe_sil) {
                    end_time=feed_bytes/1000.0;
                    // wtk_log_log(c->session->log,"vad第%d段音频: start_time==%fms end_time==%fms",vad_count,start_time,end_time);
                    eslen =  end_time - start_time;
                    feedsum += eslen;
                    wtk_debug("vad第%d段音频: start_time==%fms end_time==%fms feedsum=%fms\n",vad_count,start_time,end_time,feedsum);
                    qtk_qkws_set_vad_time(qk, start_time, end_time);
                    vad_count++;
                    qtk_sond_cluster_feed(qk->sc, qk->rlt_buf->data, qk->rlt_buf->pos, 0);
                    wtk_strbuf_reset(qk->rlt_buf);
                    qtk_var_t var;
                    qtk_qkws_get_result(qk, &var);
                    wtk_debug("=====================>>>>>>>>.get result=[%.*s]\n",var.v.str.len,var.v.str.data);
                    qk->sil = 1;
                    //speech end
                } else {
                    //speech data
                    // (char *)f->wav_data, f->frame_step << 1
                    // qtk_sond_cluster_feed(qk->sc, (char *)f->wav_data, f->frame_step << 1, 0);
                    wtk_strbuf_push(qk->rlt_buf, (char *)f->wav_data, f->frame_step << 1);
                }
            }
            wtk_vad_push_vframe(qk->vad, f);
        }
    }else{
        if(qk->is_start){
            if(qk->cfg->use_kws){
                if(qk->cfg->use_mode == 0){
                    ret = qtk_kws_feed(qk->kws, data, bytes, is_end);
                }
                if(qk->cfg->use_mode == 1){
#ifndef USE_KSD_EBNF
                    qtk_kws_feed2(qk->kws, data, bytes, is_end);
#endif
                    ret = 0;
                }
            }
            if(qk->cfg->use_sc){
                // double tm;
                // tm = time_get_ms();
                // wtk_debug("qtk_sond_cluster_feed==========1111111111111111====================>>>>>>>>>>>>>>>>>>bytes=%d\n",bytes);
                ret = qtk_sond_cluster_feed(qk->sc, data, bytes, is_end);
                // wtk_debug("qtk_sond_cluster_feed==============+>>>>>>>>>>>>>>>>>.feed bytes=%d tm=%f\n",bytes,time_get_ms() - tm);
            }
        }
    }

    return ret;
}

int qtk_qkws_feed(qtk_qkws_t *qk, char *data, int bytes, int is_end)
{
    if(qk->cfg->use_cmask_pse){
        //double tm=0.0;
        //tm = time_get_ms();
        if(qk->cfg->use_vboxebf){
            qtk_vboxebf_feed(qk->vboxebf, data, bytes, is_end);
        }else if(qk->cfg->use_aec){
            qtk_aec_feed(qk->aec, data, bytes, is_end);
        }else{
            if(bytes > 0){
                if(qk->is_start_enroll){
                    wtk_cmask_pse_feed_vp(qk->cmask_pse, (short *)data, bytes>>1, 0);
                }else{
                    wtk_cmask_pse_feed(qk->cmask_pse, (short *)data, (bytes>>1)/qk->cmask_pse->cfg->channel, 0);
                }
            }
            if(is_end){
                if(qk->is_start_enroll){
                    wtk_cmask_pse_feed_vp(qk->cmask_pse, NULL, 0, 1);
                }else{
                    wtk_cmask_pse_feed(qk->cmask_pse, NULL, 0, 1);
                }
            }
        }
        if(qk->cfg->use_add_zero && qk->is_start_enroll == 0){
            int cnt=0,pos;
            if(qk->cfg->use_vboxebf){
                cnt=qk->vboxebf->channel;
            }else if(qk->cfg->use_aec){
                cnt=qk->aec->channel;
            }else{
                cnt=qk->cfg->cmask_pse_cfg->channel;
            }
            pos = bytes/cnt;
            // wtk_debug("==============>>>>>>>>>>>>>>>>>>>bytes=%d cnt=%d pos=%d bufpos=%d\n",bytes,cnt,pos,qk->outbuf->pos);
            if(qk->outbuf->pos >= pos){
                if(qk->notify){
                    qtk_var_t var;
                    var.type = QTK_SPEECH_DATA_PCM;
                    var.v.str.len = pos;
                    var.v.str.data = qk->outbuf->data;
                    qk->notify(qk->this, &var);
                }
                wtk_strbuf_pop(qk->outbuf, NULL, pos);
            }else{
                if(qk->notify){
                    qtk_var_t var;
                    var.type = QTK_SPEECH_DATA_PCM;
                    var.v.str.len = pos;
                    var.v.str.data = qk->zdata;
                    qk->notify(qk->this, &var);
                }
            }
        }
        //wtk_debug("%s:%d==================>>>>>>>>>ffffffffffffeed tm=%f bytes=%d\n",__FUNCTION__,__LINE__,time_get_ms() - tm, bytes);
    }else{
        if(qk->cfg->use_vboxebf){
            qtk_vboxebf_feed(qk->vboxebf, data, bytes, is_end);
        }else{
            if(qk->cfg->channel == 1){
                qtk_qkws_feed2(qk, data, bytes, is_end);
            }else{
                int i=0;
                wtk_strbuf_reset(qk->input_buf);
                while(i < bytes){
                    wtk_strbuf_push(qk->input_buf, data+i, 2);
                    i+=(qk->cfg->channel << 1);
                }
                qtk_qkws_feed2(qk, qk->input_buf->data, qk->input_buf->pos, is_end);
            }
        }
    }
}

void qtk_qkws_set_feat(qtk_qkws_t *qk, char *fn)
{
    qk->enroll_path = fn;
    wtk_debug("============>>>>>>>>>>>>>>>>>pse set<<<<<<<<<<<<<<<<<<<=========\n");
    if(qk->enroll_path == NULL){wtk_debug("enroll path is null!\n");return;}
    if(access(qk->enroll_path, F_OK) != 0){wtk_debug("enroll path connot be found!\n");return;}
    if(access(qk->enroll_path, R_OK) != 0){wtk_debug("enroll path does not have read permission!\n");return;}
    if(qk->cfg->use_cmask_pse){
        FILE *fp;
        float *feat;
        int e2len=192;

        fp=fopen(fn,"rb");
        if(fp==NULL){
                wtk_debug("open %s failed\n",fn);
                return;
        }
        feat=(float *)wtk_malloc(sizeof(float)*(qk->cmask_pse->feat_len+e2len));
        fread(feat,sizeof(float),qk->cmask_pse->feat_len+e2len,fp);
        fclose(fp);
        wtk_debug("============>>>>>>>>>>>>>>>>>pse start<<<<<<<<<<<<<<<<<<<=========\n");
        wtk_debug("===================+>>>>>>>>>>>>>feat_len=%d feat=%f/%f/%f/%f\n",qk->cmask_pse->feat_len, feat[20], feat[200], feat[1000], feat[1500]);
        if(qk->cmask_pse){
            wtk_cmask_pse_start_vp_feat(qk->cmask_pse,feat,qk->cmask_pse->feat_len+e2len);
        }
        wtk_free(feat);
    }else{
        wtk_debug("set enroll failed!\n");
    }
}

void qtk_qkws_get_result(qtk_qkws_t *qk, qtk_var_t *var)
{
    if(qk->cfg->use_sc){
        wtk_string_t v;
        // wtk_debug("qtk_qkws_get_result============================================>\n");
        qtk_sond_cluster_get_result(qk->sc, &v);
        // wtk_debug("qtk_qkws_get_result======================result len=%d\n",v.len);
        var->v.str.data = v.data;
        var->v.str.len = v.len;
    }
}

void qtk_qkws_on_kws(qtk_qkws_t *qk, int res, char *name, int name_len)
{
    qtk_var_t var;

    if(qk->notify && name_len > 0){
        int len;
        char tmp[2048]={0};
        len = sprintf(tmp, "{\"result\":\"%.*s\"}", name_len, name);
        var.type = QTK_EVAL_TEXT;
        var.v.str.len = len;
        var.v.str.data = tmp;
        qk->notify(qk->this, &var);
    }
}

void qtk_qkws_on_cmask_pse(qtk_qkws_t *qk, short *output, int len)
{
    if(qk->cfg->use_add_zero){
        wtk_strbuf_push(qk->outbuf, (char *)output, len<<1);
    }else{
        if(qk->notify && qk->is_start_enroll == 0){
            qtk_var_t var;
            var.type = QTK_SPEECH_DATA_PCM;
            var.v.str.len = len<<1;
            var.v.str.data = (char *)output;
            qk->notify(qk->this, &var);
        }
    }
}

FILE *psef=NULL;

void qtk_qkws_on_feat_pse(qtk_qkws_t *qk, float *output, int len)
{
    if(len > 0){
        // if(psef == NULL)
        // {
        //     psef = fopen("/sdcard/Android/data/com.qdreamer.reduceNoise/files/pse.txt", "w+");
        // }
        qk->efp = fopen(qk->enroll_path, "wb");
        if(!qk->efp){wtk_debug("enroll path open fail!\n");return;}
        int i=0;
        char tmppse[16]={0};
        wtk_debug("=============>>i=%d len=%d\n",i,len);
        while(i<len){
            wtk_debug("[%d]===>%f\n",i,output[i]);
            // snprintf(tmppse, 16, "%f\n",output[i]);
            // fwrite(tmppse, 16, 1, psef);
            // fflush(psef);
            i++;
        }
        wtk_debug("=============>>i=%d len=%d\n",i,len);
        fwrite(output, sizeof(float), len, qk->efp);
        fflush(qk->efp);
        fclose(qk->efp);
        wtk_debug("====================>>>>>>>>>>write end\n");
    }
}

void qtk_qkws_on_kws2(qtk_qkws_t *qk, int cnt, wtk_string_t **res, float *scores)
{
    int i;
    qtk_var_t var;

    wtk_strbuf_reset(qk->result_buf);
    wtk_strbuf_reset(qk->score_buf);
    for(i=0;i<cnt;++i)
    {
        char tmp[48]={0};
        int len;
        if(i==cnt-1){
            wtk_debug("------------------>>>>>>>>>>>reslen=%d\n",res[i]->len);
            wtk_strbuf_push(qk->result_buf, res[i]->data, res[i]->len);
            len = sprintf(tmp, "%f", scores[i]);
            wtk_strbuf_push(qk->score_buf, tmp, len);
        }else{
            wtk_strbuf_push(qk->result_buf, res[i]->data, res[i]->len);
            wtk_strbuf_push(qk->result_buf, ", ", strlen(", "));
            len = sprintf(tmp, "%f", scores[i]);
            wtk_strbuf_push(qk->score_buf, tmp, len);
            wtk_strbuf_push(qk->score_buf, ", ", strlen(", "));
        }
    }
    if(qk->notify){
        int len;
        char tmp[2048]={0};
        len = sprintf(tmp, "{\"result\":\"%.*s\",\"score\":\"%.*s\"}", qk->result_buf->pos, qk->result_buf->data,
                        qk->score_buf->pos, qk->score_buf->data);
        var.type = QTK_EVAL_TEXT;
        var.v.str.len = len;
        var.v.str.data = tmp;
        qk->notify(qk->this, &var);
    }
}

void qtk_qkws_on_vboxebf(qtk_qkws_t *qk, char *data, int len)
{
    // double tm;
    if(qk->cfg->use_cmask_pse){
        if(len > 0){
            // tm = time_get_ms();
            if(qk->is_start_enroll){
                wtk_cmask_pse_feed_vp(qk->cmask_pse, (short *)data, len>>1, 0);
            }else{
                wtk_cmask_pse_feed(qk->cmask_pse, (short *)data, (len>>1)/qk->cmask_pse->cfg->channel, 0);
            }
            // printf("pse feed tm=%f channel=%d len=%d\n", time_get_ms() - tm, qk->cmask_pse->cfg->channel, len);
        }
    }else{
        qtk_qkws_feed2(qk, data, len, 0);
    }
}

void qtk_qkws_on_aec(qtk_qkws_t *qk, qtk_var_t *var)
{
    char *data;
    int bytes;
    // double tm=0.0;
	switch(var->type){
    case QTK_SPEECH_DATA_PCM:
        //v->v.str.data; v->v.str.len;
        // printf(">>>>> pcm data len = %d\n",var->v.str.len);
        data = var->v.str.data;
        bytes = var->v.str.len;
        if(qk->cfg->use_cmask_pse){
            if(bytes > 0){
                // tm = time_get_ms();
                if(qk->is_start_enroll){
                    wtk_cmask_pse_feed_vp(qk->cmask_pse, (short *)data, bytes>>1, 0);
                }else{
                    // wtk_debug("bytes=%d channel=%d\n",bytes,qk->cmask_pse->cfg->channel);
                    wtk_cmask_pse_feed(qk->cmask_pse, (short *)data, (bytes>>1)/qk->cmask_pse->cfg->channel, 0);
                }
                // printf("pse feed tm=%f\n",time_get_ms() - tm);
            }
        }else{
            qtk_qkws_feed2(qk, data, bytes, 0);
        }
        break;
    default:
        break;
	}
}
