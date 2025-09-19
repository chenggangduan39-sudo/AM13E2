#include "qtk_hclg.h"

int qtk_hclg_new(qtk_hclg_cfg_t* cfg)
{
	qtk_hclg_t* hclg;

	hclg=(qtk_hclg_t*)wtk_malloc(sizeof(qtk_hclg_t));
	hclg->cfg=cfg;
	hclg->hclg_fp=fopen(cfg->hclg_fst_fn,"rb");

	return 0;
}

int qtk_hclg_reset(qtk_hclg_t* hclg)
{
	return 0;
}

void qtk_hclg_delete(qtk_hclg_t* hclg)
{
	if(hclg->hclg_fp)
	{
		fclose(hclg->hclg_fp);
	}
	wtk_free(hclg);
}

int qtk_hclg_parse_header(qtk_hclg_t* hclg)
{
	FILE* f;
	unsigned int len;
	int ret;
	qtk_hclg_header_t* header;

	if(!hclg->hclg_fp)
	{
		goto end;
	}
	f=hclg->hclg_fp;
	header=hclg->header;

    ret=fread(&len,sizeof(uint32_t),1,f);
    if(ret!=1)
    {
    	goto end;
    }
    header->fsttype=(char*)wtk_malloc(sizeof(char)*len);
    ret=fread(header->fsttype,sizeof(char)*len,1,f);
    if(ret!=1)
    {
    	goto end;
    }

    ret=fread(&len,sizeof(uint32_t),1,f);
    if(ret!=1)
    {
    	goto end;
    }
    header->arctype=(char*)wtk_malloc(sizeof(char)*len);
    ret=fread(header->arctype,sizeof(char)*len,1,f);
    if(ret!=1)
    {
    	goto end;
    }

	ret=fread(&header->version,sizeof(uint32_t),1,f);
	if(ret!=1)
	{
		goto end;
	}
	ret=fread(&header->flags,sizeof(uint32_t),1,f);
	if(ret!=1)
	{
		goto end;
	}
	ret=fread(&header->properties,sizeof(uint64_t),1,f);
	if(ret!=1)
	{
		goto end;
	}
    ret=fread(&header->start,sizeof(uint64_t),1,f);
	if(ret!=1)
	{
		goto end;
	}
	ret=fread(&header->numstates,sizeof(uint64_t),1,f);
    if(ret!=1)
	{
		goto end;
	}
	ret=fread(&header->numarcs,sizeof(uint64_t),1,f);
	if(ret!=1)
	{
		goto end;
	}


	return 0;
	end:
		return -1;
}
