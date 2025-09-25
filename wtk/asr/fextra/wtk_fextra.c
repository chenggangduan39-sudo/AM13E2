#include "wtk_fextra.h"
#include "wtk_sigp.h"
wtk_feat_t* wtk_extra_new_feature(wtk_fextra_t *p);
wtk_feat_t* wtk_extra_new_feature2(wtk_fextra_t *p);

wtk_fextra_t* wtk_fextra_new2(wtk_fextra_cfg_t *cfg, wtk_matrix_t* hlda_mat)
{
	wtk_fextra_t *p;

	p=(wtk_fextra_t*)wtk_malloc(sizeof(*p));
	wtk_fextra_init2(p,cfg,hlda_mat);
	if(cfg->use_nnet3)
	{
		p->nnet3=qtk_nnet3_new(&(cfg->nnet3),cfg->feature_cols,0);
		if(cfg->DELTA)
		{
			p->nnet3->delta=1;
		}
	}
	return p;
}

wtk_fextra_t* wtk_fextra_new(wtk_fextra_cfg_t *cfg)
{
	return wtk_fextra_new2(cfg, NULL);
}

int wtk_fextra_delete(wtk_fextra_t *p)
{
	if(p->cfg->use_nnet3)
	{
      		qtk_nnet3_delete(p->nnet3);
	}
	wtk_fextra_clean(p);
	wtk_free(p);
	return 0;
}

int wtk_fextra_init2(wtk_fextra_t *p,wtk_fextra_cfg_t *cfg, wtk_matrix_t* hlda_mat)
{
	int t;
	int cache=cfg->cache_size;

	memset(p,0,sizeof(*p));
	p->cfg=cfg;
	p->hlda_mat=hlda_mat;
//	if(!p->hlda_mat)
//	{
//		p->hlda_mat=cfg->hlda_mat;
//		hlda_mat=p->hlda_mat;
//	}
	if(hlda_mat)
	{
		p->xform_rows=wtk_matrix_rows(hlda_mat);
		p->xform_cols=wtk_matrix_cols(hlda_mat);
	}
	p->frame_buffer=wtk_vector_buffer_new(cfg->frame_size*cache);
	p->frame_vector=wtk_vector_new(cfg->frame_size);
	wtk_sigp_init(&(p->sigp),cfg);
	if (p->hlda_mat)
		wtk_hoard_init(&(p->feature_hoard),offsetof(wtk_feat_t,hoard_n),cache
				,(wtk_new_handler_t)wtk_extra_new_feature2,(wtk_delete_handler_t)wtk_feat_delete,p);
	else
		wtk_hoard_init(&(p->feature_hoard),offsetof(wtk_feat_t,hoard_n),cache
				,(wtk_new_handler_t)wtk_extra_new_feature,(wtk_delete_handler_t)wtk_feat_delete,p);
	if(cfg->DELTA)
	{
		int del_win=cfg->DELTAWINDOW;
		int acc_win=cfg->ACCWINDOW;
		int third_win=cfg->THIRDWINDOW;

		p->feature_pos[0]=p->cfg->static_feature_cols+1;
		p->feature_pos[1]=p->feature_pos[0]+p->cfg->static_feature_cols;
		p->feature_pos[2]=p->feature_pos[1]+p->cfg->static_feature_cols;
		p->robins[0]=wtk_robin_new(del_win*2+1);
		p->win[0]=cfg->DELTAWINDOW;
		p->win[1]=cfg->ACCWINDOW;
		p->win[2]=cfg->THIRDWINDOW;
		p->diffs=1;
		if(cfg->ACCS)
		{
			p->robins[1]=wtk_robin_new(acc_win*2+1);
			++p->diffs;
		}else
		{
			p->robins[1]=0;
		}
		if(cfg->THIRD)
		{
			p->robins[2]=wtk_robin_new(third_win*2+1);
			++p->diffs;
		}else
		{
			p->robins[2]=0;
		}
		t=del_win>acc_win?del_win:acc_win;
		t=third_win>t?third_win:t;
		t=t*2+1;
		p->v_tmp_array=(wtk_vector_t**)wtk_malloc(t*sizeof(wtk_vector_t*));
		p->f_tmp_array=(wtk_feat_t**)wtk_malloc(t*sizeof(wtk_feat_t*));
	}
	p->n_frame_index=0;
	if(cfg->ZMEAN)
	{
		if(cfg->use_cmn)
		{
			p->zpost.zmean=wtk_cmn_new(&(cfg->cmn),p);
			if(cfg->use_cmn2)
			{

			}else
			{
				wtk_fextra_set_static_post(p,(wtk_fextra_static_post_f)wtk_cmn_static_post_feed,p->zpost.zmean);
				wtk_fextra_set_post(p,(wtk_fextra_post_f)wtk_cmn_post_feed,p->zpost.zmean);
			}
		}
		if(cfg->use_cmn2)
		{
			p->zpost.zmean=wtk_cmn_new(&(cfg->cmn),p);
		}
	}
	//wtk_debug("use_dnn=%d\n",cfg->use_dnn);
	if(cfg->use_dnn)
	{
		p->dnn=wtk_fnn_new(&(cfg->dnn),p);
	}else
	{
		p->dnn=0;
	}
	p->output_queue=NULL;
	p->notify_ths=NULL;
	p->notify=NULL;
	p->win_ths=NULL;
	p->win_notify=NULL;
	p->float_vector_idx=0;
	return 0;
}

int wtk_fextra_init(wtk_fextra_t *p,wtk_fextra_cfg_t *cfg)
{
	return wtk_fextra_init2(p, cfg, NULL);
}

int wtk_fextra_clean(wtk_fextra_t *p)
{
	int i;

	for(i=0;i<3;++i)
	{
		if(p->robins[i])
		{
			wtk_robin_delete(p->robins[i]);
		}
	}
	if(p->v_tmp_array){wtk_free(p->v_tmp_array);}
	if(p->f_tmp_array){wtk_free(p->f_tmp_array);}
	wtk_sigp_clean(&(p->sigp));
	wtk_vector_buffer_delete(p->frame_buffer);
	wtk_vector_delete(p->frame_vector);
	wtk_hoard_clean(&(p->feature_hoard));
	if(p->cfg->ZMEAN)
	{
		if(p->cfg->use_cmn || p->cfg->use_cmn2)
		{
			wtk_cmn_delete(p->zpost.zmean);
		}
	}
	if(p->dnn)
	{
		wtk_fnn_delete(p->dnn);
	}
	return 0;
}

void wtk_fextra_set_output_queue(wtk_fextra_t *p,wtk_queue_t *q)
{
	p->output_queue=q;
}

void wtk_fextra_set_notify(wtk_fextra_t *p,wtk_fextra_feature_notify_f notify,void *notify_ths)
{
	p->notify_ths=notify_ths;
	p->notify=notify;
}

void wtk_fextra_set_win_notify(wtk_fextra_t *p,wtk_fextra_window_notify_f notify,void *win_ths)
{
	p->win_notify=notify;
	p->win_ths=win_ths;
}

/*
int wtk_extra_feature_size(wtk_extra_t *p)
{
	return p->xform_rows>0?p->xform_rows:p->feature_cols;
}
*/

wtk_feat_t* wtk_extra_new_feature(wtk_fextra_t *p)
{
	wtk_feat_t *f;

	//f=wtk_feature_new(p->cfg->align,p->feature_cols,p->xform_rows,p->fmpe?1:0);
	f=wtk_feat_new(&(p->cfg->feat));
	f->send_hook=p;
	f->send=(wtk_feat_sender_t)wtk_fextra_push_feature;
	return f;
}

wtk_feat_t* wtk_extra_new_feature2(wtk_fextra_t *p)
{
	wtk_feat_t *f;

	f=wtk_feat_new2(&(p->cfg->feat),p->xform_rows);
	f->send_hook=p;
	f->send=(wtk_feat_sender_t)wtk_fextra_push_feature;
	return f;
}

//void wtk_extra_inc_feature_use(wtk_extra_t *p,wtk_feature_t *f)
//{
//	if(p->cfg->use_lock)
//	{
//		wtk_lock_t *lock;
//
//		lock=&(p->feature_hoard.lock_hoard.lock);
//		wtk_lock_lock(lock);
//		++f->used;
//		wtk_lock_unlock(lock);
//	}else
//	{
//		++f->used;
//	}
//}
//
//void wtk_extra_dec_feature_use(wtk_extra_t *p,wtk_feature_t *f)
//{
//	if(p->cfg->use_lock)
//	{
//		wtk_lock_t *lock;
//
//		lock=&(p->feature_hoard.lock_hoard.lock);
//		wtk_lock_lock(lock);
//		--f->used;
//		wtk_lock_unlock(lock);
//	}else
//	{
//		--f->used;
//	}
//}

int wtk_fextra_push_feature(wtk_fextra_t *p,wtk_feat_t *f)
{
	//wtk_debug("%p:%d:%d\n",f,f->index,f->used);
	//wtk_debug("push feature %p, idx=%d used=%d, len=%d,free=%d.\n",f,f->index,f->used,p->feature_hoard.use_length,p->feature_hoard.cur_free);
	//wtk_debug("feature=%d/%p used=%d\n",f->index,f,f->used);
//	if(f->used<0)
//	{
//		wtk_debug("found bug %d/%p used=%d\n",f->index,f,f->used);
//		exit(0);
//	}
	//wtk_debug("want push feature parm=%p %d/%p used=%d.\n",p,f->index,f,f->used);
	if(f->used==0)
	{
		//wtk_feature_delete(f);
		//wtk_debug("push feature parm=%p %d/%p used=%d.\n",p,f->index,f,f->used);
		wtk_hoard_push(&(p->feature_hoard),f);
	}
	return 0;
}

int wtk_fextra_reuse_feature(wtk_fextra_t *p,wtk_feat_t *f)
{
	//wtk_debug("%p:%d:%d\n",f,f->index,f->used);
	--f->used;
	return wtk_fextra_push_feature(p,f);
}

wtk_feat_t* wtk_fextra_pop_feature(wtk_fextra_t *p)
{
	wtk_feat_t* f;

	//f=wtk_extra_new_feature(p);
	f=(wtk_feat_t*)wtk_hoard_pop(&(p->feature_hoard));
	f->used=0;
	f->index=++p->n_frame_index;
	f->app_hook=NULL;
	f->hook.p=NULL;
	//wtk_debug("pop feature %d/%p used=%d\n",f->index,f,f->used);
	//wtk_debug("pop feature parm=%p %d/%p use=%d free=%d.\n",p,f->index,f,p->feature_hoard.use_length,p->feature_hoard.cur_free);
	return f;
}

/*
 *remove the front feature in robin.
 */
void wtk_fextra_flush_robin(wtk_fextra_t *p,wtk_robin_t *r)
{
	wtk_feat_t *f;

	f=(wtk_feat_t*)wtk_robin_pop(r);
	--f->used;
	wtk_fextra_push_feature(p,f);
}

wtk_feat_t* wtk_fextra_flush_feature(wtk_fextra_t *p,wtk_robin_t *r,int win,double diff_sigma,int startv,int is_end)
{
	wtk_vector_t **pv=p->v_tmp_array;
	wtk_feat_t **fv=p->f_tmp_array;
	wtk_feat_t *f;
	int i,pad,j;

	if(r->used<=win){return 0;}
	pad=r->nslot-r->used;i=0;
	//wtk_debug("r=%p,nslot=%d,used=%d\n",r,r->nslot,r->used);
	if(pad>0 && !is_end)
	{
		//if not end, add pad to front.
		// * |f0|f1|f2|0|0|  => |f0|f0|f0|f1|f2|
		// * |f0|f1|f2|f3|0| => |f0|f0|f1|f2|f3|
		f=(wtk_feat_t*)wtk_robin_at(r,0);
		for(;i<pad;++i)
		{
			pv[i]=f->v;
			fv[i]=f;
			//wtk_debug("v[%d]=%d,%p\n",i,f->index,f);
		}
	}
	for(j=0;j<r->used;++i,++j)
	{
		f=((wtk_feat_t*)wtk_robin_at(r,j));
		pv[i]=f->v;
		fv[i]=f;
		//wtk_debug("v[%d]=%d,%p\n",i,((wtk_feature_t*)wtk_robin_at(r,j))->index,((wtk_feature_t*)wtk_robin_at(r,j)));
	}
	if(pad>0 && is_end)
	{
		//if is end and pad to the end.
		//|f0|f1|f2|f3|0| => |f0|f1|f2|f3|f3|
		//|f0|f1|f2|0|0| => |f0|f1|f2|f2|f2|
		f=(wtk_feat_t*)wtk_robin_at(r,r->used-1);
		for(j=0;j<pad;++i,++j)
		{
			pv[i]=f->v;
			fv[i]=f;
			//wtk_debug("v[%d]=%d,%p\n",i,f->index,f);
		}
	}
	if(p->cfg->SIMPLEDIFFS)
	{
		wtk_math_do_simple_diff(pv,win,startv,p->cfg->static_feature_cols);
	}else
	{
		wtk_math_do_diff(pv,win,diff_sigma,startv,p->cfg->static_feature_cols);
	}
	f=fv[win];
	//f=wtk_feature_v_to_f(pv[win]);
	if(r->nslot==r->used || is_end)
	{
		//if robin is full or got end hint, remove the front feature in the robin.
		wtk_fextra_flush_robin(p,r);
	}
	return f;
}

void wtk_fextra_process_hlda(wtk_fextra_t *p,wtk_feat_t *f)
{
	wtk_matrix_t *m=p->hlda_mat;
	int rows,cols,i,j;
	wtk_vector_t *v,*fv;
	float vi;
	float *mi;

	v=f->xf_v;
	fv=f->v;
	rows=p->xform_rows;
	cols=p->xform_cols;
	for(i=1;i<=rows;++i)
	{
		vi=0;mi=m[i];
		for(j=1;j<=cols;++j)
		{
			vi+=mi[j]*fv[j];
		}
		v[i]=vi;
	}
	//wtk_vector_print(v);
	//exit(0);
}

void wtk_fextra_output_feature_to_queue(wtk_fextra_t *p,wtk_feat_t *f)
{
	//wtk_debug("raise f=%d/%p used=%d\n",f->index,f,f->used);
	//wtk_vector_print(f->rv);
	++f->used;
	if(p->output_queue)
	{
		wtk_queue_push(p->output_queue,&(f->queue_n));
	}else if(p->notify)
	{
		p->notify(p->notify_ths,f);
	}
}

void wtk_fextra_output_feature_to_dnn(wtk_fextra_t *p,wtk_feat_t *f)
{
	if(p->dnn)
	{
		wtk_fnn_feed(p->dnn,f);
	}else
	{
		wtk_fextra_output_feature_to_queue(p,f);
	}
}

void wtk_extra_output_feature_to_hlda(wtk_fextra_t *p,wtk_feat_t *f)
{
	//wtk_debug("====\n");
	//wtk_vector_print(f->v);
	if(p->hlda_mat)
	{
		wtk_fextra_process_hlda(p,f);
	}
	wtk_fextra_output_feature_to_dnn(p,f);
}

void wtk_fextra_output_feature(wtk_fextra_t *p,wtk_feat_t *f)
{
	if(p->cfg->use_nnet3)
	{
		qtk_nnet3_run(p->nnet3,f,0);
	}else
	{
		wtk_extra_output_feature_to_hlda(p,f);
	}
}

void wtk_fextra_process_post_feature(wtk_fextra_t *p,wtk_feat_t *f)
{
	/*
	if(p->cvn && p->cvn->cfg->online)
	{
		wtk_cvn_process_online_dynamic(p->cvn,f);
	}
	*/
	if(p->post_f)
	{
		p->post_f(p->post_hook,f);
	}else
	{
		wtk_fextra_output_feature(p,f);
	}
}

void wtk_robin_print_feature(wtk_robin_t *r)
{
	wtk_feat_t *f;
	int i;

	for(i=0;i<r->used;++i)
	{
		f=((wtk_feat_t*)wtk_robin_at(r,i));
		printf("v[%d]=%d\n",i,f->index);
	}
}

void wtk_fextra_feed_feature(wtk_fextra_t *p,wtk_feat_t* f,int index)
{
	wtk_robin_t *r=p->robins[index];
	int win=p->win[index];
	int startv=p->feature_pos[index];
    //wtk_debug("index:%d\n",index);
    //wtk_vector_print(f->v);
	//wtk_debug("index=%d\n",index);
	++f->used;
	wtk_robin_push(r,f);
	//wtk_robin_print_feature(r);
	if(r->used<=win){goto end;}
	//|f0|f1|f2|f3|f4| => calc the dynamic feature of f2 and return it/
	f=wtk_fextra_flush_feature(p,r,win,p->cfg->sigma[index],startv,0);
	++index;
	if(index<3 && p->robins[index])
	{
		//if there is D T A want to do, raise the feature to the higher robin.
		wtk_fextra_feed_feature(p,f,index);
	}else
	{
		//wtk_debug("-----output-----\n");
		//if feature all get, output the feature.
		wtk_fextra_process_post_feature(p,f);
	}
end:
	return;
}

void wtk_fextra_flush_robin_feature(wtk_fextra_t *p,int index)
{
	wtk_feat_t* f;
	wtk_robin_t *r=p->robins[index];
	double diff_sigma=p->cfg->sigma[index];
	int win=p->win[index];
	int startv=p->feature_pos[index];
	int next;

	index+=1;
	next=(index<p->diffs && p->robins[index])?1:0;
	while(1)
	{
		//flush the end feature in the robin
		//|f0|f1|f2|f3|0| => |f0|f1|f2|f3|f3| => flush f2 => |f1|f2|f3|0|0|
		//|f1|f2|f3|0|0| => |f1|f2|f3|f3|f3| => flush f3 -> |f2|f3|0|0|
		f=wtk_fextra_flush_feature(p,r,win,diff_sigma,startv,1);
		if(!f){break;}
		if(next)
		{
			wtk_fextra_feed_feature(p,f,index);
		}else
		{
			wtk_fextra_process_post_feature(p,f);
		}
	}
	while(r->used>0)
	{
		//push f2 f3 cached feature in robin back to hoard.
		wtk_fextra_flush_robin(p,r);
	}
}

void wtk_fextra_flush_robins(wtk_fextra_t *p)
{
	int i;

	for(i=0;i<3;++i)
	{
		if(p->robins[i])
		{
			wtk_fextra_flush_robin_feature(p,i);
		}
	}
}

void wtk_fextra_print_hoard_feature(wtk_fextra_t *p)
{
	wtk_queue_node_t *n;
	wtk_feat_t *f;

	for(n=p->feature_hoard.use;n;n=n->prev)
	{
		f=data_offset(n,wtk_feat_t,hoard_n);
		printf("use: index=%d,used=%d,addr=%p\n",f->index,f->used,f);
	}
	for(n=p->feature_hoard.free;n;n=n->prev)
	{
		f=data_offset(n,wtk_feat_t,hoard_n);
		printf("free: index=%d,used=%d,addr=%p\n",f->index,f->used,f);
	}
}

void wtk_fextra_flush_feature_queue(wtk_fextra_t *p,wtk_queue_t *q)
{
	wtk_feat_t *f;
	wtk_queue_node_t *n;

	while(1)
	{
		n=wtk_queue_pop(q);
		if(!n){break;}
		f=wtk_queue_node_data(n,wtk_feat_t,queue_n);
		--f->used;
		wtk_fextra_push_feature(p,f);
	}
}

int wtk_fextra_reset(wtk_fextra_t *p)
{
	int i;

	p->float_vector_idx=0;
	if(p->feature_hoard.use_length>0)
	{
	//	wtk_fextra_feed(p,0,0,1);
	}
	for(i=0;i<3;++i)
	{
		if(p->robins[i])
		{
			wtk_robin_reset(p->robins[i]);
		}
	}
	if(p->output_queue)
	{
		wtk_fextra_flush_feature_queue(p,p->output_queue);
		wtk_queue_init(p->output_queue);
	}
	wtk_vector_buffer_reset(p->frame_buffer);
	p->n_frame_index=0;
	if(p->cfg->ZMEAN)
	{
		if(p->cfg->use_cmn || p->cfg->use_cmn2)
		{
			wtk_cmn_reset(p->zpost.zmean);
		}
	}
	if(p->cfg->use_nnet3)
	{
		qtk_nnet3_reset(p->nnet3);
	}
	if(p->dnn)
	{
		wtk_fnn_reset(p->dnn);
	}
	//wtk_debug("use_len=%d,cur_free=%d\n",p->feature_hoard.use_length,p->feature_hoard.cur_free);
	return 0;
}

int wtk_fextra_feed(wtk_fextra_t *p,short* data,int samples,int is_end)
{
	return wtk_fextra_feed2(p,(char*)data,samples*2,is_end);
}

void wtk_extra_add_dither(wtk_fextra_t *p,wtk_vector_t *v)
{
	int i;
	int vsize;

	vsize=wtk_vector_size(v);
    for (i=1; i<=vsize; ++i)
    {
    	v[i] +=(wtk_random_value()*2.0 - 1.0)*p->cfg->ADDDITHER;
    }
}

void wtk_fextra_feed_static_input_feature(wtk_fextra_t *p,wtk_feat_t *f)
{
	wtk_fextra_cfg_t *cfg=(p->cfg);

		if(cfg->DELTA)
		{
			//do D T A
			wtk_fextra_feed_feature(p,f,0);
		}else
		{
			//raise feature
			wtk_fextra_process_post_feature(p,f);
		}
}

void wtk_fextra_feed_input_feature(wtk_fextra_t *p,wtk_feat_t *f)
{
	if(p->cfg->use_cmn2)
	{
		wtk_cmn_static_post_feed2(p->zpost.zmean,f);
	}else
	{
		if(p->static_post_f)
		{
			p->static_post_f(p->static_post_hook,f);
		}
		wtk_fextra_feed_static_input_feature(p,f);
	}
}

void wtk_fextra_feed_end(wtk_fextra_t *p)
{
	wtk_fextra_cfg_t *cfg=p->cfg;

	wtk_fextra_flush_robins(p);
	if(cfg->ZMEAN)
	{
		if(cfg->use_cmn)
		{
			wtk_cmn_flush_extra_post_queue(p->zpost.zmean);
		}else if(cfg->use_cmn2)
		{
			wtk_cmn_flush_extra_post_queue2(p->zpost.zmean);
		}
	}
	if(p->dnn)
	{
		//wtk_debug("===============> feed end\n");
		wtk_fnn_flush(p->dnn);
		wtk_fnn_wait_end(p->dnn);
	}
}

void wtk_fextra_feed_plp(wtk_fextra_t *p,wtk_feat_t *f)
{
	wtk_fextra_feed_input_feature(p,f);
}

void wtk_extra_feed_sample(wtk_fextra_t *p,wtk_vector_t *v)
{
	wtk_fextra_cfg_t *cfg=(p->cfg);
	wtk_feat_t *feature;

	feature=wtk_fextra_pop_feature(p);
	//wtk_debug("v[%d]\n",feature->index);
	//do PLP_O or MFCC_0
	//wtk_vector_print(v);
	if(cfg->use_e)
	{
		feature->energy=wtk_float_energy(v+1,wtk_vector_size(v));
	}
	if(cfg->ADDDITHER!=0.0)
	{
		wtk_extra_add_dither(p,v);
	}
	if(p->win_notify)
	{
		p->win_notify(p->win_ths,feature);
	}
	//print_float(v+1,10);
	//wtk_vector_print(v);
	//exit(0);
	wtk_sigp_procss(&(p->sigp),v,&(feature->v[1]));
	//wtk_vector_print(feature->v);
	//print_float(feature->v+1,10);
	//exit(0);
	//if(cfg->use_nnet3)
	//{
	//	qtk_nnet3_run(p->nnet3,feature,0);
	//}else
	{
		wtk_fextra_feed_plp(p,feature);
	}
}

void wtk_fextra_flush_end(wtk_fextra_t *p)
{
	//flush robin, pad end feature and process D T A if need.
	wtk_fextra_feed_end(p);
}

void wtk_fextra_flush_layer(wtk_fextra_t *p,int force)
{
	if(p->cfg->use_cmn && p->zpost.zmean)
	{
		wtk_cmn_flush(p->zpost.zmean,force);
	}
	if(p->dnn)
	{
		wtk_fnn_flush_layer(p->dnn,force);
	}
}

int wtk_fextra_can_flush_all(wtk_fextra_t *p)
{
	if(p->cfg->use_cmn)
	{
		if(p->zpost.zmean)
		{
			return wtk_cmn_can_flush_all(p->zpost.zmean);
		}else
		{
			return 0;
		}
	}
	return 1;
}

int wtk_fextra_feed2(wtk_fextra_t *p,char* data,int bytes,int is_end)
{
	char *end=data+bytes;
	wtk_vector_t *v=p->frame_vector;
	wtk_vector_buffer_t *b=p->frame_buffer;
	wtk_fextra_cfg_t *cfg=p->cfg;

	//wtk_debug("xxxxx is end:%d\n",is_end);
	while(data<end)
	{
		data+=wtk_vector_buffer_push_c(b,data,end-data);
		while(wtk_vector_buffer_peek(b,v,0)==0)
		{
			//wtk_debug("frame_step=%d\n",cfg->frame_step);
			wtk_extra_feed_sample(p,v);
			wtk_vector_buffer_skip(p->frame_buffer,cfg->frame_step,cfg->frame_size);
		}
	}
	if(is_end)
	{
		//wtk_debug("run nnet3 end\n");
		wtk_fextra_flush_end(p);
		if(p->cfg->use_nnet3)
		{
			qtk_nnet3_run(p->nnet3,NULL,1);
		}
	}
	return 0;
}

int wtk_fextra_feed_float2(wtk_fextra_t *p,float* data,int len,int is_end)
{
	float *end=data+len;
	wtk_vector_t *v=p->frame_vector;
	wtk_vector_buffer_t *b=p->frame_buffer;
	wtk_fextra_cfg_t *cfg=p->cfg;

	while(data<end)
	{
		data+=wtk_vector_buffer_push_float(b,data,end-data);
		while(wtk_vector_buffer_peek(b,v,0)==0)
		{
			//wtk_debug("frame_step=%d\n",cfg->frame_step);
			//print_float(v+1,100);
			//exit(0);
			wtk_extra_feed_sample(p,v);
			wtk_vector_buffer_skip(p->frame_buffer,cfg->frame_step,cfg->frame_size);
		}
	}
	if(is_end)
	{	
			
		wtk_fextra_flush_end(p);
	}
	return 0;
}

int wtk_fextra_feed_float(wtk_fextra_t *p,float* data,int n,int is_end)
{
	wtk_vector_t *v=p->frame_vector;
	wtk_fextra_cfg_t *cfg=p->cfg;
	float *pv=v+1,*pv2;
	float *ps,*pe;
	int step;
	int dt=cfg->frame_size-cfg->frame_step;
	int v2;
	float scale=32767;
	int i;

	//scale=scale*1.5;
	//wtk_debug("frame_size=%d/%d v=%d\n",cfg->frame_step,cfg->frame_size,wtk_vector_size(v));
	ps=data;
	pe=ps+n;
	while(ps<pe)
	{
		step=(int)(pe-ps);
		v2=cfg->frame_size-p->float_vector_idx;
		step=min(step,v2);
		pv2=pv+p->float_vector_idx;
		for(i=0;i<step;++i)
		{
			pv2[i]=ps[i]*scale;
		}
		//memcpy(pv+p->float_vector_idx,ps,sizeof(float)*step);
		p->float_vector_idx+=step;
		ps+=step;
		//wtk_debug("idx=%d\n",p->float_vector_idx);
		if(p->float_vector_idx>=cfg->frame_size)
		{
			wtk_extra_feed_sample(p,v);
			memmove(pv,pv+cfg->frame_step,sizeof(float)*dt);
			p->float_vector_idx=dt;
		}
	}
	//exit(0);
	if(is_end)
	{
		wtk_fextra_flush_end(p);
	}
	return 0;
}

void wtk_fextra_set_post(wtk_fextra_t *p,wtk_fextra_post_f post,void *hook)
{
	p->post_f=post;
	p->post_hook=hook;
}

void wtk_fextra_set_static_post(wtk_fextra_t *p,wtk_fextra_static_post_f static_post,void *hook)
{
	p->static_post_f=static_post;
	p->static_post_hook=hook;
}

void wtk_fextra_normalize_energy(wtk_fextra_t *p,wtk_queue_t *q)
{
	wtk_fextra_cfg_t *cfg=(p->cfg);
	wtk_queue_node_t *n;
	wtk_feat_t *f;
	double ma=0,t,in;
	int e_i=p->cfg->static_feature_cols;

	for(n=q->pop;n;n=n->next)
	{
		f=data_offset(n,wtk_feat_t,queue_n);
		t=f->v[e_i];
		if(!n->prev)
		{
			ma=t;
		}else if(t>ma)
		{
			ma=t;
		}
	}
	in = ma- cfg->esilfloor;
	for(n=q->pop;n;n=n->next)
	{
		f=data_offset(n,wtk_feat_t,queue_n);
		t=f->v[e_i];
		f->v[e_i]=1.0-(ma-max(in,t))*cfg->ESCALE;
	}
}


void wtk_fextra_z(wtk_fextra_t *p,wtk_queue_t *q)
{
	wtk_vector_t *buf;
	wtk_queue_node_t *n;
	wtk_feat_t *f;
	int cols=p->cfg->static_feature_cols;
	int i;

	buf=wtk_vector_new(cols);
	for(n=q->pop;n;n=n->next)
	{
		f=data_offset(n,wtk_feat_t,queue_n);
		for(i=1;i<=cols;++i)
		{
			buf[i]+=f->v[i];
		}
	}
	for(i=1;i<=cols;++i)
	{
		buf[i]/=q->length;
	}
	//wtk_vector_delete(buf);
	for(n=q->pop;n;n=n->next)
	{
		f=data_offset(n,wtk_feat_t,queue_n);
		for(i=1;i<=cols;++i)
		{
			f->v[i]-=buf[i];
		}
	}
	wtk_vector_delete(buf);
}


int wtk_feature_queue_write(wtk_queue_t *q,char *fn,int sample_period,wtk_string_t *target_kind)
{
	wtk_feat_t *feat;
	FILE *f;
	int ret=-1;
	//int samples;
	wtk_fkind_t kind;
	wtk_queue_node_t *n;
	int swap=1;

	f=wtk_file_open(fn,"wb");
	if(!f){goto end;}
	wtk_fkind_from_string(&kind,target_kind->data,target_kind->len);
	feat=data_offset(q->pop,wtk_feat_t,queue_n);
	//samples=wtk_vector_size(feat->rv)*4;//sizeof(float);
	//wtk_write_htk_hdr(f,q->length,sample_period,samples,kind,swap);
	for(n=q->pop;n;n=n->next)
	{
		feat=data_offset(n,wtk_feat_t,queue_n);
		ret=wtk_file_write_float(f,&(feat->rv[1]),wtk_vector_size(feat->rv),1,swap);
	}
end:
	if(f)
	{
		fclose(f);
	}
	return ret;
}

void wtk_feature_queue_print(wtk_queue_t *q)
{
	wtk_feat_t *feat;
	wtk_queue_node_t *n;

	for(n=q->pop;n;n=n->next)
	{
		feat=data_offset(n,wtk_feat_t,queue_n);
		wtk_feat_print(feat);
	}
}


int wtk_fextra_bytes(wtk_fextra_t *p)
{
	int bytes=0;

	bytes=(p->feature_hoard.use_length+p->feature_hoard.cur_free)*wtk_feat_bytes(p->cfg);
	return bytes;
}


void wtk_fextra_test_file(wtk_fextra_t *p,char *fn)
{
	char *data;
	int len;
	double t;

	wtk_fextra_reset(p);
	data=file_read_buf(fn,&len);
	if(!data){goto end;}
	t=time_get_ms();
	wtk_fextra_feed2(p,data,len,1);
	t=time_get_ms()-t;
	wtk_debug("time=%f rate=%f\n",t,t/(p->n_frame_index*p->cfg->frame_dur*1000.0));
end:
	if(data)
	{
		wtk_free(data);
	}
	return;
}

void wtk_fextra_get_cache(wtk_fextra_t *p)
{
	float dur=p->cfg->frame_dur;
	double t;
	int i;
	int nx=0;

	t=wtk_vector_buffer_valid_len(p->frame_buffer)*1.0/16000;
	//wtk_debug("frame buffer=%d\n",wtk_vector_buffer_valid_len(p->frame_buffer));
	for(i=0;i<3;++i)
	{
		if(p->robins[i])
		{
			if(p->robins[i]->used>p->robins[i]->nslot/2)
			{
				nx+=p->robins[i]->used-p->robins[i]->nslot/2;
				t+=(p->robins[i]->used-p->robins[i]->nslot/2)*dur;
			}else
			{
				nx+=p->robins[i]->used;
				t+=(p->robins[i]->used)*dur;
			}
		}
	}
	wtk_debug("win=%d,%d,%d\n",p->robins[0]?p->robins[0]->used:0,p->robins[1]?p->robins[1]->used:0,p->robins[2]?p->robins[2]->used:0);
	wtk_debug("cmn: %d\n",p->zpost.zmean?p->zpost.zmean->post_feature_q.length:0);
	if(p->zpost.zmean)
	{
		nx+=p->zpost.zmean->post_feature_q.length;
		t+=p->zpost.zmean->post_feature_q.length*dur;
	}
	wtk_debug("dnn: %d\n",p->dnn?p->dnn->robin->used:0);
	if(p->dnn)
	{
		nx+=p->dnn->robin->used;
		t+=p->dnn->robin->used*dur;
	}
	wtk_debug("delay=%f nx=%d\n",t,nx);
}
