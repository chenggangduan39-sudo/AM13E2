#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/lex/lexr/wtk_lexr.h"
#include "wtk/lex/wtk_lex.h"
#include "wtk/lex/wtk_lex_cfg.h"

static void do_str_(wtk_lex_t *l, char *str) {
    wtk_string_t res;
    res = wtk_lex_process(l, str, strlen(str));
    printf("INPUT=> %s\n", str);
    printf("OUTPUT=> %.*s\n", res.len, res.data);
}

int main(int argc, char *argv[]) {
    wtk_arg_t *arg;
    char *cfn = NULL;
    char *lex_fn = NULL;
    char *str = NULL;
    wtk_main_cfg_t *main_cfg;
    wtk_lex_t *l;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfn);
    wtk_arg_get_str_s(arg, "l", &lex_fn);
    wtk_arg_get_str_s(arg, "s", &str);

    if (cfn == NULL || lex_fn == NULL) {
        wtk_debug("missing lex_fn or cfn");
        exit(-1);
    }

    main_cfg = wtk_main_cfg_new_type(wtk_lex_cfg, cfn);
    l = wtk_lex_new(cast(wtk_lex_cfg_t *, main_cfg->cfg));
    wtk_lex_compile(l, lex_fn);

    do_str_(l, str);

    wtk_arg_delete(arg);
    wtk_lex_delete(l);
    wtk_main_cfg_delete(main_cfg);
    return 0;
}