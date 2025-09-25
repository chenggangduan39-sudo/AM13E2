#ifndef  _WIN32
#define __USE_GNU
#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#else
#include <stdlib.h>
//#include <windows.h>
#include <fcntl.h>
#include <sys/types.h>
#endif
#include "wtk_fst_binet.h"

#ifdef __ANDROID__
#define USE_POSIX_NET
#endif



#if (defined WIN32) && (!defined WINCE)
static int pread(unsigned int fd, char *buf, size_t count, int offset)
{
    if (_lseek(fd, offset, SEEK_SET) != offset) {
        return -1;
    }
    return read(fd, buf, count);
}
#endif

wtk_fst_binet_t* wtk_fst_binet_new(wtk_fst_binet_cfg_t *cfg)
{
	wtk_fst_binet_t *bin;
	int ret;
	wtk_fst_binet_type_t type;
	char *fn;

	if(cfg->use_rbin)
	{
		fn=cfg->rbin->fn;
	}else
	{
		fn=cfg->bin_fn;
	}
	bin=(wtk_fst_binet_t*)wtk_malloc(sizeof(wtk_fst_binet_t));
	bin->cfg=cfg;
	type=cfg->type;
	if(type==WTK_FST_BINET_MEM || type==WTK_FST_BINET_MAP)
	{
		bin->buf=0;
		bin->buf_size=0;
	}else
	{
#ifdef USE_POSIX_NET
		bin->buf=wtk_malloc(cfg->cache_size);
		if (!bin->buf){ret = -1; goto end;}
		bin->buf_size=cfg->cache_size;
#else
		//bin->buf=wtk_malloc(cfg->cache_size);
		bin->buf_size=cfg->cache_size;
		#ifdef _WIN32
        ret = 0;
        bin->buf = wtk_malloc(cfg->cache_size);
		#else
		ret=posix_memalign((void**)&(bin->buf),4096,cfg->cache_size);
		#endif
		if(ret!=0)
		{
			wtk_debug("calloc mem[%d] failed.\n",cfg->cache_size);
			ret=-1;goto end;
		}
#endif
	}
	switch(type)
	{
	case WTK_FST_BINET_MEM:
		bin->buf=0;
		break;
	case WTK_FST_BINET_FILE:
		bin->f.file=fopen(fn,"rb");
		if(!bin->f.file){ret=-1;goto end;}
		break;
	case WTK_FST_BINET_FD:
#if (defined __APPLE__) || (defined __WIN32__) || (defined _WIN32)
		bin->f.fd=open(fn,O_RDONLY);//|O_DIRECT);
#else
		bin->f.fd=open(fn,O_RDONLY|O_LARGEFILE);//|O_DIRECT);
#endif
		if(bin->f.fd<0)
		{
			wtk_debug("open [%s] failed.\n",cfg->bin_fn);
			ret=-1;goto end;
		}

		break;
	case WTK_FST_BINET_MAP:
		bin->f.map.addr=NULL;
#if !(defined(__WIN32__) || defined(_WIN32))
#ifdef __APPLE__
		bin->f.map.fd=open(fn,O_RDONLY);//|O_DIRECT);
#else
		bin->f.map.fd=open(fn,O_RDONLY|O_LARGEFILE);//|O_DIRECT);
#endif
		if(bin->f.map.fd<0){ret=-1;goto end;}
		bin->f.map.addr=mmap(NULL,cfg->filesize,PROT_READ,MAP_PRIVATE,bin->f.map.fd,0);
		if(!bin->f.map.addr){ret=-1;goto end;}
#endif
		//madvise(bin->f.map.addr,4096,MADV_RANDOM|MADV_DONTNEED);
		break;
	}
	ret=0;
end:
	if(ret!=0)
	{
		wtk_debug("load network failed.\n");
		wtk_fst_binet_delete(bin);
		bin=0;
	}
	return bin;
}

void wtk_fst_binet_reset(wtk_fst_binet_t *bin)
{
	switch(bin->cfg->type)
	{
#ifndef USE_POSIX_NET
	case WTK_FST_BINET_FD:
#endif
		break;
	default:
		break;
	}
}

void wtk_fst_binet_delete(wtk_fst_binet_t *bin)
{
	switch(bin->cfg->type)
	{
	case WTK_FST_BINET_MEM:
		break;
	case WTK_FST_BINET_FILE:
		if(bin->f.file)
		{
			fclose(bin->f.file);
		}
		break;
	case WTK_FST_BINET_FD:
		if(bin->f.fd>0)
		{
			close(bin->f.fd);
		}
		break;
	case WTK_FST_BINET_MAP:
#if !(defined(_WIN32))
		if(bin->f.map.addr)
		{
			munmap(bin->f.map.addr,bin->cfg->filesize);
		}
		if(bin->f.map.fd>0)
		{
			close(bin->f.map.fd);
		}
#endif
		break;
	}
	if(bin->buf)
	{
		wtk_free(bin->buf);
	}
	wtk_free(bin);
}


int wtk_fst_binet_get_file(wtk_fst_binet_t* bin,unsigned int idx,wtk_string_t *result,FILE *f)
{
	wtk_fst_binet_cfg_t *cfg=bin->cfg;
	uint64_t of,len;
	int ret;

	wtk_string_set(result,0,0);
	if(bin->cfg->use_idx)
	{
		uint64_t *offset=cfg->offset+idx;

		of=*offset;
		len=*(++offset)-of;
	}else
	{
		uint64_t ofs[2];

		of=cfg->idx_offset+(idx)*sizeof(uint64_t);
		//wtk_debug("off=%lu len=%lu idx=%u\n",cfg->idx_offset,(int)(idx-1)*sizeof(uint64_t),idx);
		//exit(0);
		ret=fseek(f,of,SEEK_SET);
		if(ret!=0)
		{
#ifdef USE_POSIX_NET
			wtk_debug("seek of[%d] failed.\n",(int)of);
#else
			wtk_debug("seek of[%d] failed.\n",(int)of);
#endif
			goto end;
		}
		ret=fread(ofs,sizeof(uint64_t)*2,1,f);
		if(ret!=1)
		{
#ifdef USE_POSIX_NET
			wtk_debug("seek of[%d] failed ret=%d idx=%d.\n",(int)of,ret,idx);
#else
			wtk_debug("seek of[%d] failed ret=%d idx=%d.\n",(int)of,ret,idx);
#endif
			goto end;
		}
		//wtk_debug("[%lu %lu]\n",ofs[0],ofs[1]);
		of=ofs[0]+cfg->data_offset;
		len=ofs[1]-ofs[0];
		//wtk_debug("of=%lu len=%lu\n",of,len);
		//exit(0);
	}
	//wtk_debug("=> of=%d,len=%d,idx=%d\n",of,len,idx);
	ret=fseek(f,of,SEEK_SET);
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
#ifdef USE_POSIX_NET
		wtk_debug("seek of[%d] failed.\n",(int)of);
#else
		wtk_debug("seek of[%d] failed.\n",(int)of);
#endif
		goto end;
	}
/*
	if(len>bin->cfg->cache_size)
	{
#ifdef USE_POSIX_NET
		wtk_debug("v[%d]=[%llu,%d,%d]\n",idx,len,bin->cfg->cache_size,idx);
#else
		exit(0);
#endif
	}
*/
	if(len>bin->buf_size)
	{
		wtk_free(bin->buf);
		bin->buf=wtk_malloc(len);
		bin->buf_size=len;
	}
	ret=fread(bin->buf,len,1,f);
	//wtk_debug("ret=%d\n",ret);
	if(ret!=1)//  || ftell(f)!=*offset)
	{
		//(offset=236447099,dof=236447084,len=15,pos=4531414380/-1681244523)
#ifdef USE_POSIX_NET
		wtk_debug("read failed (dof=%d,len=%d,pos=%d/%d).\n",
				(int)of,(int)len,(int)ftell(f),(int)bin->cfg->offset[bin->cfg->ndx]);
#else
		wtk_debug("read failed (dof=%d,len=%d,pos=%d/%d).\n",(int)of,(int)len,(int)ftell(f),(int)bin->cfg->offset[bin->cfg->ndx]);
#endif
		ret=-1;
		goto end;
	}
	/*
	//if(idx<10)
	{
		wtk_debug("%p v[%d]=%lu %lu\n",bin,idx,of,len);
	}*/
	//print_char((unsigned char*)bin->buf,len);
	wtk_string_set(result,bin->buf,len);
	ret=0;
end:
	return ret;
}

#include <errno.h>

int wtk_fst_binet_get_fd2(wtk_fst_binet_t* bin,unsigned int idx,wtk_string_t *result,int f)
{
#define IDX_SIZE sizeof(uint64_t)*2
	wtk_fst_binet_cfg_t *cfg=bin->cfg;
	uint64_t of,len;
	off_t ret;

	wtk_string_set(result,0,0);
	if(bin->cfg->use_idx)
	{
		uint64_t *offset=cfg->offset+idx;

		of=*offset;
		len=*(++offset)-of;
	}else
	{
		uint64_t ofs[2];

		of=cfg->idx_offset+(idx)*sizeof(uint64_t);
		//wtk_debug("off=%lu len=%lu idx=%u\n",cfg->idx_offset,(int)(idx-1)*sizeof(uint64_t),idx);
		//exit(0);
        ret = lseek(f, of, SEEK_SET);
		if(ret<0)
		{
#ifdef USE_POSIX_NET
			wtk_debug("seek of[%d] failed.\n",(int)of);
#else
			wtk_debug("seek of[%d] failed.\n",(int)of);
#endif
			goto end;
		}
		ret=read(f,ofs,IDX_SIZE);
		if(ret!=IDX_SIZE)
		{
#ifdef USE_POSIX_NET
			wtk_debug("seek of[%d] failed ret=%d idx=%d.\n",(int)of,(int)ret,idx);
#else
			wtk_debug("seek of[%d] failed ret=%d idx=%d.\n",(int)of,(int)ret,idx);
#endif
			goto end;
		}
		//wtk_debug("[%lu %lu]\n",ofs[0],ofs[1]);
		of=ofs[0]+cfg->data_offset;
		len=ofs[1]-ofs[0];
		//wtk_debug("of=%lu len=%lu\n",of,len);
		//exit(0);
	}
	//wtk_debug("=> of=%d,len=%d,idx=%d\n",of,len,idx);
    ret = lseek(f, of, SEEK_SET);
	//wtk_debug("ret=%d\n",ret);
	if(ret<0)
	{
		perror(__FUNCTION__);
#ifdef USE_POSIX_NET
		wtk_debug("seek of[%d %d b=%d idx=%d err=%d] failed.\n",(int)of,(int)ret,(int)sizeof(off_t),idx,errno);
#else
		wtk_debug("seek of[%#x %#x b=%d idx=%d err=%d] failed.\n",(int)of,(int)ret,(int)sizeof(off_t),idx,errno);
#endif
		goto end;
	}
/*
	if(len>bin->cfg->cache_size)
	{
		wtk_debug("v[%d]=[%llu,%d,%d]\n",idx,len,bin->cfg->cache_size,idx);
		exit(0);
	}
*/
	ret=read(f,bin->buf,len);
	//wtk_debug("ret=%d\n",ret);
	if(ret!=len)//  || ftell(f)!=*offset)
	{
		//(offset=236447099,dof=236447084,len=15,pos=4531414380/-1681244523)
		perror(__FUNCTION__);
#ifdef USE_POSIX_NET
		wtk_debug("read failed (dof=%d,len=%d,ret=%d err=%d).\n",(int)of,(int)len,(int)ret,errno);
#else
		wtk_debug("read failed (dof=%d,len=%d,ret=%d err=%d).\n",(int)of,(int)len,(int)ret,errno);
#endif
		ret=-1;
		goto end;
	}
	/*
	//if(idx<10)
	{
		wtk_debug("%p v[%d]=%lu %lu\n",bin,idx,of,len);
	}*/
	//print_char((unsigned char*)bin->buf,len);
	wtk_string_set(result,bin->buf,len);
	ret=0;
end:
	return ret;
}

int wtk_fst_binet_get_memory(wtk_fst_binet_t* bin,unsigned int idx,wtk_string_t *result)
{
	uint64_t *offset=bin->cfg->offset+idx;
	uint64_t of,len;
	//int ret;

	//wtk_string_set(result,0,0);
	of=*offset;
	len=*(++offset)-of;
	//wtk_debug("len=%d\n",len);
	of=of-bin->cfg->data_offset;
	//wtk_debug("of=%d,len=%d\n",(int)of,(int)len);
	wtk_string_set(result,bin->cfg->data+of,len);
	//print_data(result->data,result->len);
	//wtk_debug("len=%d\n",result->len);
	//exit(0);
	//wtk_debug("len=%d\n",result->len);
	return 0;
}

int wtk_fst_binet_get_fd(wtk_fst_binet_t* bin,unsigned int idx,wtk_string_t *result,int f)
{
#define IDX_SIZE sizeof(uint64_t)*2
	wtk_fst_binet_cfg_t *cfg=bin->cfg;
	uint64_t of,len;
	off_t ret;

	wtk_string_set(result,0,0);
	if(bin->cfg->use_idx)
	{
		uint64_t *offset=cfg->offset+idx;

		of=*offset;
		len=*(++offset)-of;
	}else
	{
		uint64_t ofs[2];

		of=cfg->idx_offset+(idx)*sizeof(uint64_t);
		//wtk_debug("off=%lu len=%lu idx=%u\n",cfg->idx_offset,(int)(idx-1)*sizeof(uint64_t),idx);
		//exit(0);
		ret=pread(f,ofs,IDX_SIZE,of);
		if(ret!=IDX_SIZE)
		{
#ifdef USE_POSIX_NET
			wtk_debug("seek of[%d] failed ret=%d idx=%d.\n",(int)of,(int)ret,idx);
#else
			wtk_debug("seek of[%d] failed ret=%d idx=%d.\n",(int)of,(int)ret,idx);
#endif
			goto end;
		}
		//wtk_debug("[%lu %lu]\n",ofs[0],ofs[1]);
		of=ofs[0]+cfg->data_offset;
		len=ofs[1]-ofs[0];
		//wtk_debug("of=%lu len=%lu\n",of,len);
		//exit(0);
	}
	if(len>bin->buf_size)
	{
		wtk_free(bin->buf);
		bin->buf=wtk_malloc(len);
		bin->buf_size=len;
	}
	//wtk_debug("of=%d\n",of);
	ret=pread(f,bin->buf,len,of);
	//wtk_debug("ret=%d\n",ret);
	if(ret!=len)//  || ftell(f)!=*offset)
	{
		//(offset=236447099,dof=236447084,len=15,pos=4531414380/-1681244523)
#ifdef USE_POSIX_NET
		wtk_debug("read failed (dof=%d,len=%d).\n",(int)of,(int)len);
#else
		wtk_debug("read failed (dof=%d,len=%d).\n",(int)of,(int)len);
#endif
		ret=-1;
		goto end;
	}
	//printf("%d %lu %lu\n",idx,of,len);
	//printf("%d %lu %lu\n",idx,of,len);
	/*
	//if(idx<10)
	{
		wtk_debug("%p v[%d]=%lu %lu\n",bin,idx,of,len);
	}*/
	//print_char((unsigned char*)bin->buf,len);
	wtk_string_set(result,bin->buf,len);
	ret=0;
end:
	//print_data(result->data,result->len);
	//wtk_debug("ret=%d\n",(int)ret);
	//exit(0);
	return ret;
}


int wtk_fst_binet_get_map(wtk_fst_binet_t* bin,unsigned int idx,wtk_string_t *result,char *addr)
{
	wtk_fst_binet_cfg_t *cfg=bin->cfg;
	uint64_t of,len;
	uint64_t *ofs;

	wtk_string_set(result,0,0);
	of=cfg->idx_offset+(idx)*sizeof(uint64_t);
	ofs=(uint64_t*)(addr+of);

	of=ofs[0]+cfg->data_offset;
	len=ofs[1]-ofs[0];
	//memcpy(bin->buf,p,len);
	//wtk_string_set(result,bin->buf,len);
	wtk_string_set(result,addr+of,len);
	return 0;
}


int wtk_fst_binet_get(wtk_fst_binet_t* bin,unsigned int idx,wtk_string_t *result)
{
	switch(bin->cfg->type)
	{
	case WTK_FST_BINET_MEM:
		return wtk_fst_binet_get_memory(bin,idx,result);
		break;
	case WTK_FST_BINET_FILE:
		return wtk_fst_binet_get_file(bin,idx,result,bin->f.file);
		break;
	case WTK_FST_BINET_FD:
		return wtk_fst_binet_get_fd(bin,idx,result,bin->f.fd);
		break;
	case WTK_FST_BINET_MAP:
		return wtk_fst_binet_get_map(bin,idx,result,bin->f.map.addr);
		break;
	}
	return 0;
}
