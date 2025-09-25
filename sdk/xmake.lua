target('qtk')
    add_files('dns/**.c')
    add_files('httpc/**.c')
    add_files('nk/**.c')
    add_files('session/**.c')
    add_files('util/**.c')
    add_files('engine/*.c')
    add_files('engine/comm/**.c')
    add_files('engine/cloud/**.c')
    add_files('module/*.c')
    add_files('codec/keros/**.c')
    add_files('codec/timer/**.c')
    add_files('codec/upload/**.c')
    add_files('codec/oggdec/**.c')
    add_files('codec/oggenc/**.c')
    add_files('codec/*.c')
    add_files('spx/**.c')
    add_files('audio/**.c')
	
	if not is_plat('linux','android') then
		remove_files(
			'nk/module/qtk_epoll.c'
		)
	end

    if not has_config('with-asound') then
    	remove_files('audio/**.c')
    end

    if not has_config('with-sdk_module') then
        remove_files('module/**.c')
    end

    if has_config('with-jni') then
        add_files('binding/jni/**.c')

        remove_files('binding/jni/vprintWake.c')
        remove_files('binding/jni/module.c')
        remove_files('binding/jni/mulsv.c')
        --remove_files('binding/jni/audio.c')
    end
	
    if has_config('sdk-use-asr') then
        add_defines('USE_ASR')
        add_defines('USE_MBEDTLS')
        add_defines('USE_AUTH')
        add_defines('QTK_NK_USE_EPOLL')
        add_files('engine/asr/**.c')
        add_files('engine/csr/**.c')
        add_files('api_1/asr/**.c')
        add_files('api_1/csr/**.c')
    end

    if has_config('sdk-use-vboxebf') then
        --add_defines('USE_TIMELIMIT')
        add_defines('USE_CNTLIMIT')
        add_defines('USE_SESSION_NOCHECK')
        add_defines('USE_VBOXEBF3')
        add_files('engine/vboxebf3/**.c')
    end
    
    if has_config('with-vboxebf') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_VBOXEBF')
        add_files('engine/vboxebf/**.c')
        add_files('api_1/vboxebf/**.c')
    end

    if has_config('with-gdenoise') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_GDENOISE')
        add_files('engine/egdenoise/**.c')
        add_files('api_1/gdenoise/**.c')
    end

    if has_config('with-gainnetbf') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_GAINNETBF')
        add_files('engine/egainnetbf/**.c')
        add_files('api_1/gainnetbf/**.c')
    end

    if has_config('with-consist') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_CONSIST')
        add_files('engine/consist/**.c')
    end

    if has_config('with-ssl') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_SSL')
        add_files('engine/ssl/**.c')
        add_files('api_1/ssl/**.c')
    end

    if has_config('with-soundscreen') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_SOUNDSCREEN')
        add_files('engine/esoundscreen/**.c')
        add_files('api_1/soundScreen/**.c')
    end

    if has_config('with-qform') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_QFORM')
        add_files('engine/eqform/**.c')
        add_files('api_1/qform/**.c')
    end

    if has_config('with-module_mqform') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_MODULE_MQFORM')
        add_files('module/mqform/**.c')
        add_files('api_1/vboxebf/**.c')
    end

    if has_config('with-module_mqform2') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_MODULE_MQFORM2')
        add_files('module/mqform2/**.c')
        add_files('api_1/aec/**.c')
        add_files('api_1/soundScreen/**.c')
    end

    if has_config('with-module_mgainnet') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_MODULE_MGAINNET')
        add_files('module/mgainnet/**.c')
    end

    if has_config('with-mod_consist') then
        --add_defines('USE_SESSION_NOCHECK')
        --add_defines('USE_TIMELIMIT')
        --add_defines('USE_CNTLIMIT')
        --add_defines('USE_QTKCOUNT')
        add_defines('USE_MOD_CONSIST')
        remove_files('audio/**.c')
        add_files('api_1/vboxebf/**.c')
        add_files('api_1/vboxebf/qtk_mod_consist.c','api_1/vboxebf/qtk_mod_consist_cfg.c')
    end

    if has_config('with-semdlg') then
        add_defines('USE_SEMDLG')
        add_files('engine/semdlg/**.c')
        add_files('api_1/semdlg/**.c')
    end

    if has_config('with-tts_hts') or has_config('with-tts_mer') then
        add_defines('USE_TTS')
        add_files('engine/tts/**.c')
        add_files('api_1/tts/**.c')
    end
    
    if has_config('with-wakeup') then
        add_defines('USE_WAKEUP')
        add_files('engine/wakeup/**.c')
        add_files('api_1/wakeup/**.c')
	add_files('mulsv/**.c')
    end

    if has_config('with-bfio') then
        add_defines('USE_BFIO')
        add_files('engine/bfio/**.c')
    end

    if has_config('with-wdec') then
        add_defines('USE_WDEC')
        add_files('engine/wdec/**.c')
        add_files('api_1/wdec/**.c')
    end

    if has_config('with-opus') then --TODO add opus support
        add_files('codec/opus/**.c')
    end

    if has_config('with-bfio') then
        add_defines('USE_TINYBF')
        add_files('engine/tinybf/**.c')
        add_defines('USE_AEC')
        add_files('engine/aec/**.c')
    end

    if has_config('with-auth') then
        add_defines('USE_AUTH') -- use auth for default
	add_links('dl')
    end

    add_packages(sdk_requires)
target_end()
