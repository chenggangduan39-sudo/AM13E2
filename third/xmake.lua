if has_config('with-sdk') then
    target('qtk')
		add_includedirs('porting/include')
		if not is_plat('windows') then
			add_includedirs('libuuid')
			add_files('libuuid/*.c')
			check_cfuncs("HAVE_USLEEP", "usleep", {includes = "unistd.h"})
			check_cfuncs("HAVE_FTRUNCATE", "ftruncate", {includes = "unistd.h"})
			check_cfuncs("HAVE_GETTIMEOFDAY", "gettimeofday", {includes = "sys/time.h"})
			check_cfuncs("HAVE_MEMSET", "memset", {includes = "string.h"})
			check_cfuncs("HAVE_SOCKET", "socket", {includes = "sys/socket.h"})
			check_cfuncs("HAVE_STRTOUL", "strtoul", {includes = "stdlib.h"})
			check_cfuncs("HAVE_SRANDOM", "srandom", {includes = "stdlib.h"})
		end
		
		add_includedirs('speex/include')
		add_files('speex/libspeex/*.c|test*.c|kiss_fft.c|kiss_fftr.c|smallft.c')
		add_defines("FLOATING_POINT", 'EXPORT=', 'USE_SMALLFT')
		remove_files('speex/test*.c')
		
		add_includedirs('speexdsp/include')
		add_files('speexdsp/libspeexdsp/*.c|test*.c|kiss_fft.c|kiss_fftr.c')
		includes('libogg.lua')
		includes('mbedtls.lua')
	  if is_plat('windows') then
	      add_links('Advapi32') -- needed by mbedtls
	  end
    target_end()
end

if has_config('with-tinyalsa') then
    target('qtk')
    	includes('tinyalsa.lua')
    target_end()
end

target('qtk')
    add_includedirs('stb')
	add_includedirs('.')

    add_includedirs('lapack')
    add_files('lapack/*.c')

    add_includedirs('kissfft')
    add_files('kissfft/kiss_fft.c')
    add_files('kissfft/kiss_fftr.c')
    add_files('kissfft/kiss_fftnd.c')
    add_files('kissfft/kiss_fftndr.c')

	if has_config('with-semdlg') or has_config('with-lua') then
		includes('lua.lua')
		includes('sqlite.lua')
	end
target_end()
