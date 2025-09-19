/*
 * qtk_vits_synthesizertrn.h
 *
 *  Created on: Aug 23, 2022
 *      Author: dm
 */
#ifndef QTK_VITS_QTK_VITS_CFG_H_
#define QTK_VITS_QTK_VITS_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "parse/qtk_tts_parse_cfg.h"
#include "qtk_vits_onnx_cfg.h"
#include "vits/qtk_vits_syn_cfg.h"
#include "cosynthesis/wtk_wsola_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_vits_cfg qtk_vits_cfg_t;

struct qtk_vits_cfg{
//	qtk_tts_parse_cfg_t parse;
	qtk_vits_syn_cfg_t syn;
	qtk_vits_onnx_cfg_t onnx;
	qtk_vits_onnx_cfg_t dec_onnx;
	wtk_mbin_cfg_t* mbin_cfg;
	wtk_wsola_cfg_t wsola;
	int chunk_len;
	int chunk_pad;
	int step;
	int upsample;
	int nt_thd;
	unsigned int use_stream:1;
};
int qtk_vits_cfg_init(qtk_vits_cfg_t *cfg);
int qtk_vits_cfg_clean(qtk_vits_cfg_t *cfg);
int qtk_vits_cfg_update_local(qtk_vits_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_vits_cfg_update(qtk_vits_cfg_t *cfg);
int qtk_vits_cfg_update2(qtk_vits_cfg_t *cfg,wtk_source_loader_t *sl);

//qtk_vits_cfg_t* qtk_vits_cfg_new_bin(char *cfg_fn,int seek_pos);
//void qtk_vits_cfg_delete_bin(qtk_vits_cfg_t *cfg);
//int qtk_vits_cfg_delete(qtk_vits_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif /* QTK_VITS_QTK_VITS_CFG_H_ */
