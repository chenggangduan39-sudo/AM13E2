#include "qtk/ult/evm2/qtk_ultevm2.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"

static void test_fn_(qtk_ultevm2_t *evm2, char *wav) {
    wtk_riff_t *riff;
    riff = wtk_riff_new();
    wtk_riff_open(riff, wav);
    char buf[4096];
    int is_end = 0;

    while (!is_end) {
        int cnt = wtk_riff_read(riff, buf, sizeof(buf));
        if (cnt < sizeof(buf)) {
            is_end = 1;
        }
        if (cnt > 0) {
            qtk_ultevm2_feed(evm2, (short *)buf, cnt / 2);
            wtk_debug("%d\n", evm2->active);
        }
    }
    qtk_ultevm2_feed_end(evm2);
    wtk_riff_delete(riff);
}

int main(int argc, char *argv[]) {
    wtk_main_cfg_t *main_cfg;
    wtk_arg_t *arg;
    char *cfg;
    char *wav;
    qtk_ultevm2_t *evm2;

    arg = wtk_arg_new(argc, argv);
    wtk_arg_get_str_s(arg, "c", &cfg);
    wtk_arg_get_str_s(arg, "i", &wav);
    main_cfg = wtk_main_cfg_new_type(qtk_ultevm2_cfg, cfg);
    evm2 = qtk_ultevm2_new((qtk_ultevm2_cfg_t *)main_cfg->cfg);
    test_fn_(evm2, wav);
    wtk_arg_delete(arg);
    qtk_ultevm2_delete(evm2);
    wtk_main_cfg_delete(main_cfg);
}
