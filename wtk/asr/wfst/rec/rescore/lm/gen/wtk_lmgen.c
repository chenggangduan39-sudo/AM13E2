#include <math.h>
#include "wtk_lmgen.h" 

wtk_lmgen_t* wtk_lmgen_new(wtk_lmgen_cfg_t *cfg)
{
	wtk_lmgen_t *g;

	g=(wtk_lmgen_t*)wtk_malloc(sizeof(wtk_lmgen_t));
	g->cfg=cfg;
	g->backward=wtk_lmgen_rec_new(&(cfg->backward),g);
	g->forward=wtk_lmgen_rec_new(&(cfg->forward),g);
	wtk_lmgen_reset(g);
	return g;
}

void wtk_lmgen_delete(wtk_lmgen_t *g)
{
	wtk_lmgen_rec_delete(g->backward);
	wtk_lmgen_rec_delete(g->forward);
	wtk_free(g);
}

void wtk_lmgen_reset(wtk_lmgen_t *g)
{
	wtk_lmgen_rec_reset(g->backward);
	wtk_lmgen_rec_reset(g->forward);
}


void wtk_lmgen_test(wtk_lmgen_t *g)
{
//	wtk_lmgen_hit_t hits[]={
//			{wtk_string("笨"),0,-3.933617,0},
//			{wtk_string("聪明"),0,-4.016622,0},
//			{wtk_string("我"),0,-4.067784,0},
//			{wtk_string("蛋"),0,-4.089776,0},
//			{wtk_string("傻"),0,-4.112639,0},
//			{wtk_string("说"),0,-4.184053,0},
//			{wtk_string("飞"),0,-4.304547,0},
//			{wtk_string("先"),0,-4.304547,0},
//			{wtk_string("不要紧"),0,-4.304547,0},
//			{wtk_string("鸟"),0,-4.304547,0}
//	};
//	wtk_lmgen_hit_t hits[]={
//	{wtk_string("主人"),0,-3.400760,0},
//	{wtk_string("我"),0,-3.718097,0},
//	{wtk_string("你好"),0,-3.771814,0},
//	{wtk_string("好"),0,-3.986009,0},
//	{wtk_string("有"),0,-4.023724,0},
//	{wtk_string("问问"),0,-4.025709,0},
//	{wtk_string("笨"),0,-4.029265,0},
//	{wtk_string("说"),0,-4.104386,0},
//	{wtk_string("什么"),0,-4.117330,0},
//	};
	wtk_lmgen_hit_t hits[]={
			{wtk_string("我"),0,-5.679461,0},
			{wtk_string("生病"),0,-5.959224,0},
	};
	int i,n;
	//wtk_string_t *v;

	n=sizeof(hits)/sizeof(wtk_lmgen_hit_t);
	for(i=0;i<n;++i)
	{
		hits[i].hit_id=wtk_fst_insym_get_index2(g->cfg->dict.sym,hits[i].hit_key.data,
				hits[i].hit_key.len
				);
		hits[i].stop=0;
//		if(g->cfg->stop_wrd)
//		{
//			v=wtk_str_hash_find(g->cfg->stop_wrd,hits[i].hit_key.data,hits[i].hit_key.len);
//			//wtk_debug("v=%p\n",v);
//			if(v)
//			{
//				hits[i].stop=1;
//			}
//		}
//		wtk_debug("[%.*s:%d]=%f/%f\n",hits[i].hit_key.len,hits[i].hit_key.data,hits[i].stop,
//				hits[i].prob,pow(2,hits[i].prob));
		hits[i].prob=pow(2,hits[i].prob);
	}
	wtk_lmgen_rec_dec(g->forward,hits,n);
	//exit(0);
	wtk_lmgen_rec_forward(g->forward,hits,n,&(g->backward->output_q));
	wtk_lmgen_rec_print(g->forward);
	exit(0);
	//exit(0);
	wtk_lmgen_rec_backward(g->backward,hits,n);
	wtk_lmgen_rec_print(g->backward);
	//exit(0);
	wtk_debug("=======================>\n");
	wtk_lmgen_rec_forward(g->forward,hits,n,&(g->backward->output_q));
	wtk_lmgen_rec_print(g->forward);
	exit(0);
}

