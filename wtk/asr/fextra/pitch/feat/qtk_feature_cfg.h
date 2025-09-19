#ifndef __QTK_FEAT_QTK_FEATURE_CFG_H__
#define __QTK_FEAT_QTK_FEATURE_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_feature_delta_cfg qtk_feature_delta_cfg_t;

struct qtk_feature_delta_cfg {
    int order;
    int window;
};

int qtk_feature_delta_cfg_init(qtk_feature_delta_cfg_t *cfg);
int qtk_feature_delta_cfg_update(qtk_feature_delta_cfg_t *cfg);
int qtk_feature_delta_cfg_clean(qtk_feature_delta_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
