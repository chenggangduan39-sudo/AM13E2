/*
 * qtk_vits.h
 *
 *  Created on: Aug 26, 2022
 *      Author: dm
 */

#ifndef QTK_VITS_QTK_VITS_H_
#define QTK_VITS_QTK_VITS_H_
#include "qtk_vits_cfg.h"
#include "parse/qtk_tts_parse.h"
#include "vits/qtk_vits_onnx.h"
#include "cosynthesis/wtk_wsola.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef int(*qtk_vits_notify_f)(void* ths, short* data, int len, int is_end);
typedef struct qtk_vits qtk_vits_t;

struct qtk_vits{
	qtk_vits_cfg_t* cfg;
	qtk_vits_onnx_t* onnx;
	qtk_vits_onnx_t* dec_onnx;
	wtk_wsola_t* wsola;
	qtk_vits_notify_f notify;      // callback
	void* user_data;             // for callback
	short *audio_data;
	int audio_data_len;
	float maxf;
};

qtk_vits_t* qtk_vits_new(qtk_vits_cfg_t* cfg);
int qtk_vits_feed(qtk_vits_t* vits, wtk_veci_t **id_vec, int nid);
int qtk_vits_reset(qtk_vits_t* vits);
void qtk_vits_delete(qtk_vits_t* vits);
void qtk_vits_set_notify(qtk_vits_t* vits, qtk_vits_notify_f notify, void* ths);

#ifdef __cplusplus
};
#endif
#endif /* QTK_VITS_QTK_VITS_H_ */
