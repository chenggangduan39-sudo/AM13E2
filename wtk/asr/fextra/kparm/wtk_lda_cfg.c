#include "wtk_lda_cfg.h" 
#include "wtk/core/wtk_larray.h"

int wtk_lda_cfg_init(wtk_lda_cfg_t *cfg)
{
	cfg->lda=NULL;
	cfg->win=4;
	cfg->lda_fn=NULL;
	return 0;
}

int wtk_lda_cfg_clean(wtk_lda_cfg_t *cfg)
{
	if(cfg->lda)
	{
		wtk_matf_delete(cfg->lda);
	}
	return 0;
}

int wtk_lda_cfg_update_local(wtk_lda_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,lda_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,win,v);
	return 0;
}

int wtk_lda_cfg_update(wtk_lda_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_lda_cfg_update2(cfg,&sl);
}

int wtk_lda_cfg_load(wtk_lda_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret;
	int nl;
	float f;
	wtk_larray_t *a;
	int col,row;

	a=wtk_larray_new(120,sizeof(float));
	buf=wtk_strbuf_new(1024,1);
	ret=wtk_source_seek_to_s(src,"[");
	if(ret!=0){goto end;}
	ret=wtk_source_skip_sp(src,&nl);
	if(ret!=0){goto end;}
	while(1)
	{
		ret=wtk_source_skip_sp(src,&nl);
		if(nl){break;}
		ret=wtk_source_read_float(src,&f,1,0);
		if(ret!=0){goto end;}
		//wtk_debug("f[%d]=%f\n",i++,f);
		wtk_larray_push2(a,&f);
	}
	col=a->nslot;
	wtk_debug("col=%d\n",col);
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){goto end;}
		if(buf->data[0]==']')
		{
			ret=0;
			break;
		}
		f=wtk_str_atof(buf->data,buf->pos);
		wtk_larray_push2(a,&f);
	}
	row=a->nslot/col;
	//wtk_debug("col=%d row=%d\n",col,row);
	cfg->lda=wtk_matf_new(row,col);
	memcpy(cfg->lda->p,a->slot,row*col*sizeof(float));
	ret=0;
end:
	wtk_larray_delete(a);
	wtk_strbuf_delete(buf);
	//exit(0);
	return 0;
}

int wtk_lda_cfg_update2(wtk_lda_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	//wtk_debug("%s\n",cfg->lda_fn)
	ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_lda_cfg_load,cfg->lda_fn);
	if(ret!=0){goto end;}
	ret=0;
end:
	//exit(0);
	return ret;
}
