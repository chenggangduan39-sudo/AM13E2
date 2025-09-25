#ifndef D6CBDAAC_22BF_4FCC_B68D_12D4E40FB875
#define D6CBDAAC_22BF_4FCC_B68D_12D4E40FB875

#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_tracy_cfg qtk_tracy_cfg_t;

struct qtk_tracy_cfg {
    char *host;
    int port;
};

int qtk_tracy_cfg_init(qtk_tracy_cfg_t *cfg);
int qtk_tracy_cfg_clean(qtk_tracy_cfg_t *cfg);
int qtk_tracy_cfg_update(qtk_tracy_cfg_t *cfg);
int qtk_tracy_cfg_update_local(qtk_tracy_cfg_t *cfg, wtk_local_cfg_t *lc);

#endif /* D6CBDAAC_22BF_4FCC_B68D_12D4E40FB875 */
