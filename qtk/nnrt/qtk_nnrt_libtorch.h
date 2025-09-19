#ifndef AC9EE2F7_F039_909E_97BB_B70198C9DEEC
#define AC9EE2F7_F039_909E_97BB_B70198C9DEEC
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "qtk/nnrt/qtk_nnrt_value.h"

typedef struct qtk_nnrt_libtorch qtk_nnrt_libtorch_t;

struct qtk_nnrt_libtorch {
    qtk_nnrt_cfg_t *cfg;
    void *impl;
};

qtk_nnrt_libtorch_t *qtk_nnrt_libtorch_new(qtk_nnrt_cfg_t *cfg);
void qtk_nnrt_libtorch_delete(qtk_nnrt_libtorch_t *rt);
int qtk_nnrt_libtorch_run(qtk_nnrt_libtorch_t *rt);
int qtk_nnrt_libtorch_reset(qtk_nnrt_libtorch_t *rt);
int qtk_nnrt_libtorch_feed(qtk_nnrt_libtorch_t *rt, qtk_nnrt_value_t value,
                           int idx);
int qtk_nnrt_libtorch_get_output(qtk_nnrt_libtorch_t *rt, qtk_nnrt_value_t *out,
                                 int idx);

qtk_nnrt_value_t qtk_nnrt_libtorch_value_create_external(
    qtk_nnrt_libtorch_t *rt, qtk_nnrt_value_elem_type_t t, int64_t *shape,
    int shape_len, void *data);
void qtk_nnrt_libtorch_value_release(qtk_nnrt_libtorch_t *rt,
                                     qtk_nnrt_value_t value);
int qtk_nnrt_libtorch_value_get_shape(qtk_nnrt_libtorch_t *rt,
                                      qtk_nnrt_value_t value, int64_t *shape,
                                      int shape_cap);
void *qtk_nnrt_libtorch_value_get_data(qtk_nnrt_libtorch_t *rt,
                                       qtk_nnrt_value_t value);
void *qtk_nnrt_libtorch_value_get_channel_data(qtk_nnrt_libtorch_t *rt,
                                               qtk_nnrt_value_t value, int idx);

#endif /* AC9EE2F7_F039_909E_97BB_B70198C9DEEC */
