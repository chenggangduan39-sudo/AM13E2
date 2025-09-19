#ifndef EC8C7CED_1A03_0CAD_C5D4_7552156B78F8
#define EC8C7CED_1A03_0CAD_C5D4_7552156B78F8

#include "qtk/ult/qtk_ult_track_post_cfg.h"
#include "qtk/ult/qtk_ult_track_type.h"
#include "wtk/core/wtk_robin.h"

typedef struct qtk_ult_track_post qtk_ult_track_post_t;

struct qtk_ult_track_post {
    qtk_ult_track_post_cfg_t *cfg;
    wtk_robin_t *win;
    wtk_heap_t *heap;
    unsigned stand : 1;
    unsigned startup : 1;
};

qtk_ult_track_post_t *qtk_ult_track_post_new(qtk_ult_track_post_cfg_t *cfg);
int qtk_ult_track_post_feed(qtk_ult_track_post_t *p,
                            qtk_ult_track_result_t *res);
void qtk_ult_track_post_delete(qtk_ult_track_post_t *p);

#endif /* EC8C7CED_1A03_0CAD_C5D4_7552156B78F8 */
