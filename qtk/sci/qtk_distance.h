#ifndef __QBL_SCI_QBL_DISTANCE_H__
#define __QBL_SCI_QBL_DISTANCE_H__
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

float qtk_distance_cosine(float *a, float *b, int len);
float qtk_distance_euclidean(float *a, float *b, int len);

#ifdef __cplusplus
};
#endif
#endif
