#include "wtk_ann.h"

#ifndef M_PI
#   define M_PI 3.14159265358979323846
#endif

wtk_matrix_t* wtk_new_dct_matrix(int rows,int cols)
{
	wtk_matrix_t *m;
	float norm=sqrt(2.0f/cols);
	float pi=M_PI/cols;
	float v;
	int i,j;

	m=wtk_matrix_new(rows,cols);
	for(i=1;i<=cols;++i)
	{
		m[1][i]=norm;
	}
	for(i=2;i<=rows;++i)
	{
		v=pi*(i-1);
		for(j=1;j<=cols;++j)
		{
			m[i][j]=norm*cos(v*((j-1)+0.5f));
		}
	}
	return m;
}

wtk_ann_t* wtk_ann_new(wtk_ann_cfg_t *cfg)
{
	wtk_ann_t *a;

	a=(wtk_ann_t*)wtk_malloc(sizeof(*a));
	wtk_ann_init(a,cfg);
	return a;
}

int wtk_ann_delete(wtk_ann_t *a)
{
	wtk_ann_clean(a);
	wtk_free(a);
	return 0;
}

int wtk_ann_init(wtk_ann_t *a,wtk_ann_cfg_t *cfg)
{
	wtk_ann_res_t *res=&(cfg->res);
	int win=res->cfg->win;
	int i;
	double factor;
	int reduce_row=res->cfg->reduce_row;
	int f_cols;

	memset(a,0,sizeof(*a));
	a->cfg=cfg;
	a->ann_parm=wtk_fextra_new(&(cfg->ann_parm));
	wtk_queue_init(&(a->ann_queue));
	a->ann_parm->output_queue=&(a->ann_queue);
	f_cols=a->ann_parm->cfg->feature_cols;
	a->ann_robin=wtk_robin_new(win*2+1);
	a->half=1+win;
	a->left_w=wtk_matrix_new(a->half,f_cols);
	a->right_w=wtk_matrix_new(a->half,f_cols);
	a->v_array=(wtk_vector_t**)wtk_malloc(sizeof(wtk_vector_t*)*a->half);
	a->left_factor=wtk_vector_new(a->half);
	a->right_factor=wtk_vector_new(a->half);
	factor=(1-0.01)/(a->half-1);
	for(i=1;i<=a->half;++i)
	{
		a->left_factor[i]=0.01+factor*(i-1);
		a->right_factor[i]=1-factor*(i-1);
	}
	a->dct_matrix=wtk_new_dct_matrix(reduce_row,a->half);
	a->dct_multi_matrx=wtk_matrix_new(reduce_row,f_cols);
	a->left_stream=wtk_ann_stream_new(&res->left_normal,&res->left_wb);
	a->right_stream=wtk_ann_stream_new(&res->right_normal,&res->right_wb);
	a->merge_stream=wtk_ann_stream_new(&res->merge_normal,&res->merge_wb);
	a->pca_mat=wtk_matrix_new(1,wtk_matrix_cols(res->pca_mat));

	a->phn_parm=wtk_fextra_new(&(cfg->phn_parm));
	wtk_queue_init(&(a->phn_queue));
	a->phn_parm->output_queue=&(a->phn_queue);
	i=wtk_matrix_cols(res->pca_mat)+a->phn_parm->cfg->feature_cols;
	a->phn_ann_mat=wtk_matrix_new(1,i);
	if(cfg->use_hlda)
	{
		a->feature_cols=wtk_matrix_cols(res->hlda_mat);
	}else
	{
		a->feature_cols=wtk_matrix_cols(res->pca_mat)+a->phn_parm->cfg->feature_cols;
	}
	a->out_mat=wtk_matrix_new(1,a->feature_cols);
	wtk_hoard_init(&(a->feature_hoard),offsetof(wtk_feat_t,hoard_n),100
			,(wtk_new_handler_t)wtk_ann_new_feature,(wtk_delete_handler_t)wtk_feat_delete,a);
	a->n_frame_index=0;
	return 0;
}

int wtk_ann_clean(wtk_ann_t *a)
{
	wtk_hoard_clean(&(a->feature_hoard));
	wtk_robin_delete(a->ann_robin);
	wtk_matrix_delete(a->left_w);
	wtk_matrix_delete(a->right_w);
	free(a->v_array);
	wtk_vector_delete(a->left_factor);
	wtk_vector_delete(a->right_factor);
	wtk_ann_stream_delete(a->left_stream);
	wtk_ann_stream_delete(a->right_stream);
	wtk_ann_stream_delete(a->merge_stream);
	wtk_matrix_delete(a->dct_matrix);
	wtk_matrix_delete(a->dct_multi_matrx);
	wtk_matrix_delete(a->pca_mat);
	wtk_matrix_delete(a->phn_ann_mat);
	wtk_fextra_delete(a->ann_parm);
	wtk_fextra_delete(a->phn_parm);
	wtk_matrix_delete(a->out_mat);
	return 0;
}

int wtk_ann_reset(wtk_ann_t *a)
{
	wtk_queue_node_t *n,*p;
	wtk_feat_t *f;

	if(a->output_queue)
	{
		for(n=a->output_queue->pop;n;n=p)
		{
			p=n->next;
			f=data_offset(n,wtk_feat_t,queue_n);
			--f->used;
			wtk_ann_push_feature(a,f);
		}
		wtk_queue_init(a->output_queue);
	}
	wtk_fextra_reset(a->ann_parm);
	wtk_fextra_reset(a->phn_parm);
	a->n_frame_index=0;
	return 0;
}

wtk_feat_t* wtk_feat_new_ann(int size)
{
	wtk_feat_t *f;

	f=(wtk_feat_t*)wtk_malloc(sizeof(*f));
	f->cfg=0;
	f->send_hook=f->app_hook=NULL;
	f->used=0;f->index=0;
	//f=wtk_feature_new(p->cfg->align,p->feature_cols,p->xform_rows,p->fmpe?1:0);
	f->v=wtk_vector_new(size);
	f->rv=f->v;
	f->dnn_v=0;
	return f;
}

wtk_feat_t *wtk_ann_new_feature(wtk_ann_t *a)
{
	wtk_feat_t *f;

	//f=wtk_feature_new(a->cfg->align,a->feature_cols,0,0);
//	f=wtk_feat_new(a->feature_cols,0,0,0,0);
	f=wtk_feat_new_ann(a->feature_cols);
	f->send_hook=a;
	f->send=(wtk_feat_sender_t)wtk_ann_push_feature;
	return f;
}

int wtk_ann_push_feature(wtk_ann_t *a,wtk_feat_t *f)
{
	if(f->used==0)
	{
		wtk_hoard_push(&(a->feature_hoard),f);
	}
	return 0;
}

wtk_feat_t* wtk_ann_pop_feature(wtk_ann_t *a)
{
	wtk_feat_t* f;

	f=(wtk_feat_t*)wtk_hoard_pop(&(a->feature_hoard));
	f->used=0;
	f->index=++a->n_frame_index;
	return f;
}

void wtk_ann_add_win_to_feature(wtk_ann_t *a,wtk_matrix_t *m,wtk_vector_t **va,wtk_vector_t *w)
{
	int rows,cols,i,j;
	wtk_vector_t* v;

	cols=wtk_matrix_cols(m);
	rows=wtk_matrix_rows(m);
	for(i=1;i<=rows;++i)
	{
		v=va[i-1];
		for(j=1;j<=cols;++j)
		{
			m[i][j]=v[j]*w[i];
			//wtk_debug("%f,%f,%f\n",v[j],w[i],m[i][j]);
		}
	}
}

void wtk_ann_add_win(wtk_ann_t *a,int s)
{
	wtk_robin_t *r=a->ann_robin;
	wtk_feat_t *f;
	wtk_vector_t **v=a->v_array;
	int half=a->half;
	int end,pad;
	int i,j,pos;

	end=s==1;
	pad=r->nslot-r->used;
	i=0;
	if(pad>0 && !end)
	{
		f=(wtk_feat_t*)wtk_robin_at(r,0);
		for(;i<pad;++i)
		{
			v[i]=f->rv;
		}
	}
	for(j=0;i<half;++i,++j)
	{
		v[i]=((wtk_feat_t*)wtk_robin_at(r,j))->rv;
	}
	pos=j-1;
	wtk_ann_add_win_to_feature(a,a->left_w,v,a->left_factor);
	for(i=0,j=pos;j<r->used && i<half;++j,++i)
	{
		v[i]=((wtk_feat_t*)wtk_robin_at(r,j))->rv;
	}
	if(pad>0 && end)
	{
		f=(wtk_feat_t*)wtk_robin_at(r,r->used-1);
		for(;i<half;++i)
		{
			v[i]=f->rv;
		}
	}
	wtk_ann_add_win_to_feature(a,a->right_w,v,a->right_factor);
}

wtk_feat_t* wtk_ann_pop_phn_feature(wtk_ann_t* a)
{
	wtk_queue_node_t *n;
	wtk_feat_t *f=0;

	n=wtk_queue_pop(&(a->phn_queue));
	if(!n){goto end;}
	f=data_offset(n,wtk_feat_t,queue_n);
end:
	return f;
}

void wtk_ann_flush_robin(wtk_ann_t *a)
{
	wtk_feat_t* f;

	f=(wtk_feat_t*)wtk_robin_pop(a->ann_robin);
	--f->used;
	wtk_fextra_push_feature(a->ann_parm,f);
}

void wtk_ann_raise_feature(wtk_ann_t *a,wtk_feat_t *f)
{
	if(a->output_queue)
	{
		++f->used;
		wtk_queue_push(a->output_queue,&(f->queue_n));
	}
}

void wtk_ann_flush_feature(wtk_ann_t *a,int s)
{
	wtk_robin_t *r=a->ann_robin;
	int half=a->half;
	wtk_ann_stream_t *stream;
	int cols;
	wtk_feat_t *f,*outf;
	float *feat;

	if(r->used<half){return;}
	//8*13
	wtk_ann_add_win(a,s);
	//dct: 	|6*8| * |8*13| => |6*13| => |1*78| => |1*78|*|78*400| => |1*400|*|400*138| =>|1*138|
	wtk_ann_stream_process(a->left_stream,a->dct_multi_matrx,a->dct_matrix,a->left_w);
	wtk_ann_stream_process(a->right_stream,a->dct_multi_matrx,a->dct_matrix,a->right_w);
	stream=a->merge_stream;
	cols=wtk_matrix_cols(a->left_stream->out_matrix);
	memcpy(stream->dct_matrix[1]+1,a->left_stream->out_matrix[1]+1,sizeof(float)*cols);
	memcpy(stream->dct_matrix[1]+1+cols,a->right_stream->out_matrix[1]+1,sizeof(float)*cols);
	//|1*276|*|276*400|*|400*138| = |1*138|
	wtk_ann_stream_do_merge(a->merge_stream);
	//|1*138|*|138*80|=|1*80|
	wtk_matrix_multi(a->pca_mat,a->merge_stream->out_matrix,a->cfg->res.pca_mat);
	f=wtk_ann_pop_phn_feature(a);
	cols=wtk_vector_size(f->v);
	memcpy(a->phn_ann_mat[1]+1,f->v+1,sizeof(float)*cols);
	memcpy(a->phn_ann_mat[1]+1+cols,a->pca_mat[1]+1,wtk_matrix_cols(a->pca_mat)*sizeof(float));
	outf=wtk_ann_pop_feature(a);
	if(a->cfg->use_hlda)
	{
		//|1*132|*|132*99|=|1*99|
		wtk_matrix_multi(a->out_mat,a->phn_ann_mat,a->cfg->res.hlda_mat);
		feat=a->out_mat[1]+1;
	}else
	{
		feat=a->phn_ann_mat[1]+1;
	}
	memcpy(outf->v+1,feat,sizeof(float)*a->feature_cols);
	wtk_ann_raise_feature(a,outf);
	--f->used;
	wtk_fextra_push_feature(a->phn_parm,f);
	if( (r->used==r->nslot) || s==1)
	{
		wtk_ann_flush_robin(a);
	}
}

void wtk_ann_flush_robin_feature(wtk_ann_t *a)
{
	wtk_robin_t *r=a->ann_robin;

	while(r->used>=a->half)
	{
		wtk_ann_flush_feature(a,1);
	}
	while(r->used>0)
	{
		wtk_ann_flush_robin(a);
	}
}

void wtk_ann_feed_feature(wtk_ann_t *a,wtk_feat_t *f)
{
	wtk_robin_t *r=a->ann_robin;

	wtk_robin_push(r,f);
	if(r->used<a->half){goto end;}
	wtk_ann_flush_feature(a,0);

end:
	return;
}

int wtk_ann_feed(wtk_ann_t *a,int s,short *data,int samples)
{
	wtk_queue_node_t *n;
	wtk_feat_t *f;
	int ret;

	ret=wtk_fextra_feed(a->ann_parm,data,samples,s);
	if(ret!=0){goto end;}
	ret=wtk_fextra_feed(a->phn_parm,data,samples,s);
	if(ret!=0){goto end;}
	while(1)
	{
		n=wtk_queue_pop(&(a->ann_queue));
		if(!n){break;}
		f=data_offset(n,wtk_feat_t,queue_n);
		wtk_ann_feed_feature(a,f);
	}
	if(s==1)
	{
		wtk_ann_flush_robin_feature(a);
	}
end:
	return ret;
}
