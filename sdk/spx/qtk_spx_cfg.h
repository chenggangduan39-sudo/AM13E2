#ifndef QTK_UTIL_SPX_QTK_SPX_CFG
#define QTK_UTIL_SPX_QTK_SPX_CFG

#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#include "sdk/codec/oggenc/qtk_oggenc_cfg.h"
#include "sdk/session/option/qtk_option.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_spx_cfg qtk_spx_cfg_t;

struct qtk_spx_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_oggenc_cfg_t oggenc;
    wtk_string_t coreType;
    wtk_string_t res;
    wtk_string_t env;
    wtk_string_t semRes;
    wtk_string_t synRes;
    float volume;
    float speed;
    float pitch;
    int iType;
    int rate;
    int channel;
    int bytes_per_sample;
    int timeout;
    int cache;
    int bufsize;
    int lua_bufsize;
    int max_size;
    unsigned use_ogg : 1; // 上传服务器音频数据是否为ogg, 1为ogg，0为pcm
    unsigned useStream : 1; // 接收服务器数据是否为流式
    unsigned use_luabuf : 1;
    unsigned use_hint : 1;
    unsigned use_hotword : 1;
    unsigned skip_space : 1;
};

int qtk_spx_cfg_init(qtk_spx_cfg_t *cfg);
int qtk_spx_cfg_clean(qtk_spx_cfg_t *cfg);
int qtk_spx_cfg_update_local(qtk_spx_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_spx_cfg_update(qtk_spx_cfg_t *cfg);
int qtk_spx_cfg_update2(qtk_spx_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_spx_cfg_update_params(qtk_spx_cfg_t *cfg, wtk_local_cfg_t *params);
void qtk_spx_cfg_update_option(qtk_spx_cfg_t *cfg, qtk_option_t *option);

qtk_spx_cfg_t *qtk_spx_cfg_new(char *fn);
void qtk_spx_cfg_delete(qtk_spx_cfg_t *cfg);

qtk_spx_cfg_t *qtk_spx_cfg_new_bin(char *bin_fn, int seek_pos);
void qtk_spx_cfg_delete_bin(qtk_spx_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
