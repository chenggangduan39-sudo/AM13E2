#include "wtk_annvad_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/fextra/ann/wtk_ann_cfg.h"
void wtk_annvad_res_delete(wtk_annvad_res_t *r);
wtk_annvad_res_t* wtk_annvad_res_new(wtk_annvad_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_vector_t* wtk_ann_res_read_vector(wtk_source_t * s)
{
	wtk_strbuf_t *buf;
	int ret,len;
	wtk_vector_t *v=0;

	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_read_string(s,buf);
	if(ret!=0){goto end;}
	//print_data(buf->data,buf->pos);
	ret=wtk_source_read_int(s,&len,1,0);
	if(ret!=0){goto end;}
	v=wtk_vector_new(len);
	ret=wtk_source_read_vector(s,v,0);
	if(ret!=0){goto end;}
	//wtk_vector_print(v);
end:
	if((ret!=0) && v)
	{
		wtk_vector_delete(v);
		v=0;
	}
	wtk_strbuf_delete(buf);
	return v;
}

int wtk_annvad_res_load_norm(wtk_annvad_res_t *res,wtk_source_t *s)
{
	wtk_vector_t *v;
	int ret=-1;

	v=wtk_ann_res_read_vector(s);
	//wtk_vector_print(v);
	if(!v){goto end;}
	res->parm_mean=v;
	v=wtk_ann_res_read_vector(s);
	if(!v){goto end;}
	//wtk_vector_print(v);
	res->parm_var=v;
	ret=0;
end:
	return ret;
}

#define wtk_source_seek_int_value_s(s,v,name) wtk_source_seek_int_value(s,v,name,sizeof(name)-1)

int wtk_source_seek_int_value(wtk_source_t *s,int *v,char *name,int name_bytes)
{
	int ret;

	ret=wtk_source_seek_to(s,name,name_bytes);
	if(ret!=0){goto end;}
	ret=wtk_source_read_int(s,v,1,0);
	if(ret!=0){goto end;}
end:
	return ret;
}

int wtk_annvad_res_read_wb_cfg(wtk_ann_wb_cfg_t *cfg,wtk_source_t *s)
{
	int ret;
	int hide,out;
	int hide_cols,out_cols;

	ret=wtk_source_seek_int_value_s(s,&hide,"weigvec");
	if(ret!=0){goto end;}
	ret=wtk_source_seek_int_value_s(s,&out,"weigvec");
	if(ret!=0){goto end;}
	ret=wtk_source_seek_int_value_s(s,&hide_cols,"biasvec");
	if(ret!=0){goto end;}
	ret=wtk_source_seek_int_value_s(s,&out_cols,"biasvec");
	if(ret!=0){goto end;}
	cfg->hide_cols=hide_cols;
	cfg->hide_rows=hide/hide_cols;
	//wtk_debug("hide=%d/%d\n",hide,hide_cols);
	cfg->out_cols=out_cols;
	cfg->out_rows=out/out_cols;
end:
	return ret;
}

int wtk_annvad_res_load_wb(wtk_annvad_res_t* r,wtk_source_t *s)
{
	wtk_ann_wb_cfg_t *cfg=&(r->cfg->wbcfg);
	wtk_matrix_t *m;
	wtk_strbuf_t *b;
	wtk_ann_wb_t *w=&(r->wb);
	int t=1;
	int ret;

	//wtk_ann_wb_cfg_print(cfg);
	b=wtk_strbuf_new(256,1);
	m=wtk_ann_res_load_matrix_s(s,b,cfg->hide_rows,cfg->hide_cols,t,"weigvec");
	if(!m){goto end;}
	w->hid_w=m;
	m=wtk_ann_res_load_matrix_s(s,b,cfg->out_rows,cfg->out_cols,t,"weigvec");
	if(!m){goto end;}
	w->out_w=m;
	m=wtk_ann_res_load_matrix_s(s,b,1,cfg->hide_cols,t,"biasvec");
	if(!m){goto end;}
	w->hid_b=m;
	m=wtk_ann_res_load_matrix_s(s,b,1,cfg->out_cols,t,"biasvec");
	if(!m){goto end;}
	w->out_b=m;
	ret=0;
end:
	wtk_strbuf_delete(b);
	return ret;
}

wtk_annvad_res_t* wtk_annvad_res_new(wtk_annvad_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_annvad_res_t *r;
	int ret;

	r=(wtk_annvad_res_t*)wtk_calloc(1,sizeof(*r));
	r->cfg=cfg;
	wtk_ann_wb_init(&(r->wb));
	ret=wtk_source_loader_load(sl,r,(wtk_source_load_handler_t)wtk_annvad_res_load_norm,cfg->norm_fn);
	if(ret!=0){goto end;}
	ret=wtk_source_loader_load(sl,&(cfg->wbcfg),(wtk_source_load_handler_t)wtk_annvad_res_read_wb_cfg,cfg->weight_fn);
	if(ret!=0){goto end;}
	ret=wtk_source_loader_load(sl,r,(wtk_source_load_handler_t)wtk_annvad_res_load_wb,cfg->weight_fn);
	if(ret!=0){goto end;}
end:
	if(ret!=0)
	{
		wtk_annvad_res_delete(r);
	}
	return r;
}

void wtk_annvad_res_delete(wtk_annvad_res_t *r)
{
	if(r->parm_mean)
	{
		wtk_vector_delete(r->parm_mean);
	}
	if(r->parm_var)
	{
		wtk_vector_delete(r->parm_var);
	}
	wtk_ann_wb_clean(&(r->wb));
	wtk_free(r);
}

int wtk_annvad_cfg_init(wtk_annvad_cfg_t *cfg)
{
	int ret;

	cfg->cache=25;
	//cfg->win=25;
	cfg->left_win=25;
	cfg->right_win=25;
	cfg->norm_fn=0;
	cfg->weight_fn=0;
	cfg->res=0;
	cfg->siltrap=8;
	cfg->speechtrap=8;
	ret=wtk_fextra_cfg_init(&(cfg->parm));
	return ret;
}

int wtk_annvad_cfg_clean(wtk_annvad_cfg_t *cfg)
{
	if(cfg->res)
	{
		wtk_annvad_res_delete(cfg->res);
	}
	wtk_fextra_cfg_clean(&(cfg->parm));
	return 0;
}

int wtk_annvad_cfg_update_local(wtk_annvad_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc=main;
	wtk_string_t *v;
	int ret=-1;

	//wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,left_win,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_win,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,norm_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,weight_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cache,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,siltrap,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,speechtrap,v);
	lc=wtk_local_cfg_find_lc_s(main,"parm");
	if(lc)
	{
		ret=wtk_fextra_cfg_update_local(&(cfg->parm),lc);
		if(ret!=0){goto end;}
	}
end:
	return ret;
}

int wtk_annvad_cfg_update(wtk_annvad_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_fextra_cfg_update2(&(cfg->parm),sl);
	if(ret!=0){goto end;}
	cfg->res=wtk_annvad_res_new(cfg,sl);
	ret=cfg->res?0:-1;
end:
	return ret;
}

int wtk_annvad_cfg_update2(wtk_annvad_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_annvad_cfg_update(cfg,&sl);
}
