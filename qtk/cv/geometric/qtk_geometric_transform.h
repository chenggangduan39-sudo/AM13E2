#ifndef CBC2BD65_4027_987B_54FE_79F6C3CE5057
#define CBC2BD65_4027_987B_54FE_79F6C3CE5057
#include "wtk/core/wtk_type.h"

int qtk_geometric_similarity_transform(const float *src, const float *dst,
                                       int num, int dim, float *result);
int qtk_geometric_euclidean_transform(const float *src, const float *dst,
                                      int num, int dim, float *result);
int qtk_geometric_invert_affine_transform(const float *src, float *dst);
int qtk_geometric_apply_affine_transform(uint8_t *src, uint8_t *dst, int h,
                                         int w, int new_h, int new_w,
                                         int channel, float *mat);

#endif /* CBC2BD65_4027_987B_54FE_79F6C3CE5057 */
