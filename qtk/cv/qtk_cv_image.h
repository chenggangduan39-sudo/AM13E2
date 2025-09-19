#ifndef D33D03F2_17BA_432D_AC89_88124FBED726
#define D33D03F2_17BA_432D_AC89_88124FBED726
#ifdef __cplusplus
extern "C" {
#endif
#include "qtk/image/qtk_image.h"
#include <stdint.h>

enum qtk_im_evaluation {
    TENENGRAD = 0,
    LAPLACIAN,
    SMD,
    SMD2,
    BRENNER,
    VARIANCE,
    ENERGY,
    VOLLATH
};

double qtk_cv_image_definition_method(uint8_t *data, qtk_image_desc_t *desc,
                                      qtk_im_evaluation tool);

double qtk_cv_image_fov(const char *path);

double qtk_cv_image_ccdiff(const char *fn1, const char *fn2);
#ifdef __cplusplus
};
#endif
#endif /* D33D03F2_17BA_432D_AC89_88124FBED726 */
