#include "wtk_mer_bandmat.h"

wtk_mer_bandmat_t* wtk_mer_bandmat_new(int l, int u, int frame_num, int is_transposed)
{
    wtk_mer_bandmat_t *bmt = wtk_malloc(sizeof(wtk_mer_bandmat_t));
    wtk_matf_t *mf = wtk_matf_new(l+u+1, frame_num);
    wtk_mer_bandmat_init(bmt, l, u, mf, is_transposed);
    return bmt;
}

wtk_mer_bandmat_t* wtk_mer_bandmat_heap_new(wtk_heap_t *heap, int l, int u, int frame_num, int is_transposed)
{
    wtk_mer_bandmat_t *bmt = wtk_heap_malloc(heap, sizeof(wtk_mer_bandmat_t));
    wtk_matf_t *mf = wtk_matf_heap_new(heap, l+u+1, frame_num);
    wtk_mer_bandmat_init(bmt, l, u, mf, is_transposed);
    return bmt;
}

void wtk_mer_bandmat_init(wtk_mer_bandmat_t *bmt, int l, int u, wtk_matf_t *mf, int is_transposed)
{
    bmt->l = l;
    bmt->u = u;
    bmt->matf = mf;
    bmt->is_transposed = is_transposed;
}

void wtk_mer_bandmat_delete(wtk_mer_bandmat_t *bmt)
{
    if (bmt->matf != NULL)
    { wtk_matf_delete(bmt->matf); }
    wtk_free(bmt);
}

void wtk_mer_bandmat_print(wtk_mer_bandmat_t *bmt)
{
    wtk_mer_bandmat_print2(bmt, 1);
}

void wtk_mer_bandmat_print2(wtk_mer_bandmat_t *bmt, int is_print_matf)
{
    if (is_print_matf) {wtk_matf_print(bmt->matf);}
    wtk_debug("l:%d  u:%d  is_transposed:%d \n", bmt->l, bmt->u, bmt->is_transposed);
}


void wtk_mer_bandmat_T(wtk_mer_bandmat_t *bm)
{
    bm->is_transposed ^= 1;
    bm->l ^=  bm->u;
    bm->u ^= bm->l;
    bm->l ^=  bm->u;
}


void wtk_mer_bandmat_dot_mv_plus_equals(wtk_mer_bandmat_t *a_bm, int transposed_a, wtk_vecf_t *b_vf, wtk_vecf_t *dst)
{
    wtk_matf_t *data = a_bm->matf;
    int l = a_bm->l
      , u = a_bm->u
      , frame_num = data->col
      , o_a
      , frame
      , r
      , d;
    float *p;

    for (o_a=-u; o_a<l+1; o_a++)
    {
        r = transposed_a ? (l-o_a) : (u+o_a);
        d = transposed_a ? 0 : -o_a;
        for (frame=max(0, o_a); frame<max(0, frame_num + min(0, o_a)); frame++)
        {
            p = wtk_matf_row(data, r);
            dst->p[frame] += p[frame+d] * b_vf->p[frame-o_a];
        }
    }
}

void wtk_mer_bandmat_dot_mm_plus_equals(wtk_mer_bandmat_t *a_bm, int transposed_a, wtk_mer_bandmat_t *b_bm, int transposed_b, wtk_mer_bandmat_t *dst, int transposed_c, wtk_vecf_t *diag)
{
    wtk_matf_t *a_data = a_bm->matf
             , *b_data = b_bm->matf
             , *c_data = dst->matf;

    int l_a = a_bm->l
      , u_a = a_bm->u
      , l_b = b_bm->l
      , u_b = b_bm->u
      , l_c = dst->l
      , u_c = dst->u
      , frame_num = a_data->col;

    int o_c
      , o_a
      , o_b
      , row_a
      , row_b
      , row_c
      , d_a
      , d_b
      , d_c
      , frame
      , use_diag = diag != NULL;

    for (o_c=-min(u_c, u_a+u_b); o_c<min(l_c, l_a+l_b)+1; o_c++)
    {
        for (o_a=-min(u_a, l_b - o_c); o_a<min(l_a, u_b + o_c) + 1; o_a++)
        {
            o_b = o_c - o_a;
            row_a = transposed_a ? (l_a - o_a) : (u_a + o_a);
            row_b = transposed_b ? (l_b - o_b) : (u_b + o_b);
            row_c = transposed_c ? (l_c - o_c) : (u_c + o_c);
            d_a = transposed_a ? o_a : 0;
            d_b = transposed_b ? 0 : -o_b;
            d_c = transposed_c ? o_a : -o_b;

            frame=max( max(0, -o_a), o_b);
            for (; frame<max(0, frame_num+min(min(0,-o_a),o_b)); frame++)
            {
                c_data->p[row_c*frame_num + frame + d_c] += \
                a_data->p[row_a*frame_num + frame + d_a] * \
                b_data->p[row_b*frame_num + frame + d_b] * \
                (use_diag ? diag->p[frame] : 1.0);
            }
        }
    }
}


void wtk_mer_bandmat_cholesky_banded(wtk_matf_t *mat, int overwrite_ab, int lower)
{
    int frames = mat->col
      , depth = mat->row - 1
      , frame;
    int k, l;
    float *mp = mat->p
        , *vp
        , v0
        , iv0
        , siv0;
    wtk_vecf_t *v = wtk_vecf_new(depth);
    vp = v->p;
    if (!lower)
    {
        wtk_debug("Waiting for the god to fill the code\n");
        wtk_exit(1);
    } else if (!overwrite_ab)
    {

    }
    for (frame=0; frame<frames; frame++)
    {
        v0 = mp[frame];
        iv0 = 1.0 / v0;
        siv0 = sqrt(iv0);

        for (k=0; k<depth; k++)
        {
            vp[k] = mp[ (k+1)*frames + frame];
        }
        mp[frame] = 1.0 / siv0;

        for (k=0; k<depth; k++)
        {
            mp[ (k+1)*frames + frame] = vp[k] * siv0;
        }

        for (k=0; k<min(depth, frames-frame-1); k++)
        {
            for (l=0; l<depth-k; l++)
            {
                mp[ l*frames + k + frame + 1 ] -= vp[l+k] * vp[k] * iv0;
            }
        }
    }
    if (!lower)
    {
        wtk_debug("Waiting for the god to fill the code\n");
        wtk_exit(1);
    }
    wtk_vecf_delete(v);
}

wtk_mer_bandmat_t* wtk_mer_bandmat_cholesky(wtk_mer_bandmat_t *mat_bm, int lower, int alternative)
{
    if (mat_bm->is_transposed) 
    { wtk_mer_bandmat_T(mat_bm);}

    int depth = mat_bm->l
      , l = lower ? depth : 0
      , u = lower ? 0 : depth;
    
    wtk_matf_t *data = mat_bm->matf;
    wtk_matf_t *mat_half_data = wtk_matf_row_slice(data, depth - u, min(data->row - depth - u, depth + l + 1));
    wtk_mer_bandmat_t *chol_bm = wtk_malloc(sizeof(wtk_mer_bandmat_t));
    if (alternative)
    {
        wtk_debug("Waiting for the god to fill the code\n");
        wtk_exit(1);
    } else 
    {
        wtk_mer_bandmat_cholesky_banded(mat_half_data, 1, lower);
    }
    wtk_mer_bandmat_init(chol_bm, l, u, mat_half_data, 0);
    return chol_bm;
}


void wtk_mer_bandmat_solve_triangular_banded(wtk_matf_t *a_rect, wtk_vecf_t *b, int transposed, int lower)
{/* 
    overwrite_b
 */
    int frames = a_rect->col
      , depth = a_rect->row - 1
      , solve_lower = (lower != transposed);
    unsigned long
        pos
      , frame
      , k
      , framePrev;
    double diff
        , denom;
    float *bp = b->p
        , *ap = a_rect->p;
    for (pos=0; pos<frames; pos++)
    {
        frame = solve_lower ? pos : (frames - 1 - pos);
        diff = bp[frame];
        if (lower)
        {
            if (transposed)
            {
                for (k=1; k<min(depth+1, pos+1); k++)
                {
                    framePrev = frame + k;
                    diff -= ap[k*frames + frame] * bp[framePrev];
                }
                denom = ap[frame];
            } else 
            {
                for (k=1; k<min(depth+1, pos+1); k++)
                {
                    framePrev = frame - k;
                    diff -= ap[k*frames + framePrev] * bp[framePrev];
                }
                denom = ap[frame];
            }
        } else
        {
            if (transposed)
            {
                for (k=1; k<min(depth+1, pos+1); k++)
                {
                    framePrev = frame - k;
                    diff -= ap[ (depth-k)*frames + frame] * bp[framePrev];
                }
                denom = ap[depth*frames + frame];
            } else 
            {
                for (k=1; k<min(depth+1, pos+1); k++)
                {
                    framePrev = frame + k;
                    diff -= ap[ (depth-k)*frames + framePrev] * bp[framePrev];
                }
                denom = ap[depth*frames + frame];
            }
        }
        bp[frame] = diff / denom;
    }
}

void wtk_mer_bandmat_solve_triangular(wtk_mer_bandmat_t *a_bm, wtk_vecf_t *b)
{
    int transposed = a_bm->is_transposed
      , lower = (a_bm->u == 0)
      , data_lower = (lower != transposed);
    wtk_mer_bandmat_solve_triangular_banded(a_bm->matf, b, transposed, data_lower);
}

void wtk_mer_bandmat_cho_solve(wtk_mer_bandmat_t *chol_bm, wtk_vecf_t *b)
{/* return b */
    int lower = chol_bm->u == 0;
    if (!lower)
    { wtk_mer_bandmat_T(chol_bm); }
    wtk_mer_bandmat_solve_triangular(chol_bm, b);
    wtk_mer_bandmat_T(chol_bm);
    wtk_mer_bandmat_solve_triangular(chol_bm, b);
}

void wtk_mer_bandmat_solveh(wtk_mer_bandmat_t *a_bm, wtk_vecf_t *b)
{/* return b */
    wtk_mer_bandmat_t *chol_bm;
    chol_bm = wtk_mer_bandmat_cholesky(a_bm, 1, 0);
    wtk_mer_bandmat_cho_solve(chol_bm, b);
    wtk_mer_bandmat_delete(chol_bm);
}
