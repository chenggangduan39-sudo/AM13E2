#ifndef __QTK_CORE_QTK_BASE_H__
#define __QTK_CORE_QTK_BASE_H__
#include "wtk/core/wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#define QTK_ASSERT(cond, ...)                                                  \
    do {                                                                       \
        if (!(cond)) {                                                         \
            wtk_debug(__VA_ARGS__);                                            \
        }                                                                      \
    } while (0)

#ifdef __cplusplus
};
#endif
#endif
