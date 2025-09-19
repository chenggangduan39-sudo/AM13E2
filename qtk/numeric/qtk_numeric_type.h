#ifndef QBL_NUMERIC_QBL_NUMERIC_TYPE_H
#define QBL_NUMERIC_QBL_NUMERIC_TYPE_H
#pragma once
#include "qtk/core/qtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef union {
    float *f32;
    void *raw;
    char *chr;
    int64_t *i64;
    int32_t *i32;
    int16_t *i16;
    unsigned int *uint;
    unsigned short *u16;
    int8_t *boolean;
    float **fp32;
    uint8_t *u8;
    int8_t *i8;
    uint8_t **u8p;
} qtk_numeric_data_t;

typedef enum {
    QBL_NUMERIC_F32,
    QBL_NUMERIC_INT64,
} qtk_numeric_data_type_t;

qtk_maybe_unused static void qtk_numeric_i32_to_i64(qtk_numeric_data_t from,
                                                    qtk_numeric_data_t to,
                                                    uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        to.i64[i] = from.i32[i];
    }
}

qtk_maybe_unused static void qtk_numeric_i32_to_f32(qtk_numeric_data_t from,
                                                    qtk_numeric_data_t to,
                                                    uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        to.f32[i] = from.i32[i];
    }
}


qtk_maybe_unused static void qtk_numeric_boolean_to_i64(qtk_numeric_data_t from,
                                                        qtk_numeric_data_t to,
                                                        uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        to.i64[i] = from.boolean[i];
    }
}

qtk_maybe_unused static void qtk_numeric_boolean_to_i32(qtk_numeric_data_t from,
                                                        qtk_numeric_data_t to,
                                                        uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        to.i32[i] = from.boolean[i];
    }
}

qtk_maybe_unused static void qtk_numeric_i64_to_i32(qtk_numeric_data_t from,
                                                    qtk_numeric_data_t to,
                                                    uint32_t len) {
    for (uint32_t i = 0; i < len; i++) {
        to.i32[i] = from.i64[i];
    }
}

qtk_maybe_unused static uint as_uint(const float x) {
    return *(unsigned int *)&x;
}

qtk_maybe_unused static float as_float(const unsigned int x) {
    return *(float *)&x;
}

qtk_maybe_unused static float half_to_float(
    const unsigned short x) { // IEEE-754 16-bit floating-point format (without
                              // infinity): 1-5-10, exp-15, +-131008.0,
                              // +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
    const uint e = (x & 0x7C00) >> 10; // exponent
    const uint m = (x & 0x03FF) << 13; // mantissa
    const uint v =
        as_uint((float)m) >>
        23; // evil log2 bit hack to count leading zeros in denormalized format
    return as_float((x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) |
                    ((e == 0) & (m != 0)) *
                        ((v - 37) << 23 |
                         ((m << (150 - v)) &
                          0x007FE000))); // sign : normalized : denormalized
}

qtk_maybe_unused static unsigned short
float_to_half(const float x) { // IEEE-754 16-bit floating-point format (without
                               // infinity): 1-5-10, exp-15, +-131008.0,
                               // +-6.1035156E-5, +-5.9604645E-8, 3.311 digits
    const uint b = as_uint(x) + 0x00001000; // round-to-nearest-even: add last
                                            // bit after truncated mantissa
    const uint e = (b & 0x7F800000) >> 23;  // exponent
    const uint m = b & 0x007FFFFF; // mantissa; in line below: 0x007FF000 =
                                   // 0x00800000-0x00001000 = decimal indicator
                                   // flag - initial rounding
    return (b & 0x80000000) >> 16 |
           (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) |
           ((e < 113) & (e > 101)) *
               ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) |
           (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
}

qtk_maybe_unused static void qtk_numeric_f16_to_f32(qtk_numeric_data_t from,
                                                    qtk_numeric_data_t to,
                                                    uint32_t len) {
    uint32_t i;
    for (i = 0; i < len; i++) {
        to.f32[i] = half_to_float(from.u16[i]);
    }
}

qtk_maybe_unused static void qtk_numeric_f32_to_f16(qtk_numeric_data_t from,
                                                    qtk_numeric_data_t to,
                                                    uint32_t len) {
    uint32_t i;
    for (i = 0; i < len; i++) {
        to.u16[i] = float_to_half(from.f32[i]);
    }
}

#ifdef __cplusplus
};
#endif
#endif
