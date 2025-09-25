add_rules("mode.debug", "mode.release")

if is_mode("release") then
    set_optimize("fastest")
end

target('kissfft')
    set_kind('$(kind)')
    add_headerfiles(
        'qdm_kiss_fft.h',
        'qdm_kiss_fftnd.h',
        'qdm_kiss_fftr.h',
        'qdm_kiss_fftndr.h'
    )
    add_files(
        'kiss_fft.c',
        'kiss_fftr.c',
        'kiss_fftnd.c',
        'kiss_fftndr.c'
    )
