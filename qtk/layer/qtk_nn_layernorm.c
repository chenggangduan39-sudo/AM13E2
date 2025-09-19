#include "qtk_nn_layernorm.h"
#include "tts-mer/wtk-extend/wtk_mer_source_file.h"

void qtk_nn_layernorm_alldim(wtk_matf_t *input,wtk_vecf_t *gamma,wtk_vecf_t *beta,float epsilon);
void qtk_nn_layernorm_1dim(wtk_matf_t *input,wtk_vecf_t *gamma,wtk_vecf_t *beta,float epsilon);

qtk_nn_layernorm_t* qtk_nn_layernorm_new(int len,float eps,int use_1dim)
{
    qtk_nn_layernorm_t *layer = NULL;
    layer = wtk_malloc(sizeof(qtk_nn_layernorm_t));
    memset(layer, 0, sizeof(qtk_nn_layernorm_t));

    layer->eps = eps;
    layer->gamm = wtk_vecf_new(len);
    layer->beta = wtk_vecf_new(len);
    layer->use_1dim = use_1dim;
    return layer;
}

int qtk_nn_layernorm_forward_inplace(qtk_nn_layernorm_t *layer,wtk_matf_t *in)
{
    if(layer->use_1dim){
        qtk_nn_layernorm_1dim(in,layer->gamm,layer->beta,layer->eps);
    }else{
        qtk_nn_layernorm_alldim(in,layer->gamm,layer->beta,layer->eps);
    }
    return 0;
}

int qtk_nn_layernorm_delete(qtk_nn_layernorm_t *layer)
{
    if(layer->gamm)
        wtk_vecf_delete(layer->gamm);
    if(layer->beta)
        wtk_vecf_delete(layer->beta);
    wtk_free(layer);
    return 0;
}

int qtk_nn_layernorm_load_file(qtk_nn_layernorm_t *layer, char *gamma_fn, char *beta_fn)
{
    wtk_source_loader_t sl;
    wtk_source_t source;
    
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_vecf(&sl,&source,gamma_fn,layer->gamm);
    wtk_mer_source_loader_load_vecf(&sl,&source,beta_fn,layer->beta);
    return 0;
}

/**
 * 数据均化函数
 * math:  y = \frac{x - \mathrm{E}[x]}{ \sqrt{\mathrm{Var}[x] + \epsilon}} * \gamma + \beta
 * 其中的  \mathrm{E}[x] 和 \mathrm{Var}[x] 需要在本次计算
 * pythorh 实现的时候是在最后的维度上实现的
 * [x] x 为最后维度 [1,24,31] 算[31]维度上的均值和方差
 * [x,y] 算[24,31]上的均值和方差
 * 本函数是算[x,y,z] 全部值x*y*z上的均值和方差
 */
void qtk_nn_layernorm_alldim(wtk_matf_t *input,wtk_vecf_t *gamma,wtk_vecf_t *beta,float epsilon)
{
    float E = 0.0f,V = 0.0f,t = 0.0f;
    int N = 0,i = 0,j = 0;
    float *p = 0;

    N = input->col*input->row;
    p = input->p;
    for(i = 0; i < N; ++i){
        E += p[i];
    }
    E = E / N;
    p = input->p;
    for(i = 0; i < N;++i){
        t = (p[i] - E);
        V += t*t;
    }
    V = V / N;
    V = sqrtf(V+epsilon);
    p = input->p;
    for(i = 0; i < input->row; ++i,p+=input->col){
        for(j = 0; j < input->col;++j){
            p[j] = ((p[j] - E)/V)*gamma->p[j] + beta->p[j];
        }
    }
    return;
}

void qtk_nn_layernorm_1dim(wtk_matf_t *input,wtk_vecf_t *gamma,wtk_vecf_t *beta,float epsilon)
{
    float Ep = 0.0f,Vp = 0.0f,t = 0.0f;
    int i = 0,j = 0;
    float *p = 0;
    wtk_vecf_t *E,*Var;

    E = wtk_vecf_new(input->row);
    Var = wtk_vecf_new(input->row);
    // N = input->col*input->row;
    p = input->p;
    for(i = 0; i < input->row; ++i){
        for(Ep = 0.0f,j =0;j < input->col;++j){
            Ep += p[i*input->col+j];
        }
        E->p[i] = Ep/input->col;
    }
    for(i = 0; i < input->row;++i){
        for(Vp = 0,j = 0;j<input->col;++j){
            t = p[i*input->col+j]-E->p[i];
            Vp += t*t;
        }
        Vp = Vp/input->col;
        Var->p[i] = sqrtf(Vp+epsilon);
    }
    for(i = 0; i < input->row; ++i,p+=input->col){
        for(j = 0; j < input->col;++j){
            p[j] = ((p[j] - E->p[i])/Var->p[i])*gamma->p[j] + beta->p[j];
        }
    }
    wtk_vecf_delete(E);
    wtk_vecf_delete(Var);
    return;
}
