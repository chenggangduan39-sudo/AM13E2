/*
 * qtk_stress.h
 *
 *  Created on: Mar 3, 2022
 *      Author: dm
 */

#ifndef WTK_UTILS_QTK_STRESS_H_
#define WTK_UTILS_QTK_STRESS_H_
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stress_feat qtk_stress_feat_t;
typedef struct qtk_stress_sntfeat qtk_stress_sntfeat_t;


struct qtk_stress_sntfeat{
	short nwrds;
	short dim;
	float **wv;   //value of words.
};

qtk_stress_sntfeat_t* qtk_stress_sntfeat_new(int nwrds, int dim);
void qtk_stress_sntfeat_delete(qtk_stress_sntfeat_t *sf);
qtk_stress_sntfeat_t* qtk_stress_sntfeat_build(char** wrds, int nwrds, int dim, char** flabel, int nlab, char** addpos);
int qtk_stress_load(void *ths,wtk_source_t *src);
#ifdef __cplusplus
};
#endif
#endif /* WTK_UTILS_QTK_STRESS_H_ */
