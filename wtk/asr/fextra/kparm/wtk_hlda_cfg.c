#include "wtk_hlda_cfg.h"

int wtk_hlda_cfg_bytes(wtk_hlda_cfg_t *cfg)
{
	int bytes;

	bytes=sizeof(wtk_hlda_cfg_t);
	if(cfg->fix_hlda)
	{
		bytes+=wtk_mati_bytes(cfg->fix_hlda);
	}
	if(cfg->hlda)
	{
		bytes+=wtk_matf_bytes(cfg->hlda);
	}
	return bytes;
}

int wtk_hlda_cfg_init(wtk_hlda_cfg_t *cfg)
{
	cfg->fix_hlda=NULL;
	cfg->hlda=NULL;
	cfg->fn=NULL;
	return 0;
}

int wtk_hlda_cfg_clean(wtk_hlda_cfg_t *cfg)
{
	if(cfg->fix_hlda)
	{
		wtk_mati_delete(cfg->fix_hlda);
	}
	if(cfg->hlda)
	{
		wtk_matf_delete(cfg->hlda);
	}
	return 0;
}

int wtk_hlda_cfg_update_local(wtk_hlda_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,fn,v);
	return 0;
}

int wtk_hlda_cfg_update(wtk_hlda_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_hlda_cfg_update2(cfg,&sl);
}

int wtk_hlda_cfg_load_hlda(wtk_hlda_cfg_t *cfg,wtk_source_t *src)
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
	cfg->hlda=wtk_matf_new(row,col);
	memcpy(cfg->hlda->p,a->slot,row*col*sizeof(float));
	ret=0;
end:
	wtk_larray_delete(a);
	wtk_strbuf_delete(buf);
	//exit(0);
	return 0;
}

int wtk_hlda_cfg_update2(wtk_hlda_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_hlda_cfg_load_hlda,cfg->fn);
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}


void wtk_hlda_cfg_update_fix(wtk_hlda_cfg_t *cfg)
{
	int n=cfg->hlda->row*cfg->hlda->col;
	int i;

	cfg->fix_hlda=wtk_mati_new(cfg->hlda->row,cfg->hlda->col);
	for(i=0;i<n;++i)
	{
		cfg->fix_hlda->p[i]=FLOAT2FIX(cfg->hlda->p[i]);
	}
}

void wtk_hlda_cfg_write_fix(wtk_hlda_cfg_t *cfg,FILE *f)
{
#ifndef USE_RTOS_OF_5215
	int v[2];

	v[0]=cfg->fix_hlda->row;
	v[1]=cfg->fix_hlda->col;
	fwrite(v,sizeof(int)*2,1,f);
	fwrite(cfg->fix_hlda->p,sizeof(int)*cfg->fix_hlda->row*cfg->fix_hlda->col,1,f);
#endif
}

int wtk_hlda_cfg_read_fix(wtk_hlda_cfg_t *cfg,wtk_source_t *src)
{
	int v[2];
	int ret;

	ret=wtk_source_fill(src,(char*)v,sizeof(int)*2);
	if(ret!=0){goto end;}
	cfg->fix_hlda=wtk_mati_new(v[0],v[1]);
	//wtk_debug("new hlda=%p\n",cfg->fix_hlda);
	ret=wtk_source_fill(src,(char*)cfg->fix_hlda->p,sizeof(int)*cfg->fix_hlda->row*cfg->fix_hlda->col);
	if(ret!=0){goto end;}
end:
	return ret;
}


