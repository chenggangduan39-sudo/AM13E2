test_files = os.files('*.c')

local target_handler = {
    tts_hts = {'with-tts_hts'},
    tts_merlin = {'with-tts_mer'}
}

for _, filepath in ipairs(test_files) do
    local target_name = path.basename(filepath)
    local f = target_handler[target_name]
    local common_case = true
    if f then
        if type(f) == 'function' then
            common_case = false
            f(filepath)
        elseif type(f) == 'table' then
            for _, dep_flags in ipairs(f) do
                if not has_config(dep_flags) then
                    common_case = false
                end
            end
        end
    end
    if common_case then
        target(target_name)
            set_kind('binary')
            add_files(filepath)
            add_deps('qtk')
                if has_config('static-link') then
                    add_ldflags('--static')
                end
            add_packages(third_packages)
                        if is_plat('windows') then
                            add_cflags('/utf-8')
                        end
        target_end()
    end
end

