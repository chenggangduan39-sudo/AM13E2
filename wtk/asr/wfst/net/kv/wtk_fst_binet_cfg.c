#include "wtk_fst_binet_cfg.h"

int wtk_fst_binet_cfg_init(wtk_fst_binet_cfg_t *cfg)
{
	cfg->bin_fn=0;
	cfg->offset=0;
	cfg->ndx=0;
	//cfg->bytes=0;
	cfg->cache_size=4096;
	cfg->data=0;
	cfg->data_bytes=0;
	cfg->use_idx=0;

	cfg->type=WTK_FST_BINET_FD;
	cfg->use_memory=0;
	cfg->use_fd=1;
	cfg->use_file=0;
	cfg->use_map=0;
	cfg->use_pack_bin=0;
	cfg->use_vmem=0;
	cfg->use_rbin=0;

	cfg->rbin=NULL;
	return 0;
}

int wtk_fst_binet_cfg_clean(wtk_fst_binet_cfg_t *cfg)
{
	if(cfg->offset)
	{
		wtk_free(cfg->offset);
	}
	if(cfg->data)
	{
		wtk_free(cfg->data);
	}
	return 0;
}

int wtk_fst_binet_cfg_bytes(wtk_fst_binet_cfg_t *cfg)
{
	int bytes=0;

	if(cfg->offset)
	{
		bytes+=cfg->ndx*sizeof(unsigned int);
	}
	if(cfg->data)
	{
		bytes+=cfg->data_bytes;
	}
	return bytes;
}

int wtk_fst_binet_cfg_update_local(wtk_fst_binet_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,bin_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cache_size,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_memory,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_idx,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rbin,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_fd,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_file,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_map,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_vmem,v);
#ifdef __WIN32__
	//for pread not wok on windows
	if(cfg->use_fd)
	{
		cfg->use_fd=0;
		cfg->use_file=1;
	}
#endif
	return 0;
}

int wtk_fst_binet_cfg_read_idx(wtk_fst_binet_cfg_t *cfg)
{
	FILE *f;
	uint64_t ret=-1;
	uint64_t i;
	uint64_t n;
	uint64_t *xi=0;
	uint64_t *offset=0;
	uint64_t x0;
	uint64_t last_of=0;
	uint64_t len;
	char buf[10];

	cfg->filesize=wtk_file_size(cfg->bin_fn);
	f=fopen(cfg->bin_fn,"rb");
	if(!f){goto end;}
	//X==Y
	ret=fread(buf,1,4,f);
	//ret=fseek(f,4,SEEK_SET);
	if(ret<0){ret=-1;goto end;}
	//printf("ret=%d: [%.*s]\n",ret,ret,buf);
	if(wtk_str_equal_s(buf,ret,"X==A"))
	{
		cfg->use_pack_bin=1;
	}else
	{
		cfg->use_pack_bin=0;
	}
	//exit(0);
	//idx bytes
	ret=fread(&n,sizeof(n),1,f);
	if(ret<=0){ret=-1;goto end;}
	//X==Y+8+idx_bytes
	x0=4+sizeof(uint64_t)+n;
	//wtk_debug("n=%lu:%lu\n",n,n/2);
	cfg->idx_offset=4+sizeof(uint64_t);
	cfg->data_offset=x0;
	n=n/8;
	cfg->ndx=n;
	//wtk_debug("ndx=%d\n",cfg->ndx);
	if(cfg->use_idx)
	{
		xi=wtk_calloc(n,sizeof(uint64_t));
		offset=wtk_calloc(n,sizeof(uint64_t));
		ret=fread(xi,sizeof(uint64_t),n,f);
		if(ret!=n){ret=-1;goto end;}
		//offset[0]=x0;
		for(i=0;i<n;++i)
		{
			offset[i]=cfg->data_offset+xi[i];
			//wtk_debug("v[%d/%d]=%lu/%lu/%lu\n",i,n,cfg->data_offset,offset[i],xi[i]);
			//exit(0);
			len=xi[i]-last_of;
			last_of=xi[i];
			while(len>cfg->cache_size)
			{
				cfg->cache_size+=4096;
			}
		}
		//wtk_debug("[%s]=%d\n",cfg->bin_fn,cfg->cache_size);
		//exit(0)
		cfg->offset=offset;
		offset=0;
	}
	if(cfg->use_memory)
	{
		//262778
		ret=fseek(f,cfg->data_offset,SEEK_SET);
		if(ret<0){ret=-1;goto end;}
		cfg->data_bytes=cfg->filesize-x0;
		//wtk_debug("%ld %ld %ld\n",cfg->filesize,x0,cfg->data_bytes);
		cfg->data=(char*)wtk_malloc(cfg->data_bytes);
		//ret=fread(cfg->data,1,cfg->data_bytes,f);
		//wtk_debug("len=%d\n",(int)ftell(f));
		uint64_t x=0;
                ret=0;
                while(x<=cfg->data_bytes)
                {
                        if(cfg->data_bytes-x<4096)
                        {
                                ret+=fread((cfg->data+x),1,(cfg->data_bytes-x),f);
                        }else
                        {
                                ret+=fread((cfg->data+x),1,4096,f);
                        }
                        x+=4096;
                }

		if(ret!=cfg->data_bytes)
		{
			wtk_debug("read data failed[%d/%d].\n",(int)cfg->data_bytes,(int)ret);
			ret=-1;
	exit(0);
			goto end;
		}
		ret=0;
	}
	ret=0;
end:
	if(xi)
	{
		wtk_free(xi);
	}
	if(offset)
	{
		wtk_free(offset);
	}
	if(f)
	{
		fclose(f);
	}
	return ret;
}

int wtk_fst_binet_cfg_read_soure(wtk_fst_binet_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	FILE *f;
	int ret=-1;
	uint64_t i;
	uint64_t n;
	uint64_t *xi=0;
	uint64_t *offset=0;
	uint64_t x0;
	uint64_t last_of=0;
	uint64_t len;
	char buf[10];
	wtk_rbin2_item_t* item;

	item=wtk_rbin2_get(rbin,cfg->bin_fn,strlen(cfg->bin_fn));
	if(!item)
	{
		wtk_debug("[%s] not found.\n",cfg->bin_fn);
		goto end;
	}
	//wtk_debug("len=%d\n",item->len);
	f=rbin->f;
	cfg->rbin=rbin;
	cfg->filesize=item->len;
	ret=fseek(f,item->pos,SEEK_SET);
	if(ret!=0)
	{
		wtk_debug("[%s] seek failed.\n",cfg->bin_fn);
		goto end;
	}
	//X==Y
	ret=fread(buf,1,4,f);
	//ret=fseek(f,4,SEEK_SET);
	if(ret<0)
	{
		wtk_debug("[%s] read hdr failed.\n",cfg->bin_fn);
		ret=-1;goto end;
	}
	//printf("ret=%d: [%.*s]\n",ret,ret,buf);
	if(wtk_str_equal_s(buf,ret,"X==A"))
	{
		cfg->use_pack_bin=1;
	}else
	{
		cfg->use_pack_bin=0;
	}
	//exit(0);
	//idx bytes
	ret=fread(&n,sizeof(n),1,f);
	if(ret<=0)
	{
		wtk_debug("[%s] read offset failed ret=%d.\n",cfg->bin_fn,ret);
		ret=-1;goto end;
	}
	//X==Y+8+idx_bytes
	x0=4+sizeof(uint64_t)+n;
	//wtk_debug("n=%lu:%lu\n",n,n/2);
	cfg->idx_offset=4+sizeof(uint64_t)+item->pos;
	cfg->data_offset=x0+item->pos;
	n=n/8;
	cfg->ndx=n;
	//wtk_debug("ndx=%d\n",cfg->ndx);
	if(cfg->use_idx)
	{
		xi=wtk_calloc(n,sizeof(uint64_t));
		offset=wtk_calloc(n,sizeof(uint64_t));
		ret=fread(xi,sizeof(uint64_t),n,f);
		if(ret!=n)
		{
			wtk_debug("[%s] read 1 offset failed.\n",cfg->bin_fn);
			ret=-1;goto end;
		}
		//offset[0]=x0;
		for(i=0;i<n;++i)
		{
			offset[i]=cfg->data_offset+xi[i];
			//wtk_debug("v[%d/%d]=%lu/%lu/%lu\n",i,n,cfg->data_offset,offset[i],xi[i]);
			//exit(0);
			len=xi[i]-last_of;
			last_of=xi[i];
			while(len>cfg->cache_size)
			{
				cfg->cache_size+=4096;
			}
		}
		//wtk_debug("[%s]=%d\n",cfg->bin_fn,cfg->cache_size);
		//exit(0)
		cfg->offset=offset;
		offset=0;
	}
	if(cfg->use_memory)
	{
		//262778
		ret=fseek(f,cfg->data_offset,SEEK_SET);
		if(ret<0){ret=-1;goto end;}
		cfg->data_bytes=cfg->filesize-x0;
		//wtk_debug("%d\n",cfg->data_bytes);
		cfg->data=(char*)wtk_malloc(cfg->data_bytes);
		ret=fread(cfg->data,1,cfg->data_bytes,f);
		//wtk_debug("len=%d\n",(int)ftell(f));
		if(ret!=cfg->data_bytes)
		{
			wtk_debug("read data failed[%d/%d].\n",(int)cfg->data_bytes,(int)ret);
			ret=-1;
			goto end;
		}
		ret=0;
	}
	//cfg->idx_offset+=item->pos;
	//cfg->offset+=item->pos;
	//cfg->data_offset+=item->pos;
	ret=0;
end:
	if(xi)
	{
		wtk_free(xi);
	}
	if(offset)
	{
		wtk_free(offset);
	}
	return ret;
}


int wtk_fst_binet_cfg_init_mem1(wtk_fst_binet_cfg_t *cfg,wtk_string_t *data)
{
	char *s;
	uint64_t x0,n;
	uint64_t *xi,*offset;
	int i;
	uint64_t last_of=0;
	uint64_t len;

	cfg->filesize=data->len;
	s=data->data;
	if(wtk_str_equal_s(s,4,"X==A"))
	{
		cfg->use_pack_bin=1;
	}else
	{
		cfg->use_pack_bin=0;
	}
	s+=4;
	n=*(uint64_t*)s;
	//X==Y+8+idx_bytes
	x0=4+sizeof(uint64_t)+n;
	cfg->idx_offset=4+sizeof(uint64_t);
	cfg->data_offset=x0;
	cfg->ndx=n/8;
	xi=(uint64_t*)(data->data+cfg->idx_offset);
	offset=wtk_calloc(n,sizeof(uint64_t));
	for(i=0;i<cfg->ndx;++i)
	{
		offset[i]=cfg->data_offset+xi[i];
		//wtk_debug("v[%d/%d]=%lu/%lu/%lu\n",i,n,cfg->data_offset,offset[i],xi[i]);
		//exit(0);
		len=xi[i]-last_of;
		last_of=xi[i];
		while(len>cfg->cache_size)
		{
			cfg->cache_size+=4096;
		}
	}
	cfg->offset=offset;

	cfg->data_bytes=cfg->filesize-x0;
	//wtk_debug("%d\n",cfg->data_bytes);
	cfg->data=(char*)wtk_malloc(cfg->data_bytes);
	memcpy(cfg->data,data->data+cfg->data_offset,cfg->data_bytes);;
	return 0;
}


int wtk_fst_binet_cfg_read_mem1(wtk_fst_binet_cfg_t *cfg,wtk_source_t *src)
{
	wtk_string_t *data;
	int ret=-1;

	data=src->get_file(src->data);
	if(!data){goto end;}
	ret=wtk_fst_binet_cfg_init_mem1(cfg,data);
end:
	return ret;
}


int wtk_fst_binet_cfg_read_mem2(wtk_fst_binet_cfg_t *cfg,wtk_source_t *src)
{
	wtk_string_t *data;
	int ret=-1;
	char *s;
	uint64_t x0,n;
	uint64_t *xi,*offset;
	int i;
	uint64_t last_of=0;
	uint64_t len;

	data=src->get_file(src->data);
	if(!data){goto end;}
	cfg->filesize=data->len;
	s=data->data;
	if(wtk_str_equal_s(s,4,"X==A"))
	{
		cfg->use_pack_bin=1;
	}else
	{
		cfg->use_pack_bin=0;
	}
	s+=4;
	n=*(uint64_t*)s;
	//X==Y+8+idx_bytes
	x0=4+sizeof(uint64_t)+n;
	cfg->idx_offset=4+sizeof(uint64_t);
	cfg->data_offset=x0;
	cfg->ndx=n/8;
	xi=(uint64_t*)(data->data+cfg->idx_offset);
	offset=wtk_calloc(n,sizeof(uint64_t));
	for(i=0;i<cfg->ndx;++i)
	{
		offset[i]=cfg->data_offset+xi[i];
		//wtk_debug("v[%d/%d]=%lu/%lu/%lu\n",i,n,cfg->data_offset,offset[i],xi[i]);
		//exit(0);
		len=xi[i]-last_of;
		last_of=xi[i];
		while(len>cfg->cache_size)
		{
			cfg->cache_size+=4096;
		}
	}
	cfg->offset=offset;

	cfg->data_bytes=cfg->filesize-x0;
	//wtk_debug("%d\n",cfg->data_bytes);
	cfg->data=(char*)wtk_malloc(cfg->data_bytes);
	memcpy(cfg->data,data->data+cfg->data_offset,cfg->data_bytes);;
	ret=0;
end:
	return ret;
}

int wtk_fst_binet_cfg_read_mem(wtk_fst_binet_cfg_t *cfg,wtk_source_t *src)
{
	wtk_string_t *data;
	int ret;

	if(src->get_file)
	{
		return  wtk_fst_binet_cfg_read_mem1(cfg,src);
	}else
	{
		data=wtk_source_read_file(src);
		//wtk_debug("read memory failed\n");
		ret=wtk_fst_binet_cfg_init_mem1(cfg,data);
		wtk_free(data);
		return ret;
	}
}

int wtk_fst_binet_cfg_update(wtk_fst_binet_cfg_t *cfg)
{
	wtk_source_loader_t file_sl;

	file_sl.hook=0;
	file_sl.vf=wtk_source_load_file_v;
	return wtk_fst_binet_cfg_update2(cfg,&(file_sl));
}


int wtk_fst_binet_cfg_update2(wtk_fst_binet_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;

	if(cfg->use_fd)
	{
		cfg->type=WTK_FST_BINET_FD;
	}else if(cfg->use_file)
	{
		cfg->type=WTK_FST_BINET_FILE;
	}else if(cfg->use_memory)
	{
		cfg->type=WTK_FST_BINET_MEM;

	}else if(cfg->use_map)
	{
		cfg->type=WTK_FST_BINET_MAP;
	}else
	{
		cfg->type=WTK_FST_BINET_FD;
	}
	/*
	wtk_debug("use_fd=%d,use_file=%d,use_mem=%d,use_map=%d type=%d\n",
			cfg->use_fd,cfg->use_file,cfg->use_memory,cfg->use_map,cfg->type);
	*/
	if(cfg->use_vmem)
	{
		cfg->use_memory=1;
		ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_fst_binet_cfg_read_mem,cfg->bin_fn);
	}else
	{
		if(cfg->bin_fn)
		{
			if(cfg->use_rbin)
			{
				ret=wtk_fst_binet_cfg_read_soure(cfg,(wtk_rbin2_t*)sl->hook);
				//ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_fst_binet_cfg_read_soure,cfg->bin_fn);
				if(ret!=0){goto end;}
			}else
			{
				ret=wtk_fst_binet_cfg_read_idx(cfg);
				if(ret!=0){goto end;}
			}
		}
	}
end:
	return ret;
}
