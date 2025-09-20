target('qtk')
    add_files('**.c')
    if has_config('with-sdk') and has_config('with-opus') then -- conflict with sdk opus deps
        remove_files('maskdenoise/celt_lpc.c', 'maskdenoise/pitch.c')
    end
target_end()
