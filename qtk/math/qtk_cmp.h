#ifndef G_MO4SQUTZ_4UED_KJ7V_4PUH_P97KNGQL7GLL
#define G_MO4SQUTZ_4UED_KJ7V_4PUH_P97KNGQL7GLL
#pragma once
#include "qtk/core/qtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

// Function uses casting from int to unsigned to compare if value of
// parameter a is greater or equal to zero and lower than value of
// parameter b. The b parameter is of type signed and is always positive,
// therefore its value is always lower than 0x800... where casting
// negative value of a parameter converts it to value higher than 0x800...
// The casting allows to use one condition instead of two.
qtk_maybe_unused QTK_INLINE static int qtk_is_a_ge_zero_and_a_lt_b(int a,
                                                                   int b) {
    return (unsigned int)a < (unsigned int)b;
}

#ifdef __cplusplus
};
#endif
#endif /* G_MO4SQUTZ_4UED_KJ7V_4PUH_P97KNGQL7GLL */
