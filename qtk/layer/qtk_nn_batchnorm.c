#include "qtk_nn_batchnorm.h"
#include "tts-mer/wtk-extend/wtk_mer_source_file.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"

qtk_nn_batchnorm_t* qtk_nn_batchnorm_new(int n,float epsilon)
{
    qtk_nn_batchnorm_t *batchnorm = NULL;

    batchnorm = wtk_malloc(sizeof(*batchnorm));
    batchnorm->gamma = wtk_vecf_new(n);
    batchnorm->beta = wtk_vecf_new(n);
    batchnorm->mean = wtk_vecf_new(n);
    batchnorm->variance = wtk_vecf_new(n);
    batchnorm->epsilon = (fabsf(epsilon)<=1e-10?1e-5:epsilon);

    return batchnorm;
}

int qtk_nn_batchnorm_forward_inplace(qtk_nn_batchnorm_t *batchnorm,wtk_matf_t *in)
{
    int size = in->row, channel = in->col, i, j;
    float *x = NULL,*v = NULL,*g = NULL,*m = NULL,*b = NULL;

    x = in->p;
    v = malloc(sizeof(float)*channel);
    g = batchnorm->gamma->p;
    m = batchnorm->mean->p;
    b = batchnorm->beta->p;

    wtk_float_set(v, batchnorm->epsilon, channel);
    wtk_float_add(v, batchnorm->variance->p, channel);

    for (i=0; i<channel; ++i){
        v[i] = sqrtf(v[i]);
    }
    for (j=0; j<size; ++j, x+=channel){
        for (i=0; i<channel; ++i){
            x[i] = ((x[i]-m[i])/ v[i])*g[i] + b[i];
        }
    }
    wtk_free(v);
    return 0;
}

int qtk_nn_batchnorm_delete(qtk_nn_batchnorm_t *batchnorm)
{
    if(batchnorm == NULL) goto end;
    if(batchnorm->gamma) wtk_vecf_delete(batchnorm->gamma);
    if(batchnorm->beta) wtk_vecf_delete(batchnorm->beta);
    if(batchnorm->mean) wtk_vecf_delete(batchnorm->mean);
    if(batchnorm->variance) wtk_vecf_delete(batchnorm->variance);
    wtk_free(batchnorm);
end:
    return 0;
}

int qtk_nn_batchnorm_load_file(qtk_nn_batchnorm_t *batchnorm,char *gamma,char *beta,char *mean,char *var)
{
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_vecf(&sl,&source,gamma,batchnorm->gamma);
    wtk_mer_source_loader_load_vecf(&sl,&source,beta,batchnorm->beta);
    wtk_mer_source_loader_load_vecf(&sl,&source,mean,batchnorm->mean);
    wtk_mer_source_loader_load_vecf(&sl,&source,var,batchnorm->variance);
    
    return 0;
}
