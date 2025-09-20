target('qtk')
    if has_config('with-tts_hts') then
        add_files(
        'modules/wtk_engtts_api.c',
        'parser/**.c',
        'syn/**.c',
        'wtk_tts.c',
        'wtk_tts_cfg.c',
        'wtk_mtts.c',
        'wtk_mtts_cfg.c'
        )
--        add_links('pthread')
    end
    if has_config("with-tts_mer") then
        add_files(
        'parser/**.c',
        'syn/**.c',
        'tts-mer/**.c'
        )
    end
    
    if has_config("with-tts_device") then
        add_files(
        'parser/**.c',
        'tts-mer/**.c'
        )
    end
target_end()
