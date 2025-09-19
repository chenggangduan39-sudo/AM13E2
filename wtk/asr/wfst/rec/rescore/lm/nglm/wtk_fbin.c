#define __USE_GNU
#define _GNU_SOURCE
#define _LARGEFILE64_SOURCE
#include "wtk_fbin.h"

#if defined(__WIN32__) || defined(_WIN32)
#include <windows.h>
#include <fcntl.h>
#else
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#endif

#ifdef __ANDROID__
#define USE_CHAR_NET
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

wtk_fbin_t* wtk_fbin_new(char *fn)
{
	wtk_fbin_t *f;
        int ret;

        f = (wtk_fbin_t *)wtk_malloc(sizeof(wtk_fbin_t));
        f->buf=wtk_strbuf_new(1024*10,1);
	f->cache=NULL;
	f->cache_bytes=4096;
	f->of=0;
#ifdef __USE_FILE__
	f->fd=fopen(fn,"rb");
#else
	f->fd=open(fn,O_RDONLY);
#endif
	if(f->fd<0)
	{
		wtk_debug("open [%s] failed.\n",fn);
		ret=-1;goto end;
	}
#ifdef USE_CHAR_NET
	f->cache=(void*)wtk_malloc(f->cache_bytes);
	if(f->cache==0){
		ret=-1;goto end;
	}
#else
	#ifdef _WIN32
        ret = 0;
        f->cache = wtk_malloc(f->cache_bytes);
	#else
	ret=posix_memalign((void**)&(f->cache),4096,f->cache_bytes);
	#endif
	if(ret!=0)
	{
		wtk_debug("calloc mem[%d] failed.\n",f->cache_bytes);
		ret=-1;goto end;
        }
#endif
	ret=0;
end:
	if(ret!=0)
	{
		wtk_fbin_delete(f);
		ret=0;
	}
	return f;
}

void wtk_fbin_delete(wtk_fbin_t *f)
{
#ifdef __USE_FILE__
	if(f->fd)
	{
		fclose(f->fd);
	}
#else
	if(f->fd>0)
	{
		close(f->fd);
	}
#endif
	if(f->cache)
	{
		wtk_free(f->cache);
	}
	wtk_strbuf_delete(f->buf);
	wtk_free(f);
}

int wtk_fbin_get(wtk_fbin_t *f,uint64_t of,uint64_t bytes,wtk_string_t *v)
{
	wtk_strbuf_t *buf=f->buf;
#ifdef __USE_FILE__
	FILE *fd=f->fd;
#else
	int fd=f->fd;
#endif
	uint64_t nx;
	int ret=-1;
	int n;
	char *p=f->cache;
	int len=bytes;

	wtk_strbuf_reset(buf);
	of+=f->of;
	while(bytes>0)
	{
		n=min(f->cache_bytes,bytes);
		//wtk_debug("n=%d/%ld/%d\n",n,bytes,buf->pos);
#ifdef __USE_FILE__
		fseek(fd,of,SEEK_SET);
		nx=fread(p,1,n,fd);
#else
		nx=pread(fd,p,n,of);
#endif
		//print_data(p,n);
		if(nx!=n)
		{
			perror(__FUNCTION__);
#ifdef USE_CHAR_NET
			wtk_debug("seek of[%d] failed ret=%d idx=%d len=%d/%d.\n",(int)of,(int)nx,n,len,(int)bytes);
#else
			wtk_debug("seek of[%d] failed ret=%d idx=%d len=%d/%d.\n",(int)of,(int)nx,n,len,(int)bytes);
#endif
			//exit(0);
			ret=-1;
			goto end;
		}
		if(buf->pos<=0 && n==bytes)
		{
			wtk_string_set(v,p,n);
			ret=0;
			goto end;
		}
		wtk_strbuf_push(buf,p,n);
		of+=n;
		bytes-=n;
		//wtk_debug("pos=%d/%ld\n",buf->pos,bytes);
	}
	wtk_string_set(v,buf->data,buf->pos);
	ret=0;
end:
	return ret;
}

