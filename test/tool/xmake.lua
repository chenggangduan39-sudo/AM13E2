test_files = table.join(os.files('*.c'), os.files('*.cpp'), os.files('*.cc'))

for _, filepath in ipairs(test_files) do
    local target_name = path.basename(filepath)

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
