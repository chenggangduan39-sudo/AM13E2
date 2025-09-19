#include "qtk_nn_embedding.h"
#include "tts-mer/wtk-extend/wtk_mer_source_file.h"
#include <assert.h>

qtk_nn_embedding_t *qtk_nn_embedding_new(int row,int col)
{
    qtk_nn_embedding_t *embedding = NULL;
    embedding = wtk_malloc(sizeof(*embedding));
    embedding->embedding_table = wtk_matf_new(row,col);
    
    return embedding;
}

int qtk_nn_embedding_forward(qtk_nn_embedding_t *layer,wtk_veci_t *in,wtk_matf_t *out)
{
    int tkl = in->len,*tk = in->p;
    int i = 0;
    wtk_matf_t *embedding = layer->embedding_table;
    int col = embedding->col;
    int usym = 0;

    assert(out->col == col);

    for(i = 0; i < tkl; ++i){
        usym = tk[i];
        if(usym > embedding->row || usym < 0){wtk_debug("embedding error symbel %d\n",usym);continue;}
        memcpy(out->p+col * i,embedding->p + col *usym,sizeof(float)*col);
    }
    return 0;
}

int qtk_nn_embedding_delete(qtk_nn_embedding_t *layer)
{
    wtk_matf_delete(layer->embedding_table);
    wtk_free(layer);
    return 0;
}

int qtk_nn_embedding_load_file(qtk_nn_embedding_t *layer, char *embedding_fn)
{
    wtk_source_loader_t sl;
    wtk_source_t source;
    if(embedding_fn == NULL)
        return 1;
    wtk_source_loader_init_file(&sl);
    wtk_mer_source_loader_load_matf(&sl,&source,embedding_fn,layer->embedding_table);
    return 0;
}
