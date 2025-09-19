/*
 * qtk_xbnf_post_cfg.h
 *
 *  Created on: Jan 6, 2020
 *      Author: dm
 */

#ifndef QTK_XBNF_POST_CFG_H_
#define QTK_XBNF_POST_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_source.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * 1. for leak
 * 2. for self-loop, times control
 */
typedef struct qtk_xbnf_post_cfg qtk_xbnf_post_cfg_t;
struct qtk_xbnf_post_cfg
{
	float pct;  // must be < 1.0.
	int max_skip; //max skip wrds
	int min_len;   //skip when snt length > min_len
	float wrdpen; //base wrd penalty
	float step_wrdpen; // skip distance penalty
	float lpron_fct;   // len of pron(or character) for penalty fact. norm: len is too shorter, penalty is too lower.
	float mpscale;     // scale for min number of phnone in words .
	int min_selfloop;  // min  times for selfloop word
	int max_selfloop;  // min  times for selfloop word
	unsigned use_selfloop_limit:1;

};

//for post
typedef struct
{
	int from;
	wtk_array_t* to;
}qtk_xbnf_arc_skip_t;

int qtk_xbnf_post_cfg_init(qtk_xbnf_post_cfg_t* cfg);
int qtk_xbnf_post_cfg_update_local(qtk_xbnf_post_cfg_t* cfg, wtk_local_cfg_t* main);
int qtk_xbnf_post_cfg_update(qtk_xbnf_post_cfg_t* cfg);
int qtk_xbnf_post_cfg_update2(qtk_xbnf_post_cfg_t* cfg, wtk_source_loader_t* sl);
int qtk_xbnf_post_cfg_clean(qtk_xbnf_post_cfg_t* cfg);

//int qtk_xbnf_post(wtk_xbnf_t* xbnf, qtk_xbnf_post_cfg_t* cfg);
#ifdef __cplusplus
};
#endif

#endif /* QTK_xbnf_POST_CFG_H_ */
