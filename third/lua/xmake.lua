add_rules("mode.debug", "mode.release")

if is_mode("release") then
    set_optimize("fastest")
end

target('lua')
    set_kind('$(kind)')
    add_headerfiles('*.h')
    if is_plat("linux") then
        add_defines("LUA_USE_LINUX")
        add_links("dl")
    end
    add_files(
        "lapi.c",
        "lauxlib.c",
        "lbaselib.c",
        "lcode.c",
        "lcorolib.c",
        "lctype.c",
        "ldblib.c",
        "ldebug.c",
        "ldo.c",
        "ldump.c",
        "lfunc.c",
        "lgc.c",
        "linit.c",
        "liolib.c",
        "llex.c",
        "lmathlib.c",
        "lmem.c",
        "loadlib.c",
        "lobject.c",
        "lopcodes.c",
        "loslib.c",
        "lparser.c",
        "lstate.c",
        "lstring.c",
        "lstrlib.c",
        "ltable.c",
        "ltablib.c",
        "ltm.c",
        "lundump.c",
        "lutf8lib.c",
        "lvm.c",
        "lzio.c",
        "lqdrlib.c"
    )

target('luab')
    set_kind('binary')
    add_files('lua.c')
    add_deps('lua')
