#include <math.h>
#include "wtk_favg.h"
#include "wtk/core/wtk_sort.h"

void wtk_avgfeat_print(wtk_avgfeat_t *f)
{
	wtk_debug("=========== avgfeat %p ============\n",f);
	printf("index: %d\n",f->index);
	printf("used: %d\n",f->used);
	printf("f0: %f\n",f->f0);
}

wtk_favg_t* wtk_favg_new(wtk_favg_cfg_t *cfg)
{
	wtk_favg_t *avg;
	int n;

	avg=(wtk_favg_t*)wtk_malloc(sizeof(*avg));
	avg->cfg=cfg;
	//avg->raise_f=0;
	//avg->raise_ths=0;
	n=cfg->win*2+1;
	avg->win_robin=wtk_robin_new(n);
	avg->win_avgfeat=(wtk_avgfeat_t**)wtk_calloc(n,sizeof(wtk_avgfeat_t*));
	n=4096/sizeof(wtk_avgfeat_t);
	avg->avgheap=wtk_bit_heap_new(sizeof(wtk_avgfeat_t),n,n,0);
	wtk_favg_reset(avg);
	return avg;
}

void wtk_favg_delete(wtk_favg_t *avg)
{
	wtk_guassrand_clean(&(avg->guass_rand));
	wtk_free(avg->win_avgfeat);
	wtk_bit_heap_delete(avg->avgheap);
	wtk_robin_delete(avg->win_robin);
	wtk_free(avg);
}

void wtk_favg_reset(wtk_favg_t *avg)
{
	avg->index=0;
	avg->valid_f0=0;
	avg->avg_f0=0;
	avg->last_post_f0=0;
	wtk_robin_reset(avg->win_robin);
	wtk_bit_heap_reset(avg->avgheap);
	wtk_guassrand_reset(&(avg->guass_rand));
	wtk_queue_init(&(avg->outputq));
}

wtk_avgfeat_t* wtk_favg_pop_feat(wtk_favg_t *avg)
{
	wtk_avgfeat_t* f;

	f=(wtk_avgfeat_t*)wtk_bit_heap_malloc(avg->avgheap);
	f->index=++avg->index;
	f->used=0;
	//wtk_debug("%p\n",f);
	return f;
}

void wtk_favg_push_feat(wtk_favg_t *avg,wtk_avgfeat_t *f)
{
	//wtk_debug("v[%d,%p]=%d\n",f->index,f,f->used);
	if(f->used==0)
	{
		wtk_bit_heap_free(avg->avgheap,f);
	}
}

void wtk_favg_reuse(wtk_favg_t *avg,wtk_avgfeat_t *f)
{
	--f->used;
	wtk_favg_push_feat(avg,f);
}

void wtk_favg_output_feature(wtk_favg_t *avg,wtk_avgfeat_t *f)
{
	wtk_favg_cfg_t *cfg=avg->cfg;
	float avg_now;
	float avg_seen;
	float factor;

	if(f->post_f0==0)
	{
		f->post_f0=wtk_guassrand_rand(&(avg->guass_rand),cfg->norm_avg_prior,cfg->norm_var_prior);
	}else
	{
		avg->avg_f0=avg_seen=(f->post_f0+avg->valid_f0*avg->avg_f0)/(avg->valid_f0+1);
		++avg->valid_f0;
		factor=pow(cfg->alpha,avg->valid_f0);
		avg_now=factor*cfg->avg_prior+(1-factor)*avg_seen;
		f->post_f0=f->post_f0-avg_now;
	}
	//wtk_debug("f0 v[%d]=%f,%f\n",f->index,f->f0,f->post_f0);
	avg->last_post_f0=f->post_f0;
	++f->used;
	wtk_queue_push(&(avg->outputq),&(f->q_n));
}

void wtk_favg_flush_robin(wtk_favg_t *avg,wtk_robin_t *r)
{
	wtk_avgfeat_t *f;

	f=(wtk_avgfeat_t*)wtk_robin_pop(r);
	--f->used;
	wtk_favg_push_feat(avg,f);
}

void wtk_favg_print_array(wtk_favg_t *avg)
{
	wtk_robin_t *robin=avg->win_robin;
	wtk_avgfeat_t **avg_feat=avg->win_avgfeat;
	int i;

	for(i=0;i<robin->nslot;++i)
	{
		wtk_debug("v[%d,%d]=%f\n",i,avg_feat[i]->index,avg_feat[i]->f0);
	}
}

float wtk_favg_cmp_feat(wtk_favg_t *avg,wtk_avgfeat_t **f1,wtk_avgfeat_t **f2)
{
	return ((*f1)->f0-(*f2)->f0);
}

wtk_avgfeat_t* wtk_favg_flush_feat(wtk_favg_t *avg,int is_end)
{
	wtk_robin_t *robin=avg->win_robin;
	wtk_avgfeat_t **avg_feat=avg->win_avgfeat;
	wtk_avgfeat_t *f=0;
	wtk_avgfeat_t *outputf=0;

	if(robin->used<=avg->cfg->win){goto end;}
	wtk_robin_peek_win_array(robin,(void**)avg_feat,is_end);
	//wtk_favg_print_array(avg);
	outputf=avg_feat[avg->cfg->win];
	wtk_qsort3(avg_feat,robin->nslot,sizeof(wtk_avgfeat_t*),(wtk_qsort_cmp_f)wtk_favg_cmp_feat,avg,&f);
	//wtk_favg_print_array(avg);
	f=avg_feat[avg->cfg->win];
	outputf->post_f0=f->f0;
	if(robin->nslot==robin->used || is_end)
	{
		//if robin is full or got end hint, remove the front feature in the robin.
		wtk_favg_flush_robin(avg,robin);
	}
end:
	return outputf;
}

void wtk_favg_feed_feat(wtk_favg_t *avg,wtk_avgfeat_t *f)
{
	wtk_robin_t *robin=avg->win_robin;

	++f->used;
	wtk_robin_push(robin,f);
	if(robin->used<=avg->cfg->win){goto end;}
	f=wtk_favg_flush_feat(avg,0);
	wtk_favg_output_feature(avg,f);
end:
	return;
}

void wtk_favg_feed(wtk_favg_t *avg,float f0)
{
	wtk_avgfeat_t* f;

	f=wtk_favg_pop_feat(avg);
	f->f0=f0;
	//wtk_debug("f0 v[%d]=%f\n",f->index,f0);
	wtk_favg_feed_feat(avg,f);
	//wtk_debug("alloc=%d,used=%d;\n",avg->avgheap->tot_alloc,avg->avgheap->tot_used);
}

void wtk_favg_flush_end(wtk_favg_t *avg)
{
	wtk_avgfeat_t *f;

	while(1)
	{
		f=wtk_favg_flush_feat(avg,1);
		if(!f){break;}
		wtk_favg_output_feature(avg,f);
	}
}
