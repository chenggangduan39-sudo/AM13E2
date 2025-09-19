#include "wtk_ann_cfg.h"

//#define wtk_ann_res_load_matrix_s(s,b,row,col,t,n) wtk_ann_res_load_matrix(s,b,row,col,t,n,sizeof(n)-1)
#define wtk_ann_res_load_vector_s(r,s,b,n) wtk_ann_res_load_vector(r,s,b,n,sizeof(n)-1)

wtk_matrix_t* wtk_ann_res_load_matrix(wtk_source_t *s,wtk_strbuf_t *b,int rows,int cols,int t,char *n,int n_bytes)
{
	int ret,v;
	float *f=0;
	wtk_matrix_t *m;
	int i,j,k;

	m=wtk_matrix_new(rows,cols);//r->cfg->normal_rows,r->cfg->normal_cols);
	ret=wtk_source_read_string(s,b);
	if(ret!=0){goto end;}
	if(!wtk_str_equal(b->data,b->pos,n,n_bytes)){ret=-1;goto end;}
	ret=wtk_source_read_int(s,&v,1,0);
	if(ret!=0){goto end;}
	f=(float*)malloc(sizeof(float)*v);
	ret=wtk_source_read_float(s,f,v,0);
	if(ret!=0 || (v!=rows*cols))
	{
		goto end;
	}
	//wtk_debug("rows=%d,cols=%d\n",rows,cols);
	for(i=1;i<=rows;++i)
	{
		for(j=1;j<=cols;++j)
		{

			if(t)
			{
				k=(i-1)+rows*(j-1);
			}else
			{
				k=(i-1)*cols+(j-1);
			}
			m[i][j]=f[k];
			//wtk_debug("row=%d,col=%d,v=%f,k=%d\n",i,j,m[i][j],k);
			//getchar();
		}
	}
end:
	if(f){free(f);}
	if(ret!=0)
	{
		wtk_matrix_delete(m);
		m=0;
	}
	return m;
}

wtk_vector_t* wtk_ann_res_load_vector(wtk_ann_res_t *r,wtk_source_t *s,wtk_strbuf_t *b,char *n,int n_bytes)
{
	int ret,v;
	wtk_vector_t *x=0;

	ret=wtk_source_read_string(s,b);
	if(ret!=0){goto end;}
	if(!wtk_str_equal(b->data,b->pos,n,n_bytes)){ret=-1;goto end;}
	ret=wtk_source_read_int(s,&v,1,0);
	if(ret!=0){goto end;}
	x=wtk_vector_new(v);
	ret=wtk_source_read_vector(s,x,0);
end:
	if(ret!=0)
	{
		wtk_free(x);
		x=0;
	}
	return x;
}

int wtk_ann_res_load_pure_mat(wtk_ann_res_t *r,wtk_matrix_t **pm,wtk_source_t *s,wtk_strbuf_t *b)
{
	int rows,cols,ret;
	wtk_matrix_t *m;

	ret=wtk_source_read_int(s,&rows,1,0);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(s,&cols,1,0);
	if(ret!=0){goto end;}
	m=wtk_matrix_new(rows,cols);
	ret=wtk_source_read_matrix(s,m,0);
	*pm=m;
end:
	return ret;
}

int wtk_ann_res_load_tran_pure_mat(wtk_ann_res_t *r,wtk_matrix_t **pm,wtk_source_t *s,wtk_strbuf_t *b)
{
	int rows,cols,ret;
	wtk_matrix_t *m,*m1;

	ret=wtk_source_read_int(s,&rows,1,0);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(s,&cols,1,0);
	if(ret!=0){goto end;}
	m=wtk_matrix_new(rows,cols);
	ret=wtk_source_read_matrix(s,m,0);
	m1=wtk_matrix_new(cols,rows);
	wtk_matrix_transpose(m1,m);
	wtk_matrix_delete(m);
	*pm=m1;
end:
	return ret;
}

int wtk_ann_res_load_normal(wtk_ann_res_t *r,wtk_ann_normal_t* n,wtk_source_t *s,wtk_strbuf_t *b)
{
	wtk_matrix_t *m;
	int ret=-1;
	int rows,cols;

	rows=r->cfg->normal_rows;
	cols=r->cfg->normal_cols;
	m=wtk_ann_res_load_matrix_s(s,b,rows,cols,1,"vec");
	if(!m){goto end;}
	n->mean=m;
	m=wtk_ann_res_load_matrix_s(s,b,rows,cols,1,"vec");
	if(!m){goto end;}
	n->bias=m;
	ret=0;
end:
	return ret;
}

int wtk_ann_res_load_merge_normal(wtk_ann_res_t *r,wtk_ann_normal_t* n,wtk_source_t *s,wtk_strbuf_t *b)
{
	wtk_matrix_t *m;
	int ret=-1;
	int rows,cols;

	rows=1;
	cols=r->cfg->out_cols*2;
	m=wtk_ann_res_load_matrix_s(s,b,rows,cols,1,"vec");
	if(!m){goto end;}
	n->mean=m;
	m=wtk_ann_res_load_matrix_s(s,b,rows,cols,1,"vec");
	if(!m){goto end;}
	n->bias=m;
	ret=0;
end:
	return ret;
}


int wtk_ann_res_load_wb(wtk_ann_res_t* r,wtk_ann_wb_t *w,wtk_source_t *s,wtk_strbuf_t *b)
{
	wtk_matrix_t *m;
	int ret=-1;

	m=wtk_ann_res_load_matrix_s(s,b,r->cfg->hide_rows,r->cfg->hide_cols,1,"weigvec");
	if(!m){goto end;}
	w->hid_w=m;
	m=wtk_ann_res_load_matrix_s(s,b,r->cfg->out_rows,r->cfg->out_cols,1,"weigvec");
	if(!m){goto end;}
	w->out_w=m;
	m=wtk_ann_res_load_matrix_s(s,b,1,r->cfg->hide_cols,1,"biasvec");
	if(!m){goto end;}
	w->hid_b=m;
	m=wtk_ann_res_load_matrix_s(s,b,1,r->cfg->out_cols,1,"biasvec");
	if(!m){goto end;}
	w->out_b=m;
	ret=0;
end:
	return ret;
}

int wtk_ann_res_load_merge_wb(wtk_ann_res_t* r,wtk_ann_wb_t *w,wtk_source_t *s,wtk_strbuf_t *b)
{
	wtk_matrix_t *m;
	int ret=-1;

	m=wtk_ann_res_load_matrix_s(s,b,r->cfg->merge_rows,r->cfg->merge_cols,1,"weigvec");
	if(!m){goto end;}
	w->hid_w=m;
	m=wtk_ann_res_load_matrix_s(s,b,r->cfg->out_rows,r->cfg->out_cols,1,"weigvec");
	if(!m){goto end;}
	w->out_w=m;
	m=wtk_ann_res_load_matrix_s(s,b,1,r->cfg->merge_cols,1,"biasvec");
	if(!m){goto end;}
	w->hid_b=m;
	m=wtk_ann_res_load_matrix_s(s,b,1,r->cfg->out_cols,1,"biasvec");
	if(!m){goto end;}
	w->out_b=m;
	ret=0;
end:
	return ret;
}

typedef int (*wtk_ann_loader)(wtk_ann_res_t *r,void *n,wtk_source_t *s,wtk_strbuf_t *b);
typedef struct
{
	wtk_ann_res_t *res;
	wtk_strbuf_t *buf;
	void *data;
	wtk_ann_loader loader;
}wtk_ann_res_loader_t;

int wtk_ann_res_loader_load(wtk_ann_res_loader_t* l,wtk_source_t *s)
{
	return l->loader(l->res,l->data,s,l->buf);
}

int wtk_ann_res_init(wtk_ann_res_t *r,wtk_ann_cfg_t *cfg,wtk_source_loader_t *sl)
{
#define ANN_RN 8
	char *fn[ANN_RN];
	void *loader[ANN_RN];
	void *data[ANN_RN];
	wtk_strbuf_t *b;
	int ret,i;
	wtk_ann_res_loader_t rl;
	int n;

	memset(r,0,sizeof(*r));
	r->cfg=cfg;
	b=wtk_strbuf_new(64,1);
	rl.res=r;
	rl.buf=b;
	fn[0]=cfg->left_normal_fn;loader[0]=wtk_ann_res_load_normal;data[0]=&(r->left_normal);
	fn[1]=cfg->right_normal_fn;loader[1]=wtk_ann_res_load_normal;data[1]=&(r->right_normal);
	fn[2]=cfg->merge_normal_fn;loader[2]=wtk_ann_res_load_merge_normal;data[2]=&(r->merge_normal);
	fn[3]=cfg->left_wb_fn;loader[3]=wtk_ann_res_load_wb;data[3]=&(r->left_wb);
	fn[4]=cfg->right_wb_fn;loader[4]=wtk_ann_res_load_wb;data[4]=&(r->right_wb);
	fn[5]=cfg->merge_wb_fn;loader[5]=wtk_ann_res_load_merge_wb;data[5]=&(r->merge_wb);
	fn[6]=cfg->pca_fn;loader[6]=wtk_ann_res_load_pure_mat;data[6]=&(r->pca_mat);
	n=7;
	if(cfg->use_hlda)
	{
		fn[7]=cfg->hlda_fn;loader[7]=wtk_ann_res_load_tran_pure_mat;data[7]=&(r->hlda_mat);
		++n;
	}
	for(i=0;i<n;++i)
	{
		rl.data=data[i];
		rl.loader=(wtk_ann_loader)loader[i];
		ret=wtk_source_loader_load(sl,&rl,(wtk_source_load_handler_t)wtk_ann_res_loader_load,fn[i]);
		if(ret!=0)
		{
			wtk_debug("%d: %s\n",ret,fn[i]);
			goto end;
		}
	}
end:
	wtk_strbuf_delete(b);
	return ret;
}


int wtk_ann_normal_clean(wtk_ann_normal_t *n)
{
	if(n->mean)
	{
		wtk_matrix_delete(n->mean);
	}
	if(n->bias)
	{
		wtk_matrix_delete(n->bias);
	}
	return 0;
}

int wtk_ann_res_clean(wtk_ann_res_t *r)
{
	wtk_ann_normal_clean(&(r->left_normal));
	wtk_ann_normal_clean(&(r->right_normal));
	wtk_ann_normal_clean(&(r->merge_normal));
	wtk_ann_wb_clean(&(r->left_wb));
	wtk_ann_wb_clean(&(r->right_wb));
	wtk_ann_wb_clean(&(r->merge_wb));
	if(r->pca_mat)
	{
		wtk_matrix_delete(r->pca_mat);
	}
	if(r->hlda_mat)
	{
		wtk_matrix_delete(r->hlda_mat);
	}
	return 0;
}


int wtk_ann_res_delete(wtk_ann_res_t *res)
{
	wtk_ann_res_clean(res);
	free(res);
	return 0;
}

int wtk_ann_cfg_init(wtk_ann_cfg_t *cfg)
{
	memset(cfg,0,sizeof(*cfg));
	wtk_fextra_cfg_init(&(cfg->ann_parm));
	wtk_fextra_cfg_init(&(cfg->phn_parm));
	cfg->reduce_row=6;
	cfg->normal_rows=6;
	cfg->normal_cols=13;
	cfg->hide_rows=78;
	cfg->hide_cols=400;
	cfg->out_rows=400;
	cfg->out_cols=138;
	cfg->merge_rows=276;
	cfg->merge_cols=400;
	cfg->win=7;
	cfg->use_hlda=1;
	cfg->align=8;
	return 0;
}

int wtk_ann_cfg_update(wtk_ann_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	wtk_fextra_cfg_update2(&(cfg->ann_parm),sl);
	wtk_fextra_cfg_update2(&(cfg->phn_parm),sl);
	ret=wtk_ann_res_init(&(cfg->res),cfg,sl);
	return ret;
}

int wtk_ann_cfg_update2(wtk_ann_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

	return wtk_ann_cfg_update(cfg,&sl);
}



int wtk_ann_cfg_clean(wtk_ann_cfg_t *cfg)
{
	wtk_ann_res_clean(&(cfg->res));
	return 0;
}

void wtk_ann_cfg_print(wtk_ann_cfg_t *cfg)
{
	printf("---------- ANN ------------\n");
	print_cfg_s(cfg,hlda_fn);
	print_cfg_s(cfg,pca_fn);
	print_cfg_s(cfg,left_normal_fn);
	print_cfg_s(cfg,right_normal_fn);
	print_cfg_s(cfg,merge_normal_fn);
	print_cfg_s(cfg,left_wb_fn);
	print_cfg_s(cfg,right_wb_fn);
	print_cfg_s(cfg,merge_wb_fn);
	print_cfg_i(cfg,reduce_row);
	print_cfg_i(cfg,normal_rows);
	print_cfg_i(cfg,normal_cols);
	print_cfg_i(cfg,hide_rows);
	print_cfg_i(cfg,hide_cols);
	print_cfg_i(cfg,out_rows);
	print_cfg_i(cfg,out_cols);
	print_cfg_i(cfg,merge_cols);
	print_cfg_i(cfg,merge_rows);
	print_cfg_i(cfg,win);
	wtk_fextra_cfg_print(&(cfg->ann_parm));
	wtk_fextra_cfg_print(&(cfg->phn_parm));
}

int wtk_ann_cfg_update_local(wtk_ann_cfg_t* cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc=main;
	wtk_string_t *v;
	int ret=0;

	//wtk_ann_cfg_set_example(cfg);
	wtk_local_cfg_update_cfg_str(lc,cfg,hlda_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,pca_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,left_normal_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,right_normal_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,merge_normal_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,left_wb_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,right_wb_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,merge_wb_fn,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,reduce_row,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,normal_rows,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,normal_cols,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hide_rows,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hide_cols,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,out_rows,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,out_cols,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,merge_rows,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,merge_cols,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,align,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_hlda,v);
	lc=wtk_local_cfg_find_lc_s(main,"ann_parm");
	if(lc)
	{
		ret=wtk_fextra_cfg_update_local(&(cfg->ann_parm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"phn_parm");
	if(lc)
	{
		ret=wtk_fextra_cfg_update_local(&(cfg->phn_parm),lc);
		if(ret!=0){goto end;}
	}

end:
	//wtk_ann_cfg_print(cfg);
	return 0;
}

void wtk_ann_cfg_set_example(wtk_ann_cfg_t *cfg)
{
	wtk_fextra_cfg_t *p;

//  ===   init ann resource ====
	cfg->hlda_fn="./res/hlda";
	cfg->pca_fn="./res/a_mat";
	cfg->left_normal_fn="./res/leftnormFn";
	cfg->right_normal_fn="./res/rightnormFn";
	cfg->merge_normal_fn="./res/mergernormFn";
	cfg->left_wb_fn="./res/annLeftWBFn";
	cfg->right_wb_fn="./res/annRightWBFn";
	cfg->merge_wb_fn="./res/annMergerWBFn";
	cfg->reduce_row=6;
	cfg->normal_rows=6;
	cfg->normal_cols=13;
	cfg->hide_rows=78;
	cfg->hide_cols=400;
	cfg->out_rows=400;
	cfg->out_cols=138;
	cfg->merge_cols=400;
	cfg->merge_rows=276;
	cfg->win=7;

// ===   init ann parm  ===
	p=&(cfg->ann_parm);
	p->window_step=200000.0;
	p->window_size=300000.0;
	p->PREMCOEF=0;
	p->NUMCHNAS=23;
	p->USEPOWER=1;
	p->USEHAMMING=1;
	p->NUMCEPS=12;
	wtk_string_set_s(&(p->target_kind),"MFCC_0");

// == init phn parm ==
	p=&(cfg->phn_parm);
	p->window_step=200000.0;
	p->window_size=300000.0;
	p->PREMCOEF=0.97;
	p->NUMCHNAS=24;
	p->USEPOWER=1;
	p->USEHAMMING=1;
	p->NUMCEPS=12;
	p->CEPSCALE=10.0;
	p->COMPRESSFACT=0.3333333f;
	p->ZMEANSOURCE=1;
	//p->fbank_num_chans=23;
	wtk_string_set_s(&(p->target_kind),"PLP_0_D_A_T");
}

