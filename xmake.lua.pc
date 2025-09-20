set_xmakever("2.8.5")

set_project('qtk')

set_rules('mode.release', 'mode.debug', 'mode.releasedbg', 'mode.profile', 'mode.check')

add_repositories('qdr-repo http://gitea.qdreamer.lan/develop/qdr-repo.git')
add_includedirs('.')

includes('@builtin/check')

add_headerfiles('(qtk/**.h)')
add_headerfiles('(wtk/**.h)')

if is_plat('linux') then
    set_warnings('all')
end

option("with-ncnn")
    set_default(false)
    set_showmenu(true)
option_end()

option("with-libtorch")
    set_default(false)
    set_showmenu(true)
option_end()

option("with-onnxruntime")
    set_default(false)
    set_showmenu(true)
option_end()

option("with-recorder")
    set_default(false)
    set_showmenu(true)
option_end()

option("with-opus")
    set_default(false)
    set_showmenu(true)
option_end()

option("with-asr")
    set_default(false)
    set_showmenu(true)
option_end()

option("with-sdk")
    set_default(false)
    set_showmenu(true)
option_end()

option("sdk-use-audio")
    set_default(true)
    set_showmenu(true)
option_end()

option('with-semdlg')
    set_default(false)
    set_showmenu(true)
option_end()

option('static-link')
    set_default(false)
    set_showmenu(true)
option_end()

option("with-bfio")
    set_default(false)
    set_showmenu(true)
    add_deps('with-asr')
    after_check(function (option)
        if option:enabled() and not option:dep("with-asr"):enabled() then
            option:dep("with-asr"):enable(true)
        end
    end)
option_end()

option('with-lua')
    set_default(false)
    set_showmenu(true)
    add_deps('with-sdk')
    after_check(function (option)
        if option:enabled() and not option:dep("with-sdk"):enabled() then
            option:dep("with-sdk"):enable(true)
        end
    end)
option_end()

option("with-tts_hts")
    set_default(false)
    set_showmenu(true)
option_end()

option("with-tts_mer")
    set_default(false)
    set_showmenu(true)
option_end()

option('sdk-use-asr')
    set_default(false)
    set_showmenu(true)
option_end()

option('sdk-use-vboxebf')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-vboxebf')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-gdenoise')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-gainnetbf')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-consist')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-ssl')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-soundscreen')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-qform')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-module_mqform')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-module_mqform2')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-module_mgainnet')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-mod_consist')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-wakeup')
    set_default(false)
    set_showmenu(true)
    add_deps('with-asr')
    after_check(function (option)
        if option:enabled() and not option:dep("with-asr"):enabled() then
            option:dep("with-asr"):enable(true)
        end
    end)
option_end()

option('with-wdec')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-unittest')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-opencv')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-auth')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-jni')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-tinyalsa')
    set_default(false)
    set_showmenu(true)
option_end()

option('with-stitch')
    set_default(false)
    set_showmenu(true)
option_end()

if has_config('with-sdk') then
    includes('sdk')
    includes('wtk/bfio')
    add_includedirs('sdk')
    includes('test/tool/sdk')
 end

third_packages = {}

if has_config('with-onnxruntime') then
    add_defines('ONNX_DEC')
    add_requires('onnxruntime')
    table.insert(third_packages, 'onnxruntime')
end

if has_config('with-opencv') then
    add_requires('opencv', {configs={gtk=true, shared=true}})
    add_defines('WITH_OPENCV')
    table.insert(third_packages, 'opencv')
end

if has_config('with-ncnn') then
    add_defines('QTK_NNRT_NCNN')
    add_requires('ncnn-qdr')
    table.insert(third_packages, 'ncnn-qdr')
end

if has_config('with-libtorch') then                                                                                                                                                                                                                                                                                          
    add_defines('QTK_NNRT_LIBTORCH')                                                                                                                                                                                                                                                                                         
    add_requires('libtorch')                                                                                                                                                                                                                                                                                                 
    table.insert(third_packages, 'libtorch')                                                                                                                                                                                                                                                                                 
end

target('qtk')
    check_cfuncs("QTK_HAVE_BACKTRACE", "backtrace", {includes = "execinfo.h"})
    check_cfuncs("QTK_RAND_R", "rand_r", {includes = "stdlib.h"})
    set_kind('$(kind)')
    if has_config('with-semdlg') then
        add_defines('USE_SQL')
        add_files('wtk/lua/**.c')
    end
    add_files(
      'wtk/core/**.c',
      'wtk/os/**.c',
      'wtk/signal/**.c',
      'wtk/lex/**.c'
    )

    if has_config('with-asr') then
        add_files('public/**.c','qtk/sci/clustering/**.c')
    end

	if not is_plat('linux') then
		remove_files(
		  'wtk/os/wtk_cond.c',
		  'wtk/os/wtk_pipeproc.c',
		  'wtk/os/wtk_socketproc.c',
		  'wtk/os/wtk_pid.c',
		  'wtk/os/wtk_shm.c',
		  'wtk/os/wtk_semshm.c',
		  'wtk/os/daemon/wtk_daemon.c',
		  'wtk/os/daemon/wtk_daemon_cfg.c',
		  'wtk/http/nk/module/wtk_epoll.c',
		  'wtk/os/ps/*.c'
		)
	end
	
	if is_plat('windows') then
		add_links('Ole32')
		add_defines('WIN32')
		add_cflags('/utf-8')
    end

    if is_plat('linux') then
        add_links('pthread')
	end
    if has_config('with-sdk') then
        add_links('asound')
    end
    add_packages(third_packages)
target_end()

if has_config('with-asr') then
    includes('test/tool/asr')
    includes('wtk/asr')
end

if has_config('with-bfio') then
    includes('test/tool/bfio')
    includes('wtk/bfio')
end

if has_config('with-semdlg') then
    includes('wtk/semdlg')
end

if has_config('with-tts_hts') then
    includes('test/tool/tts')
    includes('wtk/tts')
end

if has_config('with-tts_mer') then
    includes('test/tool/tts')
    includes('wtk/tts')
    add_includedirs('wtk/tts')
end

if has_config('with-tts_device') then
    includes('test/tool/tts')
    includes('wtk/tts')
    add_includedirs('wtk/tts', 'qtk/tts', 'qtk')
end

if has_config('with-unittest') then
    includes('unittest')
end

if has_config('with-onnxruntime') then
    add_defines('QTK_NNRT_ONNXRUNTIME')
end

includes('qtk')
includes('third')
includes('test/tool')

