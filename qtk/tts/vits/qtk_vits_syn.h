/*
 * qtk_vits_syn.h
 *
 *  Created on: Aug 26, 2022
 *      Author: dm
 */

#ifndef QTK_VITS_QTK_VITS_SYN_H_
#define QTK_VITS_QTK_VITS_SYN_H_
#include "qtk_vits_cfg.h"
#include "parse/qtk_tts_parse.h"
#include "qtk_vits_models.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_vits_syn qtk_vits_syn_t;

struct qtk_vits_syn{
	qtk_vits_cfg_t* cfg;
	qtk_vits_models_syntrn_t* trn;
};

qtk_vits_syn_t* qtk_vits_syn_new(qtk_vits_cfg_t* cfg);
int qtk_vits_syn_proc(qtk_vits_syn_t* syn, int* x, int x_len);
void qtk_vits_syn_del(qtk_vits_syn_t* syn);

#ifdef __cplusplus
};
#endif
#endif /* QTK_VITS_QTK_VITS_SYN_H_ */
