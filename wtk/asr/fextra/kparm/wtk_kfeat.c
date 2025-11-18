#include "wtk_kfeat.h" 
#include "wtk/core/wtk_alloc.h"


wtk_kfeat_t* wtk_kfeat_new(int n)
{
	wtk_kfeat_t *f;

	f=(wtk_kfeat_t*)wtk_malloc(sizeof(wtk_kfeat_t));
	f->v=(float*)wtk_calloc(n,sizeof(float));
	f->index=0;
	f->used=0;
        f->len = n;
        return f;
}

int wtk_kfeat_bytes(int n)
{
	int bytes;

	bytes=sizeof(wtk_kfeat_t);
	bytes+=n*sizeof(float);
	return bytes;
}

int wtk_kfeat_delete(wtk_kfeat_t *f)
{
	wtk_free(f->v);
	wtk_free(f);
	return 0;
}

