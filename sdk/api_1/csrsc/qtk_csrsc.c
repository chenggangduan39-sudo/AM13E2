#include "qtk_csrsc.h"

void qtk_csrsc_get_timestamp(qtk_csrsc_t *c, char *timestamp, int len, double *s,double *e);
qtk_csrsc_msg_node_t* qtk_csrsc_msg_node_new(qtk_csrsc_t *m);
void qtk_csrsc_msg_node_delete(qtk_csrsc_msg_node_t *msg);
qtk_csrsc_msg_node_t* qtk_csrsc_msg_pop_node(qtk_csrsc_t *m);
void qtk_csrsc_msg_push_node(qtk_csrsc_t *m,qtk_csrsc_msg_node_t *msg);
#define qtk_csrsc_msg_BUFSIZE 10240

void qtk_csrsc_on_hint_notify(qtk_csrsc_t *c, char *data, int bytes) {
    qtk_var_t var;

    if (c->notify) {
        var.type = QTK_ASR_HINT;
        var.v.str.data = data;
        var.v.str.len = bytes;
        c->notify(c->notify_ths, &var);
    }
}

int qtk_csrsc_seek(qtk_csrsc_t *c, int cnt, double *st, double *et)
{
    wtk_queue_node_t *qn=NULL;
    qtk_csrsc_msg_node_t *msgnode;
    int len=c->timequeue.length;
    int i=0;
    int ret=-1;
    while(i < len)
    {
        qn = wtk_blockqueue_pop(&c->timequeue, -1, NULL);
        msgnode = data_offset2(qn,qtk_csrsc_msg_node_t,qn);
        if(cnt == msgnode->count)
        {
            *st=msgnode->start_time;
            *et=msgnode->end_time;
            qtk_csrsc_msg_push_node(c, msgnode);
            ret=0;
            break;
        }
        qtk_csrsc_msg_push_node(c, msgnode);
    }
    return ret;
}

void qtk_csrsc_on_spxfinal_notify(qtk_csrsc_t *c, char *data, int bytes) {
    qtk_var_t var;
    wtk_json_parser_t *parser = NULL;
    wtk_json_item_t *timestamp = NULL;
    wtk_json_item_t *counter = NULL;
    double st=0.0,et=0.0;
    double vst=0.0,vet=0.0;
    int tcnt=0;
    int ret;
    wtk_wavfile_t *wav;

    wav = wtk_wavfile_new(16000);

    parser = wtk_json_parser_new();
    wtk_json_parser_reset(parser);
    wtk_json_parser_parse(parser, data, bytes);
    timestamp = wtk_json_obj_get_s(parser->json->main, "timestamp");

    if(timestamp && timestamp->v.str->len > 0)
    {
        qtk_csrsc_get_timestamp(c, timestamp->v.str->data, timestamp->v.str->len, &st, &et);
    }

    counter = wtk_json_obj_get_s(parser->json->main, "counter");
    if(counter)
    {
        tcnt = (int)counter->v.number;
    }
    // char name[16]={0};
    // snprintf(name, 16, "./audio/%d.wav",tcnt);
    // wtk_wavfile_open(wav, name);
    // wtk_wavfile_set_channel2(wav, 1, 2);

    ret = qtk_csrsc_seek(c, tcnt, &vst, &vet);

    if(ret == 0)
    {
        int slen=0;
        int poslen=(vst-c->postime)*32*1000;
        poslen = poslen >> 1 << 1;
        wtk_strbuf_pop(c->vadbuf, NULL, poslen);
        c->postime += (poslen/32.0/1000.0);

        wtk_debug("====cnt=%d poslen=%d/%f pos=%d/%f ===>>>>>>>>[%f %f %f] [%f %f]\n", tcnt, poslen, poslen/1000.0/32.0, c->vadbuf->pos, c->vadbuf->pos/1000.0/32.0, vst, vet, c->postime, st, et);
        poslen = (et - st)*32*1000;
        poslen=poslen>>1<<1;
        slen = st*32*1000;
        // wtk_wavfile_write(wav, c->vadbuf->data+slen, poslen);
        qtk_sond_cluster_feed(c->sc, c->vadbuf->data+slen, poslen, 0);
        poslen=(int)((vet-vst)*1000.0f*32.0f);
        poslen=poslen>>1<<1;
        wtk_strbuf_pop(c->vadbuf, NULL, poslen);
        c->postime += (vet-vst);

        qtk_sond_cluster_set_vad_time(c->sc, vst+st, vst+et);
        wtk_string_t var2;
        qtk_sond_cluster_get_result(c->sc, &var2);
        wtk_debug("====poslen=%d/%f pos=%d =================>>>>>>>>[%.*s]\n", poslen, poslen/1000.0/32.0, c->vadbuf->pos,var2.len,var2.data);
    }

    wtk_wavfile_delete(wav);
    if (c->notify) {
        var.type = QTK_ASR_TEXT;
        var.v.str.data = data;
        var.v.str.len = bytes;
        c->notify(c->notify_ths, &var);
    }

    if (parser) {
        wtk_json_parser_delete(parser);
    }
}

static void qtk_csrsc_init(qtk_csrsc_t *c) {
    c->cfg = NULL;
    c->session = NULL;

    c->asr = NULL;
    c->vad = NULL;

    c->notify = NULL;
    c->notify_ths = NULL;
    c->tmpbuf = NULL;
}

qtk_csrsc_t *qtk_csrsc_new(qtk_csrsc_cfg_t *cfg, qtk_session_t *session) {
    qtk_csrsc_t *c;
    int ret;

    c = (qtk_csrsc_t *)wtk_malloc(sizeof(qtk_csrsc_t));
    qtk_csrsc_init(c);
    c->cfg = cfg;
    c->session = session;
    c->start_time = 0.0f;
    c->end_time = 0.0f;

    c->asr = qtk_asr_new(&cfg->asr, session);
    if (!c->asr) {
        wtk_log_log0(c->session->log, "asr new failed.");
        ret = -1;
        goto end;
    }

    c->sc = qtk_sond_cluster_new(cfg->sc_cfg);

    //wtk_queue_init(&c->vad_q);
    if (cfg->xvad) {
        c->vad = wtk_vad_new(cfg->xvad, 0);
    } else {
        c->vad = wtk_vad_new(&cfg->vad, 0);
    }
    qtk_asr_set_hint_notify(c->asr, c,
                            (qtk_iasr_set_hint_notify_f)qtk_csrsc_on_hint_notify);
    qtk_asr_set_spxfinal_notify(c->asr, c,
                            (qtk_iasr_set_spxfinal_notify_f)qtk_csrsc_on_spxfinal_notify);

    wtk_lockhoard_init(&(c->msg_hoard),offsetof(qtk_csrsc_msg_node_t,hoard_on),100,
		(wtk_new_handler_t)qtk_csrsc_msg_node_new,
		(wtk_delete_handler_t)qtk_csrsc_msg_node_delete, 
		c);
    wtk_blockqueue_init(&c->timequeue);

    c->tmpbuf = wtk_strbuf_new(1024, 1.0);
    c->vadbuf = wtk_strbuf_new(1204, 1.0);
    ret = 0;
end:
    if (ret != 0) {
        qtk_csrsc_delete(c);
        c = NULL;
    }
    return c;
}

void qtk_csrsc_delete(qtk_csrsc_t *c) {

    wtk_blockqueue_clean(&c->timequeue);
    wtk_lockhoard_clean(&(c->msg_hoard));
    if (c->vad) {
        wtk_vad_delete(c->vad);
    }
    if (c->asr) {
        qtk_asr_delete(c->asr);
    }
    if(c->sc)
    {
        qtk_sond_cluster_delete(c->sc);
    }
    if(c->tmpbuf)
    {
        wtk_strbuf_delete(c->tmpbuf);
    }
    if(c->vadbuf)
    {
        wtk_strbuf_delete(c->vadbuf);
    }
    wtk_free(c);
}

void qtk_csrsc_set_notify(qtk_csrsc_t *c, void *notify_ths,
                        qtk_csrsc_notify_f notify) {
    c->notify_ths = notify_ths;
    c->notify = notify;
}

int qtk_csrsc_start(qtk_csrsc_t *c, int left, int right) {
    int ret=-1;
    c->sil = 1;
    c->valid = 0;
    c->cancel = 0;
    c->start_time = 0.0f;
    c->end_time = 0.0f;

    if(c->sc->eval == 1 && c->sc->enroll==0)
    {
        if(c->sc->cfg->use_clu==0)
        {
            ret = qtk_sond_cluster_prepare(c->sc);
            if(ret != 0){goto end;}
            // qtk_sond_cluster_prepare2(qk->sc);
        }
    }
    ret = qtk_sond_cluster_start(c->sc);
    if(ret != 0){goto end;}

    wtk_vad_set_margin(c->vad, left, right);
    ret = wtk_vad_start(c->vad);
    if(ret != 0){goto end;}
    wtk_strbuf_reset(c->vadbuf);
    c->postime = 0.0f;

    ret=0;
end:
    return ret;
}

void qtk_csrsc_reset(qtk_csrsc_t *c) {
    wtk_vad_reset(c->vad);
    qtk_asr_reset(c->asr);
    qtk_sond_cluster_feed(c->sc, NULL, 0, 1);
    qtk_sond_cluster_reset(c->sc);
}

void qtk_csrsc_cancel(qtk_csrsc_t *c) {
    c->cancel = 1;
    qtk_asr_cancel(c->asr);
}

/**
 * 2.仅注册使用的接口,每注册一个人,这两个接口就都需要调用一次
*/
int qtk_csrsc_set_enroll(qtk_csrsc_t *qk, char *name, int len, int is_end)
{
    wtk_debug("qtk_csrsc_set_enroll==================>>>>>>>>>name=%s len=%d is_end=%d\n",name,len,is_end);
    int ret=-1;
    if(is_end < 1)
    {
        /**
         * feed音频之前,指定注册说话人的id, 如果name不为空 声纹会切换到声纹注册模式,为空则是声纹识别模式
        */
        // printf("enroll==>name=[%s] len=%d\n",name, len);
        ret = qtk_sond_cluster_enroll(qk->sc, name, len);
    }
    if(is_end==1)
    {
        /**
         * feed音频结束之后,注册好的声纹保存到文件里,这个接口调用之后,声纹会切换到声纹识别模式
         */
        // printf("enroll_end==>\n");
        ret = qtk_sond_cluster_enroll_end(qk->sc);
    }
    return ret;
}

/**
 * 3.注册的声纹文件相关接口
 * 清除注册好的声纹,不会删除文件,调用后 原来注册的所有声纹都不会生效
 */
void qtk_csrsc_clean(qtk_csrsc_t *qk)
{
    wtk_debug("qtk_csrsc_clean==================>>>>>>>>>\n");
    qtk_sond_cluster_clean(qk->sc);
}

/**
 * 注册声纹文件重新加载 重新读取并加载文件里的声纹
*/
void qtk_csrsc_reload(qtk_csrsc_t *qk)
{
    wtk_debug("qtk_csrsc_reload==================>>>>>>>>>\n");
    qtk_sond_cluster_reload(qk->sc);
}

/**
 * 设置声纹文件   vp.bin.idx  vp.bin.data  fn传入"vp.bin",接口里面会调用qtk_kws_reload,不需要在外面重新加载
*/
int qtk_csrsc_set_enroll_fn(qtk_csrsc_t *qk, char *fn, int len)
{
    wtk_debug("qtk_csrsc_set_enroll_fn==================>>>>>>>>>fn=%s len=%d\n",fn,len);
    return qtk_sond_cluster_set_enroll_fn(qk->sc, fn, len);
}

/**
 * 获取当前的声纹文件名
*/
char* qtk_csrsc_get_fn(qtk_csrsc_t *qk)
{
    return qtk_sond_cluster_get_fn(qk->sc);
}

int qtk_csrsc_set_vad_time(qtk_csrsc_t *qk, float vs, float ve)
{
    static float vadtime=0.0f;

    vadtime+=(ve-vs);
    wtk_debug("=====================>>>>.st=%f et=%f vadtime=%f\n",vs,ve,vadtime);
    return qtk_sond_cluster_set_vad_time(qk->sc, vs, ve);
}

int qtk_csrsc_set_spk_nums(qtk_csrsc_t *qk, int num)
{
    wtk_debug("====================>>>>>>>>spk nums=%d\n",num);
    return qtk_sond_cluster_set_spk_nums(qk->sc, num);
}

void qtk_csrsc_get_timestamp(qtk_csrsc_t *c, char *timestamp, int len, double *st,double *et)
{
    int i,cnt=0;
    char tc;
    // wtk_strbuf_reset(c->tmpbuf);
    char tmpdata[1024]={0};
    int pos=0;

    for(i=0;i<len;++i)
    {
        tc=timestamp[i];
        if(tc == ',')
        {
            if(cnt==0)
            {
                *st=atof(tmpdata);
            }
            if(cnt!=0 && i==len-1)
            {
                *et=atof(tmpdata);
            }
            pos=0;
            // wtk_strbuf_reset(c->tmpbuf);
            cnt++;
            continue;
        }
        // wtk_strbuf_push_c(c->tmpbuf, tc);
        tmpdata[pos++]=tc;
    }
}

int qtk_csrsc_feed(qtk_csrsc_t *c, char *data, int bytes, int is_end) {
    static int feed_bytes=0;
    static int vad_count=1;

    wtk_queue_t *vad_q = (c->vad->output_queue);
    wtk_vframe_t *f;
    wtk_queue_node_t *qn;
    wtk_json_parser_t *parser = NULL;
    wtk_json_item_t *result = NULL;
    wtk_json_item_t *timestamp = NULL;
    wtk_strbuf_t *buf = NULL;
    wtk_string_t v;
    qtk_var_t var;
    qtk_csrsc_msg_node_t *msgnode;
    int ret;
    double t1, t2;
    double st=0.0,et=0.0;

    parser = wtk_json_parser_new();
    buf = wtk_strbuf_new(256, 1);
    wtk_strbuf_push(c->vadbuf, data, bytes);
    wtk_vad_feed(c->vad, data, bytes, is_end);
    while (1) {
        qn = wtk_queue_pop(vad_q);
        if (!qn) {
            break;
        }
        f = data_offset2(qn, wtk_vframe_t, q_n);

        feed_bytes+=(f->frame_step << 1)/32;
        if (c->cancel) {
            wtk_vad_push_vframe(c->vad, f);
            continue;
        }

        if (c->sil) {
            if (f->state == wtk_vframe_speech) {
#if 1
                ret = qtk_asr_start(c->asr);
                c->valid = ret == 0 ? 1 : 0;
                ret = c->valid ? qtk_asr_feed(c->asr, (char *)f->wav_data,
                                              f->frame_step << 1, 0)
                               : -1;
                c->valid = ret == 0 ? 1 : 0;
#endif
                c->sil = 0;

                // qtk_sond_cluster_feed(c->sc,  (char *)f->wav_data, f->frame_step << 1, 0);

                var.type = QTK_SPEECH_START;
                c->start_time=feed_bytes/1000.0;
                if (c->notify) {
                    var.v.fi.on=vad_count;
                    var.v.fi.theta=c->start_time;
                    c->notify(c->notify_ths, &var);
                }
            }
        } else {
            if (f->state == wtk_vframe_sil) {
                c->end_time=feed_bytes/1000.0;
                // wtk_log_log(c->session->log,"vad第%d段音频: start_time==%fms end_time==%fms",vad_count,start_time,end_time);
                // wtk_debug("vad第%d段音频: start_time==%fms end_time==%fms\n",vad_count,start_time,end_time);
                msgnode = qtk_csrsc_msg_pop_node(c);
                msgnode->count = vad_count;
                msgnode->start_time = c->start_time;
                msgnode->end_time = c->end_time;
                wtk_blockqueue_push(&c->timequeue, &msgnode->qn);

                var.type = QTK_SPEECH_END;
                if (c->notify) {
                    var.v.fi.on=vad_count;
                    var.v.fi.theta=c->end_time;
                    c->notify(c->notify_ths, &var);
                }
#if 1
                t1 = time_get_ms();
                ret = c->valid ? qtk_asr_feed(c->asr, 0, 0, 1) : -1;
                c->valid = ret == 0 ? 1 : 0;

                if (c->valid) {
                    v = qtk_asr_get_result(c->asr);
                    t2 = time_get_ms();
                    if (v.len > 0) {
                        wtk_json_parser_reset(parser);
                        wtk_json_parser_parse(parser, v.data, v.len);
                        result = parser->json->main;
                        wtk_json_obj_add_ref_number_s(parser->json, result,
                                                      "delay", (int)(t2 - t1));
                        wtk_json_item_print(result, buf);

                        timestamp = wtk_json_obj_get_s(parser->json->main, "timestamp");

                        qtk_csrsc_get_timestamp(c, timestamp->v.str->data, timestamp->v.str->len, &st, &et);
                        wtk_debug("+=====================>>>>>>>>>>>>>timestamp=%s st=%f et=%f\n",timestamp,st,et);
                        var.type = QTK_ASR_TEXT;
                        var.v.str.data = buf->data;
                        var.v.str.len = buf->pos;
                        if (c->notify) {
                            c->notify(c->notify_ths, &var);
                        }
                    }
                }
                qtk_asr_reset(c->asr);
#endif
                c->sil = 1;
                vad_count++;
            } else {
                // qtk_sond_cluster_feed(c->sc,  (char *)f->wav_data, f->frame_step << 1, 0);
#if 1
                ret = c->valid ? qtk_asr_feed(c->asr, (char *)f->wav_data,
                                              f->frame_step << 1, 0)
                               : -1;
                c->valid = ret == 0 ? 1 : 0;
#endif
            }
        }
        wtk_vad_push_vframe(c->vad, f);
    }

#if 1
    if (is_end && !c->sil && !c->cancel) {
        t1 = time_get_ms();
        ret = c->valid ? qtk_asr_feed(c->asr, 0, 0, 1) : -1;
        c->valid = ret == 0 ? 1 : 0;

        if (c->valid) {
            v = qtk_asr_get_result(c->asr);
            t2 = time_get_ms();
            if (v.len > 0) {
                wtk_json_parser_reset(parser);
                wtk_json_parser_parse(parser, v.data, v.len);
                result = parser->json->main;
                wtk_json_obj_add_ref_number_s(parser->json, result, "delay",
                                              (int)(t2 - t1));
                wtk_json_item_print(result, buf);

                timestamp = wtk_json_obj_get_s(parser->json->main, "timestamp");

                qtk_csrsc_get_timestamp(c, timestamp->v.str->data, timestamp->v.str->len, &st, &et);
                wtk_debug("+=====================>>>>>>>>>>>>>timestamp=%s st=%f et=%f\n",timestamp,st,et);

                var.type = QTK_ASR_TEXT;
                var.v.str.data = buf->data;
                var.v.str.len = buf->pos;
                if (c->notify) {
                    c->notify(c->notify_ths, &var);
                }
            }
        }
        qtk_asr_reset(c->asr);
    }
#endif

    if (buf) {
        wtk_strbuf_delete(buf);
    }
    if (parser) {
        wtk_json_parser_delete(parser);
    }

    return 0;
}

qtk_csrsc_msg_node_t* qtk_csrsc_msg_node_new(qtk_csrsc_t *m)
{
	qtk_csrsc_msg_node_t *msg;

	msg=(qtk_csrsc_msg_node_t*)wtk_malloc(sizeof(qtk_csrsc_msg_node_t));
    msg->count=0;
    msg->start_time=0.0f;
    msg->end_time = 0.0f;

	return msg;
}

void qtk_csrsc_msg_node_delete(qtk_csrsc_msg_node_t *msg)
{
	wtk_free(msg);
}

qtk_csrsc_msg_node_t* qtk_csrsc_msg_pop_node(qtk_csrsc_t *m)
{
	return  (qtk_csrsc_msg_node_t*)wtk_lockhoard_pop(&(m->msg_hoard));
}

void qtk_csrsc_msg_push_node(qtk_csrsc_t *m,qtk_csrsc_msg_node_t *msg)
{
	wtk_lockhoard_push(&(m->msg_hoard),msg);
}
