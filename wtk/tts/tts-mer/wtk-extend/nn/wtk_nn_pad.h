#ifndef WTK_NN_PAD_H
#define WTK_NN_PAD_H
#include "tts-mer/wtk-extend/wtk_extend.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    wtk_nn_pad_type_reflect,
    wtk_nn_pad_type_center,
} wtk_nn_pad_type_enum_t;

void wtk_nn_pad_float( wtk_nn_pad_type_enum_t pad_type, float *src, int slen, float *dst, int dlen, int pad_len);

#ifdef __cplusplus
}
#endif
#endif
