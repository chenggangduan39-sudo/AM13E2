#ifndef QBL_CORE_QBL_BINARY_H
#define QBL_CORE_QBL_BINARY_H
#pragma once
#include "qtk/core/qtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#define qtk_littleEndian_uint16_from_bin(x)                                    \
    cast(uint16_t, cast(uint8_t *, x)[0] | (cast(uint8_t *, x)[1] << 8))

#define qtk_littleEndian_uint32_from_bin(x)                                    \
    cast(uint32_t, cast(uint8_t *, x)[0] | (cast(uint8_t *, x)[1] << 8) |      \
                       (cast(uint8_t *, x)[2] << 16) |                         \
                       (cast(uint8_t *, x)[3] << 24))

#ifdef __cplusplus
};
#endif
#endif
