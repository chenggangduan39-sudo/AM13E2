#include "wtk_world_plural.h"

wtk_world_plural_t* wtk_world_plural_new(int size)
{
    wtk_world_plural_t *dst;
    size_t plural_st = sizeof(*dst);
    size_t st = sizeof(double)*size;
    char *p = (char*)malloc(plural_st + st*2);
    dst = (wtk_world_plural_t*)p;
    dst->real = (double*)(p+plural_st);
    dst->imag = dst->real + size;
    dst->size = size;
    wtk_world_plural_zero(dst);
    return dst;
}

void wtk_world_plural_zero(wtk_world_plural_t *p)
{
    size_t s = p->size*sizeof(double);
    memset(p->real, 0, s);
    memset(p->imag, 0, s);
}

void wtk_world_plural_delete(wtk_world_plural_t *p)
{
    free(p);
}

void wtk_world_plural_print(wtk_world_plural_t *p, int is_save)
{
    int size = p->size;
    int i;
    if (is_save)
    {
        FILE *fp = fopen("output/pulse.c.txt", "w");
        for (i=0; i<size; i++) {
            fprintf(fp, "%lf ", p->real[i]);
        }
        fclose(fp);
        printf("output/pulse.c.txt 保存成功 size: %d! \n", size);
    } else {
        for (i=0; i<size; i++) {
            printf("%lf ", p->real[i]);
        }
    }
    
}

void wtk_world_plural_mul(wtk_world_plural_t *a, wtk_world_plural_t *b, wtk_world_plural_t *dst)
{/* 复数乘法 */
    int sa = a->size
      , sb = b->size
      , i;
    double rl, ig;
    if (sa != sb &&  sa!=dst->size) {wtk_world_debug("复数大小不匹配 %d %d %d \n", sa, sb, dst->size);exit(1);}
    
    for (i=0; i<sa; i++)
    {
        rl = a->real[i]*b->real[i] + (a->imag[i]*b->imag[i]*-1);
        ig = a->real[i]*b->imag[i] + (a->imag[i]*b->real[i]);
        dst->real[i] = rl;
        dst->imag[i] = ig;
    }
}




/* float */

wtk_world_fplural_t* wtk_world_fplural_new(int size)
{
    wtk_world_fplural_t *dst;
    size_t plural_st = sizeof(*dst);
    size_t st = sizeof(float)*size;
    char *p = (char*)malloc(plural_st + st*2);
    dst = (wtk_world_fplural_t*)p;
    dst->real = (float*)(p+plural_st);
    dst->imag = dst->real + size;
    dst->size = size;
    wtk_world_fplural_zero(dst);
    return dst;
}

void wtk_world_fplural_zero(wtk_world_fplural_t *p)
{
    size_t s = p->size*sizeof(float);
    memset(p->real, 0, 2*s);
    // memset(p->imag, 0, s);
}

void wtk_world_fplural_delete(wtk_world_fplural_t *p)
{
    free(p);
}
void wtk_world_fplural_print(wtk_world_fplural_t *p, int is_save)
{
    int size = p->size;
    int i;
    if (is_save)
    {
        FILE *fp = fopen("output/fpulse.c.txt", "w");
        for (i=0; i<size; i++) {
            fprintf(fp, "%f ", p->real[i]);
        }
        fclose(fp);
        printf("output/fpulse.c.txt 保存成功 size: %d! \n", size);
    } else {
        for (i=0; i<size; i++) {
            printf("%f ", p->real[i]);
        }
    }
}
void wtk_world_fplural_mul(wtk_world_fplural_t *a, wtk_world_fplural_t *b, wtk_world_fplural_t *dst)
{/* 复数乘法 */
    int sa = a->size
      , sb = b->size
      , i;
    float rl, ig;
    float *arp=a->real
        , *aip=a->imag
        , *brp=b->real
        , *bip=b->imag;
    float ar, ai, br, bi;
    if (sa != sb &&  sa!=dst->size) {wtk_world_debug("复数大小不匹配 %d %d %d \n", sa, sb, dst->size);exit(1);}
    
    for (i=0; i<sa; ++i)
    {
        ar = arp[i];
        ai = aip[i];
        br = brp[i];
        bi = bip[i];
        // rl = a->real[i]*b->real[i] + (a->imag[i]*b->imag[i]*-1);
        // ig = a->real[i]*b->imag[i] + (a->imag[i]*b->real[i]);
        // dst->real[i] = rl;
        // dst->imag[i] = ig;
        rl = ar*br - (ai*bi);
        ig = ar*bi + (ai*br);
        dst->real[i] = rl;
        dst->imag[i] = ig;
    }
}