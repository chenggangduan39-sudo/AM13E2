/*
 * wtk_trietree_cfg.c
 *
 *  Created on: Jan 27, 2022
 *      Author: dm
 */
#include "wtk_trietree_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"

int wtk_trietree_loadall(wtk_trietree_cfg_t *cfg, char* trie_fn, char* trie_inset_fn);
int wtk_trietree_update_file(wtk_trietree_cfg_t *cfg, char* trie_fn, char *trie_inset_fn);
int wtk_trietree_build(wtk_trietree_cfg_t *cfg,unsigned char *g_trie_res, unsigned char *g_trie_inset_res);

int wtk_trietree_cfg_init(wtk_trietree_cfg_t *cfg)
{
	cfg->trie_fn=NULL;
	cfg->trie_inset_fn=NULL;
	cfg->g_trie_inset_res=NULL;
	cfg->g_trie_res=NULL;
    cfg->root_pos=NULL;
    cfg->root_inset_pos=NULL;
    cfg->root=NULL;
    cfg->root_inset=NULL;
	cfg->load_all=1;
	cfg->nwrds=0;
	cfg->rbin=NULL;

	return 0;
}

int wtk_trietree_cfg_update_local(wtk_trietree_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    int ret=0;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_str(lc, cfg, trie_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, trie_inset_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, load_all, v);

    return ret;
}

int wtk_trietree_cfg_clean(wtk_trietree_cfg_t *cfg)
{
    if (cfg->g_trie_res)
    	wtk_free(cfg->g_trie_res);
    if (cfg->g_trie_inset_res)
    	wtk_free(cfg->g_trie_inset_res);
    if (cfg->root_pos)
    	wtk_free(cfg->root_pos);
    if (cfg->root_inset_pos)
        	wtk_free(cfg->root_inset_pos);

    int i;

     if (cfg->root)
     {
         for(i=0;i<cfg->nwrds;++i)
         {
             if(cfg->root[i])
                 wtk_trietree_delete(cfg->root[i]);
         }
     	wtk_free(cfg->root);
     }

     if (cfg->root_inset)
     {
         for(i=0;i<cfg->nwrds;++i)
         {
             if(cfg->root_inset[i])
                 wtk_trietree_delete(cfg->root_inset[i]);
         }
         wtk_free(cfg->root_inset);
     }

	return 0;
}
int wtk_trietree_cfg_update(wtk_trietree_cfg_t *cfg)
{
    if (cfg->load_all)
    {
    	wtk_trietree_loadall(cfg, cfg->trie_fn, cfg->trie_inset_fn);
    }else
    {
    	wtk_trietree_update_file(cfg, cfg->trie_fn, cfg->trie_inset_fn);
    }

    return 0;
}


int wtk_trietree_update_loadtrie(wtk_trietree_cfg_t *cfg, wtk_rbin2_t *rbin, char *fn)
{
	FILE *f;
    int i, j;
    uint16_t len, nwrds;
    uint16_t cur_idx,idx_last;
    uint8_t slen[4], deep;
    uint16_t nunit;
    int ret=0;

    if (rbin)
    	f = wtk_rbin2_get_file(rbin, fn);
    else
    {
		f=fopen(fn,"rb");
		if(!f){
			wtk_debug("open failed %s", fn);
			ret=-1;
			goto end;
		}
    }
    fread(&nwrds, sizeof(uint16_t), 1, f);
    cfg->nwrds=nwrds;
    cfg->root_pos = (wtk_trieitem_t*) wtk_calloc(nwrds, sizeof(wtk_trieitem_t));
    fread(&len, sizeof(uint16_t), 1, f);
    idx_last=nwrds+1;
    for(i=0;i<len;++i)
    {
    	fread(&cur_idx, sizeof(uint16_t), 1, f);
    	if (cur_idx != idx_last)
    	{
    		//initial cur_idx
    		cfg->root_pos[cur_idx].start=ftell(f)-sizeof(uint16_t)/sizeof(char);
    		cfg->root_pos[cur_idx].len=0;
    		cfg->root_pos[cur_idx].num=0;
    		//build for idx_last
    		if (idx_last!=cfg->nwrds+1)
    		{
    			cfg->root_pos[idx_last].len = ftell(f)-cfg->root_pos[idx_last].start-sizeof(uint16_t)/sizeof(char);
    		}
    		idx_last = cur_idx;
    	}
    	fread(&deep, sizeof(uint8_t), 1, f);
    	cfg->root_pos[cur_idx].num++;
        for(j=0;j<deep;++j)
        {
        	fread(&(slen[j]), sizeof(uint8_t), 1, f);
        	fseek(f, slen[j] * sizeof(uint8_t), SEEK_CUR);
        }
        fread(&nunit, sizeof(uint16_t), 1, f);
        fseek(f, nunit* sizeof(uint16_t), SEEK_CUR);
    }
    cfg->root_pos[cur_idx].len = ftell(f)-cfg->root_pos[cur_idx].start;
end:
	if (f)
		fclose(f);
    return ret;
}

int wtk_trietree_update_loadintrie(wtk_trietree_cfg_t *cfg, wtk_rbin2_t *rbin, char *fn)
{
	FILE *f;
    int i, j;
    uint16_t len, nwrds;
    uint16_t cur_idx,idx_last;
    uint8_t slen[4], deep;
    uint16_t nunit;
    int ret=0;

    if (rbin)
    	f = wtk_rbin2_get_file(rbin, fn);
    else
    {
		f=fopen(fn,"rb");
		if(!f){
			wtk_debug("open failed %s", fn);
			ret=-1;
			goto end;
		}
    }
    fread(&nwrds, sizeof(uint16_t), 1, f);
    if (nwrds!=cfg->nwrds)
    {
    	wtk_debug("warnning: trie_nosil nwrds[%d] diff trie nwrds[%d]\n", nwrds, cfg->nwrds);
    }
    cfg->root_inset_pos = (wtk_trieitem_t*) wtk_calloc(nwrds, sizeof(wtk_trieitem_t));
    fread(&len, sizeof(uint16_t), 1, f);
    idx_last=nwrds+1;
    for(i=0;i<len;++i)
    {
    	fread(&cur_idx, sizeof(uint16_t), 1, f);
    	if (cur_idx != idx_last)
    	{
    		//initial cur_idx
    		cfg->root_inset_pos[cur_idx].start=ftell(f)-sizeof(uint16_t);
    		cfg->root_inset_pos[cur_idx].len=0;
    		cfg->root_inset_pos[cur_idx].num=0;
    		//build for idx_last
    		if (idx_last!=cfg->nwrds+1)
    		{
    			cfg->root_inset_pos[idx_last].len = ftell(f)-cfg->root_inset_pos[idx_last].start-sizeof(uint16_t);
    		}
    		idx_last = cur_idx;
    	}
    	fread(&deep, sizeof(uint8_t), 1, f);
    	cfg->root_inset_pos[cur_idx].num++;
        for(j=0;j<deep;++j)
        {
        	fread(&(slen[j]), sizeof(uint8_t), 1, f);
        	fseek(f, slen[j] * sizeof(uint8_t), SEEK_CUR);
        }
        fread(&nunit, sizeof(uint16_t), 1, f);
        fseek(f, nunit*sizeof(uint16_t), SEEK_CUR);
    }
    cfg->root_inset_pos[cur_idx].len = ftell(f)-cfg->root_inset_pos[cur_idx].start;
end:
	if (f)
		fclose(f);
    return ret;
}

int wtk_trietree_update_file(wtk_trietree_cfg_t *cfg, char* trie_fn, char *trie_inset_fn)
{
    int ret;
    int i, j;
    uint16_t len, nwrds;
    uint16_t cur_idx,idx_last;
    uint8_t slen[4], deep;
    uint16_t nunit;
    FILE*fp;

    if(trie_fn)
    {
        fp = fopen(trie_fn,"rb");
        if(!fp)
        {
            printf("Open %s failed\n",trie_fn);
            ret=-1;goto end;
        }
        fread(&nwrds, sizeof(uint16_t), 1, fp);
        cfg->nwrds=nwrds;
        cfg->root_pos = (wtk_trieitem_t*) wtk_calloc(nwrds, sizeof(wtk_trieitem_t));
        fread(&len, sizeof(uint16_t), 1, fp);
        idx_last=nwrds+1;
        for(i=0;i<len;++i)
        {
        	fread(&cur_idx, sizeof(uint16_t), 1, fp);
        	if (cur_idx != idx_last)
        	{
        		//initial cur_idx
        		cfg->root_pos[cur_idx].start=ftell(fp)-sizeof(uint16_t)/sizeof(char);
        		cfg->root_pos[cur_idx].len=0;
        		cfg->root_pos[cur_idx].num=0;
        		//build for idx_last
        		if (idx_last!=cfg->nwrds+1)
        		{
        			cfg->root_pos[idx_last].len = ftell(fp)-cfg->root_pos[idx_last].start-sizeof(uint16_t)/sizeof(char);
        		}
        		idx_last = cur_idx;
        	}
        	fread(&deep, sizeof(uint8_t), 1, fp);
        	cfg->root_pos[cur_idx].num++;
            for(j=0;j<deep;++j)
            {
            	fread(&(slen[j]), sizeof(uint8_t), 1, fp);
            	fseek(fp, slen[j] * sizeof(uint8_t), SEEK_CUR);
            }
            fread(&nunit, sizeof(uint16_t), 1, fp);
            fseek(fp, nunit* sizeof(uint16_t), SEEK_CUR);
        }
        cfg->root_pos[cur_idx].len = ftell(fp)-cfg->root_pos[cur_idx].start;
    	fclose(fp);
    }

    if(cfg->trie_inset_fn)
    {
        fp = fopen(cfg->trie_inset_fn,"rb");
        if(!fp)
        {
            printf("Open %s failed\n",cfg->trie_inset_fn);
            ret=-1;goto end;
        }
        fread(&nwrds, sizeof(uint16_t), 1, fp);
        if (nwrds!=cfg->nwrds)
        {
        	wtk_debug("warnning: trie_nosil nwrds[%d] diff trie nwrds[%d]\n", nwrds, cfg->nwrds);
        }
        cfg->root_inset_pos = (wtk_trieitem_t*) wtk_calloc(nwrds, sizeof(wtk_trieitem_t));
        fread(&len, sizeof(uint16_t), 1, fp);
        idx_last=nwrds+1;
        for(i=0;i<len;++i)
        {
        	fread(&cur_idx, sizeof(uint16_t), 1, fp);
        	if (cur_idx != idx_last)
        	{
        		//initial cur_idx
        		cfg->root_inset_pos[cur_idx].start=ftell(fp)-sizeof(uint16_t);
        		cfg->root_inset_pos[cur_idx].len=0;
        		cfg->root_inset_pos[cur_idx].num=0;
        		//build for idx_last
        		if (idx_last!=cfg->nwrds+1)
        		{
        			cfg->root_inset_pos[idx_last].len = ftell(fp)-cfg->root_inset_pos[idx_last].start-sizeof(uint16_t);
        		}
        		idx_last = cur_idx;
        	}
        	fread(&deep, sizeof(uint8_t), 1, fp);
        	cfg->root_inset_pos[cur_idx].num++;
            for(j=0;j<deep;++j)
            {
            	fread(&(slen[j]), sizeof(uint8_t), 1, fp);
            	fseek(fp, slen[j] * sizeof(uint8_t), SEEK_CUR);
            }
            fread(&nunit, sizeof(uint16_t), 1, fp);
            fseek(fp, nunit*sizeof(uint16_t), SEEK_CUR);
        }
        cfg->root_inset_pos[cur_idx].len = ftell(fp)-cfg->root_inset_pos[cur_idx].start;
    	fclose(fp);
    }
    ret = 0;
end:
    return ret;
}

int wtk_trietree_loadall(wtk_trietree_cfg_t *cfg, char* trie_fn, char* trie_inset_fn)
{
	int ret=0, datasize;
	FILE *f;

	   if(trie_fn)
	    {
	        f = fopen(trie_fn,"rb");
	        if(!f)
	        {
	            printf("Open %s failed\n",cfg->trie_fn);
	            ret=-1;goto end;
	        }
	        fseek(f, 0, SEEK_END);
	        datasize = ftell(f);
	    	fseek(f, 0, SEEK_SET);
	    	cfg->g_trie_res = wtk_calloc(datasize, sizeof(unsigned char));
	    	fread(cfg->g_trie_res, 1, datasize, f);
	    	fclose(f);
	    }

	    if(trie_inset_fn)
	    {
	        f = fopen(trie_inset_fn,"rb");
	        if(!f)
	        {
	            printf("Open %s failed\n", trie_inset_fn);
	            ret=-1;goto end;
	        }
	        fseek(f, 0, SEEK_END);
	        datasize = ftell(f);
	    	fseek(f, 0, SEEK_SET);
	    	cfg->g_trie_inset_res = wtk_calloc(datasize, sizeof(unsigned char));
	    	fread(cfg->g_trie_inset_res, 1, datasize, f);
	    	fclose(f);
	    }

	    ret = wtk_trietree_build(cfg,cfg->g_trie_res, cfg->g_trie_inset_res);
end:
	    return ret;
}

int wtk_trietree_build(wtk_trietree_cfg_t *cfg,unsigned char *g_trie_res, unsigned char *g_trie_inset_res)
{
    int ret;
    int i, j, nwrds;
    uint16_t len;
    uint16_t idx,deep;
    char *s;
    int slen[4];
    uint16_t nunit;
    uint16_t *units;

    nwrds = *((uint16_t*)g_trie_res);
    g_trie_res+=sizeof(uint16_t);
    cfg->nwrds=nwrds;
    cfg->root = (wtk_trieroot_t**)wtk_malloc(sizeof(wtk_trieroot_t*)*cfg->nwrds);
    cfg->root_pos = (wtk_trieitem_t*) wtk_calloc(cfg->nwrds , sizeof(wtk_trieitem_t));
    for(i=0;i<cfg->nwrds;++i)
    {
        cfg->root[i] = wtk_trietree_new();
    }
    len = *((uint16_t*)g_trie_res);
    g_trie_res+=sizeof(uint16_t);
    for(i=0;i<len;++i)
    {
        idx = *((uint16_t*)g_trie_res);
        g_trie_res+=sizeof(uint16_t);
        deep = *((uint8_t*)g_trie_res);
        g_trie_res+=sizeof(uint8_t);
        s = (char*)(g_trie_res);
        for(j=0;j<deep;++j)
        {
            slen[j] = *((uint8_t*)g_trie_res);
            g_trie_res+=sizeof(uint8_t);
            g_trie_res+=slen[j];
        }
        nunit = *((uint16_t*)g_trie_res);
        g_trie_res+=sizeof(uint16_t);
        units = (uint16_t*)g_trie_res;
        g_trie_res+=nunit*sizeof(uint16_t);

        wtk_trietree_root_insert(cfg->root[idx],s,slen,units,nunit,deep, sizeof(uint8_t)/sizeof(char));
    }
    for(i=0;i<cfg->nwrds;++i)
    {
        wtk_trietree_update(cfg->root[i]);
    }

    nwrds = *((uint16_t*)g_trie_inset_res);
    g_trie_inset_res+=sizeof(uint16_t);
    if (nwrds!=cfg->nwrds)
    {
    	wtk_debug("warnning: trie_nosil nwrds[%d] diff trie nwrds[%d]\n", nwrds, cfg->nwrds);
    }
    cfg->nwrds=nwrds;
    cfg->root_inset = (wtk_trieroot_t**)wtk_malloc(sizeof(wtk_trieroot_t*)*cfg->nwrds);
    cfg->root_inset_pos = (wtk_trieitem_t*) wtk_calloc(cfg->nwrds, sizeof(wtk_trieitem_t));
    for(i=0;i<cfg->nwrds;++i)
    {
        cfg->root_inset[i] = wtk_trietree_new();
    }

    len = *((uint16_t*)g_trie_inset_res);
    g_trie_inset_res+=sizeof(uint16_t);
    for(i=0;i<len;++i)
    {
        idx = *((uint16_t*)g_trie_inset_res);
        g_trie_inset_res+=sizeof(uint16_t);
        deep = *((uint8_t*)g_trie_inset_res);
        g_trie_inset_res+=sizeof(uint8_t);
        s=(char*)(g_trie_inset_res);
        for(j=0;j<deep;++j)
        {
            slen[j] = *((uint8_t*)g_trie_inset_res);
            g_trie_inset_res+=sizeof(uint8_t);
            g_trie_inset_res+=slen[j];
        }
        nunit = *((uint16_t*)g_trie_inset_res);
        g_trie_inset_res+=sizeof(uint16_t);
        units = (uint16_t*)g_trie_inset_res;
        g_trie_inset_res+=nunit*sizeof(uint16_t);
        wtk_trietree_root_insert(cfg->root_inset[idx],s,slen,units,nunit,deep, sizeof(uint8_t)/sizeof(char));
    }

    ret = 0;
    return ret;
}

int wtk_trietree_cfg_update2(wtk_trietree_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;

	cfg->rbin=sl->hook;
    if(cfg->trie_fn)
    {
        ret=wtk_trietree_update_loadtrie(cfg, sl->hook, cfg->trie_fn);
        if(ret!=0) goto end;
    }
    if(cfg->trie_inset_fn)
    {
        ret=wtk_trietree_update_loadintrie(cfg, sl->hook, cfg->trie_inset_fn);
        if(ret!=0) goto end;
    }
end:
	return ret;
}
