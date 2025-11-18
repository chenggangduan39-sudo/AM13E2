#ifndef D7465285_0C65_423E_A1EF_B7FEE6A605B4
#define D7465285_0C65_423E_A1EF_B7FEE6A605B4
#include "qtk/ult/qtk_ult_perception_cfg.h"
#include "qtk/ult/qtk_ult_track_type.h"
#include "wtk/core/wtk_robin.h"

typedef struct qtk_ult_perception qtk_ult_perception_t;
typedef struct qtk_ult_perception_input qtk_ult_perception_input_t;
typedef enum {
    QTK_ULT_PERCEPTION_HAS_P_INIT,
    QTK_ULT_PERCEPTION_HAS_P_YES,
    QTK_ULT_PERCEPTION_HAS_P_NO,
} qtk_ult_perception_hap_p_state_t;

typedef struct {
    unsigned state_1m : 1;
    unsigned state_5m : 1;
    unsigned trusted : 1;
} qtk_ult_perception_result_t;

struct qtk_ult_perception_input {
    qtk_ult_track_result_t *trk;
    float vad_prob;
    int nobj;
};

struct qtk_ult_perception {
    qtk_ult_perception_cfg_t *cfg;
    wtk_robin_t *perception_ctx_1m;
    wtk_robin_t *perception_ctx_5m;
    wtk_robin_t *vad_trap;

    char state_1m;
    char state_5m;
    qtk_ult_perception_hap_p_state_t has_p;
};

qtk_ult_perception_t *qtk_ult_perception_new(qtk_ult_perception_cfg_t *cfg);
void qtk_ult_perception_delete(qtk_ult_perception_t *m);
int qtk_ult_perception_feed(qtk_ult_perception_t *m,
                            qtk_ult_perception_input_t *in,
                            qtk_ult_perception_result_t *out);

#endif /* D7465285_0C65_423E_A1EF_B7FEE6A605B4 */
