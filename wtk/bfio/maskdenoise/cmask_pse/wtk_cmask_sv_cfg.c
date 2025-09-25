#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_sv_cfg.h"

int wtk_cmask_sv_cfg_init(wtk_cmask_sv_cfg_t *cfg) {

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->emb));
#endif
	wtk_fbank_cfg_init(&(cfg->fbank));
	cfg->use_onnx = 1;
	cfg->use_cmvn = 1;
    return 0;
}

int wtk_cmask_sv_cfg_clean(wtk_cmask_sv_cfg_t *cfg) {
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->emb));
#endif
	wtk_fbank_cfg_clean(&(cfg->fbank));

    return 0;
}

int wtk_cmask_sv_cfg_update(wtk_cmask_sv_cfg_t *cfg) {
	int ret;

	ret=wtk_fbank_cfg_update(&(cfg->fbank));
	if(ret!=0){goto end;}
#ifdef ONNX_DEC
	if(cfg->use_onnx){
		ret = qtk_onnxruntime_cfg_update(&(cfg->emb));
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
	ret=0;
end:
	return ret;
}

int wtk_cmask_sv_cfg_update2(wtk_cmask_sv_cfg_t *cfg, wtk_source_loader_t *sl) {
	int ret;

	ret=wtk_fbank_cfg_update2(&(cfg->fbank), sl->hook);
	if(ret!=0){goto end;}

#ifdef ONNX_DEC
	if(cfg->use_onnx){
		ret = qtk_onnxruntime_cfg_update2(&(cfg->emb), sl->hook);
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
	ret=0;
end:
	return ret;
}

int wtk_cmask_sv_cfg_update_local(wtk_cmask_sv_cfg_t *cfg, wtk_local_cfg_t *m) {
	wtk_local_cfg_t *lc;
	int ret;

	lc=m;
#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(m, "emb");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->emb), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
#endif
	lc=wtk_local_cfg_find_lc_s(m,"fbank");
	if(lc)
	{
		ret = wtk_fbank_cfg_update_local(&(cfg->fbank),lc);
		if (ret != 0) {
			wtk_debug("update local fbank failed\n");
			goto end;
		}
	}
    ret = 0;
end:
    return ret;
}
