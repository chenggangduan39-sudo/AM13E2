#include "wtk_mer_min_max_norm.h"

void wtk_mer_mean_variance_norm(wtk_matf_t *src, wtk_vecf_t *min_max)
{/* 均值方差范数 */
    wtk_vecf_t 
        min_vecf,
        max_vecf;
    int len = min_max->len/2;
    
    min_vecf.p = min_max->p;
    min_vecf.len = max_vecf.len = len;
    max_vecf.p = min_max->p+len;

    wtk_matf_vecf_multi(src, &max_vecf);
    wtk_matf_vecf_add(src, &min_vecf);
}

wtk_matf_t* wtk_mer_normalise_data(wtk_matf_t *matf, wtk_vecf_t *min_max_vec)
{
    int nrow = matf->row
    , ncol = matf->col
    , vf_len = min_max_vec->len/2
    , i
    , j;
    size_t ncol_st = ncol*sizeof(float);
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_vecf_t 
        min_vec,  /* 非heap管理部分 */
        max_vec;
    wtk_vecf_t 
        *fea_max_min_diff,
        *target_max_min_diff;
    wtk_matf_t 
        *fea_min_matf,
        *diff_norm_matf,
        *norm_fea_matf;
    float *p, *p2;

    min_vec.p = min_max_vec->p;
    max_vec.p = min_max_vec->p + vf_len;
    min_vec.len = max_vec.len = vf_len;
    
    fea_max_min_diff = wtk_vecf_heap_new( heap, ncol);
    wtk_vecf_minus( fea_max_min_diff, &max_vec, &min_vec);
    
    target_max_min_diff = wtk_vecf_heap_new( heap, ncol);
    wtk_vecf_init(target_max_min_diff, 0.98);
    p = fea_max_min_diff->p;
    for (i=0; i<ncol; i++)
    {
        if (p[i] <= 0) 
        { 
            target_max_min_diff->p[i] = 1.0;
            p[i] = 1.0;
        }
    }
    
    diff_norm_matf = wtk_matf_heap_new( heap, nrow, ncol);
    p = fea_max_min_diff->p;
    p2 = diff_norm_matf->p;
    for (i=0; i<nrow; i++, p2+=ncol)
    {
        memcpy(p2, target_max_min_diff->p, ncol_st);
        for (j=0; j<ncol; ++j)
        { 
            p2[j] /= p[j];
        }
    }

    fea_min_matf = wtk_matf_heap_new( heap, nrow, ncol);
    p = fea_min_matf->p;
    for (i=0; i<nrow; i++, p+=ncol)
    {
        memcpy(p, min_vec.p, ncol_st);
    }

    norm_fea_matf = wtk_matf_new( nrow, ncol);
    wtk_matf_add_scale(norm_fea_matf, matf->p, fea_min_matf->p, 1, -1);
    p = norm_fea_matf->p;
    p2 = diff_norm_matf->p;
    j = nrow*ncol;
    for (i=0; i<j-2; i+=2)
    {
        p[i]*=p2[i];
        p[i]+=0.01;
        p[i+1]*=p2[i+1];
        p[i+1]+=0.01;
    }
    for (;i<j; ++i)
    {
        p[i]*=p2[i];
        p[i]+=0.01;
    }
    // wtk_mer_matf_write_file(norm_fea_matf, "56.lab", 0);
    
    wtk_heap_delete(heap);
    return norm_fea_matf;
}