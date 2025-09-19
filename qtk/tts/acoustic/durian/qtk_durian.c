#include "qtk_durian.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"

qtk_durian_t* qtk_durian_new(qtk_durian_cfg_t *cfg)
{
    qtk_durian_t *durian = NULL;
    durian = wtk_calloc(1, sizeof(*durian));
    int *dim = NULL;

    durian->heap = wtk_heap_new(4096);
    durian->cfg = cfg;
    dim = (int*)cfg->embedding_dim_num->slot;
    durian->embedding = qtk_nn_embedding_new(dim[0],dim[1]);
    qtk_nn_embedding_load_file(durian->embedding,cfg->embedding_fn);
    durian->encoder = qtk_durian_encoder_new(&cfg->encoder_cfg,durian->heap);
    durian->dp = qtk_durian_duration_predictor_new(&cfg->dp_cfg,durian->heap);
    durian->decoder = qtk_durian_decoder_new(&cfg->decoder_cfg,durian->heap);
    durian->postnet = qtk_durian_postnet_new(&cfg->postnet_cfg,durian->heap);

    return durian;
}

int qtk_durian_delete(qtk_durian_t *durian)
{
    qtk_durian_postnet_delete(durian->postnet);
    qtk_durian_decoder_delete(durian->decoder);
    qtk_durian_duration_predictor_delete(durian->dp);
    qtk_durian_encoder_delete(durian->encoder);
    qtk_nn_embedding_delete(durian->embedding);
    wtk_heap_delete(durian->heap);
    wtk_free(durian);
    return 0;
}

int qtk_durian_process(qtk_durian_t *durian,wtk_veci_t *tokens, int is_end)
{
    wtk_heap_t *heap = durian->heap;
    wtk_matf_t *embedding_out = NULL;
    wtk_matf_t *encoder_out = NULL;
    wtk_matf_t *aligned_features = NULL;
    qtk_nn_embedding_t *embedding = durian->embedding;
    wtk_matf_t *decoder_out = NULL;
    wtk_matf_t *postnet_out = NULL;

    qtk_durian_reset(durian);
    embedding_out = wtk_matf_heap_new(heap,tokens->len,embedding->embedding_table->col);
    qtk_nn_embedding_forward(embedding,tokens,embedding_out);
    encoder_out = qtk_durian_encoder_process(durian->encoder,embedding_out);
    aligned_features = qtk_durian_duration_predictor_forward(durian->dp,encoder_out);
    decoder_out = qtk_durian_decoder_process(durian->decoder,aligned_features);
    postnet_out = qtk_durian_postnet_process(durian->postnet,decoder_out);
    wtk_matf_add(decoder_out,decoder_out,postnet_out);
    
    if(durian->notify) durian->notify(durian->user_data,decoder_out,is_end);

    return 0;
}

int qtk_durian_reset(qtk_durian_t *durian)
{
    wtk_heap_reset(durian->heap);
    return 0;
}

int qtk_durian_set_notify(qtk_durian_t *durian, qtk_durian_notify_f notify, void *user_data)
{
    durian->user_data = user_data;
    durian->notify = notify;
    return 0;
}
