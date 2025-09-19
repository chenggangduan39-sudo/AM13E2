#include "qtk_mod_am13e2_cfg.h"

int qtk_mod_am13e2_cfg_init(qtk_mod_am13e2_cfg_t *cfg)
{
    qtk_record_cfg_init(&cfg->rcd);
    qtk_record_cfg_init(&cfg->rcd2);
    qtk_record_cfg_init(&cfg->rcd3);
    qtk_record_cfg_init(&cfg->rcd4);
	qtk_play_cfg_init(&cfg->usbaudio);
    qtk_play_cfg_init(&cfg->lineout);
    qtk_play_cfg_init(&cfg->speaker);

	wtk_string_set(&cfg->cache_path, 0, 0);
    wtk_string_set(&cfg->gain_path, 0, 0);
    wtk_string_set(&cfg->linein_check_path, 0, 0);
    wtk_string_set(&cfg->gainnetbf_fn, 0, 0);

    wtk_string_set(&cfg->mic_check_rcd_fn, 0, 0);
    wtk_string_set(&cfg->mic_check_play_fn, 0, 0);

    wtk_string_set(&cfg->vboxebf_fn, 0, 0);
    wtk_string_set(&cfg->array_vbox_fn, 0, 0);
    wtk_string_set(&cfg->denoise_vbox_fn, 0, 0);
    cfg->gainnetbf_cfg = NULL;
    cfg->vboxebf_cfg = NULL;
    cfg->avboxebf_cfg = NULL;
    cfg->nmic = 0;
	cfg->use_usbaudio = 0;
    cfg->debug = 0;
	cfg->use_log = 0;
    cfg->use_log_wav = 0;
    cfg->use_out_resample = 1;
    cfg->mic_shift = 1.0;
    cfg->echo_shift = 1.0;
    cfg->sleep_time = 3600;
    cfg->use_dev = 1;
    cfg->mic_sum = 10000.0f;
    cfg->use_test3a=0;
    cfg->sil_time = 9;
    cfg->use_3abfio = 1;
    cfg->use_out_mode = 1;
    cfg->use_lineout = 0;
    // cfg->linein_channel = 1;
    cfg->aspk_channel = 7;
    cfg->aspk_mode=0;
    cfg->use_array=1;
    cfg->use_linein_mic=0;
    cfg->linein_channel=5;
    cfg->use_linein_courseware=0;
    cfg->ch0_shifttm=0;
    cfg->ch1_shifttm=0;
    cfg->ch2_shifttm=0;
    cfg->ch3_shifttm=0;
    cfg->ch4_shifttm=0;
    cfg->ch5_shifttm=0;
    cfg->ch6_shifttm=0;
    cfg->ch7_shifttm=0;
    cfg->cha_shifttm=0;
    cfg->use_linein_check = 0;
    cfg->use_rcd3=1;
    cfg->use_rcd=1;
    cfg->use_rcd4=1;
    cfg->use_line_in=0;
    cfg->use_linein_courseware_touac=0;
    cfg->use_headset=0;
    cfg->use_spkout=0;
    cfg->use_wooferout=0;
    cfg->use_speaker_left=1;
    cfg->use_speaker_right=1;
    cfg->use_speaker =0;
    cfg->use_linein_courseware_touac=0;
    cfg->use_mainlineout=0;
    cfg->use_wooflineout=0;
    cfg->use_meetinglineout=0;
    cfg->use_expandlineout=0;

    return 0;
}

int qtk_mod_am13e2_cfg_clean(qtk_mod_am13e2_cfg_t *cfg)
{
    if(cfg->gainnetbf_cfg){
        qtk_gainnetbf_cfg_delete(cfg->gainnetbf_cfg);
    }
    if(cfg->vboxebf_cfg){
        qtk_vboxebf_cfg_delete(cfg->vboxebf_cfg);
    }
    if(cfg->avboxebf_cfg){
        qtk_vboxebf_cfg_delete(cfg->avboxebf_cfg);
    }

    qtk_record_cfg_clean(&cfg->rcd);
    qtk_record_cfg_clean(&cfg->rcd2);
    qtk_record_cfg_clean(&cfg->rcd3);
    qtk_record_cfg_clean(&cfg->rcd4);
	qtk_play_cfg_clean(&cfg->usbaudio);
    qtk_play_cfg_clean(&cfg->lineout);
    qtk_play_cfg_clean(&cfg->speaker);

    return 0;
}

int qtk_mod_am13e2_cfg_update_local(qtk_mod_am13e2_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_string_v(main, cfg, cache_path, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, gain_path, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, linein_check_path, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, gainnetbf_fn, v);
    
    wtk_local_cfg_update_cfg_string_v(main, cfg, mic_check_rcd_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, mic_check_play_fn, v);

    wtk_local_cfg_update_cfg_string_v(main, cfg, vboxebf_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, array_vbox_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, denoise_vbox_fn, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_usbaudio, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_lineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_speaker, v);
    wtk_local_cfg_update_cfg_b(main, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log_wav, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_out_resample, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_dev, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_test3a, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_3abfio, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_array, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_linein_check, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_rcd, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_linein_mic, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_rcd3, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_rcd4, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_headset, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_spkout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_wooferout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_line_in, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_linein_courseware_touac, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_linein_courseware, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_mainlineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_wooflineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_meetinglineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_expandlineout, v);

    wtk_local_cfg_update_cfg_i(main, cfg, sleep_time, v);
    wtk_local_cfg_update_cfg_i(main, cfg, sil_time, v);
    wtk_local_cfg_update_cfg_i(main, cfg, linein_channel, v);
    wtk_local_cfg_update_cfg_i(main, cfg, aspk_channel, v);
    wtk_local_cfg_update_cfg_i(main, cfg, aspk_mode, v);
    wtk_local_cfg_update_cfg_i(main, cfg, ch0_shifttm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, ch1_shifttm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, ch2_shifttm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, ch3_shifttm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, ch4_shifttm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, ch5_shifttm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, ch6_shifttm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, ch7_shifttm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, cha_shifttm, v);
    wtk_local_cfg_update_cfg_f(main, cfg, mic_sum, v);
    wtk_local_cfg_update_cfg_f(main, cfg, use_out_mode, v);
    wtk_local_cfg_update_cfg_f(main, cfg, mic_shift, v);
    wtk_local_cfg_update_cfg_f(main, cfg, echo_shift, v);

    lc = wtk_local_cfg_find_lc_s(main, "rcd");
    if(lc){
        qtk_record_cfg_update_local(&cfg->rcd, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "rcd2");
    if(lc){
        qtk_record_cfg_update_local(&cfg->rcd2, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "rcd3");
    if(lc){
        qtk_record_cfg_update_local(&cfg->rcd3, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "rcd4");
    if(lc){
        qtk_record_cfg_update_local(&cfg->rcd4, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "usbaudio");
    if(lc){
        qtk_play_cfg_update_local(&cfg->usbaudio, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "lineout");
    if(lc){
        qtk_play_cfg_update_local(&cfg->lineout, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "speaker");
    if(lc){
        qtk_play_cfg_update_local(&cfg->speaker, lc);
    }
    
    return 0;
}

int qtk_mod_am13e2_cfg_update(qtk_mod_am13e2_cfg_t *cfg)
{
    qtk_record_cfg_update(&cfg->rcd);
    qtk_record_cfg_update(&cfg->rcd2);
    qtk_record_cfg_update(&cfg->rcd3);
    qtk_record_cfg_update(&cfg->rcd4);
	qtk_play_cfg_update(&cfg->usbaudio);
    qtk_play_cfg_update(&cfg->lineout);
    qtk_play_cfg_update(&cfg->speaker);

    int ret=0;
    wtk_debug("mic_check_rcd_fn.len =%d,mic_check_play_fn.len =%d\n",cfg->mic_check_rcd_fn.len,cfg->mic_check_play_fn.len);
    if(cfg->mic_check_rcd_fn.len > 0){
        ret = -1;
        cfg->mic_check_rcd_cfg = wtk_mic_check_cfg_new(cfg->mic_check_rcd_fn.data);
        if(cfg->mic_check_rcd_cfg){ret = 0;}
    }
    if(cfg->mic_check_play_fn.len > 0){
        ret = -1;
        cfg->mic_check_play_cfg = wtk_mic_check_cfg_new(cfg->mic_check_play_fn.data);
        if(cfg->mic_check_play_cfg){ret = 0;}
    }
    if(cfg->gainnetbf_fn.len > 0){
        ret = -1;
        cfg->gainnetbf_cfg = qtk_gainnetbf_cfg_new(cfg->gainnetbf_fn.data);
        if(cfg->gainnetbf_cfg){ret = 0;}
    }

    if(cfg->vboxebf_fn.len > 0){
        ret = -1;
        cfg->vboxebf_cfg = qtk_vboxebf_cfg_new(cfg->vboxebf_fn.data);
        if(cfg->vboxebf_cfg){ret = 0;}
    }
    
    if(cfg->array_vbox_fn.len > 0 && cfg->use_array){
        ret = -1;
        cfg->avboxebf_cfg = qtk_vboxebf_cfg_new(cfg->array_vbox_fn.data);
        if(cfg->avboxebf_cfg){ret = 0;}
    }
    wtk_debug("----------denoise_vbox_fn.len=%d   ,use_linein_mic=%d\n",cfg->denoise_vbox_fn.len,cfg->use_linein_mic);
    if(cfg->denoise_vbox_fn.len > 0){
        ret = -1;
        wtk_debug("----------------------?>>>>>>\n");
        cfg->denoisebf_cfg = qtk_vboxebf_cfg_new(cfg->denoise_vbox_fn.data);
        wtk_debug("----------------------?>>>>>>\n");
        if(cfg->denoisebf_cfg){ret = 0;}
    }
    return ret;
}
