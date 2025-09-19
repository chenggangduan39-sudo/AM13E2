#ifndef WTK_BFIO_SIGNAL_DECOMPRESS_CFG
#define WTK_BFIO_SIGNAL_DECOMPRESS_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "qtk/nnrt/qtk_nnrt_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_signal_decompress_cfg qtk_signal_decompress_cfg_t;
struct qtk_signal_decompress_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	qtk_nnrt_cfg_t decoder;
	qtk_nnrt_cfg_t rvq_decoder;
    qtk_nnrt_cfg_t head;

    wtk_string_t *window;
    char *win_fn;
};

int qtk_signal_decompress_cfg_init(qtk_signal_decompress_cfg_t *cfg);
int qtk_signal_decompress_cfg_clean(qtk_signal_decompress_cfg_t *cfg);
int qtk_signal_decompress_cfg_update_local(qtk_signal_decompress_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_signal_decompress_cfg_update(qtk_signal_decompress_cfg_t *cfg);
int qtk_signal_decompress_cfg_update2(qtk_signal_decompress_cfg_t *cfg,wtk_source_loader_t *sl);

qtk_signal_decompress_cfg_t* qtk_signal_decompress_cfg_new(char *fn);
void qtk_signal_decompress_cfg_delete(qtk_signal_decompress_cfg_t *cfg);
qtk_signal_decompress_cfg_t* qtk_signal_decompress_cfg_new_bin(char *fn);
void qtk_signal_decompress_cfg_delete_bin(qtk_signal_decompress_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
