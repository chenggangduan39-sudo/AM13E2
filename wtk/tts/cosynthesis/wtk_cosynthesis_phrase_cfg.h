/*
 * wtk_cosynthesis_phrase_cfg.h
 *
 *  Created on: Jan 28, 2022
 *      Author: dm
 */

#ifndef WTK_COSYNTHESIS_WTK_COSYNTHESIS_PHRASE_CFG_H_
#define WTK_COSYNTHESIS_WTK_COSYNTHESIS_PHRASE_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct{
	wtk_rbin2_t *rbin;
	char* crf_path;
	char* weight_path;
}wtk_cosynthesis_phrase_cfg_t;
int wtk_cosynthesis_phrase_cfg_init(wtk_cosynthesis_phrase_cfg_t *cfg);
int wtk_cosynthesis_phrase_cfg_update_local(wtk_cosynthesis_phrase_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_cosynthesis_phrase_cfg_clean(wtk_cosynthesis_phrase_cfg_t *cfg);
int wtk_cosynthesis_phrase_cfg_update(wtk_cosynthesis_phrase_cfg_t *cfg);
int wtk_cosynthesis_phrase_cfg_update2(wtk_cosynthesis_phrase_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif /* WTK_COSYNTHESIS_WTK_COSYNTHESIS_PHRASE_CFG_H_ */
