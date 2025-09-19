#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse.h"
#include "qtk/math/qtk_vector.h"
#include "wtk/core/wtk_alloc.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0? (f/32767.0) : (f/32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f) ((f) > 0? floorf(f*32767.0+0.5) : floorf(f*32768.0+0.5))
#endif

void wtk_cmask_pse_print_type_info(wtk_cmask_pse_t *cmask_pse)
{
#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx = cmask_pse->emb;
    wtk_strbuf_t *fbank_buf = cmask_pse->fbank_buf;
    wtk_strbuf_t *vp_real_buf = cmask_pse->vp_real_buf;
	OrtStatus *status;
	OrtTypeInfo** type_info=(OrtTypeInfo**)wtk_malloc(sizeof(OrtTypeInfo*)*onnx->num_out);
	OrtTypeInfo** type_info2=(OrtTypeInfo**)wtk_malloc(sizeof(OrtTypeInfo*)*onnx->num_in);
	OrtTensorTypeAndShapeInfo* tensor_info2=NULL;//(OrtTensorTypeAndShapeInfo**)wtk_malloc(sizeof(OrtTensorTypeAndShapeInfo*)*onnx->num_in);

	int i,j;
	int len;
    int outer_in_num = onnx->cfg->outer_in_num;
    onnx->stream_len = 0;

	onnx->in_items = (qtk_onnx_item_t*)wtk_calloc(onnx->num_in,sizeof(qtk_onnx_item_t));
	for(i=0; i<onnx->num_in; ++i)
	{
		qtk_onnx_item_t *item = onnx->in_items + i;
        if(i<outer_in_num){
            len = 1;
            //wtk_debug("%d %d\n",onnx->num_in,i);
            status = onnx->api->SessionGetInputTypeInfo(onnx->session,i,type_info2);
            status = onnx->api->CastTypeInfoToTensorInfo(*type_info2,cast(const OrtTensorTypeAndShapeInfo **, &tensor_info2));

            //wtk_debug("%p\n",tensor_info2);
            //item->name = (char*)wtk_malloc(sizeof(char*));
            status = onnx->api->SessionGetInputName(onnx->session,i,onnx->allocator,&(item->name));

            status = onnx->api->GetDimensionsCount(tensor_info2, cast(size_t *, &(item->shape_len)));
            item->shape = (int64_t*)wtk_calloc(item->shape_len,sizeof(int64_t));
            status = onnx->api->GetDimensions(tensor_info2,item->shape,item->shape_len);
            if(i==0){
                len = fbank_buf->pos/sizeof(float);
                for(j = 0; j < item->shape_len; j++){
                    if(item->shape[j] != -1){
                        len /= item->shape[j];
                    }
                }
                for(j = 0; j < item->shape_len; j++){
                    if(item->shape[j] == -1){
                        item->shape[j] = len;
                    }
                }
                len = fbank_buf->pos/sizeof(float);
            }else{
                len = vp_real_buf->pos/sizeof(float);
                for(j = 0; j < item->shape_len; j++){
                    if(item->shape[j] != -1){
                        len /= item->shape[j];
                    }
                }
                for(j = 0; j < item->shape_len; j++){
                    if(item->shape[j] == -1){
                        item->shape[j] = len;
                    }
                }
                len = vp_real_buf->pos/sizeof(float);
            }

            status = onnx->api->GetTensorElementType(tensor_info2,&(item->type));
            onnx->api->ReleaseTypeInfo(*type_info2);
            item->in_dim = len;
        }else{
            len = 1;
            //wtk_debug("%d %d\n",onnx->num_in,i);
            status = onnx->api->SessionGetInputTypeInfo(onnx->session,i,type_info2);
            status = onnx->api->CastTypeInfoToTensorInfo(*type_info2,cast(const OrtTensorTypeAndShapeInfo **, &tensor_info2));

            //wtk_debug("%p\n",tensor_info2);
            //item->name = (char*)wtk_malloc(sizeof(char*));
            status = onnx->api->SessionGetInputName(onnx->session,i,onnx->allocator,&(item->name));

            status = onnx->api->GetDimensionsCount(tensor_info2, cast(size_t *, &(item->shape_len)));
            item->shape = (int64_t*)wtk_calloc(item->shape_len,sizeof(int64_t));
            status = onnx->api->GetDimensions(tensor_info2,item->shape,item->shape_len);
            for(j = 0; j < item->shape_len; j++){
                if(item->shape[j] == -1){
                    item->shape[j] = 1;
                }
                len *= item->shape[j];
            }
            status = onnx->api->GetTensorElementType(tensor_info2,&(item->type));
            onnx->api->ReleaseTypeInfo(*type_info2);
            item->in_dim = len;
        }
        switch (item->type)
        {
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
            item->bytes = sizeof(float);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
            item->bytes = sizeof(int);
            break;
        case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
            item->bytes = sizeof(int64_t);
            break;
        default:
            break;
        }
        if(i < outer_in_num){
            item->val = (char*)wtk_calloc(item->bytes*item->in_dim,sizeof(char));
        }else{
            onnx->stream_len += item->bytes*item->in_dim;
        }
	}
    
    if(onnx->stream_len > 0){
        long unsigned int p_len = 0;
        onnx->stream_val = (char*)wtk_calloc(onnx->stream_len,sizeof(char));
        for(i=outer_in_num; i<onnx->num_in; ++i)
        {
            qtk_onnx_item_t *item = onnx->in_items + i;
            item->val = onnx->stream_val + p_len;
            p_len += item->bytes*item->in_dim;
        }
    }

	// for(i=0; i<onnx->num_out; ++i)
	// {
	// 	ONNXType type;
	// 	status = onnx->api->SessionGetOutputTypeInfo(onnx->session,i,type_info+i);
	// 	status = onnx->api->GetOnnxTypeFromTypeInfo(type_info[i],&type);
	// 	//wtk_debug("output:%d type:%d\n",i,type);
	// }
    (void)status;

	wtk_free(type_info);
	wtk_free(type_info2);
#endif
}

void wtk_cmask_pse_fbank_on(wtk_cmask_pse_t *cmask_pse, float *data, int len)
{
    wtk_strbuf_push(cmask_pse->fbank_buf, (char *)data, len * sizeof(float));
    int i;
    for(i=0; i<len; ++i){
        cmask_pse->fbank_mean[i] += data[i];
    }
    ++cmask_pse->fbank_frame;
}


wtk_cmask_pse_t *wtk_cmask_pse_new(wtk_cmask_pse_cfg_t *cfg) {
    wtk_cmask_pse_t *cmask_pse;

    cmask_pse = (wtk_cmask_pse_t *)wtk_malloc(sizeof(wtk_cmask_pse_t));
    cmask_pse->cfg = cfg;
    cmask_pse->ths = NULL;
    cmask_pse->notify = NULL;
    cmask_pse->ths2 = NULL;
    cmask_pse->notify2 = NULL;
    cmask_pse->mic = wtk_strbufs_new(cmask_pse->cfg->nmicchannel);
    cmask_pse->sp = wtk_strbufs_new(cmask_pse->cfg->nspchannel);

    cmask_pse->nbin = cfg->wins / 2 + 1;
    cmask_pse->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);  /// 2);
    cmask_pse->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins); /// 2);
    cmask_pse->analysis_mem = wtk_float_new_p2(cfg->nmicchannel, cmask_pse->nbin - 1);
    cmask_pse->analysis_mem_sp = wtk_float_new_p2(cfg->nspchannel, cmask_pse->nbin - 1);
    cmask_pse->synthesis_mem = wtk_malloc(sizeof(float) * (cmask_pse->nbin - 1));
    cmask_pse->rfft = wtk_drft_new(cfg->wins);
    cmask_pse->rfft_in = (float *)wtk_malloc(sizeof(float) * (cfg->wins));

    cmask_pse->fft = wtk_complex_new_p2(cfg->nmicchannel, cmask_pse->nbin);
    cmask_pse->fft_sp = wtk_complex_new_p2(cfg->nspchannel, cmask_pse->nbin);
    cmask_pse->fftx = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cmask_pse->nbin);
    cmask_pse->ffty = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cmask_pse->nbin);

    cmask_pse->out = wtk_malloc(sizeof(float) * (cmask_pse->nbin - 1));

#ifdef ONNX_DEC
    cmask_pse->pse = NULL;
    cmask_pse->pse_caches = NULL;
    cmask_pse->pse_out_len = NULL;
    if(cfg->use_onnx){
        cmask_pse->pse = qtk_onnxruntime_new(&(cfg->pse));
        cmask_pse->pse_caches = wtk_calloc(sizeof(OrtValue *), cmask_pse->pse->num_in - cfg->pse.outer_in_num);
        if (cmask_pse->pse->num_in - cfg->pse.outer_in_num != cmask_pse->pse->num_out - cfg->pse.outer_out_num) {
            wtk_debug("err inner_item num_in:%ld outer_in_num:%d num_out:%ld outer_out_num:%d\n",cmask_pse->pse->num_in,cmask_pse->pse->cfg->outer_in_num,cmask_pse->pse->num_out,cmask_pse->pse->cfg->outer_out_num);
            exit(0);
        }
        cmask_pse->pse_out_len = (int *)wtk_malloc(sizeof(int) * (cfg->pse.outer_out_num));
    }
#endif

    cmask_pse->emb0=NULL;
    cmask_pse->emb1=NULL;
    cmask_pse->emb2=NULL;
    cmask_pse->emb3=NULL;
    cmask_pse->gamma=NULL;
    cmask_pse->beta=NULL;
    cmask_pse->gamma1=NULL;
    cmask_pse->beta1=NULL;
    cmask_pse->gamma2=NULL;
    cmask_pse->beta2=NULL;
    cmask_pse->emb_mask=NULL;
    cmask_pse->feat=NULL;

    if(cfg->emb_mask_len > 0){
        cmask_pse->emb_mask = (float *)wtk_malloc(sizeof(float)*cfg->emb_mask_len);
    }

    if(cfg->use_sv_check){
        cmask_pse->stable_mask = (float *)wtk_malloc(sizeof(float)*cfg->emb_mask_len);
        cmask_pse->mask1 = (float *)wtk_malloc(sizeof(float)*cfg->emb_mask_len);
        cmask_pse->mask2 = (float *)wtk_malloc(sizeof(float)*cfg->emb_mask_len);
    }

    cmask_pse->c_onnx_len=cmask_pse->nbin*cfg->num_frame;
    cmask_pse->pse_in = (float *)wtk_malloc(sizeof(float) * cmask_pse->c_onnx_len * 2);
    cmask_pse->pse_out = (float *)wtk_malloc(sizeof(float) * cmask_pse->c_onnx_len * 2);
    cmask_pse->ee=NULL;
    if(cfg->use_aec_model){
        cmask_pse->ee = (float *)wtk_malloc(sizeof(float) * cmask_pse->c_onnx_len * 2);
    }
    cmask_pse->c_onnx_out = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_pse->c_onnx_len);
    cmask_pse->c_onnx_err = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_pse->c_onnx_len);
    cmask_pse->c_onnx_raw = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_pse->c_onnx_len);
    cmask_pse->c_onnx_echo = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cmask_pse->c_onnx_len);

    cmask_pse->x_phase=NULL;
    if(cfg->use_ccm){
        cmask_pse->x_phase=(float *)wtk_malloc(sizeof(float)*cmask_pse->c_onnx_len);
    }

    cmask_pse->erls3 = NULL;
    if (cfg->use_rls3) {
        cmask_pse->erls3 = wtk_malloc(sizeof(wtk_rls3_t));
        wtk_rls3_init(cmask_pse->erls3, &(cfg->echo_rls3), cmask_pse->nbin);
    }
    cmask_pse->covm=NULL;
    cmask_pse->echo_covm=NULL;
    cmask_pse->bf=NULL;
    if(cfg->use_bf){
        cmask_pse->covm=wtk_covm_new(&(cfg->covm),cmask_pse->nbin,cfg->nbfchannel);
        cmask_pse->echo_covm=wtk_covm_new(&(cfg->echo_covm),cmask_pse->nbin,cfg->nbfchannel);
        cmask_pse->bf=wtk_bf_new(&(cfg->bf),cfg->wins);
    }
	cmask_pse->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		cmask_pse->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}

    cmask_pse->eq = NULL;
    if (cfg->use_eq) {
        cmask_pse->eq = wtk_equalizer_new(&(cfg->eq));
    }
    cmask_pse->power_k = wtk_malloc(sizeof(float) * cmask_pse->nbin);
    cmask_pse->entropy_E=(float *)wtk_malloc(sizeof(float)*cmask_pse->nbin);
    cmask_pse->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->wins);

    cmask_pse->sv = NULL;
    cmask_pse->sv_base = NULL;
    cmask_pse->sv_cur = NULL;
    cmask_pse->sv_len = 0;
    cmask_pse->cache_out_wav = NULL;
    if(cfg->use_sv_check){
        cmask_pse->cache_idx = 9;
        cmask_pse->sv = wtk_cmask_sv_new(&(cfg->sv));
        cmask_pse->cache_out_wav = wtk_strbuf_new(1024,1); 
        cmask_pse->res_len = cmask_pse->cfg->num_frame * cmask_pse->cfg->wins/2;
    }

    wtk_cmask_pse_reset(cmask_pse);

    return cmask_pse;
}

void wtk_cmask_pse_reset(wtk_cmask_pse_t *cmask_pse) {
    int wins = cmask_pse->cfg->wins;
    int i;

    wtk_strbufs_reset(cmask_pse->mic, cmask_pse->cfg->nmicchannel);
    wtk_strbufs_reset(cmask_pse->sp, cmask_pse->cfg->nspchannel);

    if(cmask_pse->sv){
        cmask_pse->flag = 0;
        cmask_pse->last_flag = 0;
        cmask_pse->check_cnt = 0;
        cmask_pse->flag_mask = 0;
        cmask_pse->state_spk = 0;
        cmask_pse->counter_above_low = 0;
        cmask_pse->counter_below_low = 0;
        cmask_pse->counter_below_high = 0;
        wtk_strbuf_reset(cmask_pse->cache_out_wav);
        wtk_cmask_sv_reset(cmask_pse->sv);
        wtk_strbuf_reset(cmask_pse->sv->wav_buf);
    }

    for (i = 0; i < wins; ++i) {
        cmask_pse->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    wtk_drft_init_synthesis_window(cmask_pse->synthesis_window,
                                   cmask_pse->analysis_window, wins);

    wtk_float_zero_p2(cmask_pse->analysis_mem, cmask_pse->cfg->nmicchannel,
                      (cmask_pse->nbin - 1));
    wtk_float_zero_p2(cmask_pse->analysis_mem_sp, cmask_pse->cfg->nspchannel,
                      (cmask_pse->nbin - 1));
    memset(cmask_pse->synthesis_mem, 0, sizeof(float) * (cmask_pse->nbin - 1));

    wtk_complex_zero_p2(cmask_pse->fft, cmask_pse->cfg->nmicchannel,
                         (cmask_pse->nbin));
    wtk_complex_zero_p2(cmask_pse->fft_sp, cmask_pse->cfg->nspchannel,
                         (cmask_pse->nbin));
    memset(cmask_pse->fftx, 0, sizeof(wtk_complex_t) * (cmask_pse->nbin));
    memset(cmask_pse->ffty, 0, sizeof(wtk_complex_t) * (cmask_pse->nbin));
#ifdef ONNX_DEC
    if(cmask_pse->cfg->use_onnx){
        qtk_onnxruntime_reset(cmask_pse->pse);
        {
            int n = cmask_pse->pse->num_in - cmask_pse->pse->cfg->outer_in_num;
            if (cmask_pse->pse_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    cmask_pse->pse->api->ReleaseValue(cmask_pse->pse_caches[i]);
                }
                memset(cmask_pse->pse_caches, 0, sizeof(OrtValue *) * n);
            }
        }
        memset(cmask_pse->pse_out_len, 0, sizeof(int) * (cmask_pse->pse->cfg->outer_out_num));
    }
#endif
    if(cmask_pse->emb_mask){
        int emb_mask_len = cmask_pse->cfg->emb_mask_len;
        int num_frame = cmask_pse->cfg->num_frame;
        int emb_len1 = emb_mask_len / num_frame;
        int n1;
        if(!cmask_pse->cfg->use_sv_check){

            for(i=0;i<num_frame;++i){
                for(n1=0;n1<emb_len1-num_frame;++n1){
                    cmask_pse->emb_mask[i*emb_len1+n1]=1.0;
                }
                for(n1=emb_len1-num_frame;n1<emb_len1-num_frame+i+1;++n1){
                    cmask_pse->emb_mask[i*emb_len1+n1]=0.0;
                }
                for(n1=emb_len1-num_frame+i+1;n1<emb_len1;++n1){
                    cmask_pse->emb_mask[i*emb_len1+n1]=1.0;
                }
            }
        }else{
            float *mask1;
            int rows = num_frame;
            int cols = 96;

            mask1 = cmask_pse->stable_mask;
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    if (j < 32) {
                        *(mask1 + i*cols + j) = (j <= i - 1) ? 1 : 0;
                    } else if (j < 64) {
                        *(mask1 + i*cols + j) = 0;
                    } else {
                        int j_second = j - 64;
                        *(mask1 + i*cols + j) = (j_second > i) ? 1 : 0;
                    }
                }
            }

            mask1 = cmask_pse->mask1;
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < 64; j++) {
                    *(mask1 + i*cols + j) = 1;
                }
            }
            for (int i = 0; i < rows; i++) {
                for (int j = 64; j < cols; j++) {
                    int j_second = j - 64;
                    if (j_second > i) {
                        *(mask1 + i*cols + j) = 1;
                    } else {
                        *(mask1 + i*cols + j) = 0;
                    }
                }
            }

            mask1 = cmask_pse->mask2;
            for (int i = 0; i < rows; i++) {
                for (int j = 0; j < cols; j++) {
                    if (j < 32) { 
                        *(mask1 + i*cols + j) = 1;
                    } else if (j < 64) { 
                        *(mask1 + i*cols + j) = 0;
                    } else { 
                        int j_second = j - 64;
                        *(mask1 + i*cols + j) = (j_second > i) ? 1 : 0;
                    }
                }
            }
            *cmask_pse->stable_mask = 1;
        }
    }
    memset(cmask_pse->pse_in, 0, sizeof(float) * cmask_pse->c_onnx_len * 2);
    memset(cmask_pse->pse_out, 0, sizeof(float) * cmask_pse->c_onnx_len * 2);
    if(cmask_pse->ee){
        memset(cmask_pse->ee, 0, sizeof(float) * cmask_pse->c_onnx_len * 2);
    }
    memset(cmask_pse->c_onnx_out, 0,
        sizeof(wtk_complex_t) * cmask_pse->c_onnx_len);
    memset(cmask_pse->c_onnx_err, 0,
            sizeof(wtk_complex_t) * cmask_pse->c_onnx_len);
    memset(cmask_pse->c_onnx_raw, 0,
            sizeof(wtk_complex_t) * cmask_pse->c_onnx_len);
    memset(cmask_pse->c_onnx_echo, 0,
            sizeof(wtk_complex_t) * cmask_pse->c_onnx_len);
    if(cmask_pse->x_phase){
        memset(cmask_pse->x_phase, 0, sizeof(float)*cmask_pse->c_onnx_len);
    }
    if (cmask_pse->erls3) {
        wtk_rls3_reset(cmask_pse->erls3, cmask_pse->nbin);
    }
    if(cmask_pse->covm){
        wtk_covm_reset(cmask_pse->covm);
    }
    if(cmask_pse->echo_covm){
        wtk_covm_reset(cmask_pse->echo_covm);
    }
    if(cmask_pse->bf){
        wtk_bf_reset(cmask_pse->bf);
    }
    if(cmask_pse->qmmse){
        wtk_qmmse_reset(cmask_pse->qmmse);
    }
    if (cmask_pse->eq) {
        wtk_equalizer_reset(cmask_pse->eq);
    }
    memset(cmask_pse->power_k, 0, sizeof(float) * cmask_pse->nbin);
    memset(cmask_pse->entropy_E, 0, sizeof(float) * cmask_pse->nbin);
    memset(cmask_pse->entropy_Eb, 0, sizeof(float) * cmask_pse->cfg->wins);

    cmask_pse->sp_silcnt = 0;
    cmask_pse->sp_sil = 1;
    cmask_pse->mic_silcnt = 0;
    cmask_pse->mic_sil = 1;

    cmask_pse->entropy_in_cnt = 0;
    cmask_pse->entropy_silcnt = 0;
    cmask_pse->entropy_sil = 1;
    cmask_pse->entropy_sp_in_cnt = 0;
    cmask_pse->entropy_sp_silcnt = 0;
    cmask_pse->entropy_sp_sil = 1;
    cmask_pse->bs_scale = 1.0;
    cmask_pse->bs_last_scale = 1.0;
    cmask_pse->bs_max_cnt = 0;

    cmask_pse->eng_cnt = 0;
    cmask_pse->eng_times = 0;

    cmask_pse->feat_len = cmask_pse->cfg->emb0_len + \
        cmask_pse->cfg->emb1_len + \
        cmask_pse->cfg->emb2_len + \
        cmask_pse->cfg->emb3_len + \
        cmask_pse->cfg->gamma_len + \
        cmask_pse->cfg->beta_len + \
        cmask_pse->cfg->gamma1_len + \
        cmask_pse->cfg->beta1_len + \
        cmask_pse->cfg->gamma2_len + \
        cmask_pse->cfg->beta2_len;
    cmask_pse->emb_mask_len = cmask_pse->cfg->emb_mask_len/cmask_pse->cfg->num_frame/cmask_pse->cfg->num_frame;
    cmask_pse->nframe = 0;
    cmask_pse->feed_frame = 0;
    cmask_pse->frame_pos = 0;
}

void wtk_cmask_pse_delete(wtk_cmask_pse_t *cmask_pse) {
    wtk_strbufs_delete(cmask_pse->mic, cmask_pse->cfg->nmicchannel);
    wtk_strbufs_delete(cmask_pse->sp, cmask_pse->cfg->nspchannel);

    wtk_free(cmask_pse->analysis_window);
    wtk_free(cmask_pse->synthesis_window);
    wtk_float_delete_p2(cmask_pse->analysis_mem, cmask_pse->cfg->nmicchannel);
    wtk_float_delete_p2(cmask_pse->analysis_mem_sp, cmask_pse->cfg->nspchannel);
    wtk_free(cmask_pse->synthesis_mem);
    wtk_free(cmask_pse->rfft_in);
    wtk_drft_delete(cmask_pse->rfft);
    wtk_complex_delete_p2(cmask_pse->fft, cmask_pse->cfg->nmicchannel);
    wtk_complex_delete_p2(cmask_pse->fft_sp, cmask_pse->cfg->nspchannel);

    wtk_free(cmask_pse->fftx);
    wtk_free(cmask_pse->ffty);

    wtk_free(cmask_pse->out);
    if(cmask_pse->sv){
        wtk_free(cmask_pse->stable_mask);
        wtk_free(cmask_pse->mask1);
        wtk_free(cmask_pse->mask2);
        wtk_cmask_sv_delete(cmask_pse->sv);
        wtk_strbuf_delete(cmask_pse->cache_out_wav);
        if(cmask_pse->sv_base){
            wtk_free(cmask_pse->sv_base);
        }
        if(cmask_pse->sv_cur){
            wtk_free(cmask_pse->sv_cur);
        }
    }
#ifdef ONNX_DEC
    if(cmask_pse->cfg->use_onnx){
        {
            int n = cmask_pse->pse->num_in - cmask_pse->pse->cfg->outer_in_num;
            if (cmask_pse->pse_caches[0]) {
                int i;
                for (i = 0; i < n; i++) {
                    cmask_pse->pse->api->ReleaseValue(cmask_pse->pse_caches[i]);
                }
            }
        }
        if (cmask_pse->pse) {
            qtk_onnxruntime_delete(cmask_pse->pse);
        }
        wtk_free(cmask_pse->pse_out_len);
        wtk_free(cmask_pse->pse_caches);
    }
#endif
    if(cmask_pse->emb0){
        wtk_free(cmask_pse->emb0);
    }
    if(cmask_pse->emb1){
        wtk_free(cmask_pse->emb1);
    }
    if(cmask_pse->emb2){
        wtk_free(cmask_pse->emb2);
    }
    if(cmask_pse->emb3){
        wtk_free(cmask_pse->emb3);
    }
    if(cmask_pse->gamma){
        wtk_free(cmask_pse->gamma);
    }
    if(cmask_pse->beta){
        wtk_free(cmask_pse->beta);
    }
    if(cmask_pse->gamma1){
        wtk_free(cmask_pse->gamma1);
    }
    if(cmask_pse->beta1){
        wtk_free(cmask_pse->beta1);
    }
    if(cmask_pse->gamma2){
        wtk_free(cmask_pse->gamma2);
    }
    if(cmask_pse->beta2){
        wtk_free(cmask_pse->beta2);
    }
    if(cmask_pse->emb_mask){
        wtk_free(cmask_pse->emb_mask);
    }
    if(cmask_pse->feat){
        wtk_free(cmask_pse->feat);
    }

    wtk_free(cmask_pse->pse_in);
    wtk_free(cmask_pse->pse_out);
    if(cmask_pse->ee){
        wtk_free(cmask_pse->ee);
    }
    wtk_free(cmask_pse->c_onnx_out);
    wtk_free(cmask_pse->c_onnx_err);
    wtk_free(cmask_pse->c_onnx_raw);
    wtk_free(cmask_pse->c_onnx_echo);

    if(cmask_pse->x_phase){
        wtk_free(cmask_pse->x_phase);
    }
    if (cmask_pse->erls3) {
        wtk_rls3_delete(cmask_pse->erls3);
    }
    if(cmask_pse->covm){
        wtk_covm_delete(cmask_pse->covm);
    }
    if(cmask_pse->echo_covm){
        wtk_covm_delete(cmask_pse->echo_covm);
    }
    if(cmask_pse->bf){
        wtk_bf_delete(cmask_pse->bf);
    }
    if(cmask_pse->qmmse){
        wtk_qmmse_delete(cmask_pse->qmmse);
    }
    if (cmask_pse->eq) {
        wtk_equalizer_delete(cmask_pse->eq);
    }
    wtk_free(cmask_pse->power_k);
    wtk_free(cmask_pse->entropy_E);
    wtk_free(cmask_pse->entropy_Eb);

    wtk_free(cmask_pse);
}

void wtk_cmask_pse_start(wtk_cmask_pse_t *cmask_pse) {
}

void wtk_cmask_pse_set_notify(wtk_cmask_pse_t *cmask_pse, void *ths, wtk_cmask_pse_notify_f notify) {
    cmask_pse->notify = notify;
    cmask_pse->ths = ths;
}


void wtk_cmask_pse_new_vp(wtk_cmask_pse_t *cmask_pse)
{
#ifdef ONNX_DEC
    cmask_pse->emb = NULL;
    cmask_pse->emb_caches = NULL;
    cmask_pse->emb_out_len = NULL;
    if(cmask_pse->cfg->use_onnx){
        cmask_pse->emb = qtk_onnxruntime_new(&(cmask_pse->cfg->emb));
        // cmask_pse->emb_caches = wtk_calloc(sizeof(OrtValue *), cmask_pse->emb->num_in - cmask_pse->cfg->emb.outer_in_num);
        if (cmask_pse->emb->num_in - cmask_pse->cfg->emb.outer_in_num != cmask_pse->emb->num_out - cmask_pse->cfg->emb.outer_out_num) {
            wtk_debug("err inner_item\n");
            exit(0);
        }
        cmask_pse->emb_out_len = (int *)wtk_malloc(sizeof(int) * (cmask_pse->cfg->emb.outer_out_num));
    }
#endif

    cmask_pse->fbank = wtk_fbank_new(&(cmask_pse->cfg->fbank));
    cmask_pse->fbank_len = cmask_pse->cfg->fbank.num_fbank;
    cmask_pse->fbank_mean = (float *)wtk_malloc(sizeof(float) * cmask_pse->fbank_len);
    cmask_pse->fbank_buf = wtk_strbuf_new(80, 1);
    cmask_pse->vp_real_buf = wtk_strbuf_new(1024, 1);
    cmask_pse->vp_imag_buf = wtk_strbuf_new(1024, 1);
    wtk_fbank_set_notify(cmask_pse->fbank, cmask_pse, (wtk_fbank_notify_f)wtk_cmask_pse_fbank_on);

    wtk_fbank_reset(cmask_pse->fbank);
    memset(cmask_pse->fbank_mean, 0, sizeof(float) * cmask_pse->fbank_len);
    wtk_strbuf_reset(cmask_pse->fbank_buf);
    wtk_strbuf_reset(cmask_pse->vp_real_buf);
    wtk_strbuf_reset(cmask_pse->vp_imag_buf);
    cmask_pse->fbank_frame = 0;
}

void wtk_cmask_pse_reset_vp(wtk_cmask_pse_t *cmask_pse)
{
#ifdef ONNX_DEC
    if(cmask_pse->cfg->use_onnx){
        qtk_onnxruntime_reset(cmask_pse->emb);
        // {
        //     int n = cmask_pse->emb->num_in - cmask_pse->emb->cfg->outer_in_num;
        //     if (cmask_pse->emb_caches[0]) {
        //         int i;
        //         for (i = 0; i < n; i++) {
        //             cmask_pse->emb->api->ReleaseValue(cmask_pse->emb_caches[i]);
        //         }
        //         memset(cmask_pse->emb_caches, 0, sizeof(OrtValue *) * n);
        //     }
        // }
        memset(cmask_pse->emb_out_len, 0, sizeof(int) * (cmask_pse->emb->cfg->outer_out_num));

    }
#endif
}

void wtk_cmask_pse_cache_reset(wtk_cmask_pse_t *cmask_pse){
#ifdef ONNX_DEC
    qtk_onnxruntime_t *pse = cmask_pse->pse;
    qtk_onnx_item_t *item;
    int i;
    for(i = cmask_pse->cache_idx; i < pse->num_in; i++){
        item = pse->in_items + i;
        memset(item->val, 0, item->bytes * item->in_dim);
    }
#endif
}

void  wtk_cmask_pse_info_clean(wtk_cmask_pse_t *cmask_pse)
{
#ifdef ONNX_DEC
	int i;
    qtk_onnxruntime_t *onnx = cmask_pse->emb;
	qtk_onnx_item_t *item;

	for(i=0; i<onnx->num_in; ++i)
	{
		item = onnx->in_items + i;
        if(i < onnx->cfg->outer_in_num){
            wtk_free(item->val);
        }
        if(item->name != NULL){
			wtk_free(item->name);
        }
		wtk_free(item->shape);
	}
	wtk_free(onnx->in_items);
#endif
}

void wtk_cmask_pse_delete_vp(wtk_cmask_pse_t *cmask_pse)
{
#ifdef ONNX_DEC
    if(cmask_pse->cfg->use_onnx){
        // {
        //     int n = cmask_pse->emb->num_in - cmask_pse->emb->cfg->outer_in_num;
        //     if (cmask_pse->emb_caches[0]) {
        //         int i;
        //         for (i = 0; i < n; i++) {
        //             cmask_pse->emb->api->ReleaseValue(cmask_pse->emb_caches[i]);
        //         }
        //     }
        // }
        if (cmask_pse->emb) {
            wtk_cmask_pse_info_clean(cmask_pse);
            qtk_onnxruntime_delete(cmask_pse->emb);
        }
        wtk_free(cmask_pse->emb_out_len);
        wtk_free(cmask_pse->emb_caches);
    }
#endif
    wtk_fbank_delete(cmask_pse->fbank);
    wtk_free(cmask_pse->fbank_mean);
    wtk_strbuf_delete(cmask_pse->fbank_buf);
    wtk_strbuf_delete(cmask_pse->vp_real_buf);
    wtk_strbuf_delete(cmask_pse->vp_imag_buf);
}
void wtk_cmask_pse_start_vp(wtk_cmask_pse_t *cmask_pse)
{
}

void wtk_cmask_pse_set_notify2(wtk_cmask_pse_t *cmask_pse, void *ths, wtk_cmask_pse_notify_f2 notify) {
    cmask_pse->notify2 = notify;
    cmask_pse->ths2 = ths;
}

void wtk_cmask_pse_feed_emb(wtk_cmask_pse_t *cmask_pse)
{
#ifdef ONNX_DEC
    wtk_strbuf_t *fbank_buf = cmask_pse->fbank_buf;
    wtk_strbuf_t *vp_real_buf = cmask_pse->vp_real_buf;
    int i, j;
    const OrtApi *api = cmask_pse->emb->api;
    OrtMemoryInfo *meminfo = cmask_pse->emb->meminfo;
    qtk_onnxruntime_t *emb = cmask_pse->emb;
    OrtStatus *status;
    int num_in = emb->num_in;
    int outer_in_num = emb->cfg->outer_in_num;
    int outer_out_num = emb->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    void *onnx_out;
    int feat_state;
    int emb0_state, emb1_state, emb2_state, emb3_state;
    int gamma_state, beta_state, gamma1_state, beta1_state, gamma2_state, beta2_state;
    int tmp_len;
    // int64_t size = 0, *out_shape;

    for (i = 0; i < outer_in_num; ++i) {
        item = emb->in_items + i;
        if (i == 0) {
            memcpy(item->val, fbank_buf->data, item->bytes * item->in_dim);
        } else if (i == 1) {
            memcpy(item->val, vp_real_buf->data, item->bytes * item->in_dim);
        }
    }

    // printf("num_in:\n");
    for (i = 0; i < num_in; ++i) {
        item = emb->in_items + i;
        status = api->CreateTensorWithDataAsOrtValue(
            meminfo, item->val, item->bytes * item->in_dim, item->shape,
            item->shape_len, item->type, emb->in + i);
        // printf("%d\n", i);
        // for(j=0;j<item->shape_len;++j){
        // 	printf("%d %ld\n", j, item->shape[j]);
        // }
        // printf("%ld\n", item->bytes*item->in_dim/sizeof(float));
    }

    status = api->Run(emb->session, NULL,
                        cast(const char *const *, emb->name_in),
                        cast(const OrtValue *const *, emb->in), emb->num_in,
                        cast(const char *const *, emb->name_out),
                        emb->num_out, emb->out);

    cmask_pse->feat_len = 0;
    emb0_state = emb1_state = emb2_state = emb3_state = -1;
    gamma_state = beta_state = gamma1_state = beta1_state = gamma2_state = beta2_state = -1;
    for(j=0;j<outer_out_num;++j){
        feat_state = -1;
        if(cmask_pse->emb_out_len[j]==0){
            int64_t size = 0, *out_shape;
            int d_len;
            d_len = 1;
            out_shape = qtk_onnxruntime_get_outshape(emb, j, &size);
            for(int k=0;k<size;++k){
                d_len *= out_shape[k];
            }
            wtk_free(out_shape);
            cmask_pse->emb_out_len[j] = d_len;
        }
        if(!cmask_pse->emb0 && cmask_pse->cfg->emb0_len > 0 && feat_state == -1){
            cmask_pse->emb0 = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("emb0_init:\n");
        }
        if(!cmask_pse->emb1 && cmask_pse->cfg->emb1_len > 0 && feat_state == -1){
            cmask_pse->emb1 = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("emb1_init:\n");
        }
        if(!cmask_pse->emb2 && cmask_pse->cfg->emb2_len > 0 && feat_state == -1){
            cmask_pse->emb2 = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("emb2_init:\n");
        }
        if(!cmask_pse->emb3 && cmask_pse->cfg->emb3_len > 0 && feat_state == -1){
            cmask_pse->emb3 = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("emb3_init:\n");
        }
        if(!cmask_pse->gamma && cmask_pse->cfg->gamma_len > 0 && feat_state == -1){
            cmask_pse->gamma = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("gamma_init:\n");
        }
        if(!cmask_pse->beta && cmask_pse->cfg->beta_len > 0 && feat_state == -1){
            cmask_pse->beta = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("beta_init:\n");
        }
        if(!cmask_pse->gamma1 && cmask_pse->cfg->gamma1_len > 0 && feat_state == -1){
            cmask_pse->gamma1 = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("gamma1_init:\n");
        }
        if(!cmask_pse->beta1 && cmask_pse->cfg->beta1_len > 0 && feat_state == -1){
            cmask_pse->beta1 = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("beta1_init:\n");
        }
        if(!cmask_pse->gamma2 && cmask_pse->cfg->gamma2_len > 0 && feat_state == -1){
            cmask_pse->gamma2 = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("gamma2_init:\n");
        }
        if(!cmask_pse->beta2 && cmask_pse->cfg->beta2_len > 0 && feat_state == -1){
            cmask_pse->beta2 = (float *)wtk_malloc(sizeof(float)*cmask_pse->emb_out_len[j]);
            feat_state = j;
            // printf("beta2_init:\n");
        }
        onnx_out = qtk_onnxruntime_getout(emb, j);
        feat_state = -1;
        // printf("%d %d %d %d %d %d %d %d %d %d %d\n", feat_state, emb0_state, emb1_state, emb2_state, emb3_state, gamma_state, beta_state, gamma1_state, beta1_state, gamma2_state, beta2_state);
        if(cmask_pse->cfg->emb0_len > 0 && feat_state == -1 && emb0_state == -1){
            memcpy(cmask_pse->emb0, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            emb0_state = j;
            // printf("emb0:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->emb0[k]);
            // }
        }
        if(cmask_pse->cfg->emb1_len > 0 && feat_state == -1 && emb1_state == -1){
            memcpy(cmask_pse->emb1, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            emb1_state = j;
            // printf("emb1:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->emb1[k]);
            // }
        }
        if(cmask_pse->cfg->emb2_len > 0 && feat_state == -1 && emb2_state == -1){
            memcpy(cmask_pse->emb2, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            emb2_state = j;
            // printf("emb2:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->emb2[k]);
            // }
        }
        if(cmask_pse->cfg->emb3_len > 0 && feat_state == -1 && emb3_state == -1){
            memcpy(cmask_pse->emb3, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            emb3_state = j;
            // printf("emb3:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->emb3[k]);
            // }
        }
        if(cmask_pse->cfg->gamma_len > 0 && feat_state == -1 && gamma_state == -1){
            memcpy(cmask_pse->gamma, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            gamma_state = j;
            // printf("gamma:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->gamma[k]);
            // }
        }
        if(cmask_pse->cfg->beta_len > 0 && feat_state == -1 && beta_state == -1){
            memcpy(cmask_pse->beta, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            beta_state = j;
            // printf("beta:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->beta[k]);
            // }
        }
        if(cmask_pse->cfg->gamma1_len > 0 && feat_state == -1 && gamma1_state == -1){
            memcpy(cmask_pse->gamma1, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            gamma1_state = j;
            // printf("gamma1:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->gamma1[k]);
            // }
        }
        if(cmask_pse->cfg->beta1_len > 0 && feat_state == -1 && beta1_state == -1){
            memcpy(cmask_pse->beta1, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            beta1_state = j;
            // printf("beta1:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->beta1[k]);
            // }
        }
        if(cmask_pse->cfg->gamma2_len > 0 && feat_state == -1 && gamma2_state == -1){
            memcpy(cmask_pse->gamma2, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            gamma2_state = j;
            // printf("gamma2:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->gamma2[k]);
            // }
        }
        if(cmask_pse->cfg->beta2_len > 0 && feat_state == -1 && beta2_state == -1){
            memcpy(cmask_pse->beta2, onnx_out, cmask_pse->emb_out_len[j]*sizeof(float));
            feat_state = j;
            beta2_state = j;
            // printf("beta2:\n");
            // for(int k=0;k<cmask_pse->emb_out_len[j];++k){
            //     printf("%d %f\n", k, cmask_pse->beta2[k]);
            // }
        }
        cmask_pse->feat_len += cmask_pse->emb_out_len[j];
    }
    if(!cmask_pse->feat){
        if(cmask_pse->sv){
            cmask_pse->feat = (float *)wtk_malloc(sizeof(float)*(cmask_pse->feat_len + cmask_pse->sv_len));
        }else{
            cmask_pse->feat = (float *)wtk_malloc(sizeof(float)*cmask_pse->feat_len);
        }
    }
    tmp_len = 0;
    if(cmask_pse->emb0){
        memcpy(cmask_pse->feat, cmask_pse->emb0, cmask_pse->cfg->emb0_len*sizeof(float));
        tmp_len += cmask_pse->cfg->emb0_len;
    }
    if(cmask_pse->emb1){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->emb1, cmask_pse->cfg->emb1_len*sizeof(float));
        tmp_len += cmask_pse->cfg->emb1_len;
    }
    if(cmask_pse->emb2){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->emb2, cmask_pse->cfg->emb2_len*sizeof(float));
        tmp_len += cmask_pse->cfg->emb2_len;
    }
    if(cmask_pse->emb3){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->emb3, cmask_pse->cfg->emb3_len*sizeof(float));
        tmp_len += cmask_pse->cfg->emb3_len;
    }
    if(cmask_pse->gamma){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->gamma, cmask_pse->cfg->gamma_len*sizeof(float));
        tmp_len += cmask_pse->cfg->gamma_len;
    }
    if(cmask_pse->beta){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->beta, cmask_pse->cfg->beta_len*sizeof(float));
        tmp_len += cmask_pse->cfg->beta_len;
    }
    if(cmask_pse->gamma1){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->gamma1, cmask_pse->cfg->gamma1_len*sizeof(float));
        tmp_len += cmask_pse->cfg->gamma1_len;
    }
    if(cmask_pse->beta1){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->beta1, cmask_pse->cfg->beta1_len*sizeof(float));
        tmp_len += cmask_pse->cfg->beta1_len;
    }
    if(cmask_pse->gamma2){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->gamma2, cmask_pse->cfg->gamma2_len*sizeof(float));
        tmp_len += cmask_pse->cfg->gamma2_len;
    }
    if(cmask_pse->beta2){
        memcpy(cmask_pse->feat+tmp_len, cmask_pse->beta2, cmask_pse->cfg->beta2_len*sizeof(float));
        tmp_len += cmask_pse->cfg->beta2_len;
    }

    for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
        item = emb->in_items + i;
        onnx_out = qtk_onnxruntime_getout(emb, j);
        memcpy(item->val, onnx_out, item->bytes * item->in_dim);
    }
    qtk_onnxruntime_reset(emb);
    (void)status;
#endif
}

void wtk_cmask_pse_ccm(wtk_cmask_pse_t *cmask_pse, float *x_mag_unfold, float *x_phase, float *m_mag, float *m_real, float *m_imag)
{
    int nbin = cmask_pse->nbin;
    int i, n;
    float mag;
    float mag_mask;
    float phase_mask;
    float tmp, tmp1;
    // float scale = 64.0;
    float scale = 1.0;
    wtk_complex_t *c_onnx_out = cmask_pse->c_onnx_out;
    int num_frame = cmask_pse->cfg->num_frame;

    for(n=0;n<num_frame;++n){
        mag = 0;
        for(i=0;i<nbin;++i)
        {
            // mag = x_mag_unfold[i]*m_mag[i]+x_mag_unfold[i+nbin]*m_mag[i+nbin]+x_mag_unfold[i+2*nbin]*m_mag[i+2*nbin];
            // mag_mask = sqrtf(m_real[i]*m_real[i] + m_imag[i]*m_imag[i]);
            // phase_mask = atan2f(m_imag[i], m_real[i]);
            mag = x_mag_unfold[i+n*nbin]*m_mag[i+n*nbin]+x_mag_unfold[i+(num_frame+n)*nbin]*m_mag[i+(num_frame+n)*nbin]+x_mag_unfold[i+(num_frame*2+n)*nbin]*m_mag[i+(num_frame*2+n)*nbin];
            mag_mask = sqrtf(m_real[i+n*nbin]*m_real[i+n*nbin] + m_imag[i+n*nbin]*m_imag[i+n*nbin]);
            phase_mask = atan2f(m_imag[i+n*nbin], m_real[i+n*nbin]);
            // real = mag * tanhf(mag_mask) * cosf(x_phase[i]+phase_mask);
            // imag = mag * tanhf(mag_mask) * sinf(x_phase[i]+phase_mask);
            // fftx[i].a = real*scale;
            // fftx[i].b = imag*scale;

            // tmp = x_phase[i]+phase_mask;
            // tmp1 = mag * tanhf(mag_mask)*scale;
            // pse_out[i] = tmp1 * cosf(tmp);
            // pse_out[i+nbin] = tmp1 * sinf(tmp);
            tmp = x_phase[i+n*nbin]+phase_mask;
            tmp1 = mag * tanhf(mag_mask)*scale;
            c_onnx_out[i+n*nbin].a = tmp1 * cosf(tmp);
            c_onnx_out[i+n*nbin].b = tmp1 * sinf(tmp);
        }
    }
}

void wtk_cmask_pse_feed_pse(wtk_cmask_pse_t *cmask_pse, wtk_complex_t *fftx, wtk_complex_t *ffty, wtk_complex_t *fft, wtk_complex_t *fft_sp)
{
#ifdef ONNX_DEC
    int i, j;
    int nbin=cmask_pse->nbin;
    float *emb0=cmask_pse->emb0;
    float *emb1=cmask_pse->emb1;
    float *emb2=cmask_pse->emb2;
    float *emb3=cmask_pse->emb3;
    float *gamma=cmask_pse->gamma;
    float *beta=cmask_pse->beta;
    float *gamma1=cmask_pse->gamma1;
    float *beta1=cmask_pse->beta1;
    float *gamma2=cmask_pse->gamma2;
    float *beta2=cmask_pse->beta2;
    float *emb_mask=cmask_pse->emb_mask;
    if(cmask_pse->sv){
        if(cmask_pse->flag_mask == 0){
            emb_mask=cmask_pse->mask1;
        }else if(cmask_pse->flag_mask == 1){
            emb_mask=cmask_pse->mask2;
        }else{
            emb_mask=cmask_pse->stable_mask;
        }
    }
    float *x_mag_unfold=NULL;
    float *m_mag=NULL;
    float *m_real=NULL;
    float *m_imag=NULL;
    float *x_phase=cmask_pse->x_phase;
    const OrtApi *api = cmask_pse->pse->api;
    OrtMemoryInfo *meminfo = cmask_pse->pse->meminfo;
    qtk_onnxruntime_t *pse = cmask_pse->pse;
    OrtStatus *status;
    int num_in = pse->num_in;
    int outer_in_num = pse->cfg->outer_in_num;
    int outer_out_num = pse->cfg->outer_out_num;
    qtk_onnx_item_t *item;
    void *onnx_out;
    int feat_state;
    int emb_mask_state;
    int emb0_state, emb1_state, emb2_state, emb3_state;
    int gamma_state, beta_state, gamma1_state, beta1_state, gamma2_state, beta2_state;
    // int64_t size = 0, *out_shape;
    // int step=0;
    int num_frame = cmask_pse->cfg->num_frame;
    int pos = cmask_pse->frame_pos;
    float *pse_in = cmask_pse->pse_in;
    float *pse_out = cmask_pse->pse_out;
    float *ee = cmask_pse->ee;
    wtk_complex_t *c_onnx_out = cmask_pse->c_onnx_out;
    wtk_complex_t *c_onnx_err = cmask_pse->c_onnx_err;
    wtk_complex_t *c_onnx_raw = cmask_pse->c_onnx_raw;
    wtk_complex_t *c_onnx_echo = cmask_pse->c_onnx_echo;
    int emb_mask_len = cmask_pse->cfg->emb_mask_len;
    int emb_len1 = emb_mask_len / num_frame;
    int emb_len2 = emb_mask_len / num_frame / num_frame;
    int n1, n2;

    cmask_pse->feed_frame++;
    if(x_phase){
        for (i = 0; i < nbin; ++i) {
            x_phase[pos+i] = atan2f(fftx[i].b, fftx[i].a);
        }
    }

    for (i = 0; i < nbin; ++i) {
        pse_in[pos + i] = fftx[i].a;
        pse_in[pos + i + nbin * num_frame] = fftx[i].b;
        c_onnx_raw[pos + i].a = fft[i].a;
        c_onnx_raw[pos + i].b = fft[i].b;
    }
    if(ee){
        for (i = 0; i < nbin; ++i) {
            ee[pos + i] = ffty[i].a;
            ee[pos + i + nbin * num_frame] = ffty[i].b;
            c_onnx_err[pos + i].a = ffty[i].a;
            c_onnx_err[pos + i].b = ffty[i].b;
            c_onnx_echo[pos + i].a = fft_sp[i].a;
            c_onnx_echo[pos + i].b = fft_sp[i].b;
        }
    }
    cmask_pse->frame_pos += nbin;
    if (cmask_pse->feed_frame >= num_frame) {
        emb_mask_state = -1;
        emb0_state = emb1_state = emb2_state = emb3_state = -1;
        gamma_state = beta_state = gamma1_state = beta1_state = gamma2_state = beta2_state = -1;
        for (i = 0; i < outer_in_num; ++i) {
            feat_state = -1;
            item = pse->in_items + i;
            if(ee){
                if (i == 0) {
                    memcpy(item->val, pse_in, item->bytes * item->in_dim);
                }else if (i == 1) {
                    memcpy(item->val, ee, item->bytes * item->in_dim);
                }else{
                    if(emb_mask && feat_state == -1 && emb_mask_state == -1){
                        memcpy(item->val, emb_mask, item->bytes * item->in_dim);
                        feat_state = i;
                        emb_mask_state = i;
                        if(cmask_pse->emb_mask_len>0){
                            if(cmask_pse->emb_mask_len<=emb_len2 && cmask_pse->emb_mask_len>2){
                                for(n1=0;n1<num_frame;++n1){
                                    for(n2=(cmask_pse->emb_mask_len-2)*num_frame;n2<(cmask_pse->emb_mask_len-1)*num_frame;++n2){
                                        emb_mask[n1*emb_len1+n2]=0;
                                    }
                                }
                            }else if(cmask_pse->emb_mask_len==2){
                                for(n1=0;n1<num_frame;++n1){
                                    for(n2=n1;n2<num_frame;++n2){
                                        emb_mask[n1*emb_len1+n2]=0;
                                    }
                                }
                            }
                            --cmask_pse->emb_mask_len;
                        }
                    }
                    if(emb0 && feat_state == -1 && emb0_state == -1){
                        memcpy(item->val, emb0, item->bytes * item->in_dim);
                        feat_state = i;
                        emb0_state = i;
                    }
                    if(emb1 && feat_state == -1 && emb1_state == -1){
                        memcpy(item->val, emb1, item->bytes * item->in_dim);
                        feat_state = i;
                        emb1_state = i;
                    }
                    if(emb2 && feat_state == -1 && emb2_state == -1){
                        memcpy(item->val, emb2, item->bytes * item->in_dim);
                        feat_state = i;
                        emb2_state = i;
                    }
                    if(emb3 && feat_state == -1 && emb3_state == -1){
                        memcpy(item->val, emb3, item->bytes * item->in_dim);
                        feat_state = i;
                        emb3_state = i;
                    }
                    if(gamma && feat_state == -1 && gamma_state == -1){
                        memcpy(item->val, gamma, item->bytes * item->in_dim);
                        feat_state = i;
                        gamma_state = i;
                    }
                    if(beta && feat_state == -1 && beta_state == -1){
                        memcpy(item->val, beta, item->bytes * item->in_dim);
                        feat_state = i;
                        beta_state = i;
                    }
                    if(gamma1 && feat_state == -1 && gamma1_state == -1){
                        memcpy(item->val, gamma1, item->bytes * item->in_dim);
                        feat_state = i;
                        gamma1_state = i;
                    }
                    if(beta1 && feat_state == -1 && beta1_state == -1){
                        memcpy(item->val, beta1, item->bytes * item->in_dim);
                        feat_state = i;
                        beta1_state = i;
                    }
                    if(gamma2 && feat_state == -1 && gamma2_state == -1){
                        memcpy(item->val, gamma2, item->bytes * item->in_dim);
                        feat_state = i;
                        gamma2_state = i;
                    }
                    if(beta2 && feat_state == -1 && beta2_state == -1){
                        memcpy(item->val, beta2, item->bytes * item->in_dim);
                        feat_state = i;
                        beta2_state = i;
                    }
                }
            }else{
                if (i == 0) {
                    memcpy(item->val, pse_in, item->bytes * item->in_dim);
                }else{
                    if(emb_mask && feat_state == -1 && emb_mask_state == -1){
                        memcpy(item->val, emb_mask, item->bytes * item->in_dim);
                        feat_state = i;
                        emb_mask_state = i;
                        // if(cmask_pse->emb_mask_len>0){
                        //     for(n1=0;n1<num_frame;++n1){
                        //         for(n2=0;n2<emb_len1;++n2){
                        //             printf("%f ", emb_mask[n1*emb_len1+n2]);
                        //         }
                        //         printf("\n");
                        //     }
                        //     getchar();
                        // }
                        if(cmask_pse->emb_mask_len>0){
                            if(cmask_pse->emb_mask_len<=emb_len2 && cmask_pse->emb_mask_len>2){
                                for(n1=0;n1<num_frame;++n1){
                                    for(n2=(cmask_pse->emb_mask_len-2)*num_frame;n2<(cmask_pse->emb_mask_len-1)*num_frame;++n2){
                                        emb_mask[n1*emb_len1+n2]=0;
                                    }
                                }
                            }else if(cmask_pse->emb_mask_len==2){
                                if(cmask_pse->cfg->emb_mask_type==0){
                                    for(n1=0;n1<num_frame;++n1){
                                        for(n2=n1;n2<num_frame;++n2){
                                            emb_mask[n1*emb_len1+n2]=0;
                                        }
                                    }
                                }else if(cmask_pse->cfg->emb_mask_type==1){
                                    for(n1=0;n1<num_frame;++n1){
                                        for(n2=n1+1;n2<num_frame;++n2){
                                            emb_mask[n1*emb_len1+n2]=0;
                                        }
                                    }
                                }else if(cmask_pse->cfg->emb_mask_type==3){
                                    for(n1=0;n1<num_frame;++n1){
                                        for(n2=n1;n2<num_frame;++n2){
                                            emb_mask[n1*emb_len1+n2]=0;
                                        }
                                    }
                                    emb_mask[0] = 1.0;
                                }
                            }
                            --cmask_pse->emb_mask_len;
                        }
                    }
                    if(emb0 && feat_state == -1 && emb0_state == -1){
                        memcpy(item->val, emb0, item->bytes * item->in_dim);
                        feat_state = i;
                        emb0_state = i;
                    }
                    if(emb1 && feat_state == -1 && emb1_state == -1){
                        memcpy(item->val, emb1, item->bytes * item->in_dim);
                        feat_state = i;
                        emb1_state = i;
                    }
                    if(emb2 && feat_state == -1 && emb2_state == -1){
                        memcpy(item->val, emb2, item->bytes * item->in_dim);
                        feat_state = i;
                        emb2_state = i;
                    }
                    if(emb3 && feat_state == -1 && emb3_state == -1){
                        memcpy(item->val, emb3, item->bytes * item->in_dim);
                        feat_state = i;
                        emb3_state = i;
                    }
                    if(gamma && feat_state == -1 && gamma_state == -1){
                        memcpy(item->val, gamma, item->bytes * item->in_dim);
                        feat_state = i;
                        gamma_state = i;
                    }
                    if(beta && feat_state == -1 && beta_state == -1){
                        memcpy(item->val, beta, item->bytes * item->in_dim);
                        feat_state = i;
                        beta_state = i;
                    }
                    if(gamma1 && feat_state == -1 && gamma1_state == -1){
                        memcpy(item->val, gamma1, item->bytes * item->in_dim);
                        feat_state = i;
                        gamma1_state = i;
                    }
                    if(beta1 && feat_state == -1 && beta1_state == -1){
                        memcpy(item->val, beta1, item->bytes * item->in_dim);
                        feat_state = i;
                        beta1_state = i;
                    }
                    if(gamma2 && feat_state == -1 && gamma2_state == -1){
                        memcpy(item->val, gamma2, item->bytes * item->in_dim);
                        feat_state = i;
                        gamma2_state = i;
                    }
                    if(beta2 && feat_state == -1 && beta2_state == -1){
                        memcpy(item->val, beta2, item->bytes * item->in_dim);
                        feat_state = i;
                        beta2_state = i;
                    }
                }
            }
        }

        // printf("num_in:\n");
        for (i = 0; i < num_in; ++i) {
            item = pse->in_items + i;
            status = api->CreateTensorWithDataAsOrtValue(
                meminfo, item->val, item->bytes * item->in_dim, item->shape,
                item->shape_len, item->type, pse->in + i);
            // printf("%d\n", i);
            // for(j=0;j<item->shape_len;++j){
            // 	printf("%d %ld\n", j, item->shape[j]);
            // }
            // printf("%ld\n", item->bytes*item->in_dim/sizeof(float));
        }

        status = api->Run(pse->session, NULL,
                            cast(const char *const *, pse->name_in),
                            cast(const OrtValue *const *, pse->in), pse->num_in,
                            cast(const char *const *, pse->name_out),
                            pse->num_out, pse->out);

        if(cmask_pse->cfg->use_ccm){
            x_mag_unfold = qtk_onnxruntime_getout(pse, 0);
            m_mag = qtk_onnxruntime_getout(pse, 1);
            m_real = qtk_onnxruntime_getout(pse, 2);
            m_imag = qtk_onnxruntime_getout(pse, 3);
            wtk_cmask_pse_ccm(cmask_pse, x_mag_unfold, x_phase, m_mag, m_real, m_imag);
        }else{
            for(j=0;j<outer_out_num;++j){
                if(cmask_pse->pse_out_len[j]==0){
                    int64_t size = 0, *out_shape;
                    int d_len;
                    d_len = 1;
                    out_shape = qtk_onnxruntime_get_outshape(pse, j, &size);
                    for(int k=0;k<size;++k){
                        d_len *= out_shape[k];
                    }
                    wtk_free(out_shape);
                    cmask_pse->pse_out_len[j] = d_len;
                }
                onnx_out = qtk_onnxruntime_getout(pse, j);
                if(j==0){
                    memcpy(pse_out, onnx_out, cmask_pse->pse_out_len[j]*sizeof(float));
                    for(i=0;i<cmask_pse->pse_out_len[j]/2;++i){
                        c_onnx_out[i].a = pse_out[i];
                        c_onnx_out[i].b = pse_out[i+cmask_pse->pse_out_len[j]/2];
                    }
                }
            }
        }
        for (i = outer_in_num, j = outer_out_num; i < num_in; ++i, ++j) {
            item = pse->in_items + i;
            onnx_out = qtk_onnxruntime_getout(pse, j);
            memcpy(item->val, onnx_out, item->bytes * item->in_dim);
        }
        qtk_onnxruntime_reset(pse);
        (void)status;
        cmask_pse->feed_frame = 0;
        cmask_pse->frame_pos = 0;
    }
#endif
}

void wtk_cmask_pse_start_vp_feat(wtk_cmask_pse_t *cmask_pse, float *feat, int len)
{
    if(feat){
        wtk_cmask_pse_reset(cmask_pse);
        int emb0_len=cmask_pse->cfg->emb0_len;
        int emb1_len=cmask_pse->cfg->emb1_len;
        int emb2_len=cmask_pse->cfg->emb2_len;
        int emb3_len=cmask_pse->cfg->emb3_len;
        int gamma_len=cmask_pse->cfg->gamma_len;
        int beta_len=cmask_pse->cfg->beta_len;
        int gamma1_len=cmask_pse->cfg->gamma1_len;
        int beta1_len=cmask_pse->cfg->beta1_len;
        int gamma2_len=cmask_pse->cfg->gamma2_len;
        int beta2_len=cmask_pse->cfg->beta2_len;
        int tmp_len=0;
        if(cmask_pse->emb0==NULL && emb0_len>0){
            cmask_pse->emb0 = (float *)wtk_malloc(sizeof(float)*emb0_len);
        }
        if(cmask_pse->emb1==NULL && emb1_len>0){
            cmask_pse->emb1 = (float *)wtk_malloc(sizeof(float)*emb1_len);
        }
        if(cmask_pse->emb2==NULL && emb2_len>0){
            cmask_pse->emb2 = (float *)wtk_malloc(sizeof(float)*emb2_len);
        }
        if(cmask_pse->emb3==NULL && emb3_len>0){
            cmask_pse->emb3 = (float *)wtk_malloc(sizeof(float)*emb3_len);
        }
        if(cmask_pse->gamma==NULL && gamma_len>0){
            cmask_pse->gamma = (float *)wtk_malloc(sizeof(float)*gamma_len);
        }
        if(cmask_pse->beta==NULL && beta_len>0){
            cmask_pse->beta = (float *)wtk_malloc(sizeof(float)*beta_len);
        }
        if(cmask_pse->gamma1==NULL && gamma1_len>0){
            cmask_pse->gamma1 = (float *)wtk_malloc(sizeof(float)*gamma1_len);
        }
        if(cmask_pse->beta1==NULL && beta1_len>0){
            cmask_pse->beta1 = (float *)wtk_malloc(sizeof(float)*beta1_len);
        }
        if(cmask_pse->gamma2==NULL && gamma2_len>0){
            cmask_pse->gamma2 = (float *)wtk_malloc(sizeof(float)*gamma2_len);
        }
        if(cmask_pse->beta2==NULL && beta2_len>0){
            cmask_pse->beta2 = (float *)wtk_malloc(sizeof(float)*beta2_len);
        }
        if(cmask_pse->emb0){
            memcpy(cmask_pse->emb0, feat+tmp_len, emb0_len*sizeof(float));
            tmp_len += emb0_len;
        }
        if(cmask_pse->emb1){
            memcpy(cmask_pse->emb1, feat+tmp_len, emb1_len*sizeof(float));
            tmp_len += emb1_len;
        }
        if(cmask_pse->emb2){
            memcpy(cmask_pse->emb2, feat+tmp_len, emb2_len*sizeof(float));
            tmp_len += emb2_len;
        }
        if(cmask_pse->emb3){
            memcpy(cmask_pse->emb3, feat+tmp_len, emb3_len*sizeof(float));
            tmp_len += emb3_len;
        }
        if(cmask_pse->gamma){
            memcpy(cmask_pse->gamma, feat+tmp_len, gamma_len*sizeof(float));
            tmp_len += gamma_len;
        }
        if(cmask_pse->beta){
            memcpy(cmask_pse->beta, feat+tmp_len, beta_len*sizeof(float));
            tmp_len += beta_len;
        }
        if(cmask_pse->gamma1){
            memcpy(cmask_pse->gamma1, feat+tmp_len, gamma1_len*sizeof(float));
            tmp_len += gamma1_len;
        }
        if(cmask_pse->beta1){
            memcpy(cmask_pse->beta1, feat+tmp_len, beta1_len*sizeof(float));
            tmp_len += beta1_len;
        }
        if(cmask_pse->gamma2){
            memcpy(cmask_pse->gamma2, feat+tmp_len, gamma2_len*sizeof(float));
            tmp_len += gamma2_len;
        }
        if(cmask_pse->beta2){
            memcpy(cmask_pse->beta2, feat+tmp_len, beta2_len*sizeof(float));
            tmp_len += beta2_len;
        }
        cmask_pse->feat_len = tmp_len;
        if(cmask_pse->cfg->use_sv_check){
            cmask_pse->sv_len = len - tmp_len;
            if(!cmask_pse->sv_base){
                cmask_pse->sv_base = (float*)wtk_malloc(sizeof(float)*cmask_pse->sv_len);
            }
            memcpy(cmask_pse->sv_base, feat+tmp_len, sizeof(float) * cmask_pse->sv_len);
        }
    }else{

    }
}

void wtk_cmask_pse_feed_vp(wtk_cmask_pse_t *cmask_pse, short *data, int len, int is_end)
{
    wtk_strbuf_t *mic=cmask_pse->mic[0];
    wtk_strbuf_t *vp_real_buf=cmask_pse->vp_real_buf;
    wtk_strbuf_t *vp_imag_buf=cmask_pse->vp_imag_buf;
    wtk_complex_t *fft = cmask_pse->fft[0];
    wtk_drft_t *rfft = cmask_pse->rfft;
    float *rfft_in = cmask_pse->rfft_in;
    float *analysis_mem = cmask_pse->analysis_mem[0];
    float *analysis_window = cmask_pse->analysis_window;
    int nbin = cmask_pse->nbin;
    float fv, fv1;
    int i;
    int length;
    int wins = cmask_pse->cfg->wins;
    int fsize = wins / 2;

    if(cmask_pse->sv){
        //wtk_cmask_sv_feed(cmask_pse->sv, data, len, is_end);
        wtk_cmask_sv_feed_cycle(cmask_pse->sv, data, len, is_end);
        if(is_end){
            if(cmask_pse->sv->output){
                if(!cmask_pse->sv_base && cmask_pse->sv->output_len != 0){
                    cmask_pse->sv_len = cmask_pse->sv->output_len;
                    cmask_pse->sv_base = wtk_malloc(sizeof(float)*cmask_pse->sv->output_len);
                }
                memcpy(cmask_pse->sv_base, cmask_pse->sv->output, cmask_pse->sv->output_len*sizeof(float));
                // if(cmask_pse->notify3){
                //     cmask_pse->notify3(cmask_pse->ths3, cmask_pse->sv_base, cmask_pse->sv_len);
                // }
            }
            // else{
            //     cmask_pse->notify3(cmask_pse->ths3, NULL, 0);
            // }
            wtk_cmask_sv_reset(cmask_pse->sv);
        }
    }

    if(is_end){
        if(cmask_pse->fbank_frame==0){
            wtk_debug("error need feed data first\n");
            exit(0);
        }
        int pos=0;
        float *fbank;
        for(i=0; i<cmask_pse->fbank_len; ++i){
            cmask_pse->fbank_mean[i] /= cmask_pse->fbank_frame;
        }
        length = cmask_pse->fbank_buf->pos/sizeof(float);
        while(length>pos){
            fbank = (float *)(cmask_pse->fbank_buf->data+pos*sizeof(float));
            for(i=0;i<cmask_pse->fbank_len;++i){
                fbank[i] -= cmask_pse->fbank_mean[i];
            }
            pos += cmask_pse->fbank_len;
        }
        wtk_strbuf_push(vp_real_buf, vp_imag_buf->data, vp_imag_buf->pos);
        wtk_cmask_pse_print_type_info(cmask_pse);
        wtk_cmask_pse_reset_vp(cmask_pse);
        wtk_cmask_pse_feed_emb(cmask_pse);

        if(cmask_pse->notify2){
            if(cmask_pse->sv && cmask_pse->sv_base){
                memcpy(cmask_pse->feat + cmask_pse->feat_len,cmask_pse->sv_base, cmask_pse->sv_len * sizeof(float));
                cmask_pse->notify2(cmask_pse->ths2, cmask_pse->feat, cmask_pse->feat_len + cmask_pse->sv_len);
            }else{
                cmask_pse->notify2(cmask_pse->ths2, cmask_pse->feat, cmask_pse->feat_len);
            }
        }
        wtk_strbuf_reset(mic);
    }else{
        wtk_fbank_feed(cmask_pse->fbank, data, len, is_end);

        for(i=0;i<len;++i)
        {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[i]);
            wtk_strbuf_push(mic,(char *)&(fv),sizeof(float));
        }
        length = mic->pos/sizeof(float);
        while(length>=fsize){
            wtk_drft_stft(rfft, rfft_in, analysis_mem, fft, (float *)(mic->data), wins, analysis_window);
            for(i=0;i<nbin;++i){
                fv = fft[i].a;
                fv1 = fft[i].b;
                wtk_strbuf_push(vp_real_buf,(char *)&(fv),sizeof(float));
                wtk_strbuf_push(vp_imag_buf,(char *)&(fv1),sizeof(float));
            }

            wtk_strbuf_pop(mic, NULL, fsize*sizeof(float));
            length = mic->pos/sizeof(float);
        }
    }
}

void wtk_cmask_pse_feed_aec(wtk_cmask_pse_t *cmask_pse, wtk_complex_t *fft,
                             wtk_complex_t *fft_sp) {
    int nspchannel = cmask_pse->cfg->nspchannel;
    int k;
    int nbin = cmask_pse->nbin;
    wtk_rls3_t *erls3 = cmask_pse->erls3;
    wtk_complex_t *fftx = cmask_pse->fftx;
    wtk_complex_t *ffty = cmask_pse->ffty;
    int num_frame = cmask_pse->cfg->num_frame;
    int pos = cmask_pse->frame_pos;

    if(nspchannel==0){
        memcpy(fftx, fft, nbin * sizeof(wtk_complex_t));
        memset(ffty, 0, nbin * sizeof(wtk_complex_t));
    }else if (erls3) {
        wtk_rls3_feed3(erls3, fft, fft_sp, cmask_pse->sp_sil==0, nbin);
        if (cmask_pse->sp_sil == 0) {
            memcpy(fftx, erls3->out, nbin * sizeof(wtk_complex_t));
            memcpy(ffty, erls3->lsty, nbin * sizeof(wtk_complex_t));
        } else {
            memcpy(fftx, fft, nbin * sizeof(wtk_complex_t));
            memset(ffty, 0, nbin * sizeof(wtk_complex_t));
        }
    }else {
        for (k = 0; k < nbin; ++k) {
            fftx[k] = fft[k];
            ffty[k] = fft_sp[k];
        }
    }
    // for(i=0;i<nbin;++i){
    //     pse_in[i] = fftx[i].a;
    //     pse_in[nbin+i] = fftx[i].b;
    //     ee[i] = ffty[i].a;
    //     ee[nbin+i] = ffty[i].b;
    // }

    if(cmask_pse->cfg->use_onnx){
        wtk_cmask_pse_feed_pse(cmask_pse, fftx, ffty, fft, fft_sp);
    }else{
        cmask_pse->feed_frame++;
        memcpy(cmask_pse->c_onnx_out+pos, fftx, nbin*sizeof(wtk_complex_t));
        memcpy(cmask_pse->c_onnx_err+pos, ffty, nbin * sizeof(wtk_complex_t));
        memcpy(cmask_pse->c_onnx_raw+pos, fft, nbin * sizeof(wtk_complex_t));
        memcpy(cmask_pse->c_onnx_echo+pos, fft_sp, nbin * sizeof(wtk_complex_t));
        cmask_pse->frame_pos += nbin;
        if (cmask_pse->feed_frame >= num_frame) {
            cmask_pse->feed_frame = 0;
            cmask_pse->frame_pos = 0;
        }
    }
}

static float wtk_cmask_pse_sp_energy(float *p, int n) {
    float f, f2;
    int i;

    f = 0;
    for (i = 0; i < n; ++i) {
        f += WTK_WAV_FLOAT_TO_SHORT(p[i]);
    }
    f /= n;

    f2 = 0;
    for (i = 0; i < n; ++i) {
        f2 += (WTK_WAV_FLOAT_TO_SHORT(p[i]) - f) * (WTK_WAV_FLOAT_TO_SHORT(p[i]) - f);
    }
    f2 /= n;

    return f2;
}

static float wtk_cmask_pse_fft_energy(wtk_complex_t *fftx, int nbin) {
    return qtk_vector_cpx_mag_squared_sum(fftx + 1, nbin - 2);
}

float wtk_cmask_pse_entropy(wtk_cmask_pse_t *cmask_pse, wtk_complex_t *fftx)
{
    int rate = cmask_pse->cfg->rate;
    int wins = cmask_pse->cfg->wins;
    int i;
    int fx1 = (250*1.0*wins)/rate;
    int fx2 = (3500*1.0*wins)/rate;
    int km = floor(wins*1.0/8);
    float K = 0.5;
    float *E=cmask_pse->entropy_E;
    float P1;
    float *Eb=cmask_pse->entropy_Eb;
    float sum;
    float prob;
    float Hb;

    memset(E, 0, sizeof(float) * cmask_pse->nbin);
    memset(Eb, 0, sizeof(float) * wins);
    qtk_vector_cpx_mag_squared(fftx + fx1, E + fx1, fx2 - fx1);
    sum = 1e-10;
    for(i=fx1;i<fx2;++i)
    {
        sum += E[i];
    }
    for(i=fx1;i<fx2;++i)
    {
        P1 = E[i]/sum;
        if(P1>=0.9){
            E[i] = 0;
        }
    }
    sum = 0;
    for(i=0;i<km;++i)
    {
        Eb[i] = K;
        Eb[i] += E[i*4]+E[i*4+1]+E[i*4+2]+E[i*4+3];
        sum += Eb[i];
    }
    Hb = 0;
    for(i=0;i<wins;++i)
    {
        prob = Eb[i]/sum;
        Hb += -prob*logf(prob+1e-10);
    }
    // printf("%f\n", Hb);

    return Hb;
}

void wtk_cmask_pse_feed_bf(wtk_cmask_pse_t *cmask_pse, wtk_complex_t *fft, wtk_complex_t *fft_sp)
{
	int k;
	wtk_bf_t *bf=cmask_pse->bf;
	wtk_covm_t *covm;
	int b;
	wtk_complex_t fft2;
	wtk_complex_t ffts;
	wtk_complex_t ffty;
	int clip_s=cmask_pse->cfg->clip_s;
	int clip_e=cmask_pse->cfg->clip_e;
	int bf_clip_s=cmask_pse->cfg->bf_clip_s;
	int bf_clip_e=cmask_pse->cfg->bf_clip_e;

    for(k=clip_s+1; k<clip_e; ++k)
    {
        if(cmask_pse->sp_sil==0)
        {
            if(k>=bf_clip_s && k<bf_clip_e){
                bf->cfg->mu=cmask_pse->cfg->echo_bfmu;
            }else{
                bf->cfg->mu=cmask_pse->cfg->echo_bfmu2;
            }
            if(cmask_pse->cfg->use_echocovm){
                covm = cmask_pse->echo_covm;
            }else{
                covm = cmask_pse->covm;
            }
        }else
        {
            if(k>=bf_clip_s && k<bf_clip_e){
                bf->cfg->mu=cmask_pse->cfg->bfmu;
            }else{
                bf->cfg->mu=cmask_pse->cfg->bfmu2;
            }
            covm = cmask_pse->covm;
        }
        ffts.a=fft[k].a;
        ffts.b=fft[k].b;

        if(cmask_pse->cfg->use_echo_bf){
            ffty.a=fft_sp[k].a;
            ffty.b=fft_sp[k].b;
        }else{
            ffty.a=0;
            ffty.b=0;
        }
        fft2.a=ffts.a;
        fft2.b=ffts.b;

        b=0;
        b=wtk_covm_feed_fft3(covm, &ffty, k, 1);
        if(b==1)
        {
            wtk_bf_update_ncov(bf, covm->ncov, k);
        }
        if(covm->scov)
        {
            b=wtk_covm_feed_fft3(covm, &ffts, k, 0);
            if(b==1)
            {
                wtk_bf_update_scov(bf, covm->scov, k);
            }
        }
        if(b==1)
        {
            wtk_bf_update_w(bf, k);
        }
        wtk_bf_output_fft_k(bf, &fft2, fft+k, k);
	}
}


void wtk_cmask_pse_notify_data(wtk_cmask_pse_t *cmask_pse, wtk_complex_t *fftx, wtk_complex_t *ffty, wtk_complex_t *fft)
{
    int i;
    int nbin = cmask_pse->nbin;
    int clip_s = cmask_pse->cfg->clip_s;
    int clip_e = cmask_pse->cfg->clip_e;
    float entropy=0;
    float entropy_thresh = cmask_pse->cfg->entropy_thresh;
    float entropy_sp_thresh = cmask_pse->cfg->entropy_sp_thresh;
    int entropy_cnt = cmask_pse->cfg->entropy_cnt;
    
    float raw_alpha1 = cmask_pse->cfg->raw_alpha1;
    float raw_alpha2 = cmask_pse->cfg->raw_alpha2;
    float raw_alpha3 = cmask_pse->cfg->raw_alpha3;
    float raw_alpha4 = cmask_pse->cfg->raw_alpha4;
    float raw_alpha;
    float raw_alpha_1;
    float *power_k = cmask_pse->power_k;
    float raw_alphak;
    float raw_alphak_1;
    float alpha1 = cmask_pse->cfg->alpha1;
    float alpha2 = cmask_pse->cfg->alpha2;
    float alpha3 = cmask_pse->cfg->alpha3;
    float alpha4 = cmask_pse->cfg->alpha4;
    float scale1 = cmask_pse->cfg->scale1;
    float scale2 = cmask_pse->cfg->scale2;
    float scale3 = cmask_pse->cfg->scale3;
    float scale4 = cmask_pse->cfg->scale4;
    float micenr;
    float micenr_thresh = cmask_pse->cfg->micenr_thresh;
    int micenr_cnt = cmask_pse->cfg->micenr_cnt;
    float de_eng_sum=0;
    int de_clip_s=cmask_pse->cfg->de_clip_s;
    int de_clip_e=cmask_pse->cfg->de_clip_e;
    float de_thresh=cmask_pse->cfg->de_thresh;
    float de_alpha=cmask_pse->cfg->de_alpha;
    float de_alpha2;
    float entropy_scale;

    if(entropy_thresh>0 || entropy_sp_thresh>0){
        entropy=wtk_cmask_pse_entropy(cmask_pse, fftx);
    }
    if(cmask_pse->cfg->eng_scale!=1.0){
        float eng_1=0;
        float eng_2=0;
        float freq=cmask_pse->cfg->eng_freq;
        int freq_idx=floor(nbin/(8000/freq));
        float eng1_thresh;
        float eng2_thresh;
        if(cmask_pse->sp_sil){
            eng1_thresh=cmask_pse->cfg->eng1_thresh;
            eng2_thresh=cmask_pse->cfg->eng2_thresh;
        }else{
            eng1_thresh=cmask_pse->cfg->eng1_thresh2;
            eng2_thresh=cmask_pse->cfg->eng2_thresh2;
        }
        for(i=0;i<freq_idx;++i){
            eng_1+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
        }
        for(i=freq_idx;i<nbin;++i){
            eng_2+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
        }
        eng_1 = eng_1/(eng_2+1e-9);
        if(eng_1>eng1_thresh || eng_2 < eng2_thresh){
            --cmask_pse->eng_cnt;
            if(cmask_pse->eng_times==1 || cmask_pse->eng_cnt<=0){
                cmask_pse->eng_cnt=0;
                cmask_pse->eng_times = 0;
            }
        }else{
            cmask_pse->eng_cnt=cmask_pse->cfg->eng_cnt;
            cmask_pse->eng_times += 1;
        }
        if(cmask_pse->eng_cnt <= 0){
            scale1 = cmask_pse->cfg->eng_scale;
            scale2 = cmask_pse->cfg->eng_scale;
            scale3 = cmask_pse->cfg->eng_scale;
            scale4 = cmask_pse->cfg->eng_scale;
        }
    }
    if(entropy_thresh>0){
        if(entropy<entropy_thresh){
            ++cmask_pse->entropy_in_cnt;
        }else{
            cmask_pse->entropy_in_cnt = 0;
        }
        if(cmask_pse->entropy_in_cnt>=cmask_pse->cfg->entropy_in_cnt){
            cmask_pse->entropy_sil = 0;
            cmask_pse->entropy_silcnt = entropy_cnt;
        }else if(cmask_pse->entropy_sil==0){
            cmask_pse->entropy_silcnt -= 1;
            if(cmask_pse->entropy_silcnt<=0){
                cmask_pse->entropy_sil = 1;
            }
        }
    }
    if(entropy_sp_thresh>0){
        if(entropy<entropy_sp_thresh){
            ++cmask_pse->entropy_sp_in_cnt;
        }else{
            cmask_pse->entropy_sp_in_cnt = 0;
        }
        if(cmask_pse->entropy_sp_in_cnt>=cmask_pse->cfg->entropy_in_cnt){
            cmask_pse->entropy_sp_sil = 0;
            cmask_pse->entropy_sp_silcnt = entropy_cnt;
        }else if(cmask_pse->entropy_sp_sil==0){
            cmask_pse->entropy_sp_silcnt -= 1;
            if(cmask_pse->entropy_sp_silcnt<=0){
                cmask_pse->entropy_sp_sil = 1;
            }
        }
    }
    if(raw_alpha1!=1 || raw_alpha2!=1 || raw_alpha3!=1 || raw_alpha4!=1){
        if(cmask_pse->sp_sil==0){
            if(cmask_pse->entropy_sp_sil==0){
                raw_alpha = raw_alpha1;
                raw_alpha_1 = 1.0 - raw_alpha;
            }else{
                raw_alpha = raw_alpha2;
                raw_alpha_1 = 1.0 - raw_alpha;
            }
        }else{
            if(cmask_pse->entropy_sil==0){
                raw_alpha = raw_alpha3;
                raw_alpha_1 = 1.0 - raw_alpha;
            }else{
                raw_alpha = raw_alpha4;
                raw_alpha_1 = 1.0 - raw_alpha;
            }
        }
        if(raw_alpha!=1){
            for(i=0;i<nbin;++i){
                raw_alphak_1 = power_k[i] * raw_alpha_1;
                raw_alphak = 1.0 - raw_alphak_1;
                fftx[i].a = raw_alphak * fftx[i].a + raw_alphak_1 * fft[i].a;
                fftx[i].b = raw_alphak * fftx[i].b + raw_alphak_1 * fft[i].b;
            }
        }
    }
    // printf("%d\n", cmask_pse->sp_sil);
    // printf("%d\n", cmask_pse->entropy_sil);
    // printf("%d\n", cmask_pse->entropy_sp_sil);
    // printf("%f\n", entropy);
    if(cmask_pse->cfg->use_bf){
        wtk_cmask_pse_feed_bf(cmask_pse, fftx, ffty);
    }
    if(cmask_pse->qmmse){
        if(cmask_pse->sp_sil==0){  // 有回声
            if(cmask_pse->entropy_sp_sil==0){  // 双讲
                cmask_pse->qmmse->cfg->io_alpha = alpha1;
            }else{  // 单讲
                cmask_pse->qmmse->cfg->io_alpha = alpha2;
            }
        }else{  // 无回声语音
            if(cmask_pse->entropy_sil==0){
                cmask_pse->qmmse->cfg->io_alpha = alpha3;
            }else{  // 纯噪声
                cmask_pse->qmmse->cfg->io_alpha = alpha4;
            }
        }
        if(cmask_pse->cfg->mic_max_smooth_gain==-1){
            cmask_pse->cfg->mic_max_smooth_gain = cmask_pse->qmmse->cfg->max_smooth_gain;
            cmask_pse->cfg->mic_min_smooth_gain = cmask_pse->qmmse->cfg->min_smooth_gain;
        }
        if(cmask_pse->cfg->sp_max_smooth_gain!=-1){
            if(cmask_pse->sp_sil){
                cmask_pse->qmmse->cfg->max_smooth_gain = cmask_pse->cfg->sp_max_smooth_gain;
                cmask_pse->qmmse->cfg->min_smooth_gain = cmask_pse->cfg->sp_min_smooth_gain;
            }else{
                cmask_pse->qmmse->cfg->max_smooth_gain = cmask_pse->cfg->mic_max_smooth_gain;
                cmask_pse->qmmse->cfg->min_smooth_gain = cmask_pse->cfg->mic_min_smooth_gain;
            }
        }
        // if(cmask_pse->sp_sil==0){
        //     wtk_qmmse_feed_echo_denoise3(cmask_pse->qmmse, fftx, ffty);
        // }else{
        wtk_qmmse_denoise(cmask_pse->qmmse, fftx);
        // }
        if(cmask_pse->sp_sil==0){
            if(cmask_pse->entropy_sp_sil==0){
                for(i=0;i<nbin;++i){
                    fftx[i].a *= scale1;
                    fftx[i].b *= scale1;
                }
            }else{
                for(i=0;i<nbin;++i){
                    fftx[i].a *= scale2;
                    fftx[i].b *= scale2;
                }
            }
        }else{
            if(cmask_pse->entropy_sil==0){
                for(i=0;i<nbin;++i){
                    fftx[i].a *= scale3;
                    fftx[i].b *= scale3;
                }
            }else{
                for(i=0;i<nbin;++i){
                    fftx[i].a *= scale4;
                    fftx[i].b *= scale4;
                }
            }
        }
    }
    for (i = 0; i <= clip_s; ++i) {
        fftx[i].a = fftx[i].b = cmask_pse->cfg->init_low_freq;
    }
    for (i = clip_e; i < nbin; ++i) {
        fftx[i].a = fftx[i].b = cmask_pse->cfg->init_high_freq;
    }

    // static int cnt=0;
    // cnt++;
    micenr = wtk_cmask_pse_fft_energy(fftx, nbin);
    if (micenr > micenr_thresh) {
        // if(cmask_pse->mic_sil==1)
        // {
        // 	printf("sp start %f %f
        // %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
        // }
        cmask_pse->mic_sil = 0;
        cmask_pse->mic_silcnt = micenr_cnt;
    } else if (cmask_pse->mic_sil == 0) {
        cmask_pse->mic_silcnt -= 1;
        if (cmask_pse->mic_silcnt <= 0) {
            // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
            cmask_pse->mic_sil = 1;
        }
    }

    if(de_alpha!=1.0){
        for(i=de_clip_s;i<de_clip_e;++i){
            de_eng_sum+=fftx[i].a*fftx[i].a+fftx[i].b*fftx[i].b;
        }
        // printf("%f\n", logf(de_eng_sum+1e-9));
        if(de_eng_sum>de_thresh){
            float de_pow_ratio = cmask_pse->cfg->de_pow_ratio;
            float de_mul_ratio = cmask_pse->cfg->de_mul_ratio;
            float alpha;
            de_alpha2 = de_alpha * min(max(0.1, 5.0/logf(de_eng_sum+1e-9)), 1.0);
            for(i=de_clip_s;i<de_clip_e;++i){
                alpha = (de_mul_ratio - de_alpha2) * (pow(de_clip_e-i, de_pow_ratio))/pow((nbin-de_clip_s), de_pow_ratio) + de_alpha2;
                // fftx[i].a*=de_alpha2;
                // fftx[i].b*=de_alpha2;
                fftx[i].a*=alpha;
                fftx[i].b*=alpha;
            }
        }
    }
    if(entropy_thresh>0){
        if(entropy>entropy_thresh){
            if(cmask_pse->entropy_silcnt > 0){
                entropy_scale = powf(cmask_pse->entropy_silcnt * 1.0/entropy_cnt, cmask_pse->cfg->entropy_ratio)+cmask_pse->cfg->entropy_min_scale;
            }else{
                entropy_scale = powf(1.0/entropy_cnt, cmask_pse->cfg->entropy_ratio)+cmask_pse->cfg->entropy_min_scale;
            }
            entropy_scale = min(entropy_scale, 1.0);
            for(i=0;i<nbin;++i){
                fftx[i].a*=entropy_scale;
                fftx[i].b*=entropy_scale;
            }
        }
    }
}

void wtk_cmask_pse_feed_cnon(wtk_cmask_pse_t *cmask_pse, wtk_complex_t *fft) {
    int nbin = cmask_pse->nbin;
    float sym = cmask_pse->cfg->sym;
    static float fx = 2.0f * PI / RAND_MAX;
    float f, f2;
    int i;

    for (i = 1; i < nbin - 1; ++i) {
        f = rand() * fx;
        f2 = 1.f;
        if (f2 > 0) {
            // f2=sqrtf(f2);
            fft[i].a += sym * cosf(f) * f2;
            fft[i].b += sym * sinf(f) * f2;
        }
    }
}

void wtk_cmask_pse_control_bs(wtk_cmask_pse_t *cmask_pse, float *out, int len) {
    float out_max;
    int i;

    if (cmask_pse->mic_sil == 0) {
        out_max = wtk_float_abs_max(out, len);
        if (out_max > cmask_pse->cfg->max_bs_out) {
            cmask_pse->bs_scale = cmask_pse->cfg->max_bs_out / out_max;
            if (cmask_pse->bs_scale < cmask_pse->bs_last_scale) {
                cmask_pse->bs_last_scale = cmask_pse->bs_scale;
            } else {
                cmask_pse->bs_scale = cmask_pse->bs_last_scale;
            }
            cmask_pse->bs_max_cnt = 5;
        }
        for (i = 0; i < len; ++i) {
            out[i] *= cmask_pse->bs_scale;
        }
        if (cmask_pse->bs_max_cnt > 0) {
            --cmask_pse->bs_max_cnt;
        }
        if (cmask_pse->bs_max_cnt <= 0 && cmask_pse->bs_scale < 1.0) {
            cmask_pse->bs_scale *= 1.1f;
            cmask_pse->bs_last_scale = cmask_pse->bs_scale;
            if (cmask_pse->bs_scale > 1.0) {
                cmask_pse->bs_scale = 1.0;
                cmask_pse->bs_last_scale = 1.0;
            }
        }
    } else {
        cmask_pse->bs_scale = 1.0;
        cmask_pse->bs_last_scale = 1.0;
        cmask_pse->bs_max_cnt = 0;
    }
}

void wtk_cmask_pse_feed(wtk_cmask_pse_t *cmask_pse, short *data, int len,
                        int is_end) {
    int i, j, n;
    int nmicchannel = cmask_pse->cfg->nmicchannel;
    int nspchannel = cmask_pse->cfg->nspchannel;
    int channel = cmask_pse->cfg->channel;
	int *mic_channel=cmask_pse->cfg->mic_channel;
	int *sp_channel=cmask_pse->cfg->sp_channel;
    int wins = cmask_pse->cfg->wins;
    int fsize = wins / 2;
    int nbin = cmask_pse->nbin;
    int length;
    wtk_drft_t *rfft = cmask_pse->rfft;
    float *rfft_in = cmask_pse->rfft_in;
    wtk_complex_t **fft = cmask_pse->fft;
    wtk_complex_t **fft_sp = cmask_pse->fft_sp;
    wtk_complex_t *fftx = cmask_pse->fftx;
    wtk_complex_t *ffty = cmask_pse->ffty;
    float **analysis_mem = cmask_pse->analysis_mem,
          **analysis_mem_sp = cmask_pse->analysis_mem_sp;
	float *synthesis_mem=cmask_pse->synthesis_mem;
    float *synthesis_window=cmask_pse->synthesis_window;
    float *analysis_window = cmask_pse->analysis_window;
    float *out = cmask_pse->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic=cmask_pse->mic;
    wtk_strbuf_t **sp=cmask_pse->sp;
    float fv;
	float spenr;
	float spenr_thresh=cmask_pse->cfg->spenr_thresh;
	int spenr_cnt=cmask_pse->cfg->spenr_cnt;

	for(i=0;i<len;++i)
	{
		for(j=0; j<nmicchannel; ++j)
		{
            fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[j]]);
            wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
		for(j=0; j<nspchannel; ++j)
		{
            fv = WTK_WAV_SHORT_TO_FLOAT(data[sp_channel[j]]);
            wtk_strbuf_push(sp[j],(char *)&(fv),sizeof(float));
		}
        data += channel;
    }
    length = mic[0]->pos/sizeof(float);
    while(length>=fsize){
        ++cmask_pse->nframe;
        for(i=0;i<nmicchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i], (float *)(mic[i]->data), wins, analysis_window);
        }
        for(i=0;i<nspchannel;++i){
            wtk_drft_stft(rfft, rfft_in, analysis_mem_sp[i], fft_sp[i], (float *)(sp[i]->data), wins, analysis_window);
        }

        wtk_strbufs_pop(mic, nmicchannel, fsize*sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize*sizeof(float));
        length = mic[0]->pos/sizeof(float);

        if(cmask_pse->cfg->use_freq_preemph){
            int pre_clip_s = cmask_pse->cfg->pre_clip_s;
            int pre_clip_e = cmask_pse->cfg->pre_clip_e;
            float pre_pow_ratio = cmask_pse->cfg->pre_pow_ratio;
            float pre_mul_ratio = cmask_pse->cfg->pre_mul_ratio;
            float alpha;
            for(i=0;i<nmicchannel;++i){
                for(j=pre_clip_s;j<pre_clip_e;++j){
                    alpha = (pre_mul_ratio - 1) * (pow(j-pre_clip_s, pre_pow_ratio))/pow((nbin-pre_clip_s), pre_pow_ratio) + 1;
                    fft[i][j].a *= alpha;
                    fft[i][j].b *= alpha;
                }
            }
        }
        if(cmask_pse->cfg->use_aec_model){
            spenr = nspchannel > 0 ? wtk_cmask_pse_sp_energy((float *)sp[0]->data, fsize) : 0;
            if (spenr > spenr_thresh) {
                // if(cmask_pse->sp_sil==1)
                // {
                // 	printf("sp start %f %f
                // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
                // }
                cmask_pse->sp_sil = 0;
                cmask_pse->sp_silcnt = spenr_cnt;
            } else if (cmask_pse->sp_sil == 0) {
                cmask_pse->sp_silcnt -= 1;
                if (cmask_pse->sp_silcnt <= 0) {
                    // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                    cmask_pse->sp_sil = 1;
                }
            }
            wtk_cmask_pse_feed_aec(cmask_pse, fft[0], fft_sp[0]);
        }else{
            if(cmask_pse->cfg->use_onnx){
                wtk_cmask_pse_feed_pse(cmask_pse, fft[0], NULL, fft[0], NULL);
            }else{
                int num_frame = cmask_pse->cfg->num_frame;
                int pos = cmask_pse->frame_pos;
                cmask_pse->feed_frame++;
                memcpy(cmask_pse->c_onnx_out+pos, fftx, nbin*sizeof(wtk_complex_t));
                memcpy(cmask_pse->c_onnx_err+pos, ffty, nbin * sizeof(wtk_complex_t));
                memcpy(cmask_pse->c_onnx_raw+pos, fft[0], nbin * sizeof(wtk_complex_t));
                cmask_pse->frame_pos += nbin;
                if (cmask_pse->feed_frame >= num_frame) {
                    cmask_pse->feed_frame = 0;
                    cmask_pse->frame_pos = 0;
                }
            }
        }

        if(cmask_pse->feed_frame==0){
            for (n = 0; n < cmask_pse->cfg->num_frame; ++n) {
                for(i=0;i<nbin;++i){
                    fftx[i].a = cmask_pse->c_onnx_out[i+n*nbin].a;
                    fftx[i].b = cmask_pse->c_onnx_out[i+n*nbin].b;
                    ffty[i].a = cmask_pse->c_onnx_err[i+n*nbin].a;
                    ffty[i].b = cmask_pse->c_onnx_err[i+n*nbin].b;
                    fft[0][i].a = cmask_pse->c_onnx_raw[i+n*nbin].a;
                    fft[0][i].b = cmask_pse->c_onnx_raw[i+n*nbin].b;
                }
                if(cmask_pse->cfg->use_trick){
                    wtk_cmask_pse_notify_data(cmask_pse, fftx, ffty, fft[0]);
                }

                if (cmask_pse->cfg->use_cnon) {
                    wtk_cmask_pse_feed_cnon(cmask_pse, fftx);
                }
                wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);

                if (cmask_pse->eq) {
                    wtk_equalizer_feed_float(cmask_pse->eq, out, fsize);
                }
                wtk_cmask_pse_control_bs(cmask_pse, out, fsize);

                for(i=0;i<fsize;++i){
                    pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
                }
                if(!cmask_pse->cfg->use_sv_check){
                    if(cmask_pse->notify){
                        cmask_pse->notify(cmask_pse->ths, pv, fsize);
                    }
                }else{
                    wtk_cmask_sv_feed2(cmask_pse->sv,pv,fsize,0,0);
                    wtk_strbuf_push(cmask_pse->cache_out_wav,(char*)pv,fsize*sizeof(short));
                }
            }
            if(cmask_pse->cfg->use_sv_check){
                if(cmask_pse->check_cnt > 0 && cmask_pse->sv_base){
                    cmask_pse->sv->normalize = 1;
                    cmask_pse->sv->enroll_cycle = 0;
                    wtk_cmask_sv_feed2(cmask_pse->sv,NULL,0,1,cmask_pse->cache_out_wav->pos);
                    float prob = wtk_cmask_sv_eval(cmask_pse->sv,cmask_pse->sv_base,cmask_pse->sv->output,cmask_pse->sv_len);
                    // if(prob < cmask_pse->cfg->sv_thresh){
                    //     cmask_pse->flag++;
                    // }else{
                    //     cmask_pse->flag = 0;
                    // }
                    // if(cmask_pse->last_flag > 0 && cmask_pse->flag == 0){
                    //     wtk_cmask_pse_cache_reset(cmask_pse);
                    //     cmask_pse->flag_mask = -1;
                    // }
                    if(cmask_pse->state_spk == 0){
                        if(prob > cmask_pse->cfg->sv_thresh){
                            cmask_pse->state_spk = 1;
                        }else if(prob > cmask_pse->cfg->sv_thresh_low){
                            cmask_pse->state_spk = 2;
                        }
                    }else if(cmask_pse->state_spk == 2){
                        if(prob < cmask_pse->cfg->sv_thresh_low){
                            cmask_pse->state_spk = 0;
                        }else if(prob >= cmask_pse->cfg->sv_thresh_low && prob <= cmask_pse->cfg->sv_thresh_mid){
                             cmask_pse->state_spk = 2;
                        }else{
                            cmask_pse->state_spk = 1;
                        }
                    }else{
                        if(prob < cmask_pse->cfg->sv_thresh_low){
                            cmask_pse->counter_below_low += 1;
                        }else{
                            cmask_pse->counter_below_low = 0;
                        }
                        if(prob < cmask_pse->cfg->sv_thresh){
                            cmask_pse->counter_below_high += 1;
                        }else{
                            cmask_pse->counter_below_high = 0;
                        }
                        if(cmask_pse->counter_below_low >= 4 || cmask_pse->counter_below_high >= 8){
                            cmask_pse->state_spk = 0;
                            cmask_pse->counter_below_low = 0;
                            cmask_pse->counter_below_high = 0;
                        }
                    }
                    if(cmask_pse->state_spk == 0){
                        memset(cmask_pse->cache_out_wav->data,0,cmask_pse->res_len*sizeof(short));
                    }else if(cmask_pse->state_spk == 2){
                        int ij;
                        short *ov = (short*)cmask_pse->cache_out_wav->data;
                        int ov_len = cmask_pse->res_len;
                        for(ij = 0; ij < ov_len; ++ij){
                            ov[ij] /= 10;
                        }
                    }

                    // if(cmask_pse->flag >= 4){
                    //     memset(cmask_pse->cache_out_wav->data,0,cmask_pse->res_len*sizeof(short));
                    // }
                    if(cmask_pse->notify && cmask_pse->cache_out_wav->pos > 0){
                        cmask_pse->notify(cmask_pse->ths, (short*)cmask_pse->cache_out_wav->data, cmask_pse->res_len);
                    }
                    wtk_strbuf_pop(cmask_pse->cache_out_wav,NULL,cmask_pse->res_len*sizeof(short));
                    cmask_pse->flag_mask++;
                    wtk_cmask_sv_reset(cmask_pse->sv);
                }
            }
            cmask_pse->check_cnt++;
        }
    }
    if(is_end && length>0){
        if(cmask_pse->notify)
        {
            pv=(short *)mic[0]->data;
            cmask_pse->notify(cmask_pse->ths,pv,length);
        }
    }
}