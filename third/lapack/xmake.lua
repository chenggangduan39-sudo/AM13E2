add_rules("mode.debug", "mode.release")

if is_mode("release") then
    set_optimize("fastest")
end

target('lapack')
    set_kind('$(kind)')
    add_headerfiles('./f2c.h')
    add_files('*.c')
