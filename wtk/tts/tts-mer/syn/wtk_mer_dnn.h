#ifndef WTK_MER_DNN_H_
#define WTK_MER_DNN_H_
#include "tts-mer/wtk_mer_common.h"
#ifdef __cplusplus
extern "C" {
#endif

wtk_matf_t* wtk_mer_dnn_model(wtk_matf_t *in_x, int layer_size, wtk_string_t **layer_type, int *layer_num, wtk_matf_t **w_arr, wtk_vecf_t **b_arr, const int n_final, char *mname);

#ifdef __cplusplus
}
#endif
#endif
