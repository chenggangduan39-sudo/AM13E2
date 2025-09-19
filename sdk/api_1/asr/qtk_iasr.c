#include "qtk_iasr.h"

// int qtkcount=0.0;

void qtk_iasr_on_spx(qtk_iasr_t *iasr, qtk_spx_msg_t* msg) {
    switch (msg->type) {
    case QTK_SPX_MSG_APPEND:
        if (iasr->hint_notify) {
            iasr->hint_notify(iasr->hint_ths, msg->body->data, msg->body->pos);
        }
        break;
    case QTK_SPX_MSG_END:
        if (iasr->spxfinal_notify) {
            iasr->spxfinal_notify(iasr->spxfinal_ths, msg->body->data, msg->body->pos);
        }
        break;
    default:
        wtk_log_warn(iasr->session->log, "unexpect spx msg type %d",msg->type);
        break;
    }
}

void qtk_iasr_on_hw(qtk_iasr_t *iasr, char *data, int bytes) {
    if (iasr->hw_notify) {
        iasr->hw_notify(iasr->hw_ths, data, bytes);
    }
}

void qtk_iasr_asr_cb(qtk_iasr_t *iasr, char *res, int bytes)
{
    if(bytes!= 19 && strncmp(res, "{\"rec\":\"\",\"conf\":0}", 19)!=0)
    {
        if(iasr->result_notify)
        {
            iasr->result_notify(iasr->result_ths, res, bytes, -1, -1, 0);
        }
    }
}

void qtk_iasr_wake_cb(qtk_iasr_t *iasr, float fs, float fe)
{
    if(iasr->result_notify)
    {
        iasr->result_notify(iasr->result_ths, NULL, 0, fs, fe, 1);
    }
}

static void qtk_iasr_add_cldebnf(qtk_iasr_t *iasr, wtk_json_t *json,
                                 wtk_json_item_t *item) {
    wtk_string_t *cnfId;
    wtk_json_item_t *tmp;
    char buffer[128];
    int len;

    cnfId = qtk_cldebnf_check(iasr->cnf);
    if (cnfId) {
        tmp = wtk_json_obj_get_s(item, "env");
        if (tmp && tmp->type == WTK_JSON_STRING) {
            len = snprintf(buffer, 128, "%.*suse_ebnfdec=1;", tmp->v.str->len,
                           tmp->v.str->data);
            wtk_json_obj_remove_s(item, "env");
        } else {
            len = snprintf(buffer, 128, "use_ebnfdec=1;");
        }
        wtk_json_obj_add_str2_s(json, item, "env", buffer, len);
        wtk_json_obj_add_str2_s(json, item, "ebnfRes", cnfId->data, cnfId->len);
    } else {
        wtk_log_warn0(iasr->session->log, "get cldebnf id failed");
    }
}

static void qtk_iasr_init(qtk_iasr_t *iasr) {
    iasr->cfg = NULL;
    iasr->session = NULL;

    iasr->ins.spx = NULL;
    iasr->cnf = NULL;
    iasr->egram = NULL;
    iasr->hw = NULL;

    iasr->buf = NULL;
    iasr->resample = NULL;
    iasr->hint_ths = NULL;
    iasr->hint_notify = NULL;
    iasr->spxfinal_notify=NULL;
    iasr->spxfinal_ths=NULL;
    iasr->hw_ths = NULL;
    iasr->hw_notify = NULL;
    iasr->spx_wav=NULL;
    iasr->index=0;
}

qtk_iasr_t *qtk_iasr_new(qtk_iasr_cfg_t *cfg, qtk_session_t *session) {
    qtk_iasr_t *iasr;
    wtk_rbin2_item_t *item;
    char *data;
    int len;
    int ret;
    char tmp[64];

    iasr = (qtk_iasr_t *)wtk_malloc(sizeof(qtk_iasr_t));
    qtk_iasr_init(iasr);
    iasr->cfg = cfg;
    iasr->session = session;

    switch (cfg->type) {
    case QTK_IASR_SPX:
        if(iasr->cfg->save_spx_wav){
            iasr->spx_wav=wtk_wavfile_new(16000);
        }
        iasr->ins.spx = qtk_spx_new(cfg->cfg.scfg, session);
        if (!iasr->ins.spx) {
            wtk_log_warn0(iasr->session->log, "QTK_IASR_SPX new spx failed.");
            ret = -1;
            goto end;
        }
        iasr->ins.spx->iasr_cfg=cfg;
        qtk_spx_set_notify(iasr->ins.spx, iasr,(qtk_spx_notify_func)qtk_iasr_on_spx);
        iasr->ins.spx->cfg->use_hint = cfg->use_hint;
        ret = snprintf(tmp, 64, "use_hint=%d;use_vad=%d;use_lex=%d;use_punc=%d;max_hint=%d;use_timestamp=%d;",
						cfg->use_hint, cfg->use_vad,cfg->use_lex,cfg->use_punc,cfg->max_hint,cfg->use_timestamp);
        qtk_spx_set_env(iasr->ins.spx, tmp, ret);

        wtk_log_log(iasr->session->log, "use_cldebnf = %d", cfg->use_cldebnf);
        if (cfg->use_cldebnf && cfg->usr_ebnf.len > 0) {
            iasr->cnf = qtk_cldebnf_new(&cfg->cldebnf, session);
            if (cfg->rbin) {
                item = wtk_rbin2_get2(cfg->rbin, cfg->usr_ebnf.data,
                                      cfg->usr_ebnf.len);
                if (!item) {
                    wtk_log_warn(iasr->session->log,
                                 "usr_ebnf [%.*s] not in bin file",
                                 cfg->usr_ebnf.len, cfg->usr_ebnf.data);
                    ret = -1;
                    goto end;
                }
                qtk_cldebnf_process(iasr->cnf, item->data->data,
                                    item->data->len);
            } else {
                data = file_read_buf(cfg->usr_ebnf.data, &len);
                if (!data) {
                    ret = -1;
                    wtk_log_warn(iasr->session->log,
                                 "usr_ebnf [%.*s] read failed",
                                 cfg->usr_ebnf.len, cfg->usr_ebnf.data);
                    goto end;
                }
                qtk_cldebnf_process(iasr->cnf, data, len);
                wtk_free(data);
            }
            qtk_spx_set_add_hdr(iasr->ins.spx, iasr,
                                (qtk_spx_add_hdr_func)qtk_iasr_add_cldebnf);
        }
        wtk_log_log(iasr->session->log, "use_hw_upload = %d",
                    cfg->use_hw_upload);
        if (cfg->use_hw_upload || cfg->use_hotword) {
            iasr->hw = qtk_hw_new(&cfg->hw, iasr->session);
            if (!iasr->hw) {
                ret = -1;
                wtk_log_log0(iasr->session->log, "hotword new failed.");
                goto end;
            }
            qtk_hw_set_notify(iasr->hw, iasr, (qtk_hw_notify_f)qtk_iasr_on_hw);
        }
        if (cfg->use_hotword) {
            iasr->ins.spx->cfg->use_hotword = 1;
        }
        break;
    case QTK_IASR_LC:
        if (cfg->use_bin) {
            if (cfg->cfg.dwcfg) {
                iasr->ins.dw = qtk_decoder_wrapper_new(cfg->cfg.dwcfg);
                if (!iasr->ins.dw) {
                    wtk_log_warn0(iasr->session->log, "gr new failed.");
                    ret = -1;
                    goto end;
                }
            } else {
                wtk_log_warn0(iasr->session->log, "gr cfg is null.");
                ret = -1;
                goto end;
            }
        } else {
            if (cfg->cfg.mcfg) {
                iasr->ins.dw = qtk_decoder_wrapper_new(
                    (qtk_decoder_wrapper_cfg_t *)cfg->cfg.mcfg->cfg);
                if (!iasr->ins.dw) {
                    wtk_log_warn0(iasr->session->log, "gr new failed.");
                    ret = -1;
                    goto end;
                }
            } else {
                wtk_log_warn0(iasr->session->log, "gr cfg is null.");
                ret = -1;
                goto end;
            }
        }
        // wtk_debug(">>>>>>>use_xbnf = %d    xbnf_fn:%s\n", cfg->use_xbnf,
        // cfg->xbnf_fn.data);
        if (cfg->use_xbnf) {
            if (cfg->usr_xbnf.len > 0) {
                data = file_read_buf(cfg->usr_xbnf.data, &len);
                if (!data) {
                    wtk_log_warn(iasr->session->log, "xbnf [%.*s] read failed.",
                                 cfg->usr_xbnf.len, cfg->usr_xbnf.data);
                    ret = -1;
                    goto end;
                } else {
                    qtk_decoder_wrapper_set_xbnf(iasr->ins.dw, data, len);
                }
            }
        }
        break;
    case QTK_IASR_GR:
        wtk_log_log0(iasr->session->log, "use gr");
        if (cfg->usr_ebnf.len > 0) {
            iasr->usr_ebnf_data =
                file_read_buf(cfg->usr_ebnf.data, &iasr->usr_ebnf_len);
        }
        if (iasr->usr_ebnf_len > 0) {
            if (cfg->use_bin) {
                if (cfg->cfg.dwcfg) {
                    iasr->ins.dw = qtk_decoder_wrapper_new(cfg->cfg.dwcfg);
                    if (!iasr->ins.dw) {
                        wtk_log_warn0(iasr->session->log, "gr new failed.");
                        ret = -1;
                        goto end;
                    }
                } else {
                    wtk_log_warn0(iasr->session->log, "gr cfg is null.");
                    ret = -1;
                    goto end;
                }
                if (cfg->eg_mbcfg) {
                    iasr->egram = wtk_egram_new2(cfg->eg_mbcfg);
                    if (!iasr->egram) {
                        wtk_log_warn0(iasr->session->log, "egram new failed.");
                        ret = -1;
                        goto end;
                    }
                } else {
                    wtk_log_warn0(iasr->session->log, "egram cfg new failed.");
                    ret = -1;
                    goto end;
                }
            } else {
                if (cfg->cfg.mcfg) {
                    iasr->ins.dw = qtk_decoder_wrapper_new(
                        (qtk_decoder_wrapper_cfg_t *)cfg->cfg.mcfg->cfg);
                    if (!iasr->ins.dw) {
                        wtk_log_warn0(iasr->session->log, "gr new failed.");
                        ret = -1;
                        goto end;
                    }
                } else {
                    wtk_log_warn0(iasr->session->log, "gr cfg is null.");
                    ret = -1;
                    goto end;
                }
                if (cfg->eg_mcfg) {
                    iasr->egram = wtk_egram_new(
                        (wtk_egram_cfg_t *)cfg->eg_mcfg->cfg, NULL);
                    if (!iasr->egram) {
                        wtk_log_warn0(iasr->session->log, "egram new failed.");
                        ret = -1;
                        goto end;
                    }
                } else {
                    wtk_log_warn0(iasr->session->log, "egram cfg new failed.");
                    ret = -1;
                    goto end;
                }
            }
            wtk_egram_ebnf2fst(iasr->egram, iasr->usr_ebnf_data,
                               iasr->usr_ebnf_len);
            if (iasr->ins.dw->cfg->use_lite) {
                wtk_egram_dump2(iasr->egram,
                                iasr->ins.dw->asr[0]->dec_lite->net);
            } else {
                wtk_egram_dump2(iasr->egram, iasr->ins.dw->asr[0]->dec->net);
            }
            // wtk_debug(">>>>>>use_xbnf  = %d\n", cfg->use_xbnf);
            if (cfg->use_xbnf) {
                if (cfg->usr_xbnf.len > 0) {
                    data = file_read_buf(cfg->usr_xbnf.data, &len);
                    if (!data) {
                        wtk_log_warn(iasr->session->log,
                                     "xbnf [%.*s] read failed.",
                                     cfg->usr_xbnf.len, cfg->usr_xbnf.data);
                        ret = -1;
                        goto end;
                    } else {
                        qtk_decoder_wrapper_set_xbnf(iasr->ins.dw, data, len);
                    }
                    wtk_free(data);
                }
            }
        } else {
            ret = -1;
            wtk_log_warn0(iasr->session->log, "main.ebnf is not exit.");
            goto end;
        }
        break;
    case QTK_IASR_GR_NEW:
        wtk_log_log0(iasr->session->log, "use new gr");
        if (cfg->cfg.awcfg) {
            iasr->ins.aw = qtk_asr_wrapper_new(cfg->cfg.awcfg);
            if (!iasr->ins.aw) {
                wtk_log_warn0(iasr->session->log, "gr new failed.");
                ret = -1;
                goto end;
            }
#ifndef USE_KSD_EBNF
            if(iasr->cfg->use_general_asr == 0)
            {
                qtk_asr_wrapper_set_asr_notify(iasr->ins.aw, iasr, (qtk_asr_wrapper_asr_notify_t)qtk_iasr_asr_cb);
                qtk_asr_wrapper_set_wakeup_notify(iasr->ins.aw, iasr, (qtk_asr_wrapper_wakeup_notify_t)qtk_iasr_wake_cb);

                if(cfg->use_mode >= 0 && cfg->use_mode < 3){
                    qtk_asr_wrapper_set_mode(iasr->ins.aw, cfg->use_mode);
                }
                qtk_asr_wrapper_set_idle_time(iasr->ins.aw, cfg->idle_time);
                if(cfg->keyword_fn.len > 0)
                {
                    if (access(cfg->keyword_fn.data, R_OK) == 0)
                    {
                        int klen=0;
                        char *kwdata=file_read_buf(cfg->keyword_fn.data, &klen);
                        wtk_debug("klen=%d data=[%.*s]\n",klen,klen,kwdata);
#ifdef USE_HUACHUANG
                        ret = qtk_asr_wrapper_set_contacts(iasr->ins.aw, kwdata, klen);
#else
                        ret = qtk_asr_wrapper_set_text(iasr->ins.aw, kwdata, klen);
#endif
                        if(ret != 0)
                        {
                            wtk_debug("set contacts fialed!\n");
                        }
                        wtk_free(kwdata);
                    }else{
                        wtk_debug("This keyword file does not have read permission！\n");
                    }
                }
            }
#endif
        }else {
            wtk_log_warn0(iasr->session->log, "gr cfg is null.");
            ret = -1;
            goto end;
        }
    	break;
    }

    if (cfg->use_resample) {
        iasr->buf = wtk_strbuf_new(4096, 1);
        iasr->resample = wtk_resample_new(cfg->resample_cache);
        wtk_resample_start(iasr->resample, cfg->src_rate, cfg->dst_rate);
        wtk_resample_set_notify(iasr->resample, iasr->buf,
                                (wtk_resample_notify_f)wtk_strbuf_push);
    }

    ret = 0;
end:
    if (ret != 0) {
        qtk_iasr_delete(iasr);
        iasr = NULL;
    }
    return iasr;
}

void qtk_iasr_delete(qtk_iasr_t *iasr) {
    switch (iasr->cfg->type) {
    case QTK_IASR_SPX:
        if (iasr->ins.spx) {
            qtk_spx_delete(iasr->ins.spx);
        }
        if (iasr->cnf) {
            qtk_cldebnf_delete(iasr->cnf);
        }
        if (iasr->hw) {
            qtk_hw_delete(iasr->hw);
        }
        if(iasr->cfg->save_spx_wav && iasr->spx_wav){
            wtk_wavfile_delete(iasr->spx_wav);
            iasr->spx_wav=NULL;
        }
        break;
    case QTK_IASR_LC:
        if (iasr->ins.dw) {
            qtk_decoder_wrapper_delete(iasr->ins.dw);
        }
        break;
    case QTK_IASR_GR:
        if (iasr->ins.dw) {
            qtk_decoder_wrapper_delete(iasr->ins.dw);
        }
        if (iasr->usr_ebnf_data) {
            wtk_free(iasr->usr_ebnf_data);
        }
        if (iasr->egram) {
            wtk_egram_delete(iasr->egram);
        }
        break;
    case QTK_IASR_GR_NEW:
    	if (iasr->ins.aw) {
            qtk_asr_wrapper_delete(iasr->ins.aw);
    	}
    	break;
    }

    if (iasr->resample) {
        wtk_resample_delete(iasr->resample);
        wtk_strbuf_delete(iasr->buf);
    }

    wtk_free(iasr);
}

int qtk_iasr_start(qtk_iasr_t *iasr) {
    int ret = 0;

    if (iasr->resample) {
        wtk_strbuf_reset(iasr->buf);
    }

    char filename[128]={0};
    switch (iasr->cfg->type) {
    case QTK_IASR_SPX:
    if(iasr->cfg->save_spx_wav && iasr->spx_wav){
#ifdef ANDROID
        snprintf(filename, 128, "/sdcard/Android/data/com.qdreamer.asr/files/spx/spx%d.wav",++iasr->index);
#else
        snprintf(filename, 128, "wav_file/spx%d.wav",++iasr->index);
#endif
        iasr->spx_wav->max_pend=0;
        wtk_wavfile_open(iasr->spx_wav, filename);
    }

        ret = qtk_spx_start(iasr->ins.spx, NULL, 0, 0);
        break;
    case QTK_IASR_LC:
    case QTK_IASR_GR:
        // wtk_debug(">>>>>>lc start\n");
        iasr->end_flag = 0;
        ret = qtk_decoder_wrapper_start(iasr->ins.dw);
        break;
    case QTK_IASR_GR_NEW:
    	iasr->end_flag = 0;
        ret = qtk_asr_wrapper_start(iasr->ins.aw);
    	break;
    default:
        break;
    }
    return ret;
}

int qtk_iasr_feed(qtk_iasr_t *iasr, char *data, int bytes, int is_end) {
    int ret = -1;
    wtk_string_t v;
    static int count = 0;

    if (iasr->resample) {
        wtk_resample_feed(iasr->resample, data, bytes, is_end);
        data = iasr->buf->data;
        bytes = iasr->buf->pos;
        wtk_strbuf_reset(iasr->buf);
    }

    switch (iasr->cfg->type) {
    case QTK_IASR_SPX:
        if(bytes>0 && iasr->cfg->save_spx_wav && iasr->spx_wav){
            wtk_wavfile_write(iasr->spx_wav,data,bytes);
        }
        ret = qtk_spx_feed(iasr->ins.spx, data, bytes, is_end);
        break;
    case QTK_IASR_LC:
        // wtk_debug(">>>>>>lc feed is_end = %d\n", is_end);
        ret = qtk_decoder_wrapper_feed(iasr->ins.dw, data, bytes, is_end);
        if (is_end == 1) {
            iasr->end_flag = 1;
        }
        break;
    case QTK_IASR_GR:
        ret = qtk_decoder_wrapper_feed(iasr->ins.dw, data, bytes, is_end);
        count += bytes;
        if (count >= 1600 && iasr->cfg->use_hint) {
            qtk_decoder_wrapper_get_hint_result(iasr->ins.dw, &v);
            // wtk_debug("len = %d\n", v.len);
            if (v.len > 11) {
                if (iasr->hint_notify) {
                    iasr->hint_notify(iasr->hint_ths, v.data, v.len);
                }
            }
            count = 0;
        }

        if (is_end == 1) {
            iasr->end_flag = 1;
        }
        break;
    case QTK_IASR_GR_NEW:
        ret = qtk_asr_wrapper_feed(iasr->ins.aw, data, bytes, is_end);
        count += bytes;
        // qtkcount=count;
        // wtk_debug("=============>>>>>>>>>>>>>>>>>>bytes=%d time=%f\n",bytes,count/32.0);
        if (count >= 1600 && iasr->cfg->use_hint) {
            qtk_asr_wrapper_get_hint_result(iasr->ins.aw, &v);
            // wtk_debug("len = %d\n", v.len);
            if (v.len > 11) {
                if (iasr->hint_notify) {
                    iasr->hint_notify(iasr->hint_ths, v.data, v.len);
                }
            }
            count = 0;
        }

        if (is_end == 1) {
            iasr->end_flag = 1;
        }
    }
    return ret;
}

void qtk_iasr_cancel(qtk_iasr_t *iasr) {
    if (iasr->cfg->type == QTK_IASR_SPX) {
        qtk_spx_cancel(iasr->ins.spx);
    }
}

void qtk_iasr_reset(qtk_iasr_t *iasr) {
    switch (iasr->cfg->type) {
    case QTK_IASR_SPX:
        if(iasr->cfg->save_spx_wav && iasr->spx_wav){
            wtk_wavfile_close(iasr->spx_wav);
        }
        qtk_spx_reset(iasr->ins.spx);
        break;
    case QTK_IASR_GR:
    case QTK_IASR_LC:
        if (0 == iasr->end_flag) {
            wtk_debug(">>>>>>> reset  send end\n");
            qtk_decoder_wrapper_feed(iasr->ins.dw, NULL, 0, 1);
        }
        qtk_decoder_wrapper_reset(iasr->ins.dw);
        break;
    case QTK_IASR_GR_NEW:
        if (0 == iasr->end_flag) {
            wtk_debug(">>>>>>> reset  send end\n");
            qtk_asr_wrapper_feed(iasr->ins.aw, NULL, 0, 1);
        }
        qtk_asr_wrapper_reset(iasr->ins.aw);
        break;
    default:
        break;
    }
}

wtk_string_t qtk_iasr_get_result(qtk_iasr_t *iasr) {
    wtk_string_t v;
    wtk_string_set(&v, 0, 0);
    switch (iasr->cfg->type) {
    case QTK_IASR_LC:
    case QTK_IASR_GR:
        // wtk_debug(">>>>>>get result\n");
        qtk_decoder_wrapper_get_result(iasr->ins.dw, &v);
        break;
    case QTK_IASR_GR_NEW:
        // wtk_debug(">>>>>>get result\n");
        qtk_asr_wrapper_get_result(iasr->ins.aw, &v);
        break;
    default:
        break;
    }

end:
    return v;
}

int qtk_iasr_set_ebnf(qtk_iasr_t *iasr, char *ebnf, int bytes) {
    wtk_egram_reset(iasr->egram);
    wtk_egram_ebnf2fst(iasr->egram, ebnf, bytes);
    if (iasr->ins.dw->cfg->use_lite) {
        wtk_egram_dump2(iasr->egram, iasr->ins.dw->asr[0]->dec_lite->net);
    } else {
        wtk_egram_dump2(iasr->egram, iasr->ins.dw->asr[0]->dec->net);
    }
    return 0;
}

int qtk_iasr_set_xbnf(qtk_iasr_t *iasr, char *xbnf, int bytes) {
    if (iasr->cfg->type == QTK_IASR_SPX) {
        return 0;
    }
    if (iasr->cfg->use_xbnf) {
        qtk_decoder_wrapper_set_xbnf(iasr->ins.dw, xbnf, bytes);
    }
    return 0;
}

void qtk_iasr_set_hint_notify(qtk_iasr_t *iasr, void *ths,
                              qtk_iasr_set_hint_notify_f notify) {
    iasr->hint_notify = notify;
    iasr->hint_ths = ths;
}

void qtk_iasr_set_spxfinal_notify(qtk_iasr_t *iasr, void *ths,
                              qtk_iasr_set_spxfinal_notify_f notify) {
    if (iasr->cfg->type == QTK_IASR_SPX){
        iasr->spxfinal_notify = notify;
        iasr->spxfinal_ths = ths;
    }
}

void qtk_iasr_set_result_notify(qtk_iasr_t *iasr, void *ths, qtk_iasr_set_result_f notify)
{
    iasr->result_ths = ths;
    iasr->result_notify = notify;
}

void qtk_iasr_set_hw_notify(qtk_iasr_t *iasr, void *ths,
                            qtk_iasr_set_hw_notify_f notify_f) {
    iasr->hw_ths = ths;
    iasr->hw_notify = notify_f;
}

int qtk_iasr_hw_upload(qtk_iasr_t *iasr, char *res_fn, int flag) {
    int ret;

    if (iasr->cfg->type != QTK_IASR_SPX) {
        return 0;
    }
    if (!iasr->hw) {
        wtk_log_log0(iasr->session->log, "use_hw_upload is not set in cfg.\n");
        _qtk_error(iasr->session, _QTK_HOTWORD_OP_INVALID);
        return -1;
    }
    ret = qtk_hw_upload(iasr->hw, res_fn, flag);
    return ret;
}

int qtk_iasr_hw_update(qtk_iasr_t *iasr, char *res_fn, int flag) {
    int ret;

    if (iasr->cfg->type != QTK_IASR_SPX) {
        return 0;
    }
    if (!iasr->hw) {
        wtk_log_log0(iasr->session->log, "use_hw_upload is not set in cfg.\n");
        _qtk_error(iasr->session, _QTK_HOTWORD_OP_INVALID);
        return -1;
    }
    ret = qtk_hw_update(iasr->hw, res_fn, flag);
    return ret;
}

int qtk_iasr_get_hotword(qtk_iasr_t *iasr) {
    if (iasr->cfg->type != QTK_IASR_SPX) {
        return 0;
    }
    if (!iasr->hw) {
        wtk_log_log0(iasr->session->log, "use_hw_upload is not set in cfg.\n");
        _qtk_error(iasr->session, _QTK_HOTWORD_OP_INVALID);
        return -1;
    }
    /* null pointer warning */
    qtk_hw_get_hotword(iasr->hw);

    return 0;
}

void qtk_iasr_set_res(qtk_iasr_t *iasr, char *data, int len) {
    if (iasr->cfg->type != QTK_IASR_SPX) {
        return;
    }
    qtk_spx_set_res(iasr->ins.spx, data, len);
}
void qtk_iasr_set_coreType(qtk_iasr_t *iasr, char *data, int len) {
    if (iasr->cfg->type != QTK_IASR_SPX) {
        return;
    }
    qtk_spx_set_coreType(iasr->ins.spx, data, len);
}
void qtk_iasr_set_skip_space(qtk_iasr_t *iasr, int skip_space) {

    if (iasr->cfg->type != QTK_IASR_SPX) {
        return;
    }
    qtk_spx_set_skip_space(iasr->ins.spx, skip_space);
}
void qtk_iasr_set_idle_time(qtk_iasr_t *iasr, int val)
{
    iasr->cfg->idle_time=val;
#ifndef USE_KSD_EBNF
    if(iasr->cfg->use_general_asr == 0)
    {
        qtk_asr_wrapper_set_idle_time(iasr->ins.aw, val);
    }
#endif
}

int qtk_iasr_update_cmds(qtk_iasr_t *iasr,char* words,int len){
    if (iasr->cfg->type != QTK_IASR_GR && iasr->cfg->type != QTK_IASR_GR_NEW) return -1;

    int ret=-1;
    if(iasr->cfg->type == QTK_IASR_GR_NEW)
    {
#ifndef USE_KSD_EBNF
        if(len > 0)
        {
            wtk_debug("klen=%d data=[%.*s]\n",len,len,words);
#ifdef USE_HUACHUANG
            ret = qtk_asr_wrapper_set_contacts(iasr->ins.aw, words, len);
#else
            ret = qtk_asr_wrapper_set_text(iasr->ins.aw, words, len);
#endif
            if(ret != 0)
            {
                wtk_debug("set contacts fialed!\n");
            }
        }
#endif
        return ret;
    }

    ret=wtk_egram_update_cmds(iasr->egram,words,len);
    return ret;
}
