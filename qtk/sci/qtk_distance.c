#include "qtk/sci/qtk_distance.h"
#include "qtk/math/qtk_vector.h"

float qtk_distance_cosine(float *a, float *b, int len) {
    return qtk_vector_dotf(a, b, len) /
           (qtk_vector_normf(a, len) * qtk_vector_normf(b, len));
}

float qtk_distance_euclidean(float *a, float *b, int len) {
    int i;
    float sum = 0;
    for (i = 0; i < len; i++) {
        sum += (a[i] - b[i]) * (a[i] - b[i]);
    }
    return qtk_sqrtf(sum);
}
