#include "qtk_engine_param.h"

static wtk_string_t qtk_engine_param_str[] = {
    wtk_string("cfg"),        // 0   指定资源位置
    wtk_string("wakewrd"),    // 1   使用语法唤醒时，设置唤醒词
    wtk_string("mic"),        // 2   mic数量
    wtk_string("tts_volume"), // 3   合成音音量
    wtk_string("tts_speed"),  // 4   合成音速度 越大速度越慢
    wtk_string("tts_pitch"),  // 5   合成音音调
    wtk_string("winStep"), // 6   内置线程模式下，每帧数据传入的长度。
    wtk_string("left_margin"),  // 7   left margin
    wtk_string("right_margin"), // 8   right margin
    wtk_string("idle_time"),    // 9	  超时睡眠时间
    wtk_string("use_bin"),      // 10  资源使用方式，默认为1
    wtk_string("use_thread"),   // 11  是否使用内置thread
    wtk_string("use_json"),     // 12  输入文本格式，默认为0
    wtk_string("active"),       // 13  启动是否激活
    wtk_string("idle"),         // 14  超时是否睡眠
    wtk_string("use_oneMic"),   // 15  是否使用单麦阵列算法
    wtk_string("asr_min_time"), // 16  识别最小vad音频长度（ms）
    wtk_string("asr_max_time"), // 17  识别最大vad音频长度（ms）
    wtk_string("output_ogg"), // 18  回调音频是否进行ogg压缩(bfio,vad)
    wtk_string("syn"),        // 19  内置线程使用join机制,默认1
    wtk_string("timealive"),  // 20  engine max alive time
    wtk_string("wake_conf"),  // 21  设置唤醒门阀
    wtk_string("wake_logwav"), // 22  保存唤醒音频

    wtk_string("use_hotword"),     // 23  热词功能
    wtk_string("use_hw_upload"),   // 24  热词文件上传
    wtk_string("use_hint"),        // 25  识别中间结果回调
    wtk_string("bfio_reset_time"), // 26  bfio重置计时
    wtk_string("usr_bfio_reset"),  // 27  启用bfio重置
    wtk_string("playfn"),                   		//28  播放音频文件路径
    wtk_string("use_rearrange"),                    //29  数据重排

    wtk_string("eq_offset"),                    	//30  判断相同的差值
    wtk_string("nil_er"),                    		//31  判断为空的最小能量值
    wtk_string("mic_corr_er"),                    	//32  mic的相关性方差值
    wtk_string("mic_corr_aver"),                    //33  mic的相关性方差的平均值
    wtk_string("mic_energy_er"),                    //34  mic的能量方差值
    wtk_string("mic_energy_aver"),                  //35  mic的能量平均方差值
    wtk_string("spk_corr_er"),                    	//36  回采的相关性方差值
    wtk_string("spk_corr_aver"),                    //37  回采的相关性方差的平均值
    wtk_string("spk_energy_er"),                    //38  回采的能量方差值
    wtk_string("spk_energy_aver"),                  //39  回采的能量平均方差值
    wtk_string("use_xcorr"),                        //40  启用相关性检测
    wtk_string("use_logwav"),                       //41  保存音频
    wtk_string("consist_fn"),                       //42  产测原始音频路径

    wtk_string("log_wav_path"),                     //43  处理后音频路径
    wtk_string("gbias"),                    		//44  降噪强度
    wtk_string("theta"),                    		//45  定向拾音角度
    wtk_string("theta_range"),                      //46  定向拾音角度范围
    wtk_string("use_ssl"),                          //47  定位功能控制
    wtk_string("energy_sum"),                       //48  抛出角度的最小能量
    wtk_string("online_tms"),                       //49  检测角度的时长
    wtk_string("use_cfg"),                          //50  是否使用配置文件的参数
    wtk_string("mic_shift"),                        //51  原始mic数据的乘数
    wtk_string("spk_shift"),                        //52  原始回采数据的乘数
    wtk_string("echo_shift"),                       //53  处理后数据的乘数
    wtk_string("spk_channel"),                      //54  回采的声道数
    wtk_string("agca"),                             //55  自动增益参数
    wtk_string("use_equal"),                        //56  启用通道相同判断
    wtk_string("use_inputpcm"),                     //57  保存input.pcm 和 input-x.pcm
    wtk_string("inputpcm_fn"),                      //58  保存input.pcm的路径
    wtk_string("specsum_fs"),                       //59  声源定位声音的最低频率
    wtk_string("specsum_fe"),                       //60  声源定位声音的最高频率
    wtk_string("lf"),                               //61  频率相关度
    wtk_string("theta_step"),                       //62  声源定位精度
    wtk_string("use_fftsbf"),                       //63  降噪，取值0和1，默认为1，为0时降噪强度降低
    wtk_string("bfmu"),                             //64  拾音降噪,默认值为1，范围[0-1],值越大降噪越强
    wtk_string("echo_bfmu"),                        //65  回声消除强度等级，默认值为1，范围[0-1]值越大，回声消除越厉害
    wtk_string("use_maskssl"),                      //66  定位maskssl功能控制
    wtk_string("use_maskssl2"),                     //67  定位maskssl2功能控制
    wtk_string("spenr_thresh"),                     //68  
    wtk_string("use_cnon"),                         //69  
    wtk_string("sym"),                              //70  
    wtk_string("use_erlssingle"),                   //71  
    wtk_string("online_frame_step"),                //72  
    wtk_string("noise_suppress"),                   //73  
    wtk_string("echo_suppress"),                    //74  
    wtk_string("echo_suppress_active"),             //75
    wtk_string("channel"),                          //76  声道数，默认值为0
    wtk_string("resample_in_rate"),                 //77  重采样输入音频采样率
    wtk_string("resample_out_rate"),                //78  重采样输出音频采样率
    wtk_string("use_resample"),                     //79  重采样控制
    wtk_string("out_channel"),                      //80  输出的通道数
};

void qtk_engine_param_init(qtk_engine_param_t *param) 
{
    param->session           = NULL;
    param->cfg               = NULL;
    param->wakewrd           = NULL;
    param->mic_array         = NULL;
	param->playfn            = NULL;
	param->consist_fn        = NULL;
	param->inputpcm_fn       = "/sdcard";
    param->tts_volume        = -1.0f;
    param->tts_speed         = -1.0f;
    param->tts_pitch         = -1.0f;
    param->mic               = 0;
    param->winStep           = 20;
    param->left_margin       = 0;
    param->right_margin      = 0;
    param->idle_time         = 3000;
    param->use_bin           = 0;
    param->use_thread        = 0;
    param->use_json          = 0;
    param->active            = 0;
    param->idle              = 0;
    param->use_oneMic        = 0;
    param->asr_min_time      = 500;
    param->asr_max_time      = 10000;
    param->output_ogg        = 0;
    param->syn               = 1;
    param->timealive         = -1;
    param->wake_conf         = 0;
    param->wake_logwav       = 0;

    param->use_hotword       = 0;
    param->use_hw_upload     = 0;
    param->use_hint          = 0;

    param->bfio_reset_time   = 43200;
    param->use_bfio_reset    = 1;
	param->use_rearrange     = 0;

	param->use_equal         = 1;
	param->use_xcorr         = 1;
	param->eq_offset         = 35000.0f;
	param->mic_corr_er       = 200.0f;
	param->mic_corr_aver     = 180.0f;
	param->mic_energy_er     = 800.0f;
	param->mic_energy_aver   = 700.0f;

	param->spk_corr_er       = 200.0f;
	param->spk_corr_aver     = 180.0f;
	param->spk_energy_er     = 600.0f;
	param->spk_energy_aver   = 500.0f;

	param->nil_er            = 100.0f;

	param->use_logwav        = 0;
	param->log_wav_path      = NULL;
	param->theta             = 90;
	param->theta_range       = 15;
	param->energy_sum        = 0.75;
	param->use_cfg           = 1;
	param->mic_shift         = 1.0;
	param->spk_shift         = 1.0;
	param->echo_shift        = 1.0;
	param->spk_channel       = 1;
	param->use_inputpcm      = 0;

	param->use_ssl           = -1;
	param->use_maskssl       = -1;
    param->use_maskssl2      = -1;
	param->online_tms        = -1;
    param->online_frame_step = -1;

	param->gbias             = -1;
	param->agca              = -1;
	param->specsum_fs        = -1;
	param->specsum_fe        = -1;
	param->lf                = -1;
	param->theta_step        = -1;
	
	param->use_fftsbf        = -1;
	param->bfmu              = -1;
	param->echo_bfmu         = -1;
	param->spenr_thresh      = -100000;
	param->use_cnon          = -1;
	param->sym               = -100000;

	param->noise_suppress    = 100000;
	param->echo_suppress     = 100000;
	param->echo_suppress_active = 100000;

    param->use_erlssingle    = -1;
    param->channel           = 0;
    param->resample_in_rate  = -1;
    param->resample_out_rate = -1;

    param->use_resample      = 0;
    param->out_channel       = 1;

}

void qtk_engine_param_set_session(qtk_engine_param_t *param,
                                  qtk_session_t *session) {
    param->session = session;
}

int qtk_engine_param_feed(qtk_engine_param_t *param, wtk_local_cfg_t *lc) {
    wtk_string_t **vv;
    wtk_string_t *v;
    wtk_array_t *array;
    char buf[16];
    int ret;
    int i, j;

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[0].data, qtk_engine_param_str[0].len);
    if (v) {
        param->cfg = v->data;
    } else {
        wtk_log_warn0(param->session->log, "cfg fn not specified.");
        _qtk_error(param->session, _QTK_CFG_NOTSET);
        ret = -1;
        goto end;
    }

    ret = wtk_file_exist(param->cfg);
    if (ret != 0) {
        wtk_log_warn(param->session->log, "cfg fn [%s] not exist.", param->cfg);
        _qtk_error(param->session, _QTK_CFG_NOTEXIST);
        goto end;
    }

    ret = wtk_file_readable(param->cfg);
    if (ret != 0) {
        wtk_log_warn(param->session->log, "cfg_fn [%s] not readable.", param->cfg);
        _qtk_error(param->session, _QTK_CFG_UNREADABLE);
        goto end;
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[1].data, qtk_engine_param_str[1].len);
    if (v) {
        param->wakewrd = wtk_string_dup_data_pad0(v->data, v->len);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[2].data, qtk_engine_param_str[2].len);
    if (v) {
        param->mic = atoi(v->data);
        wtk_debug("=====mic = %d\n", param->mic);
        param->mic_array = (float **)wtk_malloc(sizeof(float *) * param->mic);

        for (i = 0; i < param->mic; ++i) {
            ret = sprintf(buf, "mic%d", i + 1);
            array = wtk_local_cfg_find_array(lc, buf, ret);
            if (!array) {
                wtk_log_warn(param->session->log, "%s set not found", buf);
                _qtk_error(param->session, _QTK_MIC_POS_FMTERR);
                // ret = -1;
                // goto end;
                continue;
            }
            if (array->nslot != 3) {
                wtk_log_warn(param->session->log, "%s quantity error", buf);
                _qtk_error(param->session, _QTK_MIC_POS_FMTERR);
                // ret = -1;
                // goto end;
                continue;
            }
            param->mic_array[i] = wtk_malloc(sizeof(float) * 3);
            vv = (wtk_string_t **)array->slot;
            for (j = 0; j < 3; ++j) {
                param->mic_array[i][j] = wtk_str_atof(vv[j]->data, vv[j]->len);
            }
        }
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[3].data, qtk_engine_param_str[3].len);
    if (v) {
        param->tts_volume = atof(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[4].data, qtk_engine_param_str[4].len);
    if (v) {
        param->tts_speed = atof(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[5].data, qtk_engine_param_str[5].len);
    if (v) {
        param->tts_pitch = atof(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[6].data, qtk_engine_param_str[6].len);
    if (v) {
        param->winStep = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[7].data, qtk_engine_param_str[7].len);
    if (v) {
        param->left_margin = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[8].data, qtk_engine_param_str[8].len);
    if (v) {
        param->right_margin = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[9].data, qtk_engine_param_str[9].len);
    if (v) {
        param->idle_time = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[10].data, qtk_engine_param_str[10].len);
    if (v) {
        param->use_bin = atoi(v->data) == 1 ? 1 : 0;
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[11].data, qtk_engine_param_str[11].len);
    if (v) {
        param->use_thread = atoi(v->data) == 1 ? 1 : 0;
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[12].data, qtk_engine_param_str[12].len);
    if (v) {
        param->use_json = atoi(v->data) == 1 ? 1 : 0;
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[13].data, qtk_engine_param_str[13].len);
    if (v) {
        param->active = atoi(v->data) == 1 ? 1 : 0;
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[14].data, qtk_engine_param_str[14].len);
    if (v) {
        param->idle = atoi(v->data) == 1 ? 1 : 0;
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[15].data, qtk_engine_param_str[15].len);
    if (v) {
        param->use_oneMic = atoi(v->data) == 1 ? 1 : 0;
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[16].data, qtk_engine_param_str[16].len);
    if (v) {
        param->asr_min_time = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[17].data, qtk_engine_param_str[17].len);
    if (v) {
        param->asr_max_time = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[18].data, qtk_engine_param_str[18].len);
    if (v) {
        param->output_ogg = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[19].data, qtk_engine_param_str[19].len);
    if (v) {
        param->syn = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[20].data, qtk_engine_param_str[20].len);
    if (v) {
        param->timealive = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[21].data, qtk_engine_param_str[21].len);
    if (v) {
        param->wake_conf = atof(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[22].data, qtk_engine_param_str[22].len);
    if (v) {
        param->wake_logwav = atol(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[23].data, qtk_engine_param_str[23].len);
    if (v) {
        param->use_hotword = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[24].data, qtk_engine_param_str[24].len);
    if (v) {
        param->use_hw_upload = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[25].data, qtk_engine_param_str[25].len);
    if (v) {
        param->use_hint = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[26].data, qtk_engine_param_str[26].len);
    if (v) {
        param->bfio_reset_time = atoi(v->data);
    }

    v = wtk_local_cfg_find_string(lc, qtk_engine_param_str[27].data, qtk_engine_param_str[27].len);
    if (v) {
        param->use_bfio_reset = atoi(v->data);
    }

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[28].data,qtk_engine_param_str[28].len);
	if(v) {
		param->playfn = v->data;
	}
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[29].data,qtk_engine_param_str[29].len);
	if(v) {
		param->use_rearrange = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[30].data,qtk_engine_param_str[30].len);
	if(v) {
		param->eq_offset = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[31].data,qtk_engine_param_str[31].len);
	if(v) {
		param->nil_er = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[32].data,qtk_engine_param_str[32].len);
	if(v) {
		param->mic_corr_er = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[33].data,qtk_engine_param_str[33].len);
	if(v) {
		param->mic_corr_aver = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[34].data,qtk_engine_param_str[34].len);
	if(v) {
		param->mic_energy_er = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[35].data,qtk_engine_param_str[35].len);
	if(v) {
		param->mic_energy_aver = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[36].data,qtk_engine_param_str[36].len);
	if(v) {
		param->spk_corr_er = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[37].data,qtk_engine_param_str[37].len);
	if(v) {
		param->spk_corr_aver = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[38].data,qtk_engine_param_str[38].len);
	if(v) {
		param->spk_energy_er = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[39].data,qtk_engine_param_str[39].len);
	if(v) {
		param->spk_energy_aver = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[40].data,qtk_engine_param_str[40].len);
	if(v) {
		param->use_xcorr = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[41].data,qtk_engine_param_str[41].len);
	if(v) {
		param->use_logwav = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[42].data,qtk_engine_param_str[42].len);
	if(v) {
		param->consist_fn = v->data;
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[43].data,qtk_engine_param_str[43].len);
	if(v) {
		param->log_wav_path = wtk_string_dup_data_pad0(v->data,v->len);
	}
    
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[44].data,qtk_engine_param_str[44].len);
	if(v) {
		param->gbias = atof(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[45].data,qtk_engine_param_str[45].len);
	if(v) {
		param->theta = atoi(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[46].data,qtk_engine_param_str[46].len);
	if(v) {
		param->theta_range = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[47].data,qtk_engine_param_str[47].len);
	if(v) {
		param->use_ssl = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[48].data,qtk_engine_param_str[48].len);
	if(v) {
		param->energy_sum = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[49].data,qtk_engine_param_str[49].len);
	if(v) {
		param->online_tms = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[50].data,qtk_engine_param_str[50].len);
	if(v) {
		param->use_cfg = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[51].data,qtk_engine_param_str[51].len);
	if(v) {
		param->mic_shift = atof(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[52].data,qtk_engine_param_str[52].len);
	if(v) {
		param->spk_shift = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[53].data,qtk_engine_param_str[53].len);
	if(v) {
		param->echo_shift = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[54].data,qtk_engine_param_str[54].len);
	if(v) {
		param->spk_channel = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[55].data,qtk_engine_param_str[55].len);
	if(v) {
		param->agca = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[56].data,qtk_engine_param_str[56].len);
	if(v) {
		param->use_equal = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[57].data,qtk_engine_param_str[57].len);
	if(v) {
		param->use_inputpcm = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[58].data,qtk_engine_param_str[58].len);
	if(v) {
		param->inputpcm_fn = v->data;
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[59].data,qtk_engine_param_str[59].len);
	if(v) {
		param->specsum_fs = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[60].data,qtk_engine_param_str[60].len);
	if(v) {
		param->specsum_fe = atoi(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[61].data,qtk_engine_param_str[61].len);
	if(v) {
		param->lf = atoi(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[62].data,qtk_engine_param_str[62].len);
	if(v) {
		param->theta_step = atoi(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[63].data,qtk_engine_param_str[63].len);
	if(v) {
		param->use_fftsbf = atoi(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[64].data,qtk_engine_param_str[64].len);
	if(v) {
		param->bfmu = atof(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[65].data,qtk_engine_param_str[65].len);
	if(v) {
		param->echo_bfmu = atof(v->data);
	}
	
	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[66].data,qtk_engine_param_str[66].len);
	if(v) {
		param->use_maskssl = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[67].data,qtk_engine_param_str[67].len);
	if(v) {
		param->use_maskssl2 = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[68].data,qtk_engine_param_str[68].len);
	if(v) {
		param->spenr_thresh = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[69].data,qtk_engine_param_str[69].len);
	if(v) {
		param->use_cnon = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[70].data,qtk_engine_param_str[70].len);
	if(v) {
		param->sym = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[71].data,qtk_engine_param_str[71].len);
	if(v) {
		param->use_erlssingle = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[72].data,qtk_engine_param_str[72].len);
	if(v) {
		param->online_frame_step = atoi(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[73].data,qtk_engine_param_str[73].len);
	if(v) {
		param->noise_suppress = atof(v->data);
	}
	
    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[74].data,qtk_engine_param_str[74].len);
	if(v) {
		param->echo_suppress = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[75].data,qtk_engine_param_str[75].len);
	if(v) {
		param->echo_suppress_active = atof(v->data);
	}

	v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[76].data,qtk_engine_param_str[76].len);
	if(v) {
		param->channel = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[77].data,qtk_engine_param_str[77].len);
	if(v) {
		param->resample_in_rate = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[78].data,qtk_engine_param_str[78].len);
	if(v) {
		param->resample_out_rate = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[79].data,qtk_engine_param_str[79].len);
	if(v) {
		param->use_resample = atoi(v->data);
	}

    v = wtk_local_cfg_find_string(lc,qtk_engine_param_str[80].data,qtk_engine_param_str[80].len);
	if(v) {
		param->out_channel = atoi(v->data);
	}
    qtk_engine_param_print(param);

    ret = 0;
end:
    return ret;
}

void qtk_engine_param_clean(qtk_engine_param_t *param) {
    int i;

    if (param->mic_array) {
        for (i = 0; i < param->mic; ++i) {
            if (param->mic_array[i]) {
                wtk_free(param->mic_array[i]);
            }
        }
        wtk_free(param->mic_array);
    }
    if (param->wakewrd) {
        wtk_string_delete(param->wakewrd);
    }
}

void qtk_engine_param_print(qtk_engine_param_t *param) {
    int i, j;

    if (param->session->log) {
        wtk_log_log0(param->session->log,
                     "================== engine param ==============.");
        if (param->cfg) {
            wtk_log_log(param->session->log, "cfg = %s", param->cfg);
        }
        if (param->wakewrd) {
            wtk_log_log(param->session->log, "wakewrd = %s", param->cfg);
        }
        if (param->mic > 0 && param->mic_array) {
            wtk_log_log(param->session->log, "mic count = %d", param->mic);
            for (i = 0; i < param->mic; ++i) {
                for (j = 0; j < 3; ++j) {
                    wtk_log_log(param->session->log, "mic[%d][%d] = %f", i, j,
                                param->mic_array[i][j]);
                }
            }
        }
        wtk_log_log(param->session->log, "tts_volume = %f", param->tts_volume);
        wtk_log_log(param->session->log, "tts_speed = %f", param->tts_speed);
        wtk_log_log(param->session->log, "tts_pitch = %f", param->tts_pitch);
        wtk_log_log(param->session->log, "winStep = %d", param->winStep);
        wtk_log_log(param->session->log, "left margin = %d",
                    param->left_margin);
        wtk_log_log(param->session->log, "right margin = %d",
                    param->right_margin);
        wtk_log_log(param->session->log, "idle time = %d", param->idle_time);
        wtk_log_log(param->session->log, "use bin = %d", param->use_bin);
        wtk_log_log(param->session->log, "use_thread = %d", param->use_thread);
        wtk_log_log(param->session->log, "use_json = %d", param->use_json);
        wtk_log_log(param->session->log, "active = %d", param->active);
        wtk_log_log(param->session->log, "idle = %d", param->active);
        wtk_log_log(param->session->log, "use_oneMic = %d", param->use_oneMic);
        wtk_log_log(param->session->log, "asr_min_time = %d",
                    param->asr_min_time);
        wtk_log_log(param->session->log, "asr_max_time = %d",
                    param->asr_max_time);
        wtk_log_log(param->session->log, "output_ogg = %d", param->output_ogg);
        wtk_log_log(param->session->log, "syn = %d", param->syn);
        wtk_log_log(param->session->log, "timealive = %d", param->timealive);
        if (param->wake_conf) {
            wtk_log_log(param->session->log, "wake_conf = %f",
                        param->wake_conf);
        }
        wtk_log_log(param->session->log, "wake_logwav = %d",
                    param->wake_logwav);
        wtk_log_log(param->session->log, "use_hotword = %d",
                    param->use_hotword);
        wtk_log_log(param->session->log, "use_hw_upload = %d",
                    param->use_hw_upload);
        wtk_log_log(param->session->log, "use_hint = %d", param->use_hint);
        wtk_log_log(param->session->log, "use_bfio_reset = %d",
                    param->use_bfio_reset);
        wtk_log_log(param->session->log, "bfio_reset_time = %d",
                    param->bfio_reset_time);
		wtk_log_log(param->session->log,"playfn = %s", param->playfn);
		wtk_log_log(param->session->log,"use_rearrange = %d", param->use_rearrange);
		wtk_log_log(param->session->log,"eq_offset = %f", param->eq_offset);
		wtk_log_log(param->session->log,"nil_er = %f", param->nil_er);
		wtk_log_log(param->session->log,"mic_corr_er = %f", param->mic_corr_er);
		wtk_log_log(param->session->log,"mic_corr_aver = %f", param->mic_corr_aver);
		wtk_log_log(param->session->log,"mic_energy_er = %f", param->mic_energy_er);
		wtk_log_log(param->session->log,"mic_energy_aver = %f", param->mic_energy_aver);
		wtk_log_log(param->session->log,"spk_corr_er = %f", param->spk_corr_er);
		wtk_log_log(param->session->log,"spk_corr_aver = %f", param->spk_corr_aver);
		wtk_log_log(param->session->log,"spk_energy_er = %f", param->spk_energy_er);
		wtk_log_log(param->session->log,"spk_energy_aver = %f", param->spk_energy_aver);
		wtk_log_log(param->session->log,"use_xcorr = %d", param->use_xcorr);
		wtk_log_log(param->session->log,"use_equal = %d", param->use_equal);
		wtk_log_log(param->session->log,"use_logwav = %d", param->use_logwav);
		wtk_log_log(param->session->log,"consist_fn = %s", param->consist_fn);
		wtk_log_log(param->session->log,"inputpcm_fn = %s", param->inputpcm_fn);

		wtk_log_log(param->session->log,"gbias = %f", param->gbias);
		wtk_log_log(param->session->log,"theta = %d", param->theta);
		wtk_log_log(param->session->log,"theta_range = %d", param->theta_range);
		wtk_log_log(param->session->log,"use_ssl = %d", param->use_ssl);
		wtk_log_log(param->session->log,"energy_sum = %f", param->energy_sum);
		wtk_log_log(param->session->log,"online_tms = %d", param->online_tms);
		wtk_log_log(param->session->log,"use_cfg = %d", param->use_cfg);
		wtk_log_log(param->session->log,"mic_shift = %f", param->mic_shift);
		wtk_log_log(param->session->log,"spk_shift = %f", param->spk_shift);
		wtk_log_log(param->session->log,"echo_shift = %f", param->echo_shift);
		wtk_log_log(param->session->log,"spk_channel = %d", param->spk_channel);
		wtk_log_log(param->session->log,"agca = %f", param->agca);
		wtk_log_log(param->session->log,"use_inputpcm = %d", param->use_inputpcm);
		wtk_log_log(param->session->log,"specsum_fs = %d", param->specsum_fs);
		wtk_log_log(param->session->log,"specsum_fe = %d", param->specsum_fe);
		wtk_log_log(param->session->log,"lf = %d", param->lf);
		wtk_log_log(param->session->log,"theta_step = %d", param->theta_step);
		wtk_log_log(param->session->log,"use_fftsbf = %d", param->use_fftsbf);
		wtk_log_log(param->session->log,"bfmu = %f", param->bfmu);
		wtk_log_log(param->session->log,"echo_bfmu = %f", param->echo_bfmu);
		wtk_log_log(param->session->log,"use_maskssl = %d", param->use_maskssl);
        wtk_log_log(param->session->log,"use_maskssl2 = %d", param->use_maskssl2);
        wtk_log_log(param->session->log,"spenr_thresh = %f", param->spenr_thresh);
        wtk_log_log(param->session->log,"use_cnon = %d", param->use_cnon);
        wtk_log_log(param->session->log,"sym = %f", param->sym);
        wtk_log_log(param->session->log,"use_erlssingle = %d", param->use_erlssingle);
        wtk_log_log(param->session->log,"online_frame_step = %d", param->online_frame_step);
        wtk_log_log(param->session->log,"noise_suppress = %f", param->noise_suppress);
        wtk_log_log(param->session->log,"echo_suppress = %f", param->echo_suppress);
        wtk_log_log(param->session->log,"echo_suppress_active = %f", param->echo_suppress_active);
        wtk_log_log(param->session->log,"channel = %d", param->channel);
        wtk_log_log(param->session->log,"resample_in_rate = %d", param->resample_in_rate);
        wtk_log_log(param->session->log,"resample_out_rate = %d", param->resample_out_rate);
        wtk_log_log(param->session->log,"use_resample = %d", param->use_resample);
        wtk_log_log(param->session->log,"out_channel = %d", param->out_channel);
        wtk_log_log0(param->session->log,"================== engine param ==============.");
    } else {
        wtk_debug("================== engine param ==============.\n");
        if (param->cfg) {
            wtk_debug("cfg = %s\n", param->cfg);
        }
        if (param->wakewrd) {
            wtk_debug("wakewrd = %s\n", param->cfg);
        }
        if (param->mic > 0 && param->mic_array) {
            wtk_debug("mic count = %d\n", param->mic);
            for (i = 0; i < param->mic; ++i) {
                for (j = 0; j < 3; ++j) {
                    wtk_debug("mic[%d][%d] = %f\n", i, j,
                              param->mic_array[i][j]);
                }
            }
        }
        wtk_debug("tts_volume = %f\n", param->tts_volume);
        wtk_debug("tts_speed = %f\n", param->tts_speed);
        wtk_debug("tts_pitch = %f\n", param->tts_pitch);
        wtk_debug("winStep = %d\n", param->winStep);
        wtk_debug("left margin = %d\n", param->left_margin);
        wtk_debug("right margin = %d\n", param->right_margin);
        wtk_debug("idle time = %d\n", param->idle_time);
        wtk_debug("use bin = %d\n", param->use_bin);
        wtk_debug("use_thread = %d\n", param->use_thread);
        wtk_debug("use_json = %d\n", param->use_json);
        wtk_debug("active = %d\n", param->active);
        wtk_debug("idle = %d\n", param->active);
        wtk_debug("use_oneMic = %d\n", param->use_oneMic);
        wtk_debug("asr_min_time = %d\n", param->asr_min_time);
        wtk_debug("asr_max_time = %d\n", param->asr_max_time);
        wtk_debug("output_ogg = %d\n", param->output_ogg);
        wtk_debug("syn = %d\n", param->syn);
        wtk_debug("timealive = %d\n", param->timealive);
        if (param->wake_conf) {
            wtk_debug("wake_conf = %f\n", param->wake_conf);
        }
        wtk_debug("wake_logwav = %d\n", param->wake_logwav);
        wtk_debug("use_hotword = %d\n", param->use_hotword);
        wtk_debug("use_hw_upload = %d\n", param->use_hw_upload);
        wtk_debug("use_hint = %d\n", param->use_hint);
        wtk_debug("use_bfio_reset = %d\n", param->use_bfio_reset);
        wtk_debug("bfio_reset_time = %d\n", param->bfio_reset_time);
		wtk_debug("playfn = %s\n", param->playfn);
		wtk_debug("use_rearrange = %d\n", param->use_rearrange);
		wtk_debug("eq_offset = %f", param->eq_offset);
		wtk_debug("nil_er = %f", param->nil_er);
		wtk_debug("mic_corr_er = %f", param->mic_corr_er);
		wtk_debug("mic_corr_aver = %f", param->mic_corr_aver);
		wtk_debug("mic_energy_er = %f", param->mic_energy_er);
		wtk_debug("mic_energy_aver = %f", param->mic_energy_aver);
		wtk_debug("spk_corr_er = %f", param->spk_corr_er);
		wtk_debug("spk_corr_aver = %f", param->spk_corr_aver);
		wtk_debug("spk_energy_er = %f", param->spk_energy_er);
		wtk_debug("spk_energy_aver = %f", param->spk_energy_aver);
		wtk_debug("use_xcorr = %d", param->use_xcorr);
		wtk_debug("use_equal = %d", param->use_equal);
		wtk_debug("use_logwav = %d", param->use_logwav);
		wtk_debug("use_inputpcm = %d", param->use_inputpcm);
		wtk_debug("consist_fn = %s\n", param->consist_fn);
		wtk_debug("inputpcm_fn = %s\n", param->inputpcm_fn);

		wtk_debug("gbias = %f\n", param->gbias);
		wtk_debug("theta = %d\n", param->theta);
		wtk_debug("theta_range = %d\n", param->theta_range);
		wtk_debug("use_ssl = %d\n", param->use_ssl);
		wtk_debug("energy_sum = %f\n", param->energy_sum);
		wtk_debug("online_tms = %d\n", param->online_tms);
		wtk_debug("use_cfg = %d\n", param->use_cfg);
		wtk_debug("mic_shift = %f\n", param->mic_shift);
		wtk_debug("spk_shift = %f\n", param->spk_shift);
		wtk_debug("echo_shift = %f\n", param->echo_shift);
		wtk_debug("spk_channel = %d\n", param->spk_channel);
		wtk_debug("agca = %f\n", param->agca);
		wtk_debug("specsum_fs = %d\n", param->specsum_fs);
		wtk_debug("specsum_fe = %d\n", param->specsum_fe);
		wtk_debug("lf = %d\n", param->lf);
		wtk_debug("theta_step = %d\n", param->theta_step);
		wtk_debug("use_fftsbf = %d\n", param->use_fftsbf);
		wtk_debug("bfmu = %f\n", param->bfmu);
		wtk_debug("echo_bfmu = %f\n", param->echo_bfmu);
		wtk_debug("use_maskssl = %d\n", param->use_maskssl);
        wtk_debug("use_maskssl2 = %d\n", param->use_maskssl2);
        wtk_debug("spenr_thresh = %f\n", param->spenr_thresh);
        wtk_debug("use_cnon = %d\n", param->use_cnon);
        wtk_debug("sym = %f\n", param->sym);
        wtk_debug("use_erlssingle = %d\n", param->use_erlssingle);
        wtk_debug("online_frame_step = %d\n", param->online_frame_step);
        wtk_debug("noise_suppress = %f\n", param->noise_suppress);
        wtk_debug("echo_suppress = %f\n", param->echo_suppress);
        wtk_debug("echo_suppress_active = %f\n", param->echo_suppress_active);
        wtk_debug("channel = %d\n", param->channel);
        wtk_debug("resample_in_rate = %f\n", param->resample_in_rate);
        wtk_debug("resample_out_rate = %f\n", param->resample_out_rate);
        wtk_debug("use_resample = %f\n", param->use_resample);
        wtk_debug("out_channel = %f\n", param->out_channel);
        wtk_debug("================== engine param ==============.\n");
    }
}
