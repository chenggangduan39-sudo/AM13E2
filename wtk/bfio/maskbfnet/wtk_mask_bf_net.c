#include "wtk/bfio/maskbfnet/wtk_mask_bf_net.h"
#include "qtk/core/qtk_mem.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "qtk/nnrt/qtk_nnrt_value.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0 ? (f / 32767.0) : (f / 32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f)                                              \
    ((f) > 0 ? floorf(f * 32767.0 + 0.5) : floorf(f * 32768.0 + 0.5))
#endif

void wtk_mask_bf_net_aec_on_gainnet2(wtk_mask_bf_net_edr_t *vdr, float *gain,
                                     int len, int is_end);
void wtk_mask_bf_net_aec_on_gainnet2_2(wtk_mask_bf_net_edr_t *vdr, float *gain,
                                       int len, int is_end);

void wtk_mask_bf_net_feed_model2(wtk_mask_bf_net_t *mask_bf_net);
void wtk_mask_bf_net_edr_init(wtk_mask_bf_net_edr_t *vdr,
                              wtk_mask_bf_net_cfg_t *cfg) {
    vdr->cfg = cfg;
    vdr->nbin = cfg->wins / 2 + 1;

    vdr->bank_mic = wtk_bankfeat_new(&(cfg->bankfeat));
    vdr->bank_sp = wtk_bankfeat_new(&(cfg->bankfeat));

    vdr->g = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    vdr->lastg = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_bands);
    vdr->gf = wtk_malloc(sizeof(float) * vdr->nbin);

    vdr->feature_sp = NULL;
    if (cfg->featm_lm + cfg->featsp_lm > 2) {
        vdr->feature_sp = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_features *
                                     (cfg->featm_lm + cfg->featsp_lm - 1));
    }

    vdr->gainnet2 = wtk_gainnet2_new(cfg->gainnet2);
    wtk_gainnet2_set_notify(
        vdr->gainnet2, vdr,
        (wtk_gainnet2_notify_f)wtk_mask_bf_net_aec_on_gainnet2);
}

void wtk_mask_bf_net_edr_clean(wtk_mask_bf_net_edr_t *vdr) {
    wtk_bankfeat_delete(vdr->bank_mic);
    wtk_bankfeat_delete(vdr->bank_sp);

    if (vdr->feature_sp) {
        wtk_free(vdr->feature_sp);
    }

    wtk_free(vdr->g);
    wtk_free(vdr->lastg);
    wtk_free(vdr->gf);

    wtk_gainnet2_delete(vdr->gainnet2);
}

void wtk_mask_bf_net_edr_reset(wtk_mask_bf_net_edr_t *vdr) {
    wtk_bankfeat_reset(vdr->bank_mic);
    wtk_bankfeat_reset(vdr->bank_sp);

    if (vdr->feature_sp) {
        memset(vdr->feature_sp, 0,
               sizeof(float) * vdr->bank_sp->cfg->nb_features *
                   (vdr->cfg->featm_lm + vdr->cfg->featsp_lm - 1));
    }

    memset(vdr->g, 0, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
    memset(vdr->lastg, 0, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
    memset(vdr->gf, 0, sizeof(float) * vdr->nbin);

    wtk_gainnet2_reset(vdr->gainnet2);
}

void wtk_mask_bf_net_edr_init2(wtk_mask_bf_net_edr_t *vdr,
                               wtk_mask_bf_net_cfg_t *cfg) {
    vdr->cfg = cfg;
    vdr->nbin = cfg->wins / 2 + 1;

    vdr->bank_mic = wtk_bankfeat_new(&(cfg->bankfeat2));
    vdr->bank_sp = wtk_bankfeat_new(&(cfg->bankfeat2));

    vdr->g = wtk_malloc(sizeof(float) * cfg->bankfeat2.nb_bands);
    vdr->lastg = wtk_malloc(sizeof(float) * cfg->bankfeat2.nb_bands);
    vdr->gf = wtk_malloc(sizeof(float) * vdr->nbin);

    vdr->feature_sp = NULL;
    if (cfg->featm_lm + cfg->featsp_lm > 2) {
        vdr->feature_sp = wtk_malloc(sizeof(float) * cfg->bankfeat.nb_features *
                                     (cfg->featm_lm + cfg->featsp_lm - 1));
    }

    vdr->gainnet2 = wtk_gainnet2_new(cfg->gainnet2_2);
    wtk_gainnet2_set_notify(
        vdr->gainnet2, vdr,
        (wtk_gainnet2_notify_f)wtk_mask_bf_net_aec_on_gainnet2_2);
}

void wtk_mask_bf_net_edr_clean2(wtk_mask_bf_net_edr_t *vdr) {
    wtk_bankfeat_delete(vdr->bank_mic);
    wtk_bankfeat_delete(vdr->bank_sp);

    if (vdr->feature_sp) {
        wtk_free(vdr->feature_sp);
    }

    wtk_free(vdr->g);
    wtk_free(vdr->lastg);
    wtk_free(vdr->gf);

    wtk_gainnet2_delete(vdr->gainnet2);
}

void wtk_mask_bf_net_edr_reset2(wtk_mask_bf_net_edr_t *vdr) {
    wtk_bankfeat_reset(vdr->bank_mic);
    wtk_bankfeat_reset(vdr->bank_sp);

    if (vdr->feature_sp) {
        memset(vdr->feature_sp, 0,
               sizeof(float) * vdr->bank_sp->cfg->nb_features *
                   (vdr->cfg->featm_lm2 + vdr->cfg->featsp_lm2 - 1));
    }

    memset(vdr->g, 0, sizeof(float) * vdr->cfg->bankfeat2.nb_bands);
    memset(vdr->lastg, 0, sizeof(float) * vdr->cfg->bankfeat2.nb_bands);
    memset(vdr->gf, 0, sizeof(float) * vdr->nbin);

    wtk_gainnet2_reset(vdr->gainnet2);
}

wtk_mask_bf_net_t *wtk_mask_bf_net_new(wtk_mask_bf_net_cfg_t *cfg) {
    int i;
    wtk_mask_bf_net_t *mask_bf_net;

    mask_bf_net = (wtk_mask_bf_net_t *)wtk_malloc(sizeof(wtk_mask_bf_net_t));
    mask_bf_net->cfg = cfg;
    mask_bf_net->ths = NULL;
    mask_bf_net->notify = NULL;
    mask_bf_net->mic = wtk_strbufs_new(mask_bf_net->cfg->nmicchannel);
    mask_bf_net->sp = wtk_strbufs_new(mask_bf_net->cfg->nspchannel);

    mask_bf_net->nbin = cfg->wins / 2 + 1;
    if (cfg->use_pffft) {
        mask_bf_net->rfft = wtk_drft_new2(cfg->wins);
    } else {
        mask_bf_net->rfft = wtk_drft_new(cfg->wins);
    }
    mask_bf_net->rfft_in = (float *)wtk_malloc(sizeof(float) * cfg->wins);
    mask_bf_net->analysis_window = wtk_malloc(sizeof(float) * cfg->wins);
    mask_bf_net->synthesis_window = wtk_malloc(sizeof(float) * cfg->wins);
    mask_bf_net->analysis_mem =
        wtk_float_new_p2(cfg->nmicchannel, mask_bf_net->nbin - 1);
    mask_bf_net->analysis_mem_sp =
        wtk_float_new_p2(cfg->nspchannel, mask_bf_net->nbin - 1);
    mask_bf_net->synthesis_mem =
        wtk_malloc(sizeof(float) * (mask_bf_net->nbin - 1));

    mask_bf_net->fft = wtk_complex_new_p2(cfg->nmicchannel,
                                          mask_bf_net->nbin * cfg->num_frame);
    mask_bf_net->fft_sp = wtk_complex_new_p2(
        max(1, cfg->nspchannel), mask_bf_net->nbin * cfg->num_frame);
    mask_bf_net->fft_chn =
        wtk_complex_new_p2(mask_bf_net->nbin, cfg->nmicchannel);

    mask_bf_net->erls3 = NULL;
    if (cfg->use_rls3) {
        mask_bf_net->erls3 = wtk_malloc(sizeof(wtk_rls3_t));
        wtk_rls3_init(mask_bf_net->erls3, &(cfg->echo_rls3), mask_bf_net->nbin);
    }
    mask_bf_net->covm = NULL;
    mask_bf_net->echo_covm = NULL;
    mask_bf_net->bf = NULL;
    if (cfg->use_bf) {
        mask_bf_net->covm =
            wtk_covm_new(&(cfg->covm), mask_bf_net->nbin, cfg->nbfchannel);
        mask_bf_net->echo_covm =
            wtk_covm_new(&(cfg->echo_covm), mask_bf_net->nbin, cfg->nbfchannel);
        mask_bf_net->bf = wtk_bf_new(&(cfg->bf), cfg->wins);
    }

    mask_bf_net->speech_covar_norm = wtk_complex_new_p2(
        mask_bf_net->nbin, cfg->nmicchannel * cfg->nmicchannel);
    mask_bf_net->noise_covar = wtk_complex_new_p2(
        mask_bf_net->nbin, cfg->nmicchannel * cfg->nmicchannel);
    mask_bf_net->speech_power_acc = NULL;
    mask_bf_net->noise_power_acc = NULL;
    mask_bf_net->scnt = NULL;
    mask_bf_net->ncnt = NULL;
    mask_bf_net->alpha_bin = NULL;
    mask_bf_net->beta_bin = NULL;
    mask_bf_net->power_channel = NULL;
    mask_bf_net->speech_power_channel = NULL;
    mask_bf_net->noise_power_channel = NULL;
    mask_bf_net->covar = NULL;
    mask_bf_net->noise_mask = NULL;
    mask_bf_net->snr = NULL;
    if (cfg->use_bf_v2) {
        mask_bf_net->speech_power_acc =
            (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
        mask_bf_net->noise_power_acc =
            (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
        mask_bf_net->scnt =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
        mask_bf_net->ncnt =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
        mask_bf_net->alpha_bin =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
        mask_bf_net->beta_bin =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);

        mask_bf_net->power_channel =
            wtk_float_new_p2(mask_bf_net->nbin, cfg->nmicchannel);
        mask_bf_net->speech_power_channel =
            (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
        mask_bf_net->noise_power_channel =
            (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
        mask_bf_net->covar = wtk_complex_new_p2(
            mask_bf_net->nbin, cfg->nmicchannel * cfg->nmicchannel);
        mask_bf_net->noise_mask =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
        mask_bf_net->snr =
            (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
    }

    mask_bf_net->fftx = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * mask_bf_net->nbin * cfg->num_frame);
    mask_bf_net->ffty = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * mask_bf_net->nbin * cfg->num_frame);
    mask_bf_net->fft_tmp = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * mask_bf_net->nbin *
        max(cfg->num_frame, cfg->nbfchannel + cfg->nspchannel));

    mask_bf_net->entropy_E =
        (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
    mask_bf_net->entropy_Eb = (float *)wtk_malloc(sizeof(float) * cfg->wins);

    mask_bf_net->pre_alpha = NULL;
    if (cfg->use_freq_preemph) {
        mask_bf_net->pre_alpha =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
    }
    mask_bf_net->mask =
        (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin * cfg->num_frame);
    mask_bf_net->m_mask =
        (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin * cfg->num_frame);
    mask_bf_net->eta = (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
    mask_bf_net->epsi = (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
    mask_bf_net->vec = wtk_complex_new_p2(mask_bf_net->nbin, cfg->nmicchannel);
    mask_bf_net->vec2 = wtk_complex_new_p2(mask_bf_net->nbin, cfg->nmicchannel);
    mask_bf_net->t = (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
    mask_bf_net->scov_tmp = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cfg->nmicchannel * cfg->nmicchannel);
    mask_bf_net->c_temp =
        (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->nmicchannel);

    mask_bf_net->ncov_tmp = (wtk_complex_t *)wtk_malloc(
        sizeof(wtk_complex_t) * cfg->nmicchannel * cfg->nmicchannel);

    mask_bf_net->ovec = NULL;
    if (cfg->use_ds) {
        mask_bf_net->ovec = wtk_complex_new_p3(cfg->ntheta, mask_bf_net->nbin,
                                               cfg->nmicchannel);
    }

    mask_bf_net->eng = NULL;
    if (cfg->use_median_chn) {
        mask_bf_net->eng =
            (float *)wtk_malloc(sizeof(float) * cfg->nmicchannel);
    }
    mask_bf_net->sim_scov = NULL;
    mask_bf_net->sim_ncov = NULL;
    mask_bf_net->sim_echo_scov = NULL;
    mask_bf_net->sim_echo_ncov = NULL;
    mask_bf_net->sim_cnt_sum = NULL;
    mask_bf_net->sim_echo_cnt_sum = NULL;
    mask_bf_net->mask_mu = NULL;
    mask_bf_net->mask_mu2 = NULL;
    mask_bf_net->mask_tmp = NULL;
    if (cfg->use_bf2) {
        mask_bf_net->sim_scov =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
        mask_bf_net->sim_ncov =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
        mask_bf_net->sim_cnt_sum =
            (int *)wtk_malloc(sizeof(int) * mask_bf_net->nbin);
        if (cfg->use_echocovm) {
            mask_bf_net->sim_echo_scov =
                (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
            mask_bf_net->sim_echo_ncov =
                (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
            mask_bf_net->sim_echo_cnt_sum =
                (int *)wtk_malloc(sizeof(int) * mask_bf_net->nbin);
        }
        mask_bf_net->mask_mu =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin *
                                (cfg->n_mask_mu + cfg->num_frame - 1));
        mask_bf_net->mask_mu2 =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin *
                                (cfg->n_mask_mu + cfg->num_frame - 1));
        mask_bf_net->mask_tmp =
            (float *)wtk_malloc(sizeof(float) * mask_bf_net->nbin);
    }

    mask_bf_net->qmmse = NULL;
    mask_bf_net->qmmse_gain = NULL;
    if (cfg->use_qmmse) {
        mask_bf_net->qmmse = wtk_qmmse_new(&(cfg->qmmse));
        mask_bf_net->qmmse_gain = (float *)wtk_malloc(
            sizeof(float) * mask_bf_net->nbin * cfg->num_frame);
    }

    mask_bf_net->qmmse2 = NULL;
    if (cfg->use_qmmse2) {
        mask_bf_net->qmmse2 = wtk_qmmse_new(&(cfg->qmmse2));
    }

    mask_bf_net->gc_mask = (float *)wtk_malloc(sizeof(float) * cfg->num_frame);
    mask_bf_net->gc = NULL;
    if (cfg->use_gc) {
        mask_bf_net->gc = qtk_gain_controller_new(&(cfg->gc));
        qtk_gain_controller_set_mode(mask_bf_net->gc, 0);
        mask_bf_net->gc->kalman.Z_k = cfg->gc_gain;
    }

    mask_bf_net->eq = NULL;
    if (cfg->use_eq) {
        mask_bf_net->eq = wtk_equalizer_new(&(cfg->eq));
    }
    mask_bf_net->limiter = NULL;
    if (cfg->use_limiter) {
        mask_bf_net->limiter = wtk_limiter_new(&(cfg->limiter));
    }

    mask_bf_net->out = wtk_malloc(sizeof(float) * (mask_bf_net->nbin - 1));

    mask_bf_net->stage1_rt = NULL;
    if (cfg->use_stage1_rt) {
        mask_bf_net->stage1_rt = qtk_nnrt_new(&cfg->stage1_rt);
        mask_bf_net->stage1_inputs = wtk_malloc(sizeof(qtk_nnrt_value_t) *
                                                mask_bf_net->stage1_rt->num_in);
        for (i = 0; i < mask_bf_net->stage1_rt->num_in; i++) {
            mask_bf_net->stage1_inputs[i] =
                qtk_nnrt_create_input(mask_bf_net->stage1_rt, i);
        }
    }
    if (cfg->use_stage2_rt) {
        mask_bf_net->stage2_rt = qtk_nnrt_new(&cfg->stage2_rt);
        mask_bf_net->stage2_inputs = wtk_malloc(sizeof(qtk_nnrt_value_t) *
                                                mask_bf_net->stage2_rt->num_in);
        for (i = 0; i < mask_bf_net->stage2_rt->num_in; i++) {
            mask_bf_net->stage2_inputs[i] =
                qtk_nnrt_create_input(mask_bf_net->stage2_rt, i);
        }
    }
    if (cfg->use_dr_stage2_rt) {
        mask_bf_net->dr_stage2_rt = qtk_nnrt_new(&cfg->dr_stage2_rt);
        mask_bf_net->dr_stage2_inputs = wtk_malloc(
            sizeof(qtk_nnrt_value_t) * mask_bf_net->dr_stage2_rt->num_in);
        for (i = 0; i < mask_bf_net->dr_stage2_rt->num_in; i++) {
            mask_bf_net->dr_stage2_inputs[i] =
                qtk_nnrt_create_input(mask_bf_net->dr_stage2_rt, i);
        }
    }
    if (cfg->use_qnn) {
        int i, j;
        int ndim, nelem;
        if (cfg->qnn1_buf) {
            qtk_mem_t mem;
            qtk_mem_init(&mem, cfg->qnn1_buf->data, cfg->qnn1_buf->pos);
            qtk_nn_vm_load(&mask_bf_net->nv1, (qtk_io_reader)qtk_mem_read,
                           &mem);
            mask_bf_net->nv1_num_in = 30; //// TODO: fix this
            qtk_nn_vm_get_input(&mask_bf_net->nv1, &mask_bf_net->nv1_num_in,
                                mask_bf_net->nv1_input);
            mask_bf_net->nv1_in_sizes = (uint32_t *)wtk_malloc(
                sizeof(uint32_t) * mask_bf_net->nv1_num_in);
            mask_bf_net->nv1_out_sizes = NULL;
            for (i = 0; i < mask_bf_net->nv1_num_in; i++) {
                uint32_t *shape =
                    qtk_nn_vm_get_input_shape(&mask_bf_net->nv1, i, &ndim);
                nelem = 1;
                for (j = 0; j < ndim; j++) {
                    nelem *= shape[j];
                }
                mask_bf_net->nv1_in_sizes[i] = nelem * sizeof(float);
                memset(mask_bf_net->nv1_input[i], 0,
                       mask_bf_net->nv1_in_sizes[i]);
            }
        }
        if (cfg->qnn2_buf) {
            qtk_mem_t mem;
            qtk_mem_init(&mem, cfg->qnn2_buf->data, cfg->qnn2_buf->pos);
            qtk_nn_vm_load(&mask_bf_net->nv2, (qtk_io_reader)qtk_mem_read,
                           &mem);
            mask_bf_net->nv2_num_in = 30; //// TODO: fix this
            qtk_nn_vm_get_input(&mask_bf_net->nv2, &mask_bf_net->nv2_num_in,
                                mask_bf_net->nv2_input);
            mask_bf_net->nv2_in_sizes = (uint32_t *)wtk_malloc(
                sizeof(uint32_t) * mask_bf_net->nv2_num_in);
            mask_bf_net->nv2_out_sizes = NULL;
            for (i = 0; i < mask_bf_net->nv2_num_in; i++) {
                uint32_t *shape =
                    qtk_nn_vm_get_input_shape(&mask_bf_net->nv2, i, &ndim);
                nelem = 1;
                for (j = 0; j < ndim; j++) {
                    nelem *= shape[j];
                }
                mask_bf_net->nv2_in_sizes[i] = nelem * sizeof(float);
                memset(mask_bf_net->nv2_input[i], 0,
                       mask_bf_net->nv2_in_sizes[i]);
            }
        }
        if (cfg->qnn3_buf) {
            qtk_mem_t mem;
            qtk_mem_init(&mem, cfg->qnn3_buf->data, cfg->qnn3_buf->pos);
            qtk_nn_vm_load(&mask_bf_net->nv3, (qtk_io_reader)qtk_mem_read,
                           &mem);
            mask_bf_net->nv3_num_in = 30; //// TODO: fix this
            qtk_nn_vm_get_input(&mask_bf_net->nv3, &mask_bf_net->nv3_num_in,
                                mask_bf_net->nv3_input);
            mask_bf_net->nv3_in_sizes = (uint32_t *)wtk_malloc(
                sizeof(uint32_t) * mask_bf_net->nv3_num_in);
            mask_bf_net->nv3_out_sizes = NULL;
            for (i = 0; i < mask_bf_net->nv3_num_in; i++) {
                uint32_t *shape =
                    qtk_nn_vm_get_input_shape(&mask_bf_net->nv3, i, &ndim);
                nelem = 1;
                for (j = 0; j < ndim; j++) {
                    nelem *= shape[j];
                }
                mask_bf_net->nv3_in_sizes[i] = nelem * sizeof(float);
                memset(mask_bf_net->nv3_input[i], 0,
                       mask_bf_net->nv3_in_sizes[i]);
            }
        }
    }

    mask_bf_net->vdr2 = NULL;
    if (cfg->use_gainnet2_2) {
        mask_bf_net->vdr2 =
            (wtk_mask_bf_net_edr_t *)wtk_malloc(sizeof(wtk_mask_bf_net_edr_t));
        wtk_mask_bf_net_edr_init2(mask_bf_net->vdr2, cfg);
    }

    mask_bf_net->vdr = NULL;
    mask_bf_net->err = NULL;
    mask_bf_net->ee = NULL;
    mask_bf_net->x_phase = NULL;
    mask_bf_net->model_mask = NULL;
    if (cfg->use_gainnet2) {
        mask_bf_net->vdr =
            (wtk_mask_bf_net_edr_t *)wtk_malloc(sizeof(wtk_mask_bf_net_edr_t));
        wtk_mask_bf_net_edr_init(mask_bf_net->vdr, cfg);
    }

    if (cfg->use_stage1_rt || cfg->use_stage2_rt || cfg->use_dr_stage2_rt ||
        cfg->use_qnn) {
        if (cfg->use_mask_model) {
            mask_bf_net->err = (float *)wtk_malloc(
                sizeof(float) * mask_bf_net->nbin * cfg->num_frame);
            mask_bf_net->ee = (float *)wtk_malloc(
                sizeof(float) * mask_bf_net->nbin * cfg->num_frame);
            mask_bf_net->model_mask = (float *)wtk_malloc(
                sizeof(float) * mask_bf_net->nbin * cfg->num_frame);
        } else {
            mask_bf_net->err = (float *)wtk_malloc(
                sizeof(float) * mask_bf_net->nbin * cfg->num_frame * 2);
            mask_bf_net->ee = (float *)wtk_malloc(
                sizeof(float) * mask_bf_net->nbin * cfg->num_frame * 2);
            if (cfg->use_ccm) {
                mask_bf_net->x_phase = (float *)wtk_malloc(
                    sizeof(float) * mask_bf_net->nbin * cfg->num_frame);
            }
        }
    }

    mask_bf_net->bs_win = NULL;
    if (cfg->use_bs_win) {
        mask_bf_net->bs_win = wtk_math_create_hanning_window2(cfg->wins / 2);
    }
    mask_bf_net->sp_state = (int *)wtk_malloc(sizeof(int) * cfg->num_frame);

    wtk_mask_bf_net_reset(mask_bf_net);
    return mask_bf_net;
}
void wtk_mask_bf_net_delete(wtk_mask_bf_net_t *mask_bf_net) {
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int nspchannel = mask_bf_net->cfg->nspchannel;
    int nbin = mask_bf_net->nbin;

    wtk_strbufs_delete(mask_bf_net->mic, nmicchannel);
    wtk_strbufs_delete(mask_bf_net->sp, nspchannel);

    if (mask_bf_net->cfg->use_pffft) {
        wtk_drft_delete2(mask_bf_net->rfft);
    } else {
        wtk_drft_delete(mask_bf_net->rfft);
    }
    wtk_free(mask_bf_net->rfft_in);
    wtk_free(mask_bf_net->analysis_window);
    wtk_free(mask_bf_net->synthesis_window);
    wtk_float_delete_p2(mask_bf_net->analysis_mem, nmicchannel);
    wtk_float_delete_p2(mask_bf_net->analysis_mem_sp, nspchannel);
    wtk_free(mask_bf_net->synthesis_mem);
    wtk_complex_delete_p2(mask_bf_net->fft, nmicchannel);
    wtk_complex_delete_p2(mask_bf_net->fft_sp, max(1, nspchannel));
    wtk_complex_delete_p2(mask_bf_net->fft_chn, nbin);

    if (mask_bf_net->erls3) {
        wtk_rls3_clean(mask_bf_net->erls3);
        wtk_free(mask_bf_net->erls3);
    }
    if (mask_bf_net->covm) {
        wtk_covm_delete(mask_bf_net->covm);
    }
    if (mask_bf_net->echo_covm) {
        wtk_covm_delete(mask_bf_net->echo_covm);
    }
    if (mask_bf_net->bf) {
        wtk_bf_delete(mask_bf_net->bf);
    }
    if (mask_bf_net->speech_covar_norm) {
        wtk_complex_delete_p2(mask_bf_net->speech_covar_norm, nbin);
    }
    if (mask_bf_net->noise_covar) {
        wtk_complex_delete_p2(mask_bf_net->noise_covar, nbin);
    }
    if (mask_bf_net->speech_power_acc) {
        wtk_free(mask_bf_net->speech_power_acc);
    }
    if (mask_bf_net->noise_power_acc) {
        wtk_free(mask_bf_net->noise_power_acc);
    }
    if (mask_bf_net->scnt) {
        wtk_free(mask_bf_net->scnt);
    }
    if (mask_bf_net->ncnt) {
        wtk_free(mask_bf_net->ncnt);
    }
    if (mask_bf_net->alpha_bin) {
        wtk_free(mask_bf_net->alpha_bin);
    }
    if (mask_bf_net->beta_bin) {
        wtk_free(mask_bf_net->beta_bin);
    }
    if (mask_bf_net->power_channel) {
        wtk_float_delete_p2(mask_bf_net->power_channel, nbin);
    }
    if (mask_bf_net->speech_power_channel) {
        wtk_free(mask_bf_net->speech_power_channel);
    }
    if (mask_bf_net->noise_power_channel) {
        wtk_free(mask_bf_net->noise_power_channel);
    }
    if (mask_bf_net->covar) {
        wtk_complex_delete_p2(mask_bf_net->covar, nbin);
    }
    if (mask_bf_net->noise_mask) {
        wtk_free(mask_bf_net->noise_mask);
    }
    if (mask_bf_net->snr) {
        wtk_free(mask_bf_net->snr);
    }

    wtk_free(mask_bf_net->fftx);
    wtk_free(mask_bf_net->ffty);
    wtk_free(mask_bf_net->fft_tmp);
    wtk_free(mask_bf_net->entropy_E);
    wtk_free(mask_bf_net->entropy_Eb);
    if (mask_bf_net->pre_alpha) {
        wtk_free(mask_bf_net->pre_alpha);
    }
    wtk_free(mask_bf_net->mask);
    wtk_free(mask_bf_net->m_mask);
    wtk_free(mask_bf_net->eta);
    wtk_free(mask_bf_net->epsi);
    wtk_complex_delete_p2(mask_bf_net->vec, nbin);
    wtk_complex_delete_p2(mask_bf_net->vec2, nbin);
    wtk_free(mask_bf_net->t);
    wtk_free(mask_bf_net->scov_tmp);
    wtk_free(mask_bf_net->c_temp);
    wtk_free(mask_bf_net->ncov_tmp);

    if (mask_bf_net->ovec) {
        wtk_complex_delete_p3(mask_bf_net->ovec, mask_bf_net->cfg->ntheta,
                              mask_bf_net->nbin);
    }

    if (mask_bf_net->eng) {
        wtk_free(mask_bf_net->eng);
    }

    if (mask_bf_net->sim_scov) {
        wtk_free(mask_bf_net->sim_scov);
        wtk_free(mask_bf_net->sim_ncov);
        wtk_free(mask_bf_net->sim_cnt_sum);
    }
    if (mask_bf_net->sim_echo_scov) {
        wtk_free(mask_bf_net->sim_echo_scov);
        wtk_free(mask_bf_net->sim_echo_ncov);
        wtk_free(mask_bf_net->sim_echo_cnt_sum);
    }
    if (mask_bf_net->mask_mu) {
        wtk_free(mask_bf_net->mask_mu);
        wtk_free(mask_bf_net->mask_mu2);
        wtk_free(mask_bf_net->mask_tmp);
    }

    if (mask_bf_net->qmmse) {
        wtk_qmmse_delete(mask_bf_net->qmmse);
    }
    if (mask_bf_net->qmmse_gain) {
        wtk_free(mask_bf_net->qmmse_gain);
    }
    if (mask_bf_net->qmmse2) {
        wtk_qmmse_delete(mask_bf_net->qmmse2);
    }

    if (mask_bf_net->gc) {
        qtk_gain_controller_delete(mask_bf_net->gc);
    }
    if (mask_bf_net->gc_mask) {
        wtk_free(mask_bf_net->gc_mask);
    }

    if (mask_bf_net->eq) {
        wtk_equalizer_delete(mask_bf_net->eq);
    }
    if (mask_bf_net->limiter) {
        wtk_limiter_delete(mask_bf_net->limiter);
    }

    wtk_free(mask_bf_net->out);
    if (mask_bf_net->cfg->use_stage1_rt) {
        int i;
        for (i = 0; i < mask_bf_net->stage1_rt->num_in; i++) {
            qtk_nnrt_value_release(mask_bf_net->stage1_rt,
                                   mask_bf_net->stage1_inputs[i]);
        }
        wtk_free(mask_bf_net->stage1_inputs);
        qtk_nnrt_delete(mask_bf_net->stage1_rt);
    }
    if (mask_bf_net->cfg->use_stage2_rt) {
        int i;
        for (i = 0; i < mask_bf_net->stage2_rt->num_in; i++) {
            qtk_nnrt_value_release(mask_bf_net->stage2_rt,
                                   mask_bf_net->stage2_inputs[i]);
        }
        wtk_free(mask_bf_net->stage2_inputs);
        qtk_nnrt_delete(mask_bf_net->stage2_rt);
    }
    if (mask_bf_net->cfg->use_dr_stage2_rt) {
        int i;
        for (i = 0; i < mask_bf_net->dr_stage2_rt->num_in; i++) {
            qtk_nnrt_value_release(mask_bf_net->dr_stage2_rt,
                                   mask_bf_net->dr_stage2_inputs[i]);
        }
        wtk_free(mask_bf_net->dr_stage2_inputs);
        qtk_nnrt_delete(mask_bf_net->dr_stage2_rt);
    }

    if (mask_bf_net->vdr2) {
        wtk_mask_bf_net_edr_clean2(mask_bf_net->vdr2);
        wtk_free(mask_bf_net->vdr2);
    }

    if (mask_bf_net->vdr) {
        wtk_mask_bf_net_edr_clean(mask_bf_net->vdr);
        wtk_free(mask_bf_net->vdr);
    }
    if (mask_bf_net->err) {
        wtk_free(mask_bf_net->err);
    }
    if (mask_bf_net->ee) {
        wtk_free(mask_bf_net->ee);
    }
    if (mask_bf_net->x_phase) {
        wtk_free(mask_bf_net->x_phase);
    }
    if (mask_bf_net->model_mask) {
        wtk_free(mask_bf_net->model_mask);
    }

    if (mask_bf_net->bs_win) {
        wtk_free(mask_bf_net->bs_win);
    }
    wtk_free(mask_bf_net->sp_state);

    if (mask_bf_net->cfg->use_qnn) {
        int i;
        int outer_in_num = 1;
        if (mask_bf_net->cfg->use_edr_stage1_rt) {
            outer_in_num = 2;
        }
        if (mask_bf_net->cfg->qnn1_buf) {
            qtk_nn_vm_clean(&mask_bf_net->nv1);
            wtk_free(mask_bf_net->nv1_in_sizes);
            if (mask_bf_net->nv1_out_sizes) {
                wtk_free(mask_bf_net->nv1_out_sizes);
                for (i = outer_in_num; i < mask_bf_net->nv1_num_in; i++) {
                    wtk_free(mask_bf_net->nv1_cache[i - outer_in_num + 1]);
                }
            }
        }
        if (mask_bf_net->cfg->qnn2_buf) {
            qtk_nn_vm_clean(&mask_bf_net->nv2);
            wtk_free(mask_bf_net->nv2_in_sizes);
            if (mask_bf_net->nv2_out_sizes) {
                wtk_free(mask_bf_net->nv2_out_sizes);
                for (i = 2; i < mask_bf_net->nv2_num_in; i++) {
                    wtk_free(mask_bf_net->nv2_cache[i - 1]);
                }
            }
        }
        if (mask_bf_net->cfg->qnn3_buf) {

            qtk_nn_vm_clean(&mask_bf_net->nv3);
            wtk_free(mask_bf_net->nv3_in_sizes);
            if (mask_bf_net->nv3_out_sizes) {
                wtk_free(mask_bf_net->nv3_out_sizes);
                for (i = 1; i < mask_bf_net->nv3_num_in; i++) {
                    wtk_free(mask_bf_net->nv3_cache[i]);
                }
            }
        }
    }
    wtk_free(mask_bf_net);
}

void wtk_mask_bf_net_start(wtk_mask_bf_net_t *mask_bf_net) {
    if (mask_bf_net->bf) {
        wtk_bf_update_ovec(mask_bf_net->bf, 90, 0);
        wtk_bf_init_w(mask_bf_net->bf);
    }

    if (mask_bf_net->ovec) {
        int i;
        for (i = 0; i < mask_bf_net->cfg->ntheta; ++i) {
            // wtk_bf_update_ovec5(
            //     i * 45.0, 0, mask_bf_net->cfg->nmicchannel,
            //     mask_bf_net->nbin, mask_bf_net->cfg->rate,
            //     mask_bf_net->cfg->sv, mask_bf_net->cfg->bf.mic_pos,
            //     mask_bf_net->ovec[i], mask_bf_net->cfg->sdb_alpha, 1e-9);
            wtk_bf_update_ovec4(
                i * 45.0, 0, mask_bf_net->cfg->nmicchannel, mask_bf_net->nbin,
                mask_bf_net->cfg->rate, mask_bf_net->cfg->sv,
                mask_bf_net->cfg->bf.mic_pos, mask_bf_net->ovec[i]);
        }
    }
}
void wtk_mask_bf_net_reset(wtk_mask_bf_net_t *mask_bf_net) {
    int wins = mask_bf_net->cfg->wins;
    int fsize = wins / 2;
    int i, nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int nbfchannel = mask_bf_net->cfg->nbfchannel;
    int nspchannel = mask_bf_net->cfg->nspchannel;

    wtk_strbufs_reset(mask_bf_net->mic, nmicchannel);
    wtk_strbufs_reset(mask_bf_net->sp, nspchannel);

    if (num_frame > 1) {
        wtk_strbufs_push_float(mask_bf_net->mic, nmicchannel, NULL,
                               (num_frame - 1) * fsize);
        wtk_strbufs_push_float(mask_bf_net->sp, nspchannel, NULL,
                               (num_frame - 1) * fsize);
    }

    memset(mask_bf_net->rfft_in, 0, sizeof(float) * wins);
    for (i = 0; i < wins; ++i) {
        mask_bf_net->analysis_window[i] = sin((0.5 + i) * PI / (wins));
    }
    // wtk_math_init_blackman_window(mask_bf_net->analysis_window, wins);
    wtk_drft_init_synthesis_window(mask_bf_net->synthesis_window,
                                   mask_bf_net->analysis_window, wins);

    wtk_float_zero_p2(mask_bf_net->analysis_mem, nmicchannel, (nbin - 1));
    wtk_float_zero_p2(mask_bf_net->analysis_mem_sp, nspchannel, (nbin - 1));
    memset(mask_bf_net->synthesis_mem, 0, sizeof(float) * (nbin - 1));

    wtk_complex_zero_p2(mask_bf_net->fft, nmicchannel, nbin * num_frame);
    wtk_complex_zero_p2(mask_bf_net->fft_sp, max(1, nspchannel),
                        nbin * num_frame);
    wtk_complex_zero_p2(mask_bf_net->fft_chn, nbin, nmicchannel);

    if (mask_bf_net->erls3) {
        wtk_rls3_reset(mask_bf_net->erls3, nbin);
    }
    if (mask_bf_net->covm) {
        wtk_covm_reset(mask_bf_net->covm);
    }
    if (mask_bf_net->echo_covm) {
        wtk_covm_reset(mask_bf_net->echo_covm);
    }
    if (mask_bf_net->bf) {
        wtk_bf_reset(mask_bf_net->bf);
    }

    if (mask_bf_net->speech_covar_norm) {
        wtk_complex_zero_p2(mask_bf_net->speech_covar_norm, nbin,
                            nmicchannel * nmicchannel);
    }
    if (mask_bf_net->noise_covar) {
        wtk_complex_zero_p2(mask_bf_net->noise_covar, nbin,
                            nmicchannel * nmicchannel);
    }
    if (mask_bf_net->speech_power_acc) {
        memset(mask_bf_net->speech_power_acc, 0, sizeof(float) * nmicchannel);
    }
    if (mask_bf_net->noise_power_acc) {
        memset(mask_bf_net->noise_power_acc, 0, sizeof(float) * nmicchannel);
    }
    if (mask_bf_net->scnt) {
        memset(mask_bf_net->scnt, 0, sizeof(float) * nbin);
    }
    if (mask_bf_net->ncnt) {
        memset(mask_bf_net->ncnt, 0, sizeof(float) * nbin);
    }
    if (mask_bf_net->alpha_bin) {
        memset(mask_bf_net->alpha_bin, 0, sizeof(float) * nbin);
    }
    if (mask_bf_net->beta_bin) {
        memset(mask_bf_net->beta_bin, 0, sizeof(float) * nbin);
    }
    if (mask_bf_net->power_channel) {
        for (i = 0; i < nbin; ++i) {
            memset(mask_bf_net->power_channel[i], 0,
                   sizeof(float) * nmicchannel);
        }
    }
    if (mask_bf_net->speech_power_channel) {
        memset(mask_bf_net->speech_power_channel, 0,
               sizeof(float) * nmicchannel);
    }
    if (mask_bf_net->noise_power_channel) {
        memset(mask_bf_net->noise_power_channel, 0,
               sizeof(float) * nmicchannel);
    }
    if (mask_bf_net->covar) {
        for (i = 0; i < nbin; ++i) {
            memset(mask_bf_net->covar[i], 0,
                   sizeof(wtk_complex_t) * nmicchannel * nmicchannel);
        }
    }
    if (mask_bf_net->noise_mask) {
        memset(mask_bf_net->noise_mask, 0, sizeof(float) * nbin);
    }
    if (mask_bf_net->snr) {
        memset(mask_bf_net->snr, 0, sizeof(float) * nmicchannel);
    }

    memset(mask_bf_net->fftx, 0, sizeof(wtk_complex_t) * nbin * num_frame);
    memset(mask_bf_net->ffty, 0, sizeof(wtk_complex_t) * nbin * num_frame);
    memset(mask_bf_net->fft_tmp, 0,
           sizeof(wtk_complex_t) * nbin *
               max(num_frame, nbfchannel + nspchannel));
    memset(mask_bf_net->entropy_E, 0, sizeof(float) * nbin);
    memset(mask_bf_net->entropy_Eb, 0, sizeof(float) * wins);
    if (mask_bf_net->pre_alpha) {
        int pre_clip_s = mask_bf_net->cfg->pre_clip_s;
        float pre_pow_ratio = mask_bf_net->cfg->pre_pow_ratio;
        float pre_mul_ratio = mask_bf_net->cfg->pre_mul_ratio;
        for (i = 0; i < nbin; ++i) {
            mask_bf_net->pre_alpha[i] =
                (pre_mul_ratio - 1) * (pow(i - pre_clip_s, pre_pow_ratio)) /
                    pow((nbin - pre_clip_s), pre_pow_ratio) +
                1;
            ;
        }
    }
    memset(mask_bf_net->mask, 0, sizeof(float) * nbin * num_frame);
    memset(mask_bf_net->m_mask, 0, sizeof(float) * nbin * num_frame);
    memset(mask_bf_net->eta, 0, sizeof(float) * nbin);
    memset(mask_bf_net->epsi, 0, sizeof(float) * nbin);
    wtk_complex_zero_p2(mask_bf_net->vec, nbin, nmicchannel);
    wtk_complex_zero_p2(mask_bf_net->vec2, nbin, nmicchannel);
    memset(mask_bf_net->t, 0, sizeof(float) * nbin);
    memset(mask_bf_net->scov_tmp, 0,
           sizeof(wtk_complex_t) * nmicchannel * nmicchannel);
    memset(mask_bf_net->c_temp, 0, sizeof(wtk_complex_t) * nmicchannel);
    memset(mask_bf_net->ncov_tmp, 0,
           sizeof(wtk_complex_t) * nmicchannel * nmicchannel);

    if (mask_bf_net->ovec) {
        wtk_complex_zero_p3(mask_bf_net->ovec, mask_bf_net->cfg->ntheta, nbin,
                            nmicchannel);
    }

    if (mask_bf_net->eng) {
        memset(mask_bf_net->eng, 0, sizeof(float) * nmicchannel);
    }
    if (mask_bf_net->sim_scov) {
        memset(mask_bf_net->sim_scov, 0, sizeof(float) * mask_bf_net->nbin);
        memset(mask_bf_net->sim_ncov, 0, sizeof(float) * mask_bf_net->nbin);
        memset(mask_bf_net->sim_cnt_sum, 0, sizeof(int) * mask_bf_net->nbin);
    }
    if (mask_bf_net->sim_echo_scov) {
        memset(mask_bf_net->sim_echo_scov, 0,
               sizeof(float) * mask_bf_net->nbin);
        memset(mask_bf_net->sim_echo_ncov, 0,
               sizeof(float) * mask_bf_net->nbin);
        memset(mask_bf_net->sim_echo_cnt_sum, 0,
               sizeof(int) * mask_bf_net->nbin);
    }
    if (mask_bf_net->mask_mu) {
        memset(mask_bf_net->mask_mu, 0,
               sizeof(float) * mask_bf_net->nbin *
                   (mask_bf_net->cfg->n_mask_mu + mask_bf_net->cfg->num_frame -
                    1));
        memset(mask_bf_net->mask_mu2, 0,
               sizeof(float) * mask_bf_net->nbin *
                   (mask_bf_net->cfg->n_mask_mu + mask_bf_net->cfg->num_frame -
                    1));
        memset(mask_bf_net->mask_tmp, 0, sizeof(float) * mask_bf_net->nbin);
    }

    if (mask_bf_net->qmmse) {
        wtk_qmmse_reset(mask_bf_net->qmmse);
    }
    if (mask_bf_net->qmmse_gain) {
        memset(mask_bf_net->qmmse_gain, 0, sizeof(float) * nbin);
    }
    if (mask_bf_net->qmmse2) {
        wtk_qmmse_reset(mask_bf_net->qmmse2);
    }

    if (mask_bf_net->gc) {
        qtk_gain_controller_reset(mask_bf_net->gc);
    }
    if (mask_bf_net->gc_mask) {
        memset(mask_bf_net->gc_mask, 0, sizeof(float) * num_frame);
    }
    memset(mask_bf_net->out, 0, sizeof(float) * (nbin - 1));
    if (mask_bf_net->limiter) {
        wtk_limiter_reset(mask_bf_net->limiter);
    }

    if (mask_bf_net->cfg->use_stage1_rt) {
        qtk_nnrt_reset(mask_bf_net->stage1_rt);
        for (i = 0; i < mask_bf_net->stage1_rt->num_in; i++) {
            void *data_ptr = qtk_nnrt_value_get_data(
                mask_bf_net->stage1_rt, mask_bf_net->stage1_inputs[i]);
            size_t nbytes =
                qtk_nnrt_get_input_nbytes(mask_bf_net->stage1_rt, i);
            memset(data_ptr, 0, nbytes);
        }
    }
    if (mask_bf_net->cfg->use_stage2_rt) {
        for (i = 0; i < mask_bf_net->stage2_rt->num_in; i++) {
            void *data_ptr = qtk_nnrt_value_get_data(
                mask_bf_net->stage2_rt, mask_bf_net->stage2_inputs[i]);
            size_t nbytes =
                qtk_nnrt_get_input_nbytes(mask_bf_net->stage2_rt, i);
            memset(data_ptr, 0, nbytes);
        }
        qtk_nnrt_reset(mask_bf_net->stage2_rt);
    }
    if (mask_bf_net->cfg->use_dr_stage2_rt) {
        for (i = 0; i < mask_bf_net->dr_stage2_rt->num_in; i++) {
            void *data_ptr = qtk_nnrt_value_get_data(
                mask_bf_net->dr_stage2_rt, mask_bf_net->dr_stage2_inputs[i]);
            size_t nbytes =
                qtk_nnrt_get_input_nbytes(mask_bf_net->dr_stage2_rt, i);
            memset(data_ptr, 0, nbytes);
        }
        qtk_nnrt_reset(mask_bf_net->dr_stage2_rt);
    }

    if (mask_bf_net->vdr2) {
        wtk_mask_bf_net_edr_reset2(mask_bf_net->vdr2);
    }

    if (mask_bf_net->vdr) {
        wtk_mask_bf_net_edr_reset(mask_bf_net->vdr);
    }

    if (mask_bf_net->err) {
        if (mask_bf_net->cfg->use_mask_model) {
            memset(mask_bf_net->err, 0, sizeof(float) * nbin * num_frame);
        } else {
            memset(mask_bf_net->err, 0, sizeof(float) * nbin * num_frame * 2);
        }
    }
    if (mask_bf_net->ee) {
        if (mask_bf_net->cfg->use_mask_model) {
            memset(mask_bf_net->ee, 0, sizeof(float) * nbin * num_frame);
        } else {
            memset(mask_bf_net->ee, 0, sizeof(float) * nbin * num_frame * 2);
        }
    }
    if (mask_bf_net->x_phase) {
        memset(mask_bf_net->x_phase, 0, sizeof(float) * nbin * num_frame);
    }
    if (mask_bf_net->model_mask) {
        memset(mask_bf_net->model_mask, 0, sizeof(float) * nbin * num_frame);
    }
    memset(mask_bf_net->sp_state, 0, sizeof(int) * num_frame);

    mask_bf_net->update_w = 0;
    mask_bf_net->sp_silcnt = 0;
    mask_bf_net->sp_sil = 1;
    mask_bf_net->mic_silcnt = 0;
    mask_bf_net->mic_sil = 1;

    mask_bf_net->entropy_silcnt = 0;
    mask_bf_net->entropy_in_cnt = 0;
    mask_bf_net->entropy_sil = 1;
    mask_bf_net->entropy_sp_silcnt = 0;
    mask_bf_net->entropy_sp_in_cnt = 0;
    mask_bf_net->entropy_sp_sil = 1;

    mask_bf_net->bs_scale = 1.0;
    mask_bf_net->bs_last_scale = 1.0;
    mask_bf_net->bs_real_scale = 1.0;
    mask_bf_net->bs_max_cnt = 0;

    mask_bf_net->nframe = 0;
    mask_bf_net->gc_cnt = 0;

    mask_bf_net->bf_start = 0;
    mask_bf_net->sum_sp_sil = 0;
    mask_bf_net->bf_init_frame = 0;

    mask_bf_net->mic_scale = mask_bf_net->cfg->mic_scale;
    mask_bf_net->sp_scale = mask_bf_net->cfg->sp_scale;

    mask_bf_net->agc_enable = 1;
    mask_bf_net->echo_enable = 1;
    mask_bf_net->denoise_enable = 1;

    if (mask_bf_net->cfg->use_qnn) {
        mask_bf_net->nv1_idx = 0;
        mask_bf_net->nv2_idx = 0;
        mask_bf_net->nv3_idx = 0;
    }

    mask_bf_net->spower_cnt = 0;
    mask_bf_net->npower_cnt = 0;
    mask_bf_net->snr_acc = 0;
    mask_bf_net->refchannel = 0;
    mask_bf_net->echo_in = 0;
    mask_bf_net->echo_out = 0;

    mask_bf_net->bf2_alpha_cnt = 0;
    mask_bf_net->bf2_alpha = 1.0;
}
void wtk_mask_bf_net_set_notify(wtk_mask_bf_net_t *mask_bf_net, void *ths,
                                wtk_mask_bf_net_notify_f notify) {
    mask_bf_net->notify = notify;
    mask_bf_net->ths = ths;
}

static float wtk_mask_bf_net_sp_energy(float *p, int n) {
    float f, f2;
    int i;

    f = 0;
    for (i = 0; i < n; ++i) {
        f += p[i] * 32768.0;
    }
    f /= n;

    f2 = 0;
    for (i = 0; i < n; ++i) {
        f2 += (p[i] * 32768.0 - f) * (p[i] * 32768.0 - f);
    }
    f2 /= n;

    return f2;
}

static float wtk_mask_bf_net_fft_energy(wtk_complex_t *fftx, int nbin) {
    return qtk_vector_cpx_mag_squared_sum(fftx + 1, nbin - 2);
}

float wtk_mask_bf_net_entropy(wtk_mask_bf_net_t *mask_bf_net,
                              wtk_complex_t *fftx, int fx1, int fx2) {
    int rate = mask_bf_net->cfg->rate;
    int wins = mask_bf_net->cfg->wins;
    int nbin = mask_bf_net->nbin;
    int i;
    int km = floor(wins * 1.0 / 8);
    float K = 0.5;
    float *E = mask_bf_net->entropy_E;
    float P1;
    float *Eb = mask_bf_net->entropy_Eb;
    float sum;
    float prob;
    float Hb;
    fx1 = (fx1 * 1.0 * wins) / rate;
    fx2 = (fx2 * 1.0 * wins) / rate;

    memset(E, 0, sizeof(float) * nbin);
    memset(Eb, 0, sizeof(float) * wins);
    qtk_vector_cpx_mag_squared(fftx + fx1, E + fx1, fx2 - fx1);
    sum = 1e-10;
    for (i = fx1; i < fx2; ++i) {
        sum += E[i];
    }
    for (i = fx1; i < fx2; ++i) {
        P1 = E[i] / sum;
        if (P1 >= 0.9) {
            E[i] = 0;
        }
    }
    sum = 0;
    for (i = 0; i < km; ++i) {
        Eb[i] = K;
        Eb[i] += E[i * 4 + 1] + E[i * 4 + 2] + E[i * 4 + 3] + E[i * 4 + 4];
        sum += Eb[i];
    }
    Hb = 0;
    for (i = 0; i < nbin; ++i) {
        prob = (E[i] + K) / sum;
        Hb += -prob * logf(prob + 1e-12);
    }
    // printf("%f\n", Hb);

    return Hb;
}

void wtk_mask_bf_net_feed_qnn(wtk_mask_bf_net_t *mask_bf_net) {
    int i, j;
    float *err = mask_bf_net->err;
    float *ee = mask_bf_net->ee;
    float *mask = mask_bf_net->mask;
    qtk_nn_vm_t *nv = &mask_bf_net->nv1;
    int outer_in_num = 1;

    int nout = 30, ndim, nelem;
    if (mask_bf_net->cfg->use_edr_stage1_rt) {
        outer_in_num = 2;
    }

    if (mask_bf_net->nv1_idx != 0) {
        qtk_nn_vm_get_input(nv, &mask_bf_net->nv1_num_in,
                            (void **)mask_bf_net->nv1_input);
        for (i = outer_in_num; i < mask_bf_net->nv1_num_in; i++) {
            memcpy(mask_bf_net->nv1_input[i],
                   mask_bf_net->nv1_cache[i - outer_in_num + 1],
                   mask_bf_net->nv1_out_sizes[i - outer_in_num + 1]);
        }
    }
    memcpy(mask_bf_net->nv1_input[0], err, mask_bf_net->nv1_in_sizes[0]);

    if (mask_bf_net->cfg->use_edr_stage1_rt) {
        memcpy(mask_bf_net->nv1_input[1], ee, mask_bf_net->nv1_in_sizes[1]);
    }
    qtk_nn_vm_run(nv);
    qtk_nn_vm_get_output(nv, &nout, (void **)mask_bf_net->nv1_output);

    if (!mask_bf_net->nv1_out_sizes) {
        mask_bf_net->nv1_out_sizes = wtk_malloc(sizeof(int) * nout);
        for (i = 0; i < nout; i++) {
            nelem = 1;
            uint32_t *shape = qtk_nn_vm_get_output_shape(nv, i, &ndim);
            for (j = 0; j < ndim; j++) {
                nelem *= shape[j];
            }
            mask_bf_net->nv1_out_sizes[i] = nelem * sizeof(float);
            if (i > 0) {
                mask_bf_net->nv1_cache[i] =
                    wtk_malloc(mask_bf_net->nv1_out_sizes[i]);
            }
        }
    }
    memcpy(mask, mask_bf_net->nv1_output[0], mask_bf_net->nv1_out_sizes[0]);
    for (i = 1; i < nout; i++) {
        memcpy(mask_bf_net->nv1_cache[i], mask_bf_net->nv1_output[i],
               mask_bf_net->nv1_out_sizes[i]);
    }

    mask_bf_net->nv1_idx++;
    qtk_nn_vm_reset(nv);
    // memcpy(item->val, xx, item->bytes * item->in_dim);
}

void wtk_mask_bf_net_feed_onnx(wtk_mask_bf_net_t *mask_bf_net) {
    int i;
    int nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    float *mask = mask_bf_net->mask;
    float *err = mask_bf_net->err;
    float *ee = mask_bf_net->ee;
    float *err_data = qtk_nnrt_value_get_data(mask_bf_net->stage1_rt,
                                              mask_bf_net->stage1_inputs[0]);
    int numout = mask_bf_net->stage1_rt->num_out;
    int numin = mask_bf_net->stage1_rt->num_in;
    int outer_in_num = 1;
    int outer_out_num = 1;

    memcpy(err_data, err, sizeof(float) * nbin * num_frame);

    if (mask_bf_net->cfg->use_edr_stage1_rt) {
        outer_in_num = 2;
        float *ee_data = qtk_nnrt_value_get_data(mask_bf_net->stage1_rt,
                                                 mask_bf_net->stage1_inputs[1]);
        memcpy(ee_data, ee, sizeof(float) * nbin * num_frame);
    }

    for (i = 0; i < numin; i++) {
        qtk_nnrt_feed(mask_bf_net->stage1_rt, mask_bf_net->stage1_inputs[i], i);
    }

    qtk_nnrt_run(mask_bf_net->stage1_rt);

    for (i = outer_out_num; i < numout; i++) {
        int in_idx = i - outer_out_num + outer_in_num;
        if (mask_bf_net->stage1_rt->cfg->use_ss_ipu) {
            qtk_nnrt_value_t out_val;
            void *src, *dst;
            qtk_nnrt_get_output(mask_bf_net->stage1_rt, &out_val, i);
            src = qtk_nnrt_value_get_data(mask_bf_net->stage1_rt, out_val);
            dst = qtk_nnrt_value_get_data(mask_bf_net->stage1_rt,
                                          mask_bf_net->stage1_inputs[in_idx]);
            memcpy(dst, src,
                   qtk_nnrt_get_input_nbytes(mask_bf_net->stage1_rt, in_idx));
            qtk_nnrt_value_release(mask_bf_net->stage1_rt, out_val);
        } else {
            qtk_nnrt_value_release(mask_bf_net->stage1_rt,
                                   mask_bf_net->stage1_inputs[in_idx]);
            qtk_nnrt_get_output(mask_bf_net->stage1_rt,
                                &mask_bf_net->stage1_inputs[in_idx], i);
        }
    }

    {
        qtk_nnrt_value_t output_mask;
        qtk_nnrt_get_output(mask_bf_net->stage1_rt, &output_mask, 0);
        memcpy(mask,
               qtk_nnrt_value_get_data(mask_bf_net->stage1_rt, output_mask),
               sizeof(float) * nbin * num_frame);
        qtk_nnrt_value_release(mask_bf_net->stage1_rt, output_mask);
        qtk_nnrt_reset(mask_bf_net->stage1_rt);
    }
}

void wtk_mask_bf_net_edr_feed_gainnet2_2(wtk_mask_bf_net_t *mask_bf_net,
                                         wtk_complex_t *fftx,
                                         wtk_complex_t *ffty) {

    wtk_mask_bf_net_edr_t *vdr = mask_bf_net->vdr2;
    int nbin = vdr->nbin;
    float *g = vdr->g, *gf = vdr->gf;
    wtk_gainnet2_t *gainnet2 = vdr->gainnet2;
    wtk_bankfeat_t *bank_mic = vdr->bank_mic;
    wtk_bankfeat_t *bank_sp = vdr->bank_sp;
    int featsp_lm = vdr->cfg->featsp_lm2;
    int featm_lm = vdr->cfg->featm_lm2;
    float *feature_sp = vdr->feature_sp;
    int nb_features = bank_mic->cfg->nb_features;

    wtk_bankfeat_flush_frame_features(bank_mic, fftx);
    wtk_bankfeat_flush_frame_features(bank_sp, ffty);
    if (feature_sp && featsp_lm > 1) {
        memmove(feature_sp + nb_features * featm_lm,
                feature_sp + nb_features * (featm_lm - 1),
                sizeof(float) * nb_features * (featsp_lm - 1));
        memcpy(feature_sp + nb_features * (featm_lm - 1), bank_sp->features,
               sizeof(float) * nb_features);
    }
    if (feature_sp) {
        wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features,
                           feature_sp, nb_features * (featm_lm + featsp_lm - 1),
                           0);
    } else {
        wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features,
                           bank_sp->features, nb_features, 0);
    }
    wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
    if (feature_sp && featm_lm > 1) {
        memmove(feature_sp + nb_features, feature_sp,
                sizeof(float) * nb_features * (featm_lm - 2));
        memcpy(feature_sp, bank_mic->features, sizeof(float) * nb_features);
    }
}

void wtk_mask_bf_net_feed_edr(wtk_mask_bf_net_t *mask_bf_net) {
    int nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    int nbfchannel = mask_bf_net->cfg->nbfchannel;
    int nspchannel = mask_bf_net->cfg->nspchannel;
    wtk_rls3_t *erls3 = mask_bf_net->erls3;
    wtk_complex_t **fft = mask_bf_net->fft;
    wtk_complex_t **fft_sp = mask_bf_net->fft_sp;
    wtk_complex_t *fft_tmp = mask_bf_net->fft_tmp;
    wtk_complex_t *fftx = mask_bf_net->fftx;
    wtk_complex_t *ffty = mask_bf_net->ffty;
    int i, k, n;

    if (erls3) {
        if (mask_bf_net->cfg->use_echo_bf) {
            ffty = mask_bf_net->ffty;
            for (n = 0; n < num_frame; ++n, ffty += nbin) {
                for (k = 0; k < nbin; ++k) {
                    for (i = 0; i < nbfchannel; ++i) {
                        fft_tmp[i + k * nbfchannel].a = fft[i][n * nbin + k].a;
                        fft_tmp[i + k * nbfchannel].b = fft[i][n * nbin + k].b;
                    }
                    for (i = 0; i < nspchannel; ++i) {
                        fft_tmp[i + nbin * nbfchannel + k * nspchannel].a =
                            fft_sp[i][n * nbin + k].a;
                        fft_tmp[i + nbin * nbfchannel + k * nspchannel].b =
                            fft_sp[i][n * nbin + k].b;
                    }
                }
                wtk_rls3_feed3(erls3, fft_tmp, fft_tmp + nbfchannel * nbin,
                               mask_bf_net->sp_state[n] == 0, nbin);
                if (mask_bf_net->sp_state[n] == 0) {
                    for (k = 0; k < nbin; ++k) {
                        for (i = 0; i < nbfchannel; ++i) {
                            fft[i][n * nbin + k].a =
                                erls3->out[i + k * nbfchannel].a;
                            fft[i][n * nbin + k].b =
                                erls3->out[i + k * nbfchannel].b;
                        }
                        ffty[k].a = erls3->lsty[k * nbfchannel].a;
                        ffty[k].b = erls3->lsty[k * nbfchannel].b;
                    }
                } else {
                    memset(ffty, 0, sizeof(wtk_complex_t) * nbin);
                }
            }
        } else {
            fftx = mask_bf_net->fftx;
            ffty = mask_bf_net->ffty;
            for (n = 0; n < num_frame; ++n, fftx += nbin, ffty += nbin) {
                memcpy(fft_tmp, fftx, sizeof(wtk_complex_t) * nbin);
                for (k = 0; k < nbin; ++k) {
                    for (i = 0; i < nspchannel; ++i) {
                        fft_tmp[i + nbin + k * nspchannel].a =
                            fft_sp[i][n * nbin + k].a;
                        fft_tmp[i + nbin + k * nspchannel].b =
                            fft_sp[i][n * nbin + k].b;
                    }
                }
                wtk_rls3_feed3(erls3, fft_tmp, fft_tmp + nbin,
                               mask_bf_net->sp_state[n] == 0, nbin);
                if (mask_bf_net->sp_state[n] == 0) {
                    memcpy(fftx, erls3->out, sizeof(wtk_complex_t) * nbin);
                    memcpy(ffty, erls3->lsty, sizeof(wtk_complex_t) * nbin);
                } else {
                    memset(ffty, 0, sizeof(wtk_complex_t) * nbin);
                }
            }
        }
    }
}

void wtk_mask_bf_net_feed_cnon(wtk_mask_bf_net_t *mask_bf_net,
                               wtk_complex_t *fft) {
    int nbin = mask_bf_net->nbin;
    float sym = mask_bf_net->cfg->sym;
    static float fx = 2.0f * PI / RAND_MAX;
    int cnon_clip_s = mask_bf_net->cfg->cnon_clip_s;
    int cnon_clip_e = mask_bf_net->cfg->cnon_clip_e;
    float f, f2;
    int i;

    for (i = max(1, cnon_clip_s); i < min(nbin - 1, cnon_clip_e); ++i) {
        f = rand() * fx;
        f2 = 1.f - powf((nbin - i) * 1.0 / nbin, 0.5f);
        if (f2 > 0) {
            // f2=sqrtf(f2);
            fft[i].a += sym * cosf(f) * f2;
            fft[i].b += sym * sinf(f) * f2;
        }
    }
}

void wtk_mask_bf_net_control_bs(wtk_mask_bf_net_t *mask_bf_net, float *out,
                                int len) {
    float *bs_win = mask_bf_net->bs_win;
    float out_max;
    int i;

    if (mask_bf_net->mic_sil == 0) {
        out_max = wtk_float_abs_max(out, len);
        if (out_max > mask_bf_net->cfg->max_out) {
            mask_bf_net->bs_scale = mask_bf_net->cfg->max_out / out_max;
            if (mask_bf_net->bs_scale < mask_bf_net->bs_last_scale) {
                mask_bf_net->bs_last_scale = mask_bf_net->bs_scale;
            } else {
                mask_bf_net->bs_scale = mask_bf_net->bs_last_scale;
            }
            mask_bf_net->bs_max_cnt = 5;
        }
        if (bs_win) {
            for (i = 0; i < len / 2; ++i) {
                out[i] *= mask_bf_net->bs_scale * bs_win[i] +
                          mask_bf_net->bs_real_scale * (1.0 - bs_win[i]);
            }
            for (i = len / 2; i < len; ++i) {
                out[i] *= mask_bf_net->bs_scale;
            }
            mask_bf_net->bs_real_scale = mask_bf_net->bs_scale;
        } else {
            for (i = 0; i < len; ++i) {
                out[i] *= mask_bf_net->bs_scale;
            }
        }
        if (mask_bf_net->bs_max_cnt > 0) {
            --mask_bf_net->bs_max_cnt;
        }
        if (mask_bf_net->bs_max_cnt <= 0 && mask_bf_net->bs_scale < 1.0) {
            mask_bf_net->bs_scale *= 1.1f;
            mask_bf_net->bs_last_scale = mask_bf_net->bs_scale;
            if (mask_bf_net->bs_scale > 1.0) {
                mask_bf_net->bs_scale = 1.0;
                mask_bf_net->bs_last_scale = 1.0;
            }
        }
    } else {
        mask_bf_net->bs_scale = 1.0;
        mask_bf_net->bs_last_scale = 1.0;
        mask_bf_net->bs_max_cnt = 0;
    }
}

void _phase_correction(wtk_mask_bf_net_t *mask_bf_net) {
    wtk_complex_t **vec = mask_bf_net->vec;
    wtk_complex_t **vec2 = mask_bf_net->vec2;
    int nbin = mask_bf_net->nbin;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int i, k;
    wtk_complex_t *tmp1, *tmp2, *tmp3;
    wtk_complex_t a, b;
    float c = 0;

    memcpy(vec2[0], vec[0], sizeof(wtk_complex_t) * nmicchannel);
    for (k = 1; k < nbin; ++k) {
        a.a = a.b = 0;
        b.a = b.b = 0;
        tmp1 = vec[k - 1];
        tmp2 = vec[k];
        for (i = 0; i < nmicchannel; ++i) {
            a.a += tmp1[i].a * tmp2[i].a + tmp1[i].b * tmp2[i].b;
            a.b += tmp1[i].b * tmp2[i].a - tmp1[i].a * tmp2[i].b;
        }
        c = atan2f(a.b, a.a);
        b.a = cosf(c);
        b.b = -sinf(c);
        tmp3 = vec2[k];
        for (i = 0; i < nmicchannel; ++i) {
            tmp3[i].a = tmp2[i].a * b.a - tmp2[i].b * b.b;
            tmp3[i].b = tmp2[i].a * b.b + tmp2[i].b * b.a;
        }
    }
}

void _phase_correction2(wtk_mask_bf_net_t *mask_bf_net) {
    wtk_complex_t **vec = mask_bf_net->vec;
    wtk_complex_t **vec2 = mask_bf_net->vec2;
    int nbin = mask_bf_net->nbin;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int nmicchannel_1 = (int)(nmicchannel >> 2) << 2;
    int i, k;

    for (k = 0; k < nbin; ++k) {
        if (vec[k][0].b == 0) {
            memcpy(vec2[k], vec[k], sizeof(wtk_complex_t) * nmicchannel);
        } else {
            /////////////////////////////////////////////////////////////////////
            // float scale = 0;
            // scale = 1.0 / sqrtf(vec[k][0].a * vec[k][0].a +
            //                     vec[k][0].b * vec[k][0].b);
            // for (i = 0; i < nmicchannel; ++i) {
            //     vec2[k][i].a =
            //         vec[k][i].a * vec[k][0].a + vec[k][i].b * vec[k][0].b;
            //     vec2[k][i].b =
            //         vec[k][i].b * vec[k][0].a - vec[k][i].a * vec[k][0].b;
            //     vec2[k][i].a *= scale;
            //     vec2[k][i].b *= scale;
            // }
            /////////////////////////////////////////////////////////////////////
            // 预计算关键标量值
            float ref_real = vec[k][0].a;
            float ref_imag = vec[k][0].b;
            float ref_norm_sq = ref_real * ref_real + ref_imag * ref_imag;
            float scale_val = 1.0f / sqrtf(ref_norm_sq);

            // 计算中间结果的重用值
            float scaled_real = ref_real * scale_val;
            float scaled_imag = ref_imag * scale_val;

            // 使用4路展开处理其余元素
            for (i = 0; i < nmicchannel_1; i += 4) {
                // 加载当前要处理的4个元素
                wtk_complex_t v0 = vec[k][i];
                wtk_complex_t v1 = vec[k][i + 1];
                wtk_complex_t v2 = vec[k][i + 2];
                wtk_complex_t v3 = vec[k][i + 3];

                // 计算复数乘法（一次性计算实部和虚部）
                // vec2[k][i] = conjugate(v0) * vec[k][0]
                vec2[k][i].a = v0.a * scaled_real + v0.b * scaled_imag;
                vec2[k][i].b = v0.b * scaled_real - v0.a * scaled_imag;

                vec2[k][i + 1].a = v1.a * scaled_real + v1.b * scaled_imag;
                vec2[k][i + 1].b = v1.b * scaled_real - v1.a * scaled_imag;

                vec2[k][i + 2].a = v2.a * scaled_real + v2.b * scaled_imag;
                vec2[k][i + 2].b = v2.b * scaled_real - v2.a * scaled_imag;

                vec2[k][i + 3].a = v3.a * scaled_real + v3.b * scaled_imag;
                vec2[k][i + 3].b = v3.b * scaled_real - v3.a * scaled_imag;
            }
            // 处理剩余元素（如果nmicchannel不是4的倍数）
            for (i = nmicchannel_1; i < nmicchannel; i++) {
                wtk_complex_t v = vec[k][i];
                vec2[k][i].a = v.a * scaled_real + v.b * scaled_imag;
                vec2[k][i].b = v.b * scaled_real - v.a * scaled_imag;
            }
            /////////////////////////////////////////////////////////////////////
        }
    }
}

#ifdef USE_NEON
void wtk_mask_bf_net_feed_bf(wtk_mask_bf_net_t *mask_bf_net) {
    float *mask = mask_bf_net->mask;
    int nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    int rss_iter = mask_bf_net->cfg->rss_iter;
    int update_w_cnt = mask_bf_net->cfg->update_w_cnt;
    int *update_w_freq = mask_bf_net->cfg->update_w_freq;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int nmicchannel_1 = (int)(nmicchannel >> 2) << 2;
    float *all_scov_alpha = mask_bf_net->cfg->all_scov_alpha;
    float *eta_thresh = mask_bf_net->cfg->eta_thresh;
    int *scov_alpha_idx = mask_bf_net->cfg->scov_alpha_idx;
    int scov_alpha_n = mask_bf_net->cfg->scov_alpha_n;
    float min_mask = mask_bf_net->cfg->min_mask;
    float eta_mean_thresh = mask_bf_net->cfg->eta_mean_thresh;
    float scov_entropy_thresh;
    float epsi_thresh = mask_bf_net->cfg->epsi_thresh;
    float vec_init = mask_bf_net->cfg->vec_init;
    wtk_complex_t **fft = mask_bf_net->fft;
    float *m_mask = mask_bf_net->m_mask;
    float *eta = mask_bf_net->eta;
    float *epsi = mask_bf_net->epsi;
    float eta_mean = 0;
    wtk_complex_t *fft_tmp = mask_bf_net->fft_tmp;
    wtk_complex_t **vec = mask_bf_net->vec, *vec_tmp;
    wtk_complex_t **vec2;
    wtk_complex_t **fft_chn = mask_bf_net->fft_chn;
    wtk_complex_t *fftx;
    // float *t = mask_bf_net->t;
    wtk_complex_t *scov_tmp = mask_bf_net->scov_tmp, *scov_tmp2;
    wtk_complex_t *ncov_tmp;
    wtk_complex_t *c_temp = mask_bf_net->c_temp;
    wtk_covm_t *covm = mask_bf_net->covm;
    wtk_covm_t *echo_covm = mask_bf_net->echo_covm;
    wtk_covm_t *covm_tmp;
    wtk_complex_t **speech_covar_norm = mask_bf_net->speech_covar_norm;
    wtk_complex_t **noise_covar = mask_bf_net->noise_covar;
    float entropy = 1e5;
    float tmp = 0;
    float inv_tmp = 0;
    int idx = 0;
    int i, j, k, n, m;
    float gamma = mask_bf_net->cfg->gamma;
    int eta_clip_s = mask_bf_net->cfg->eta_clip_s;
    int eta_clip_e = mask_bf_net->cfg->eta_clip_e;

    int nmicchannelx2 = nmicchannel * nmicchannel;
    float32x4_t one_vec = vdupq_n_f32(1.0f);

    int debug = 0;

    fftx = mask_bf_net->fftx;
    for (n = 0; n < num_frame; ++n, fftx += nbin, mask += nbin) {
        ++mask_bf_net->update_w;
        entropy = 1e5;
        if (mask_bf_net->cfg->use_freq_preemph) {
            int pre_clip_s = mask_bf_net->cfg->pre_clip_s;
            int pre_clip_e = mask_bf_net->cfg->pre_clip_e;
            float alpha;
            float *pre_alpha = mask_bf_net->pre_alpha;
            for (k = pre_clip_s; k < pre_clip_e; ++k) {
                idx = k + n * nbin;
                alpha = pre_alpha[k];
                for (i = 0; i < nmicchannel; ++i) {
                    fft[i][idx].a *= alpha;
                    fft[i][idx].b *= alpha;
                }
            }
        }
        for (i = 0; i < nmicchannel; ++i) {
            for (k = 0; k < nbin; ++k) {
                idx = k + n * nbin;
                fft_chn[k][i].a = fft[i][idx].a;
                fft_chn[k][i].b = fft[i][idx].b;
            }
        }

        if (debug) {
            for (k = 0; k < nbin; ++k) {
                eta[k] = mask[k];
                epsi[k] = 1 - eta[k];
                eta[k] = powf(eta[k], nmicchannel);
                epsi[k] = powf(epsi[k], nmicchannel);
            }
        } else {
            for (k = 0; k <= nbin - 4; k += 4) {
                float32x4_t mask_vec = vld1q_f32(&mask[k]);
                vst1q_f32(&eta[k], mask_vec);
                float32x4_t epsi_vec = vsubq_f32(one_vec, mask_vec);
                vst1q_f32(&epsi[k], epsi_vec);
            }
            for (; k < nbin; ++k) {
                eta[k] = mask[k];
                epsi[k] = 1 - mask[k];
            }
            for (k = 0; k < nbin; ++k) {
                eta[k] = powf(eta[k], nmicchannel);
                epsi[k] = powf(epsi[k], nmicchannel);
            }
        }

        if (debug) {
            eta_mean = wtk_float_mean(eta+eta_clip_s, eta_clip_e-eta_clip_s);
        } else {
            float32x4_t v_sum = vdupq_n_f32(0.0f);
            for (k = eta_clip_s; k <= eta_clip_e - 4; k += 4) {
                float32x4_t v_eta = vld1q_f32(&eta[k]);
                v_sum = vaddq_f32(v_sum, v_eta);
            }
            float32x2_t sum_half =
                vadd_f32(vget_low_f32(v_sum), vget_high_f32(v_sum));
            sum_half = vpadd_f32(sum_half, sum_half);
            vst1_lane_f32(&eta_mean, sum_half, 0);
            for (; k < eta_clip_e; ++k) {
                eta_mean += eta[k];
            }
            eta_mean /= (eta_clip_e - eta_clip_s);
        }

        if (debug) {
            for (k = 0; k < nbin; ++k) {
                m_mask[k] = min(max(min_mask, mask[k]), 1.0);
            }
        } else {
            float32x4_t min_vec = vdupq_n_f32(min_mask);
            float32x4_t max_vec = vdupq_n_f32(1.0);
            for (k = 0; k <= nbin - 4; k += 4) {
                float32x4_t v_mask = vld1q_f32(&mask[k]);
                float32x4_t clipped = vmaxq_f32(min_vec, v_mask);
                clipped = vminq_f32(max_vec, clipped);
                vst1q_f32(&m_mask[k], clipped);
            }
            for (; k < nbin; ++k) {
                m_mask[k] = min(max(min_mask, mask[k]), 1.0);
            }
        }

        for (i = 0; i < nmicchannel; ++i) {
            wtk_complex_t *fft_i = fft[i];
            if (debug) {
                for (k = 0; k < nbin; ++k) {
                    fft_tmp[k].a = fft_i[k].a * mask[k];
                    fft_tmp[k].b = fft_i[k].b * mask[k];
                }
            } else {
                for (k = 0; k <= nbin - 2; k += 2) {
                    float32x4_t fft_vec =
                        vld1q_f32(&fft_i[k].a); // 加载[a0,b0,a1,b1]

                    // 加载2个mask值并扩展为[m0,m0,m1,m1]
                    float32x2_t mask_half = vld1_f32(&mask[k]); // 加载[m0,m1]
                    float32x4_t mask_vec =
                        vcombine_f32(mask_half, mask_half);    // [m0,m1,m0,m1]
                    mask_vec = vzip1q_f32(mask_vec, mask_vec); // [m0,m0,m1,m1]

                    // 复数乘法：实部=a*mask，虚部=b*mask
                    float32x4_t result = vmulq_f32(fft_vec, mask_vec);

                    // 存储结果
                    vst1q_f32(&fft_tmp[k].a, result);
                }
                for (; k < nbin; ++k) {
                    fft_tmp[k].a = fft_i[k].a * mask[k];
                    fft_tmp[k].b = fft_i[k].b * mask[k];
                }
            }

            entropy =
                min(wtk_mask_bf_net_entropy(mask_bf_net, fft_tmp, 200, 3500),
                    entropy);
        }

        if (mask_bf_net->sp_state[n] == 0) {
            if (mask_bf_net->echo_in == 0) {
                for (k = 0; k < nbin; ++k) {
                    memcpy(echo_covm->scov[k], covm->scov[k],
                           nmicchannel * nmicchannel * sizeof(wtk_complex_t));
                }
                mask_bf_net->echo_in = 1;
            }
            scov_entropy_thresh = mask_bf_net->cfg->echo_scov_entropy_thresh;
            covm_tmp = echo_covm;
        } else {
            mask_bf_net->echo_in = 0;
            scov_entropy_thresh = mask_bf_net->cfg->scov_entropy_thresh;
            covm_tmp = covm;
        }

        if (mask_bf_net->cfg->use_edr_stage1_rt) {
            covm_tmp = covm;
        }

        for (k = 0; k < nbin; ++k) {
            for (i = 0; i < scov_alpha_n; ++i) {
                if (k <= scov_alpha_idx[i]) {
                    idx = i;
                    break;
                }
            }
            covm_tmp->cfg->scov_alpha = all_scov_alpha[idx];
            if (eta_mean > eta_mean_thresh && entropy < scov_entropy_thresh) {
                if (eta[k] > eta_thresh[idx]) {
                    wtk_covm_feed_fft4(covm_tmp, fft_chn[k], k, 0, eta[k]);
                }
            } else if ((eta_mean <= eta_mean_thresh &&
                        entropy < scov_entropy_thresh) ||
                       (eta_mean > eta_mean_thresh &&
                        entropy >= scov_entropy_thresh)) {
                if (epsi[k] > epsi_thresh) {
                    wtk_covm_feed_fft4(covm_tmp, fft_chn[k], k, 1, epsi[k]);
                }
            } else {
                wtk_covm_feed_fft4(covm_tmp, fft_chn[k], k, 1, epsi[k]);
            }
        }

        for (k = 0; k < nbin; ++k) {
            memcpy(speech_covar_norm[k], covm_tmp->scov[k],
                   nmicchannel * nmicchannel * sizeof(wtk_complex_t));
            memcpy(noise_covar[k], covm_tmp->ncov[k],
                   nmicchannel * nmicchannel * sizeof(wtk_complex_t));
            float s_scale = 1.0 / (covm_tmp->s_mask[k] + 1e-16);
            float n_scale = 1.0 / (covm_tmp->n_mask[k] + 1e-16);
            wtk_complex_t *s_norm = speech_covar_norm[k];
            wtk_complex_t *n_norm = noise_covar[k];
            // /////////////////////////////////////////////////////////////////////
            // // for (i = 0; i < nmicchannel * nmicchannel; ++i) {
            // //     s_norm[i].a *= s_scale;
            // //     s_norm[i].b *= s_scale;
            // //     n_norm[i].a *= n_scale;
            // //     n_norm[i].b *= n_scale;
            // // }
            // /////////////////////////////////////////////////////////////////////
            if (debug) {
                for (i = 0; i <= nmicchannelx2 - 4; i += 4) {
                    s_norm->a *= s_scale;
                    s_norm->b *= s_scale;
                    n_norm->a *= n_scale;
                    n_norm->b *= n_scale;
                    ++s_norm;
                    ++n_norm;
                    s_norm->a *= s_scale;
                    s_norm->b *= s_scale;
                    n_norm->a *= n_scale;
                    n_norm->b *= n_scale;
                    ++s_norm;
                    ++n_norm;
                    s_norm->a *= s_scale;
                    s_norm->b *= s_scale;
                    n_norm->a *= n_scale;
                    n_norm->b *= n_scale;
                    ++s_norm;
                    ++n_norm;
                    s_norm->a *= s_scale;
                    s_norm->b *= s_scale;
                    n_norm->a *= n_scale;
                    n_norm->b *= n_scale;
                    ++s_norm;
                    ++n_norm;
                }
                for (; i < nmicchannelx2; ++i) {
                    s_norm->a *= s_scale;
                    s_norm->b *= s_scale;
                    n_norm->a *= n_scale;
                    n_norm->b *= n_scale;
                    ++s_norm;
                    ++n_norm;
                }
                /////////////////////////////////////////////////////////////////////
            } else {
                for (i = 0; i <= nmicchannelx2 - 2; i += 2) {
                    float32x4_t s_real = vld1q_f32(&s_norm[i].a);
                    float32x4_t n_real = vld1q_f32(&n_norm[i].a);

                    s_real = vmulq_n_f32(s_real, s_scale);
                    n_real = vmulq_n_f32(n_real, n_scale);

                    vst1q_f32(&s_norm[i].a, s_real);
                    vst1q_f32(&n_norm[i].a, n_real);
                }
                for (; i < nmicchannelx2; ++i) {
                    s_norm[i].a *= s_scale;
                    s_norm[i].b *= s_scale;
                    n_norm[i].a *= n_scale;
                    n_norm[i].b *= n_scale;
                }
            }
        }

        if (gamma > 0) {
            float speech_diag;
            float noise_diag;
            float scaler;
            float min_scaler = 0;
            for (k = 0; k < nbin; ++k) {
                speech_diag = noise_diag = 0;
                scov_tmp = speech_covar_norm[k];
                ncov_tmp = noise_covar[k];
                for (i = 0; i < nmicchannel; ++i) {
                    speech_diag = scov_tmp[i * nmicchannel + i].a;
                    noise_diag = ncov_tmp[i * nmicchannel + i].a;
                    scaler =
                        min(max(speech_diag / (gamma * noise_diag + 1e-10), 0),
                            1.0);
                    min_scaler = min(min_scaler, scaler);
                }
                min_scaler = min(max(min_scaler, 0), 1.0);

                if (debug) {
                    for (i = 0; i < nmicchannelx2; ++i) {
                        scov_tmp[i].a -= min_scaler * ncov_tmp[i].a;
                        scov_tmp[i].b -= min_scaler * ncov_tmp[i].b;
                    }
                } else {
                    for (i = 0; i <= nmicchannelx2 - 2; i += 2) {
                        float32x4_t s_real = vld1q_f32(&scov_tmp[i].a);
                        float32x4_t n_real = vld1q_f32(&ncov_tmp[i].a);

                        s_real =
                            vsubq_f32(s_real, vmulq_n_f32(n_real, min_scaler));

                        vst1q_f32(&scov_tmp[i].a, s_real);
                        vst1q_f32(&ncov_tmp[i].a, n_real);
                    }
                    for (; i < nmicchannelx2; ++i) {
                        scov_tmp[i].a -= min_scaler * ncov_tmp[i].a;
                        scov_tmp[i].b -= min_scaler * ncov_tmp[i].b;
                    }
                }
            }
        }

        if (mask_bf_net->bf_start == 0) {
            for (k = 0; k < nbin; ++k) {
                vec_tmp = vec[k];
                memset(vec_tmp, 0, nmicchannel * sizeof(wtk_complex_t));
                // vec_tmp[nmicchannel - 1].a = 1;
                for (i = 0; i < nmicchannel; ++i) {
                    vec_tmp[i].a = vec_init;
                }
            }
            mask_bf_net->bf_start = 1;
        } else {
            // memset(t, 0, nbin * sizeof(float));
            for (k = 0; k < nbin; ++k) {
                scov_tmp2 = speech_covar_norm[k];
                memcpy(scov_tmp, scov_tmp2,
                       nmicchannel * nmicchannel * sizeof(wtk_complex_t));
                vec_tmp = vec[k];
                tmp = 0;
                if (debug) {
                    for (i = 0; i < nmicchannel; ++i) {
                        tmp += scov_tmp[i * nmicchannel + i].a *
                               scov_tmp[i * nmicchannel + i].a;
                    }
                } else {
                    float32x4_t sum_vec = vdupq_n_f32(0.0f);
                    for (i = 0; i <= nmicchannel - 4; i += 4) {
                        // 计算对角元素索引：i*(nmicchannel+1)
                        int idx0 = i * (nmicchannel + 1);
                        int idx1 = (i + 1) * (nmicchannel + 1);
                        int idx2 = (i + 2) * (nmicchannel + 1);
                        int idx3 = (i + 3) * (nmicchannel + 1);

                        // 加载4个对角元素的实部
                        float32x4_t diag_vec = {
                            scov_tmp[idx0].a, scov_tmp[idx1].a,
                            scov_tmp[idx2].a, scov_tmp[idx3].a};

                        // 平方并累加
                        sum_vec = vmlaq_f32(sum_vec, diag_vec,
                                            diag_vec); // sum += a*a
                    }
                    // 水平求和：sum_vec的4个通道相加
                    float32x2_t sum_half =
                        vadd_f32(vget_low_f32(sum_vec), vget_high_f32(sum_vec));
                    sum_half = vpadd_f32(sum_half, sum_half);
                    vst1_lane_f32(&tmp, sum_half, 0);

                    // 处理尾部剩余元素（不足4个）
                    for (; i < nmicchannel; ++i) {
                        int idx = i * (nmicchannel + 1);
                        tmp += scov_tmp[idx].a * scov_tmp[idx].a;
                    }
                }

                tmp = sqrtf(tmp) + 1e-16;

                inv_tmp = 1.0 / tmp;
                if (debug) {
                    for (i = 0; i < nmicchannelx2; ++i) {
                        scov_tmp[i].a = scov_tmp[i].a * inv_tmp;
                        scov_tmp[i].b = scov_tmp[i].b * inv_tmp;
                    }
                } else {
                    // 创建缩放因子向量
                    float32x4_t inv_vec =
                        vdupq_n_f32(inv_tmp); // [inv, inv, inv, inv]

                    // 每次处理4个复数（8个float）
                    for (i = 0; i <= nmicchannelx2 - 2; i += 2) {
                        // 加载4个复数（实部和虚部交错）
                        float32x4_t data_vec =
                            vld1q_f32(&scov_tmp[i].a); // [a0,b0,a1,b1]

                        // 向量化缩放：实部和虚部同时乘以inv_tmp
                        float32x4_t result = vmulq_f32(data_vec, inv_vec);

                        // 存储结果
                        vst1q_f32(&scov_tmp[i].a, result);
                    }

                    // 处理尾部剩余数据（不足4个复数）
                    for (; i < nmicchannelx2; ++i) {
                        scov_tmp[i].a *= inv_tmp;
                        scov_tmp[i].b *= inv_tmp;
                    }
                }

                for (m = 0; m < rss_iter; ++m) {
                    /////////////////////////////////////////////////////////////////////
                    // memset(c_temp, 0, nmicchannel * sizeof(wtk_complex_t));
                    // for (i = 0; i < nmicchannel; ++i) {
                    //     for (j = 0; j < nmicchannel; ++j) {
                    //         c_temp[i].a +=
                    //             scov_tmp[i * nmicchannel + j].a *
                    //             vec_tmp[j].a - scov_tmp[i * nmicchannel +
                    //             j].b * vec_tmp[j].b;
                    //         c_temp[i].b +=
                    //             scov_tmp[i * nmicchannel + j].a *
                    //             vec_tmp[j].b + scov_tmp[i * nmicchannel +
                    //             j].b * vec_tmp[j].a;
                    //     }
                    // }
                    // memcpy(vec_tmp, c_temp,
                    //        nmicchannel * sizeof(wtk_complex_t));
                    // /////////////////////////////////////////////////////////////////////
                    if (debug || 1) { //// Todo: 优化
                        memset(c_temp, 0, nmicchannel * sizeof(wtk_complex_t));
                        for (i = 0; i < nmicchannel; ++i) {
                            float sum_real = 0.0f;
                            float sum_imag = 0.0f;
                            wtk_complex_t *scov_row =
                                &scov_tmp[i * nmicchannel]; // 行指针优化

                            // 主循环：每次处理4个通道（展开4次）
                            for (j = 0; j < nmicchannel_1; j += 4) {
                                // 预加载向量元素
                                wtk_complex_t v0 = vec_tmp[j];
                                wtk_complex_t v1 = vec_tmp[j + 1];
                                wtk_complex_t v2 = vec_tmp[j + 2];
                                wtk_complex_t v3 = vec_tmp[j + 3];

                                // 预加载矩阵行元素
                                wtk_complex_t m0 = scov_row[j];
                                wtk_complex_t m1 = scov_row[j + 1];
                                wtk_complex_t m2 = scov_row[j + 2];
                                wtk_complex_t m3 = scov_row[j + 3];

                                // 第一组复数乘法 (m0*v0)
                                sum_real += m0.a * v0.a - m0.b * v0.b;
                                sum_imag += m0.a * v0.b + m0.b * v0.a;

                                // 第二组复数乘法 (m1*v1)
                                sum_real += m1.a * v1.a - m1.b * v1.b;
                                sum_imag += m1.a * v1.b + m1.b * v1.a;

                                // 第三组复数乘法 (m2*v2)
                                sum_real += m2.a * v2.a - m2.b * v2.b;
                                sum_imag += m2.a * v2.b + m2.b * v2.a;

                                // 第四组复数乘法 (m3*v3)
                                sum_real += m3.a * v3.a - m3.b * v3.b;
                                sum_imag += m3.a * v3.b + m3.b * v3.a;
                            }
                            // 处理剩余的通道（如果nmicchannel不是4的倍数）
                            for (j = nmicchannel_1; j < nmicchannel; ++j) {
                                sum_real += scov_row[j].a * vec_tmp[j].a -
                                            scov_row[j].b * vec_tmp[j].b;
                                sum_imag += scov_row[j].a * vec_tmp[j].b +
                                            scov_row[j].b * vec_tmp[j].a;
                            }

                            // 保存结果
                            c_temp[i].a = sum_real;
                            c_temp[i].b = sum_imag;
                        }
                        memcpy(vec_tmp, c_temp,
                               nmicchannel * sizeof(wtk_complex_t));
                        // /////////////////////////////////////////////////////////////////////
                    }
                }

                /////////////////////////////////////////////////////////////////////
                // tmp = 0;
                // for (i = 0; i < nmicchannel; ++i) {
                //     tmp += vec_tmp[i].a * vec_tmp[i].a +
                //            vec_tmp[i].b * vec_tmp[i].b;
                // }
                // tmp = sqrtf(tmp);
                /////////////////////////////////////////////////////////////////////
                tmp = 0;
                if (debug) {
                    // 计算模长的平方和（4路展开）
                    for (i = 0; i < nmicchannel_1; i += 4) {
                        wtk_complex_t v0 = vec_tmp[i];
                        wtk_complex_t v1 = vec_tmp[i + 1];
                        wtk_complex_t v2 = vec_tmp[i + 2];
                        wtk_complex_t v3 = vec_tmp[i + 3];

                        tmp += v0.a * v0.a + v0.b * v0.b;
                        tmp += v1.a * v1.a + v1.b * v1.b;
                        tmp += v2.a * v2.a + v2.b * v2.b;
                        tmp += v3.a * v3.a + v3.b * v3.b;
                    }
                    // 处理剩余的通道（如果nmicchannel不是4的倍数）
                    for (i = nmicchannel_1; i < nmicchannel; ++i) {
                        tmp += vec_tmp[i].a * vec_tmp[i].a +
                               vec_tmp[i].b * vec_tmp[i].b;
                    }
                } else {
                    // 每次处理2个复数（4个float）
                    for (i = 0; i <= nmicchannel - 2; i += 2) {
                        // 加载2个复数（实部和虚部交错）
                        float32x4_t vec =
                            vld1q_f32(&vec_tmp[i].a); // [a0,b0,a1,b1]

                        // 计算实部平方和虚部平方
                        float32x4_t square =
                            vmulq_f32(vec, vec); // [a0²,b0²,a1²,b1²]

                        float32_t sum_half = vaddvq_f32(square);
                        tmp += sum_half;
                    }
                    // 处理尾部剩余数据（不足4个复数）
                    for (; i < nmicchannel; ++i) {
                        tmp += vec_tmp[i].a * vec_tmp[i].a +
                               vec_tmp[i].b * vec_tmp[i].b;
                    }
                }

                tmp = sqrtf(tmp);
                /////////////////////////////////////////////////////////////////////
                if (tmp < 1e-16) {
                    memset(vec_tmp, 0, nmicchannel * sizeof(wtk_complex_t));
                    // vec_tmp[nmicchannel - 1].a = 1;
                    for (i = 0; i < nmicchannel; ++i) {
                        vec_tmp[i].a = vec_init;
                    }
                }
                /////////////////////////////////////////////////////////////////////
                // tmp = 0;
                // for (i = 0; i < nmicchannel; ++i) {
                //     tmp += vec_tmp[i].a * vec_tmp[i].a +
                //            vec_tmp[i].b * vec_tmp[i].b;
                // }
                // tmp = sqrtf(tmp);
                // for (i = 0; i < nmicchannel; ++i) {
                //     vec_tmp[i].a /= tmp;
                //     vec_tmp[i].b /= tmp;
                // }
                /////////////////////////////////////////////////////////////////////
                tmp = 0;
                if (debug) {
                    // 计算模长的平方和（4路展开）
                    for (i = 0; i < nmicchannel_1; i += 4) {
                        wtk_complex_t v0 = vec_tmp[i];
                        wtk_complex_t v1 = vec_tmp[i + 1];
                        wtk_complex_t v2 = vec_tmp[i + 2];
                        wtk_complex_t v3 = vec_tmp[i + 3];

                        tmp += v0.a * v0.a + v0.b * v0.b;
                        tmp += v1.a * v1.a + v1.b * v1.b;
                        tmp += v2.a * v2.a + v2.b * v2.b;
                        tmp += v3.a * v3.a + v3.b * v3.b;
                    }
                    // 处理剩余的通道（如果nmicchannel不是4的倍数）
                    for (i = nmicchannel_1; i < nmicchannel; ++i) {
                        tmp += vec_tmp[i].a * vec_tmp[i].a +
                               vec_tmp[i].b * vec_tmp[i].b;
                    }
                } else {
                    // 每次处理2个复数（4个float）
                    for (i = 0; i <= nmicchannel - 2; i += 2) {
                        // 加载2个复数（实部和虚部交错）
                        float32x4_t vec =
                            vld1q_f32(&vec_tmp[i].a); // [a0,b0,a1,b1]

                        // 计算实部平方和虚部平方
                        float32x4_t square =
                            vmulq_f32(vec, vec); // [a0²,b0²,a1²,b1²]

                        float32_t sum_half = vaddvq_f32(square);
                        tmp += sum_half;
                    }
                    // 处理尾部剩余数据（不足4个复数）
                    for (; i < nmicchannel; ++i) {
                        tmp += vec_tmp[i].a * vec_tmp[i].a +
                               vec_tmp[i].b * vec_tmp[i].b;
                    }
                }

                tmp = sqrtf(tmp);
                inv_tmp = 1.0 / tmp;
                if (debug) {
                    for (i = 0; i < nmicchannel; ++i) {
                        vec_tmp[i].a *= inv_tmp;
                        vec_tmp[i].b *= inv_tmp;
                    }
                } else {
                    // 创建缩放因子向量
                    float32x4_t inv_vec =
                        vdupq_n_f32(inv_tmp); // [inv, inv, inv, inv]

                    // 每次处理4个复数（8个float）
                    for (i = 0; i <= nmicchannel - 2; i += 2) {
                        // 加载4个复数（实部和虚部交错）
                        float32x4_t vec =
                            vld1q_f32(&vec_tmp[i].a); // [a0,b0,a1,b1]

                        // 向量化缩放：实部和虚部同时乘以inv_tmp
                        float32x4_t result = vmulq_f32(vec, inv_vec);

                        // 存储结果
                        vst1q_f32(&vec_tmp[i].a, result);
                    }

                    // 处理尾部剩余数据（不足4个复数）
                    for (; i < nmicchannel; ++i) {
                        vec_tmp[i].a *= inv_tmp;
                        vec_tmp[i].b *= inv_tmp;
                    }
                }
                /////////////////////////////////////////////////////////////////////
                for (i = 0; i < nmicchannel; ++i) {
                    if (isnan(vec_tmp[i].a) || isnan(vec_tmp[i].b)) {
                        wtk_debug("nan detected in vec[%d][%d].a=%f, "
                                  "vec[%d][%d].b=%f\n",
                                  k, i, vec_tmp[i].a, k, i, vec_tmp[i].b);
                        exit(1);
                    }
                }
            }
        }

        if (mask_bf_net->cfg->use_phase_corr) {
            // _phase_correction(mask_bf_net);
            _phase_correction2(mask_bf_net);
            vec2 = mask_bf_net->vec2;
        } else {
            vec2 = mask_bf_net->vec;
        }
        tmp = 1.0 / nmicchannel * 2;
        if (mask_bf_net->bf_init_frame < mask_bf_net->cfg->bf_init_frame) {
            ++mask_bf_net->bf_init_frame;
            memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
            for (k = 0; k < nbin; ++k) {
                fftx[k].a = fft_chn[k][0].a * m_mask[k];
                fftx[k].b = fft_chn[k][0].b * m_mask[k];
            }
        } else {
            for (k = 0; k < nbin; ++k) {
                memcpy(mask_bf_net->bf->ovec[k], vec2[k],
                       nmicchannel * sizeof(wtk_complex_t));

                wtk_bf_update_ncov(mask_bf_net->bf, noise_covar, k);
                if (mask_bf_net->update_w == update_w_freq[k]) {
                    wtk_bf_update_mvdr_w_f(mask_bf_net->bf, 0, k);
                }
                wtk_bf_output_fft_k(mask_bf_net->bf, fft_chn[k], fftx + k, k);

                if (isnan(fftx[k].a) || isnan(fftx[k].b)) {
                    wtk_debug("nan detected in fftx[%d].a=%f, fftx[%d].b=%f, "
                              "please check mic\n",
                              k, fftx[k].a, k, fftx[k].b);
                    exit(1);
                }
                fftx[k].a *= m_mask[k] * tmp;
                fftx[k].b *= m_mask[k] * tmp;
            }
        }
        if (mask_bf_net->update_w % update_w_cnt == 0) {
            mask_bf_net->update_w = 0;
        }
        if (mask_bf_net->ovec) {
            wtk_complex_t **ovec;
            wtk_complex_t *ovec1;
            float eng_sum = 0;
            float max_eng = 0;
            int ds_idx = 0;
            int n;
            for (n = 0; n < mask_bf_net->cfg->ntheta; ++n) {
                ovec = mask_bf_net->ovec[n];
                memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
                eng_sum = 0;
                for (k = 0; k < nbin; ++k) {
                    ovec1 = ovec[k];
                    for (i = 0; i < nmicchannel; ++i) {
                        fftx[k].a +=
                            ovec1[i].a * fft[i][k].a + ovec1[i].b * fft[i][k].b;
                        fftx[k].b +=
                            ovec1[i].a * fft[i][k].b - ovec1[i].b * fft[i][k].a;
                    }
                    eng_sum += fftx[k].a * fftx[k].a + fftx[k].b * fftx[k].b;
                }
                if (eng_sum > max_eng) {
                    max_eng = eng_sum;
                    ds_idx = n;
                }
            }
            memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
            for (k = 0; k < nbin; ++k) {
                ovec1 = mask_bf_net->ovec[ds_idx][k];
                for (i = 0; i < nmicchannel; ++i) {
                    fftx[k].a += ovec1[i].a * fft_chn[k][i].a +
                                 ovec1[i].b * fft_chn[k][i].b;
                    fftx[k].b += ovec1[i].a * fft_chn[k][i].b -
                                 ovec1[i].b * fft_chn[k][i].a;
                }
            }
        }
    }
}

#else

void wtk_mask_bf_net_feed_bf(wtk_mask_bf_net_t *mask_bf_net) {
    float *mask = mask_bf_net->mask;
    int nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    int rss_iter = mask_bf_net->cfg->rss_iter;
    int update_w_cnt = mask_bf_net->cfg->update_w_cnt;
    int *update_w_freq = mask_bf_net->cfg->update_w_freq;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int nmicchannel_1 = (int)(nmicchannel >> 2) << 2;
    float *all_scov_alpha = mask_bf_net->cfg->all_scov_alpha;
    float *eta_thresh = mask_bf_net->cfg->eta_thresh;
    int *scov_alpha_idx = mask_bf_net->cfg->scov_alpha_idx;
    int scov_alpha_n = mask_bf_net->cfg->scov_alpha_n;
    float min_mask = mask_bf_net->cfg->min_mask;
    float eta_mean_thresh = mask_bf_net->cfg->eta_mean_thresh;
    float scov_entropy_thresh;
    float epsi_thresh = mask_bf_net->cfg->epsi_thresh;
    float vec_init = mask_bf_net->cfg->vec_init;
    wtk_complex_t **fft = mask_bf_net->fft;
    float *m_mask = mask_bf_net->m_mask;
    float *eta = mask_bf_net->eta;
    float *epsi = mask_bf_net->epsi;
    float eta_mean = 0;
    wtk_complex_t *fft_tmp = mask_bf_net->fft_tmp;
    wtk_complex_t **vec = mask_bf_net->vec, *vec_tmp;
    wtk_complex_t **vec2;
    wtk_complex_t **fft_chn = mask_bf_net->fft_chn;
    wtk_complex_t *fftx;
    // float *t = mask_bf_net->t;
    wtk_complex_t *scov_tmp = mask_bf_net->scov_tmp, *scov_tmp2;
    wtk_complex_t *ncov_tmp;
    wtk_complex_t *c_temp = mask_bf_net->c_temp;
    wtk_covm_t *covm = mask_bf_net->covm;
    wtk_covm_t *echo_covm = mask_bf_net->echo_covm;
    wtk_covm_t *covm_tmp;
    wtk_complex_t **speech_covar_norm = mask_bf_net->speech_covar_norm;
    wtk_complex_t **noise_covar = mask_bf_net->noise_covar;
    float entropy = 1e5;
    float tmp = 0;
    float inv_tmp = 0;
    int idx = 0;
    int i, j, k, n, m;
    float gamma = mask_bf_net->cfg->gamma;
    int eta_clip_s = mask_bf_net->cfg->eta_clip_s;
    int eta_clip_e = mask_bf_net->cfg->eta_clip_e;

    // wtk_drft_t *rfft = mask_bf_net->rfft;
    // float *rfft_in = mask_bf_net->rfft_in;
    // float *synthesis_mem = mask_bf_net->synthesis_mem;
    // float *synthesis_window = mask_bf_net->synthesis_window;
    // float *out = mask_bf_net->out;
    // short *pv = (short *)out;
    // int wins = mask_bf_net->cfg->wins;
    // int fsize = wins / 2;

    fftx = mask_bf_net->fftx;
    for (n = 0; n < num_frame; ++n, fftx += nbin, mask += nbin) {
        ++mask_bf_net->update_w;
        entropy = 1e5;
        if (mask_bf_net->cfg->use_freq_preemph) {
            int pre_clip_s = mask_bf_net->cfg->pre_clip_s;
            int pre_clip_e = mask_bf_net->cfg->pre_clip_e;
            float alpha;
            float *pre_alpha = mask_bf_net->pre_alpha;
            for (k = pre_clip_s; k < pre_clip_e; ++k) {
                idx = k + n * nbin;
                alpha = pre_alpha[k];
                for (i = 0; i < nmicchannel; ++i) {
                    fft[i][idx].a *= alpha;
                    fft[i][idx].b *= alpha;
                }
            }
        }
        for (i = 0; i < nmicchannel; ++i) {
            for (k = 0; k < nbin; ++k) {
                idx = k + n * nbin;
                fft_chn[k][i].a = fft[i][idx].a;
                fft_chn[k][i].b = fft[i][idx].b;
            }
        }

        for (k = 0; k < nbin; ++k) {
            eta[k] = mask[k];
            epsi[k] = 1 - eta[k];
            eta[k] = powf(eta[k], nmicchannel);
            epsi[k] = powf(epsi[k], nmicchannel);
        }

        eta_mean = wtk_float_mean(eta+eta_clip_s, eta_clip_e-eta_clip_s);
        for (k = 0; k < nbin; ++k) {
            m_mask[k] = min(max(min_mask, mask[k]), 1.0);
        }

        for (i = 0; i < nmicchannel; ++i) {
            for (k = 0; k < nbin; ++k) {
                fft_tmp[k].a = fft[i][k].a * mask[k];
                fft_tmp[k].b = fft[i][k].b * mask[k];
            }
            entropy =
                min(wtk_mask_bf_net_entropy(mask_bf_net, fft_tmp, 200, 3500),
                    entropy);
        }
        // printf("%f\n", eta_mean);
        // printf("%f\n", entropy);
        // float epsi_mean = 0;
        // epsi_mean = wtk_float_mean(epsi+eta_clip_s, eta_clip_e-eta_clip_s);
        // printf("%f\n", epsi_mean);

        if (mask_bf_net->sp_state[n] == 0) {
            if (mask_bf_net->echo_in == 0) {
                for (k = 0; k < nbin; ++k) {
                    memcpy(echo_covm->scov[k], covm->scov[k],
                           nmicchannel * nmicchannel * sizeof(wtk_complex_t));
                    // memcpy(echo_covm->ncov[k], covm->ncov[k],
                    //        nmicchannel * nmicchannel *
                    //        sizeof(wtk_complex_t));
                }
                mask_bf_net->echo_in = 1;
            }
            scov_entropy_thresh = mask_bf_net->cfg->echo_scov_entropy_thresh;
            covm_tmp = echo_covm;
        } else {
            mask_bf_net->echo_in = 0;
            scov_entropy_thresh = mask_bf_net->cfg->scov_entropy_thresh;
            covm_tmp = covm;
        }

        if (mask_bf_net->cfg->use_edr_stage1_rt) {
            covm_tmp = covm;
        }

        for (k = 0; k < nbin; ++k) {
            for (i = 0; i < scov_alpha_n; ++i) {
                if (k <= scov_alpha_idx[i]) {
                    idx = i;
                    break;
                }
            }
            covm_tmp->cfg->scov_alpha = all_scov_alpha[idx];
            if (eta_mean > eta_mean_thresh && entropy < scov_entropy_thresh) {
                if (eta[k] > eta_thresh[idx]) {
                    wtk_covm_feed_fft4(covm_tmp, fft_chn[k], k, 0, eta[k]);
                }
            } else if ((eta_mean <= eta_mean_thresh &&
                        entropy < scov_entropy_thresh) ||
                       (eta_mean > eta_mean_thresh &&
                        entropy >= scov_entropy_thresh)) {
                if (epsi[k] > epsi_thresh) {
                    wtk_covm_feed_fft4(covm_tmp, fft_chn[k], k, 1, epsi[k]);
                }
            } else {
                wtk_covm_feed_fft4(covm_tmp, fft_chn[k], k, 1, epsi[k]);
            }
        }

        for (k = 0; k < nbin; ++k) {
            memcpy(speech_covar_norm[k], covm_tmp->scov[k],
                   nmicchannel * nmicchannel * sizeof(wtk_complex_t));
            memcpy(noise_covar[k], covm_tmp->ncov[k],
                   nmicchannel * nmicchannel * sizeof(wtk_complex_t));
            float s_scale = 1.0 / (covm_tmp->s_mask[k] + 1e-16);
            float n_scale = 1.0 / (covm_tmp->n_mask[k] + 1e-16);
            wtk_complex_t *s_norm = speech_covar_norm[k];
            wtk_complex_t *n_norm = noise_covar[k];
            /////////////////////////////////////////////////////////////////////
            // for (i = 0; i < nmicchannel * nmicchannel; ++i) {
            //     speech_covar_norm[k][i].a *= s_scale;
            //     speech_covar_norm[k][i].b *= s_scale;
            //     noise_covar[k][i].a *= n_scale;
            //     noise_covar[k][i].b *= n_scale;
            // }
            /////////////////////////////////////////////////////////////////////
            int chunk_size = nmicchannel * nmicchannel;
            for (i = 0; i <= chunk_size - 4; i += 4) {
                s_norm->a *= s_scale;
                s_norm->b *= s_scale;
                n_norm->a *= n_scale;
                n_norm->b *= n_scale;
                ++s_norm;
                ++n_norm;
                s_norm->a *= s_scale;
                s_norm->b *= s_scale;
                n_norm->a *= n_scale;
                n_norm->b *= n_scale;
                ++s_norm;
                ++n_norm;
                s_norm->a *= s_scale;
                s_norm->b *= s_scale;
                n_norm->a *= n_scale;
                n_norm->b *= n_scale;
                ++s_norm;
                ++n_norm;
                s_norm->a *= s_scale;
                s_norm->b *= s_scale;
                n_norm->a *= n_scale;
                n_norm->b *= n_scale;
                ++s_norm;
                ++n_norm;
            }
            for (; i < chunk_size; ++i) {
                s_norm->a *= s_scale;
                s_norm->b *= s_scale;
                n_norm->a *= n_scale;
                n_norm->b *= n_scale;
                ++s_norm;
                ++n_norm;
            }
            /////////////////////////////////////////////////////////////////////
        }

        if (gamma > 0) {
            float speech_diag;
            float noise_diag;
            float scaler;
            float min_scaler = 0;
            for (k = 0; k < nbin; ++k) {
                speech_diag = noise_diag = 0;
                scov_tmp = speech_covar_norm[k];
                ncov_tmp = noise_covar[k];
                for (i = 0; i < nmicchannel; ++i) {
                    speech_diag = scov_tmp[i * nmicchannel + i].a;
                    noise_diag = ncov_tmp[i * nmicchannel + i].a;
                    scaler =
                        min(max(speech_diag / (gamma * noise_diag + 1e-10), 0),
                            1.0);
                    min_scaler = min(min_scaler, scaler);
                }
                min_scaler = min(max(min_scaler, 0), 1.0);

                for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                    scov_tmp[i].a -= min_scaler * ncov_tmp[i].a;
                    scov_tmp[i].b -= min_scaler * ncov_tmp[i].b;
                }
            }
        }

        if (mask_bf_net->bf_start == 0) {
            for (k = 0; k < nbin; ++k) {
                vec_tmp = vec[k];
                memset(vec_tmp, 0, nmicchannel * sizeof(wtk_complex_t));
                // vec_tmp[nmicchannel - 1].a = 1;
                for (i = 0; i < nmicchannel; ++i) {
                    vec_tmp[i].a = vec_init;
                }
            }
            mask_bf_net->bf_start = 1;
        } else {
            // memset(t, 0, nbin * sizeof(float));
            for (k = 0; k < nbin; ++k) {
                scov_tmp2 = speech_covar_norm[k];
                memcpy(scov_tmp, scov_tmp2,
                       nmicchannel * nmicchannel * sizeof(wtk_complex_t));
                vec_tmp = vec[k];
                tmp = 0;
                for (i = 0; i < nmicchannel; ++i) {
                    tmp += scov_tmp[i * nmicchannel + i].a *
                           scov_tmp[i * nmicchannel + i].a;
                }
                tmp = sqrtf(tmp) + 1e-16;

                inv_tmp = 1.0 / tmp;
                for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                    scov_tmp[i].a = scov_tmp[i].a * inv_tmp;
                    scov_tmp[i].b = scov_tmp[i].b * inv_tmp;
                }

                for (m = 0; m < rss_iter; ++m) {
                    /////////////////////////////////////////////////////////////////////
                    // memset(c_temp, 0, nmicchannel * sizeof(wtk_complex_t));
                    // for (i = 0; i < nmicchannel; ++i) {
                    //     for (j = 0; j < nmicchannel; ++j) {
                    //         c_temp[i].a +=
                    //             scov_tmp[i * nmicchannel + j].a *
                    //             vec_tmp[j].a - scov_tmp[i * nmicchannel +
                    //             j].b * vec_tmp[j].b;
                    //         c_temp[i].b +=
                    //             scov_tmp[i * nmicchannel + j].a *
                    //             vec_tmp[j].b + scov_tmp[i * nmicchannel +
                    //             j].b * vec_tmp[j].a;
                    //     }
                    // }
                    // memcpy(vec_tmp, c_temp,
                    //        nmicchannel * sizeof(wtk_complex_t));
                    /////////////////////////////////////////////////////////////////////
                    memset(c_temp, 0, nmicchannel * sizeof(wtk_complex_t));
                    for (i = 0; i < nmicchannel; ++i) {
                        float sum_real = 0.0f;
                        float sum_imag = 0.0f;
                        wtk_complex_t *scov_row =
                            &scov_tmp[i * nmicchannel]; // 行指针优化

                        // 主循环：每次处理4个通道（展开4次）
                        for (j = 0; j < nmicchannel_1; j += 4) {
                            // 预加载向量元素
                            wtk_complex_t v0 = vec_tmp[j];
                            wtk_complex_t v1 = vec_tmp[j + 1];
                            wtk_complex_t v2 = vec_tmp[j + 2];
                            wtk_complex_t v3 = vec_tmp[j + 3];

                            // 预加载矩阵行元素
                            wtk_complex_t m0 = scov_row[j];
                            wtk_complex_t m1 = scov_row[j + 1];
                            wtk_complex_t m2 = scov_row[j + 2];
                            wtk_complex_t m3 = scov_row[j + 3];

                            // 第一组复数乘法 (m0*v0)
                            sum_real += m0.a * v0.a - m0.b * v0.b;
                            sum_imag += m0.a * v0.b + m0.b * v0.a;

                            // 第二组复数乘法 (m1*v1)
                            sum_real += m1.a * v1.a - m1.b * v1.b;
                            sum_imag += m1.a * v1.b + m1.b * v1.a;

                            // 第三组复数乘法 (m2*v2)
                            sum_real += m2.a * v2.a - m2.b * v2.b;
                            sum_imag += m2.a * v2.b + m2.b * v2.a;

                            // 第四组复数乘法 (m3*v3)
                            sum_real += m3.a * v3.a - m3.b * v3.b;
                            sum_imag += m3.a * v3.b + m3.b * v3.a;
                        }
                        // 处理剩余的通道（如果nmicchannel不是4的倍数）
                        for (j = nmicchannel_1; j < nmicchannel; ++j) {
                            sum_real += scov_row[j].a * vec_tmp[j].a -
                                        scov_row[j].b * vec_tmp[j].b;
                            sum_imag += scov_row[j].a * vec_tmp[j].b +
                                        scov_row[j].b * vec_tmp[j].a;
                        }

                        // 保存结果
                        c_temp[i].a = sum_real;
                        c_temp[i].b = sum_imag;
                    }
                    memcpy(vec_tmp, c_temp,
                           nmicchannel * sizeof(wtk_complex_t));
                    /////////////////////////////////////////////////////////////////////
                }

                /////////////////////////////////////////////////////////////////////
                // tmp = 0;
                // for (i = 0; i < nmicchannel; ++i) {
                //     tmp += vec_tmp[i].a * vec_tmp[i].a +
                //            vec_tmp[i].b * vec_tmp[i].b;
                // }
                // tmp = sqrtf(tmp);
                /////////////////////////////////////////////////////////////////////
                tmp = 0;
                // 计算模长的平方和（4路展开）
                for (i = 0; i < nmicchannel_1; i += 4) {
                    wtk_complex_t v0 = vec_tmp[i];
                    wtk_complex_t v1 = vec_tmp[i + 1];
                    wtk_complex_t v2 = vec_tmp[i + 2];
                    wtk_complex_t v3 = vec_tmp[i + 3];

                    tmp += v0.a * v0.a + v0.b * v0.b;
                    tmp += v1.a * v1.a + v1.b * v1.b;
                    tmp += v2.a * v2.a + v2.b * v2.b;
                    tmp += v3.a * v3.a + v3.b * v3.b;
                }
                // 处理剩余的通道（如果nmicchannel不是4的倍数）
                for (i = nmicchannel_1; i < nmicchannel; ++i) {
                    tmp += vec_tmp[i].a * vec_tmp[i].a +
                           vec_tmp[i].b * vec_tmp[i].b;
                }
                tmp = sqrtf(tmp);
                /////////////////////////////////////////////////////////////////////
                if (tmp < 1e-16) {
                    memset(vec_tmp, 0, nmicchannel * sizeof(wtk_complex_t));
                    // vec_tmp[nmicchannel - 1].a = 1;
                    for (i = 0; i < nmicchannel; ++i) {
                        vec_tmp[i].a = vec_init;
                    }
                }
                /////////////////////////////////////////////////////////////////////
                // tmp = 0;
                // for (i = 0; i < nmicchannel; ++i) {
                //     tmp += vec_tmp[i].a * vec_tmp[i].a +
                //            vec_tmp[i].b * vec_tmp[i].b;
                // }
                // tmp = sqrtf(tmp);
                // for (i = 0; i < nmicchannel; ++i) {
                //     vec_tmp[i].a /= tmp;
                //     vec_tmp[i].b /= tmp;
                // }
                /////////////////////////////////////////////////////////////////////
                tmp = 0;
                // 计算模长的平方和（4路展开）
                for (i = 0; i < nmicchannel_1; i += 4) {
                    wtk_complex_t v0 = vec_tmp[i];
                    wtk_complex_t v1 = vec_tmp[i + 1];
                    wtk_complex_t v2 = vec_tmp[i + 2];
                    wtk_complex_t v3 = vec_tmp[i + 3];

                    tmp += v0.a * v0.a + v0.b * v0.b;
                    tmp += v1.a * v1.a + v1.b * v1.b;
                    tmp += v2.a * v2.a + v2.b * v2.b;
                    tmp += v3.a * v3.a + v3.b * v3.b;
                }
                // 处理剩余的通道（如果nmicchannel不是4的倍数）
                for (i = nmicchannel_1; i < nmicchannel; ++i) {
                    tmp += vec_tmp[i].a * vec_tmp[i].a +
                           vec_tmp[i].b * vec_tmp[i].b;
                }
                tmp = sqrtf(tmp);
                inv_tmp = 1.0 / tmp;
                for (i = 0; i < nmicchannel; ++i) {
                    vec_tmp[i].a *= inv_tmp;
                    vec_tmp[i].b *= inv_tmp;
                }
                /////////////////////////////////////////////////////////////////////
                for (i = 0; i < nmicchannel; ++i) {
                    if (isnan(vec_tmp[i].a) || isnan(vec_tmp[i].b)) {
                        wtk_debug("nan detected in vec[%d][%d].a=%f, "
                                  "vec[%d][%d].b=%f\n",
                                  k, i, vec_tmp[i].a, k, i, vec_tmp[i].b);
                        exit(1);
                    }
                }
            }
        }

        if (mask_bf_net->cfg->use_phase_corr) {
            // _phase_correction(mask_bf_net);
            _phase_correction2(mask_bf_net);
            vec2 = mask_bf_net->vec2;
        } else {
            vec2 = mask_bf_net->vec;
        }
        tmp = 1.0 / nmicchannel * 2;
        if (mask_bf_net->bf_init_frame < mask_bf_net->cfg->bf_init_frame) {
            ++mask_bf_net->bf_init_frame;
            memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
            for (k = 0; k < nbin; ++k) {
                fftx[k].a = fft_chn[k][0].a * m_mask[k];
                fftx[k].b = fft_chn[k][0].b * m_mask[k];
            }
        } else {
            for (k = 0; k < nbin; ++k) {
                memcpy(mask_bf_net->bf->ovec[k], vec2[k],
                       nmicchannel * sizeof(wtk_complex_t));

                wtk_bf_update_ncov(mask_bf_net->bf, noise_covar, k);
                if (mask_bf_net->update_w == update_w_freq[k]) {
                    wtk_bf_update_mvdr_w_f(mask_bf_net->bf, 0, k);
                }
                wtk_bf_output_fft_k(mask_bf_net->bf, fft_chn[k], fftx + k, k);

                if (isnan(fftx[k].a) || isnan(fftx[k].b)) {
                    wtk_debug("nan detected in fftx[%d].a=%f, fftx[%d].b=%f, "
                              "please check mic\n",
                              k, fftx[k].a, k, fftx[k].b);
                    exit(1);
                }
                fftx[k].a *= m_mask[k] * tmp;
                fftx[k].b *= m_mask[k] * tmp;
            }
        }
        // memcpy(fftx, fft[0] + n * nbin, sizeof(wtk_complex_t) * nbin);
        // for (k = 0; k < nbin; ++k) {
        //     fftx[k].a *= m_mask[k];
        //     fftx[k].b *= m_mask[k];
        // }
        if (mask_bf_net->update_w % update_w_cnt == 0) {
            mask_bf_net->update_w = 0;
        }
        if (mask_bf_net->ovec) {
            wtk_complex_t **ovec;
            wtk_complex_t *ovec1;
            float eng_sum = 0;
            float max_eng = 0;
            int ds_idx = 0;
            int n;
            for (n = 0; n < mask_bf_net->cfg->ntheta; ++n) {
                ovec = mask_bf_net->ovec[n];
                memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
                eng_sum = 0;
                for (k = 0; k < nbin; ++k) {
                    ovec1 = ovec[k];
                    for (i = 0; i < nmicchannel; ++i) {
                        fftx[k].a +=
                            ovec1[i].a * fft[i][k].a + ovec1[i].b * fft[i][k].b;
                        fftx[k].b +=
                            ovec1[i].a * fft[i][k].b - ovec1[i].b * fft[i][k].a;
                    }
                    eng_sum += fftx[k].a * fftx[k].a + fftx[k].b * fftx[k].b;
                }
                if (eng_sum > max_eng) {
                    max_eng = eng_sum;
                    ds_idx = n;
                }
            }
            memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
            for (k = 0; k < nbin; ++k) {
                ovec1 = mask_bf_net->ovec[ds_idx][k];
                for (i = 0; i < nmicchannel; ++i) {
                    fftx[k].a += ovec1[i].a * fft_chn[k][i].a +
                                 ovec1[i].b * fft_chn[k][i].b;
                    fftx[k].b += ovec1[i].a * fft_chn[k][i].b -
                                 ovec1[i].b * fft_chn[k][i].a;
                }
            }
        }
        // for(k=0;k<nbin;++k){
        //     fftx[k].a = fft_chn[k][0].a;
        //     fftx[k].b = fft_chn[k][0].b;
        // }
        // for(k=0;k<nbin;++k){
        //     fftx[k].a = 32.0;
        //     fftx[k].a *= eta[k];
        //     fftx[k].b = 0.0;
        // }
        // if (mask_bf_net->cfg->use_cnon) {
        //     wtk_mask_bf_net_feed_cnon(mask_bf_net, fftx);
        // }

        // wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins,
        //                synthesis_window);

        // for (i = 0; i < fsize; ++i) {
        //     out[i] *= 32768.0;
        // }
        // if (mask_bf_net->eq) {
        //     wtk_equalizer_feed_float(mask_bf_net->eq, out, fsize);
        // }
        // wtk_mask_bf_net_control_bs(mask_bf_net, out, fsize);
        // for (i = 0; i < fsize; ++i) {
        //     pv[i] = floorf(out[i] + 0.5);
        // }
        // if (mask_bf_net->notify) {
        //     mask_bf_net->notify(mask_bf_net->ths, pv, fsize);
        // }
    }
}
#endif

float wtk_mask_bf_net_get_reference_channel_snr2(
    wtk_mask_bf_net_t *mask_bf_net) {
    float *speech_power_acc = mask_bf_net->speech_power_acc;
    float *noise_power_acc = mask_bf_net->noise_power_acc;
    float *snr = mask_bf_net->snr;

    float speech_power;
    float noise_power;
    int max_snr = 0;
    int max_idx = 0;
    float snr_ref;
    float snr_new;
    int i;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int refchannel = mask_bf_net->refchannel;
    int ref_channel_new;
    float snr_thresh = 1.1;

    for (i = 0; i < nmicchannel; ++i) {
        speech_power = speech_power_acc[i] / (mask_bf_net->spower_cnt + 1e-3);
        noise_power = noise_power_acc[i] / (mask_bf_net->npower_cnt + 1e-3);

        snr[i] = speech_power / (noise_power + 1e-9);
        if (snr[i] > max_snr) {
            max_snr = snr[i];
            max_idx = i;
        }
    }

    ref_channel_new = max_idx;
    snr_ref = snr[refchannel];
    if (ref_channel_new != refchannel) {
        snr_new = snr[ref_channel_new];
        if (snr_new > snr_ref * snr_thresh) {
            mask_bf_net->refchannel = ref_channel_new;
            snr_ref = snr_new;
        }
    }
    return snr_ref;
}

void wtk_mask_bf_net_feed_bf_v2(wtk_mask_bf_net_t *mask_bf_net) {
    int nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    int rss_iter = mask_bf_net->cfg->rss_iter;
    int update_w_cnt = mask_bf_net->cfg->update_w_cnt;
    int *update_w_freq = mask_bf_net->cfg->update_w_freq;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int nmicchannel_1 = (int)(nmicchannel >> 2) << 2;
    float vec_init = mask_bf_net->cfg->vec_init;
    wtk_complex_t **fft = mask_bf_net->fft;
    wtk_complex_t **vec = mask_bf_net->vec, *vec_tmp;
    wtk_complex_t **vec2;
    wtk_complex_t **fft_chn = mask_bf_net->fft_chn;
    wtk_complex_t *fftx;
    wtk_complex_t *scov_tmp = mask_bf_net->scov_tmp, *scov_tmp2;
    wtk_complex_t *c_temp = mask_bf_net->c_temp;
    wtk_complex_t *ncov_tmp = mask_bf_net->ncov_tmp, *ncov_tmp2;
    float tmp = 0;
    float inv_tmp = 0;
    int idx = 0;
    int i, j, k, n, m;

    fftx = mask_bf_net->fftx;

    float beta = mask_bf_net->cfg->beta;
    float gamma = mask_bf_net->cfg->gamma;
    float zeta = mask_bf_net->cfg->zeta;
    float eps = 1e-9;
    float bin_speech_thresh = mask_bf_net->cfg->bin_speech_thresh;
    float bin_noise_thresh = mask_bf_net->cfg->bin_noise_thresh;
    float frame_speech_thresh = mask_bf_net->cfg->frame_speech_thresh;
    float frame_noise_thresh = mask_bf_net->cfg->frame_noise_thresh;
    float scnt_thresh = mask_bf_net->cfg->scnt_thresh;
    float ncnt_thresh = mask_bf_net->cfg->ncnt_thresh;
    float post_clip = mask_bf_net->cfg->post_clip;
    int low_bin = (int)(nbin * 0.025);
    int high_bin = (int)(nbin * 0.75);

    wtk_complex_t **speech_covar_norm = mask_bf_net->speech_covar_norm;
    float *speech_power_acc = mask_bf_net->speech_power_acc;
    float *noise_power_acc = mask_bf_net->noise_power_acc;
    wtk_complex_t **noise_covar = mask_bf_net->noise_covar;
    float *scnt = mask_bf_net->scnt;
    float *ncnt = mask_bf_net->ncnt;

    float *alpha_bin = mask_bf_net->alpha_bin;
    float *beta_bin = mask_bf_net->beta_bin;

    float **power_channel = mask_bf_net->power_channel;
    float *speech_power_channel = mask_bf_net->speech_power_channel;
    float *noise_power_channel = mask_bf_net->noise_power_channel;

    wtk_complex_t **covar = mask_bf_net->covar;

    float *speech_mask = mask_bf_net->mask;
    float *noise_mask = mask_bf_net->noise_mask;
    float *post_mask = mask_bf_net->m_mask;

    float scov_alpha;
    int update_speech;
    int update_noise;
    float speech_prob;
    float noise_prob;
    float zeta_frame;

    for (n = 0; n < num_frame;
         ++n, fftx += nbin, speech_mask += nbin, post_mask += nbin) {
        ++mask_bf_net->update_w;
        if (mask_bf_net->cfg->use_freq_preemph) {
            int pre_clip_s = mask_bf_net->cfg->pre_clip_s;
            int pre_clip_e = mask_bf_net->cfg->pre_clip_e;
            float alpha;
            float *pre_alpha = mask_bf_net->pre_alpha;
            for (k = pre_clip_s; k < pre_clip_e; ++k) {
                idx = k + n * nbin;
                alpha = pre_alpha[k];
                for (i = 0; i < nmicchannel; ++i) {
                    fft[i][idx].a *= alpha;
                    fft[i][idx].b *= alpha;
                }
            }
        }
        for (i = 0; i < nmicchannel; ++i) {
            for (k = 0; k < nbin; ++k) {
                idx = k + n * nbin;
                fft_chn[k][i].a = fft[i][idx].a;
                fft_chn[k][i].b = fft[i][idx].b;
            }
        }

        for (k = 0; k < nbin; ++k) {
            noise_mask[k] = 1.0 - speech_mask[k];
            post_mask[k] = min(max(post_clip, speech_mask[k]), 1.0);
        }

        if (mask_bf_net->snr_acc <= 2.0) {
            scov_alpha = 0.99;
            bin_speech_thresh = 0.5;
        } else if (mask_bf_net->snr_acc <= 12.0) {
            scov_alpha = 0.99;
            bin_speech_thresh = 0.4;
        } else {
            scov_alpha = 0.9;
            bin_speech_thresh = 0.4;
        }

        update_speech =
            wtk_float_mean(speech_mask, nbin) >= frame_speech_thresh;
        update_noise = wtk_float_mean(noise_mask, nbin) >= frame_noise_thresh;

        if (update_speech || update_noise) {
            speech_prob =
                wtk_float_mean(speech_mask + low_bin, high_bin - low_bin);
            noise_prob =
                wtk_float_mean(noise_mask + low_bin, high_bin - low_bin);

            for (i = 0; i < nmicchannel; ++i) {
                speech_power_channel[i] = 0;
                noise_power_channel[i] = 0;
                for (k = low_bin; k < high_bin; ++k) {
                    power_channel[k][i] = fft_chn[k][i].a * fft_chn[k][i].a +
                                          fft_chn[k][i].b * fft_chn[k][i].b;
                    speech_power_channel[i] +=
                        power_channel[k][i] * speech_mask[k];
                    noise_power_channel[i] +=
                        power_channel[k][i] * noise_mask[k];
                }
            }

            for (k = 0; k < nbin; ++k) {
                speech_mask[k] -= bin_speech_thresh;
                noise_mask[k] -= bin_noise_thresh;
                speech_mask[k] = max(speech_mask[k], 0.0f);
                noise_mask[k] = max(noise_mask[k], 0.0f);
            }

            // 计算协方差矩阵covar
            for (k = 0; k < nbin; ++k) {
                memset(covar[k], 0,
                       sizeof(wtk_complex_t) * nmicchannel * nmicchannel);
                for (i = 0; i < nmicchannel; ++i) {
                    for (j = 0; j < nmicchannel; ++j) {
                        covar[k][i * nmicchannel + j].a =
                            fft_chn[k][i].a * fft_chn[k][j].a +
                            fft_chn[k][i].b * fft_chn[k][j].b;
                        covar[k][i * nmicchannel + j].b =
                            -fft_chn[k][i].a * fft_chn[k][j].b +
                            fft_chn[k][i].b * fft_chn[k][j].a;
                    }
                }
            }

            if (update_speech) {
                for (k = 0; k < nbin; ++k) {
                    scov_tmp2 = mask_bf_net->covm->scov[k];
                    if (scnt[k] < scnt_thresh) {
                        alpha_bin[k] = 1.0;
                    } else {
                        alpha_bin[k] = scov_alpha;
                    }
                    for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                        scov_tmp2[i].a *= alpha_bin[k];
                        scov_tmp2[i].b *= alpha_bin[k];
                    }
                    scnt[k] *= alpha_bin[k];
                }
            }
            if (update_noise) {
                for (k = 0; k < nbin; ++k) {
                    scov_tmp2 = mask_bf_net->covm->scov[k];
                    if (ncnt[k] < ncnt_thresh) {
                        beta_bin[k] = 1.0;
                    } else {
                        beta_bin[k] = beta;
                    }
                    for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                        scov_tmp2[i].a *= beta_bin[k];
                        scov_tmp2[i].b *= beta_bin[k];
                    }
                    ncnt[k] *= beta_bin[k];
                }
            }

            if (mask_bf_net->spower_cnt > scnt_thresh) {
                zeta_frame = zeta;
            } else {
                zeta_frame = 1.0;
            }
            for (i = 0; i < nmicchannel; ++i) {
                speech_power_acc[i] *= zeta_frame;
            }
            mask_bf_net->spower_cnt *= zeta_frame;

            if (mask_bf_net->npower_cnt > ncnt_thresh) {
                zeta_frame = zeta;
            } else {
                zeta_frame = 1.0;
            }
            for (i = 0; i < nmicchannel; ++i) {
                noise_power_acc[i] *= zeta_frame;
            }
            mask_bf_net->npower_cnt *= zeta_frame;

            // 更新语音协方差矩阵
            if (update_speech) {
                for (k = 0; k < nbin; ++k) {
                    scov_tmp2 = mask_bf_net->covm->scov[k]; //////
                    for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                        scov_tmp[i].a = covar[k][i].a * speech_mask[k];
                        scov_tmp[i].b = covar[k][i].b * speech_mask[k];
                        scov_tmp2[i].a += scov_tmp[i].a;
                        scov_tmp2[i].b += scov_tmp[i].b;
                    }
                    scnt[k] += speech_mask[k];
                    for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                        speech_covar_norm[k][i].a =
                            scov_tmp2[i].a / (scnt[k] + eps);
                        speech_covar_norm[k][i].b =
                            scov_tmp2[i].b / (scnt[k] + eps);
                    }
                }
                for (i = 0; i < nmicchannel; ++i) {
                    speech_power_acc[i] +=
                        speech_power_channel[i] * speech_prob;
                }
                mask_bf_net->spower_cnt += speech_prob;
            }

            if (update_noise) {
                for (k = 0; k < nbin; ++k) {
                    ncov_tmp2 = mask_bf_net->covm->ncov[k];
                    for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                        ncov_tmp[i].a = covar[k][i].a * noise_mask[k];
                        ncov_tmp[i].b = covar[k][i].b * noise_mask[k];
                        ncov_tmp2[i].a += ncov_tmp[i].a;
                        ncov_tmp2[i].b += ncov_tmp[i].b;
                    }
                    ncnt[k] += noise_mask[k];
                    for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                        noise_covar[k][i].a = ncov_tmp2[i].a / (ncnt[k] + eps);
                        noise_covar[k][i].b = ncov_tmp2[i].b / (ncnt[k] + eps);
                    }
                }
                for (i = 0; i < nmicchannel; ++i) {
                    noise_power_acc[i] += noise_power_channel[i] * noise_prob;
                }
                mask_bf_net->npower_cnt += noise_prob;
            }

            if (update_speech) {
                float snr_ref =
                    wtk_mask_bf_net_get_reference_channel_snr2(mask_bf_net);
                snr_ref = min(snr_ref, 50.0);
                mask_bf_net->snr_acc =
                    mask_bf_net->snr_acc * 0.95 + snr_ref * 0.05;
            }
        }

        if (gamma > 0) {
            float speech_diag;
            float noise_diag;
            float scaler;
            float min_scaler = 0;
            for (k = 0; k < nbin; ++k) {
                speech_diag = noise_diag = 0;
                scov_tmp = speech_covar_norm[k];
                ncov_tmp = noise_covar[k];
                for (i = 0; i < nmicchannel; ++i) {
                    speech_diag = scov_tmp[i * nmicchannel + i].a;
                    noise_diag = ncov_tmp[i * nmicchannel + i].a;
                    scaler = min(
                        max(speech_diag / (gamma * noise_diag + eps), 0), 1.0);
                    min_scaler = min(min_scaler, scaler);
                }
                min_scaler = min(max(min_scaler, 0), 1.0);

                for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                    scov_tmp[i].a = scov_tmp[i].a - min_scaler * ncov_tmp[i].a;
                    scov_tmp[i].b = scov_tmp[i].b - min_scaler * ncov_tmp[i].b;
                }
            }
        }

        if (mask_bf_net->bf_start == 0) {
            for (k = 0; k < nbin; ++k) {
                vec_tmp = vec[k];
                memset(vec_tmp, 0, nmicchannel * sizeof(wtk_complex_t));
                // vec_tmp[nmicchannel - 1].a = 1;
                for (i = 0; i < nmicchannel; ++i) {
                    vec_tmp[i].a = vec_init;
                }
            }
            mask_bf_net->bf_start = 1;
        } else {
            // memset(t, 0, nbin * sizeof(float));
            for (k = 0; k < nbin; ++k) {
                scov_tmp2 = mask_bf_net->covm->scov[k];
                memcpy(scov_tmp, scov_tmp2,
                       nmicchannel * nmicchannel * sizeof(wtk_complex_t));
                vec_tmp = vec[k];
                tmp = 0;
                for (i = 0; i < nmicchannel; ++i) {
                    tmp += scov_tmp[i * nmicchannel + i].a *
                           scov_tmp[i * nmicchannel + i].a;
                }
                tmp = sqrtf(tmp) + 1e-16;

                inv_tmp = 1.0 / tmp;
                for (i = 0; i < nmicchannel * nmicchannel; ++i) {
                    scov_tmp[i].a = scov_tmp[i].a * inv_tmp;
                    scov_tmp[i].b = scov_tmp[i].b * inv_tmp;
                }

                for (m = 0; m < rss_iter; ++m) {
                    memset(c_temp, 0, nmicchannel * sizeof(wtk_complex_t));
                    for (i = 0; i < nmicchannel; ++i) {
                        float sum_real = 0.0f;
                        float sum_imag = 0.0f;
                        wtk_complex_t *scov_row =
                            &scov_tmp[i * nmicchannel]; // 行指针优化

                        // 主循环：每次处理4个通道（展开4次）
                        for (j = 0; j < nmicchannel_1; j += 4) {
                            // 预加载向量元素
                            wtk_complex_t v0 = vec_tmp[j];
                            wtk_complex_t v1 = vec_tmp[j + 1];
                            wtk_complex_t v2 = vec_tmp[j + 2];
                            wtk_complex_t v3 = vec_tmp[j + 3];

                            // 预加载矩阵行元素
                            wtk_complex_t m0 = scov_row[j];
                            wtk_complex_t m1 = scov_row[j + 1];
                            wtk_complex_t m2 = scov_row[j + 2];
                            wtk_complex_t m3 = scov_row[j + 3];

                            // 第一组复数乘法 (m0*v0)
                            sum_real += m0.a * v0.a - m0.b * v0.b;
                            sum_imag += m0.a * v0.b + m0.b * v0.a;

                            // 第二组复数乘法 (m1*v1)
                            sum_real += m1.a * v1.a - m1.b * v1.b;
                            sum_imag += m1.a * v1.b + m1.b * v1.a;

                            // 第三组复数乘法 (m2*v2)
                            sum_real += m2.a * v2.a - m2.b * v2.b;
                            sum_imag += m2.a * v2.b + m2.b * v2.a;

                            // 第四组复数乘法 (m3*v3)
                            sum_real += m3.a * v3.a - m3.b * v3.b;
                            sum_imag += m3.a * v3.b + m3.b * v3.a;
                        }
                        // 处理剩余的通道（如果nmicchannel不是4的倍数）
                        for (j = nmicchannel_1; j < nmicchannel; ++j) {
                            sum_real += scov_row[j].a * vec_tmp[j].a -
                                        scov_row[j].b * vec_tmp[j].b;
                            sum_imag += scov_row[j].a * vec_tmp[j].b +
                                        scov_row[j].b * vec_tmp[j].a;
                        }

                        // 保存结果
                        c_temp[i].a = sum_real;
                        c_temp[i].b = sum_imag;
                    }
                    memcpy(vec_tmp, c_temp,
                           nmicchannel * sizeof(wtk_complex_t));
                }
                tmp = 0;
                // 计算模长的平方和（4路展开）
                for (i = 0; i < nmicchannel_1; i += 4) {
                    wtk_complex_t v0 = vec_tmp[i];
                    wtk_complex_t v1 = vec_tmp[i + 1];
                    wtk_complex_t v2 = vec_tmp[i + 2];
                    wtk_complex_t v3 = vec_tmp[i + 3];

                    tmp += v0.a * v0.a + v0.b * v0.b;
                    tmp += v1.a * v1.a + v1.b * v1.b;
                    tmp += v2.a * v2.a + v2.b * v2.b;
                    tmp += v3.a * v3.a + v3.b * v3.b;
                }
                // 处理剩余的通道（如果nmicchannel不是4的倍数）
                for (i = nmicchannel_1; i < nmicchannel; ++i) {
                    tmp += vec_tmp[i].a * vec_tmp[i].a +
                           vec_tmp[i].b * vec_tmp[i].b;
                }
                tmp = sqrtf(tmp);
                if (tmp < 1e-16) {
                    memset(vec_tmp, 0, nmicchannel * sizeof(wtk_complex_t));
                    // vec_tmp[nmicchannel - 1].a = 1;
                    for (i = 0; i < nmicchannel; ++i) {
                        vec_tmp[i].a = vec_init;
                    }
                }
                tmp = 0;
                // 计算模长的平方和（4路展开）
                for (i = 0; i < nmicchannel_1; i += 4) {
                    wtk_complex_t v0 = vec_tmp[i];
                    wtk_complex_t v1 = vec_tmp[i + 1];
                    wtk_complex_t v2 = vec_tmp[i + 2];
                    wtk_complex_t v3 = vec_tmp[i + 3];

                    tmp += v0.a * v0.a + v0.b * v0.b;
                    tmp += v1.a * v1.a + v1.b * v1.b;
                    tmp += v2.a * v2.a + v2.b * v2.b;
                    tmp += v3.a * v3.a + v3.b * v3.b;
                }
                // 处理剩余的通道（如果nmicchannel不是4的倍数）
                for (i = nmicchannel_1; i < nmicchannel; ++i) {
                    tmp += vec_tmp[i].a * vec_tmp[i].a +
                           vec_tmp[i].b * vec_tmp[i].b;
                }
                tmp = sqrtf(tmp);
                inv_tmp = 1.0 / tmp;
                for (i = 0; i < nmicchannel; ++i) {
                    vec_tmp[i].a *= inv_tmp;
                    vec_tmp[i].b *= inv_tmp;
                }
                for (i = 0; i < nmicchannel; ++i) {
                    if (isnan(vec_tmp[i].a) || isnan(vec_tmp[i].b)) {
                        wtk_debug("nan detected in vec[%d][%d].a=%f, "
                                  "vec[%d][%d].b=%f\n",
                                  k, i, vec_tmp[i].a, k, i, vec_tmp[i].b);
                        exit(1);
                    }
                }
            }
        }

        if (mask_bf_net->cfg->use_phase_corr) {
            // _phase_correction(mask_bf_net);
            _phase_correction2(mask_bf_net);
            vec2 = mask_bf_net->vec2;
        } else {
            vec2 = mask_bf_net->vec;
        }
        tmp = 1.0 / nmicchannel * 2;
        if (mask_bf_net->bf_init_frame < mask_bf_net->cfg->bf_init_frame) {
            ++mask_bf_net->bf_init_frame;
            memset(fftx, 0, sizeof(wtk_complex_t) * nbin);
            for (k = 0; k < nbin; ++k) {
                fftx[k].a = fft_chn[k][0].a * post_mask[k];
                fftx[k].b = fft_chn[k][0].b * post_mask[k];
            }
        } else {
            for (k = 0; k < nbin; ++k) {
                memcpy(mask_bf_net->bf->ovec[k], vec2[k],
                       nmicchannel * sizeof(wtk_complex_t));
                wtk_bf_update_ncov(mask_bf_net->bf, mask_bf_net->covm->ncov, k);
                if (mask_bf_net->update_w == update_w_freq[k]) {
                    wtk_bf_update_mvdr_w_f(mask_bf_net->bf, 0, k);
                }
                wtk_bf_output_fft_k(mask_bf_net->bf, fft_chn[k], fftx + k, k);

                if (isnan(fftx[k].a) || isnan(fftx[k].b)) {
                    wtk_debug("nan detected in fftx[%d].a=%f, fftx[%d].b=%f, "
                              "please check mic\n",
                              k, fftx[k].a, k, fftx[k].b);
                    exit(1);
                }
                fftx[k].a *= post_mask[k] * tmp;
                fftx[k].b *= post_mask[k] * tmp;
            }
        }
        // memcpy(fftx, fft[0] + n * nbin, sizeof(wtk_complex_t) * nbin);
        // for (k = 0; k < nbin; ++k) {
        //     fftx[k].a *= post_mask[k];
        //     fftx[k].b *= post_mask[k];
        // }
        if (mask_bf_net->update_w % update_w_cnt == 0) {
            mask_bf_net->update_w = 0;
        }
    }
}

void wtk_mask_bf_net_debug(wtk_mask_bf_net_t *mask_bf_net) {
    int nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    wtk_complex_t *fftx;
    wtk_complex_t *ffty;
    int i, n;

    wtk_drft_t *rfft = mask_bf_net->rfft;
    float *rfft_in = mask_bf_net->rfft_in;
    float *synthesis_mem = mask_bf_net->synthesis_mem;
    float *synthesis_window = mask_bf_net->synthesis_window;
    float *out = mask_bf_net->out;
    short *pv = (short *)out;
    int wins = mask_bf_net->cfg->wins;
    int fsize = wins / 2;

    float micenr;
    float micenr_thresh = mask_bf_net->cfg->micenr_thresh;
    int micenr_cnt = mask_bf_net->cfg->micenr_cnt;

    float qmmse2_mask_thresh = mask_bf_net->cfg->qmmse2_mask_thresh;
    float *mask = mask_bf_net->mask;
    int clip_s = mask_bf_net->cfg->clip_s;
    int clip_e = mask_bf_net->cfg->clip_e;

    fftx = mask_bf_net->fftx;
    ffty = mask_bf_net->ffty;
    for (n = 0; n < num_frame; ++n, fftx += nbin, ffty += nbin, mask += nbin) {
        // for(int k=0;k<nbin;++k){
        //     fftx[k].a = 32.0;
        //     fftx[k].b = 0.0;
        // }
        // fftx[0].a = fftx[0].b = 0.0;
        // fftx[nbin - 1].a = fftx[nbin - 1].b = 0.0;

        // static int cnt=0;
        // cnt++;
        micenr = wtk_mask_bf_net_fft_energy(fftx, nbin);
        // printf("%f\n", micenr);
        if (micenr > micenr_thresh) {
            // if(mask_bf_net->mic_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
            // }
            mask_bf_net->mic_sil = 0;
            mask_bf_net->mic_silcnt = micenr_cnt;
        } else if (mask_bf_net->mic_sil == 0) {
            mask_bf_net->mic_silcnt -= 1;
            if (mask_bf_net->mic_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                mask_bf_net->mic_sil = 1;
            }
        }

        if (mask_bf_net->qmmse2 && mask_bf_net->denoise_enable) {
            float mean_mask = wtk_float_abs_mean(mask, nbin);
            // printf("%f\n", mean_mask);
            if (mask_bf_net->sp_state[n] == 0) {
                wtk_qmmse_feed_mask(mask_bf_net->qmmse2, fftx, mask);
            } else if (mean_mask < qmmse2_mask_thresh) {
                wtk_qmmse_feed_mask(mask_bf_net->qmmse2, fftx, mask);
            } else {
                wtk_qmmse_denoise(mask_bf_net->qmmse2, fftx);
            }
        }
    }

    if (mask_bf_net->sum_sp_sil == 0 &&
        mask_bf_net->cfg->use_debug_echo_model) {
        wtk_mask_bf_net_feed_model2(mask_bf_net);
    }

    fftx = mask_bf_net->fftx;
    for (n = 0; n < num_frame; ++n, fftx += nbin) {

        for (i = 0; i < clip_s; ++i) {
            fftx[i].a = 0;
            fftx[i].b = 0;
        }
        for (i = clip_e; i < nbin; ++i) {
            fftx[i].a = 0;
            fftx[i].b = 0;
        }
        if (mask_bf_net->cfg->use_pffft) {
            wtk_drft_istft2(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                            synthesis_window);
        } else {
            wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                           synthesis_window);
        }

        for (i = 0; i < fsize; ++i) {
            out[i] *= 32768.0;
        }
        if (mask_bf_net->limiter) {
            wtk_limiter_feed(mask_bf_net->limiter, out, fsize);
        } else {
            wtk_mask_bf_net_control_bs(mask_bf_net, out, fsize);
        }
        for (i = 0; i < fsize; ++i) {
            pv[i] = floorf(out[i] + 0.5);
        }
        if (mask_bf_net->notify) {
            mask_bf_net->notify(mask_bf_net->ths, pv, fsize);
        }
    }
}

void wtk_mask_bf_net_feed_model1(wtk_mask_bf_net_t *mask_bf_net) {
    wtk_complex_t **fft = mask_bf_net->fft;
    wtk_complex_t **fft_sp = mask_bf_net->fft_sp;
    int nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    float gf;
    int i, j, k, n;
    int idx;
    wtk_complex_t *ffty = mask_bf_net->ffty;
    wtk_complex_t *fft_tmp = mask_bf_net->fft_tmp;
    float scale = 32768.0 / mask_bf_net->cfg->wins;
    float *mask = mask_bf_net->mask;
    float *eng = mask_bf_net->eng;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    float *err = mask_bf_net->err;
    float *ee = mask_bf_net->ee;
    float model1_scale = mask_bf_net->cfg->model1_scale;
    float model1_sp_scale = mask_bf_net->cfg->model1_sp_scale;

    if (mask_bf_net->cfg->use_bf_v2) {
        float omage = 0.3;
        int refchannel = mask_bf_net->refchannel;
        wtk_complex_t **w = mask_bf_net->bf->w;
        float ta, tb;
        if (mask_bf_net->bf_init_frame > mask_bf_net->cfg->bf_init_frame) {
            for (n = 0; n < num_frame; ++n) {
                for (k = 0; k < nbin; ++k) {
                    ta = tb = 0;
                    for (i = 0; i < nmicchannel; ++i) {
                        ta += w[k][i].a * fft[i][n * nbin + k].a +
                              w[k][i].b * fft[i][n * nbin + k].b;
                        tb += w[k][i].a * fft[i][n * nbin + k].b -
                              w[k][i].b * fft[i][n * nbin + k].a;
                    }
                    fft_tmp[n * nbin + k].a = ta;
                    fft_tmp[n * nbin + k].b = tb;
                }
            }
            for (j = 0; j < nbin * num_frame; ++j) {
                err[j] =
                    sqrtf(fft_tmp[j].a * fft_tmp[j].a +
                          fft_tmp[j].b * fft_tmp[j].b) +
                    omage * sqrtf(fft[refchannel][j].a * fft[refchannel][j].a +
                                  fft[refchannel][j].b * fft[refchannel][j].b);
            }
        } else {
            for (j = 0; j < nbin * num_frame; ++j) {
                err[j] = sqrtf(fft[refchannel][j].a * fft[refchannel][j].a +
                               fft[refchannel][j].b * fft[refchannel][j].b);
            }
        }

    } else {
        idx = 0;
        if (eng) {
            idx = wtk_float_median_index(eng, nmicchannel);
        }
        for (j = 0; j < nbin * num_frame; ++j) {
            err[j] = sqrtf(fft[idx][j].a * fft[idx][j].a +
                           fft[idx][j].b * fft[idx][j].b);
        }
    }

    if (mask_bf_net->cfg->use_edr_stage1_rt) {
        for (i = 0; i < nbin * num_frame; ++i) {
            ee[i] = sqrtf(fft_sp[0][i].a * fft_sp[0][i].a +
                          fft_sp[0][i].b * fft_sp[0][i].b);
            ee[i] *= model1_sp_scale;
        }
    }

    for (i = 0; i < nbin * num_frame; ++i) {
        err[i] *= model1_scale;
    }

    if (mask_bf_net->cfg->use_gainnet2_2) {
        idx = 0;
        if (eng) {
            idx = wtk_float_median_index(eng, nmicchannel);
        }
        memcpy(fft_tmp, fft[idx], sizeof(wtk_complex_t) * nbin * num_frame);
        memset(ffty, 0, sizeof(wtk_complex_t) * nbin * num_frame);
        for (n = 0; n < num_frame; ++n, fft_tmp += nbin) {
            for (k = 0; k < nbin; ++k) {
                fft_tmp[k].a *= scale;
                fft_tmp[k].b *= scale;
            }
            wtk_mask_bf_net_edr_feed_gainnet2_2(mask_bf_net, fft_tmp, ffty);
            for (k = 0; k < nbin; ++k) {
                gf = mask_bf_net->vdr2->gf[k];
                mask[n * nbin + k] = gf;
            }
        }
    } else if (mask_bf_net->cfg->use_stage1_rt) {
        wtk_mask_bf_net_feed_onnx(mask_bf_net);
    } else if (mask_bf_net->cfg->use_qnn) {
        wtk_mask_bf_net_feed_qnn(mask_bf_net);
    }
}

void wtk_mask_bf_net_aec_on_gainnet2(wtk_mask_bf_net_edr_t *vdr, float *gain,
                                     int len, int is_end) {
    memcpy(vdr->g, gain, sizeof(float) * vdr->cfg->bankfeat.nb_bands);
}

void wtk_mask_bf_net_aec_on_gainnet2_2(wtk_mask_bf_net_edr_t *vdr, float *gain,
                                       int len, int is_end) {
    memcpy(vdr->g, gain, sizeof(float) * vdr->cfg->bankfeat2.nb_bands);
}

void wtk_mask_bf_net_edr_feed_gainnet2(wtk_mask_bf_net_t *mask_bf_net,
                                       wtk_complex_t *fftx,
                                       wtk_complex_t *ffty) {

    wtk_mask_bf_net_edr_t *vdr = mask_bf_net->vdr;
    int i;
    int nbin = vdr->nbin;
    float *g = vdr->g, *gf = vdr->gf;
    wtk_gainnet2_t *gainnet2 = vdr->gainnet2;
    wtk_bankfeat_t *bank_mic = vdr->bank_mic;
    wtk_bankfeat_t *bank_sp = vdr->bank_sp;
    int featsp_lm = vdr->cfg->featsp_lm;
    int featm_lm = vdr->cfg->featm_lm;
    float *feature_sp = vdr->feature_sp;
    int nb_features = bank_mic->cfg->nb_features;
    wtk_qmmse_t *qmmse = mask_bf_net->qmmse;
    float *qmmse_gain;
    wtk_complex_t *fftytmp, sed, *fftxtmp;
    float ef, yf;
    float leak;

    wtk_bankfeat_flush_frame_features(bank_mic, fftx);
    if (1) {
        fftytmp = ffty;
        fftxtmp = fftx;
        for (i = 0; i < nbin; ++i, ++fftxtmp, ++fftytmp) {
            ef = fftxtmp->a * fftxtmp->a + fftxtmp->b * fftxtmp->b;
            yf = fftytmp->a * fftytmp->a + fftytmp->b * fftytmp->b;
            sed.a = fftytmp->a * fftxtmp->a + fftytmp->b * fftxtmp->b;
            sed.b = -fftytmp->a * fftxtmp->b + fftytmp->b * fftxtmp->a;
            leak = (sed.a * sed.a + sed.b * sed.b) / (max(ef, yf) * yf + 1e-9);
            leak = sqrtf(leak);
            fftytmp->a *= leak;
            fftytmp->b *= leak;
            leak = (sed.a * sed.a + sed.b * sed.b) / (ef * yf + 1e-9);
            gf[i] = leak * yf;
        }
    }
    wtk_bankfeat_flush_frame_features(bank_sp, ffty);
    if (feature_sp && featsp_lm > 1) {
        memmove(feature_sp + nb_features * featm_lm,
                feature_sp + nb_features * (featm_lm - 1),
                sizeof(float) * nb_features * (featsp_lm - 1));
        memcpy(feature_sp + nb_features * (featm_lm - 1), bank_sp->features,
               sizeof(float) * nb_features);
    }
    if (qmmse) {
        wtk_qmmse_flush_mask(qmmse, fftx, gf);
    }
    if (feature_sp) {
        wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features,
                           feature_sp, nb_features * (featm_lm + featsp_lm - 1),
                           0);
    } else {
        wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features,
                           bank_sp->features, nb_features, 0);
    }
    wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
    if (qmmse) {
        qmmse_gain = qmmse->gain;
        for (i = 1; i < nbin - 1; ++i) {
            if (gf[i] > qmmse_gain[i]) {
                gf[i] = qmmse_gain[i];
            }
        }
    }
    if (feature_sp && featm_lm > 1) {
        memmove(feature_sp + nb_features, feature_sp,
                sizeof(float) * nb_features * (featm_lm - 2));
        memcpy(feature_sp, bank_mic->features, sizeof(float) * nb_features);
    }
}

void wtk_mask_bf_net_ccm(wtk_mask_bf_net_t *mask_bf_net, float *x_mag_unfold,
                         float *x_phase, float *m_mag, float *m_real,
                         float *m_imag) {
    int nbin = mask_bf_net->nbin;
    int i, n;
    float mag;
    float mag_mask;
    float phase_mask;
    float tmp, tmp1;
    int num_frame = mask_bf_net->cfg->num_frame;
    wtk_complex_t *fftx;

    for (n = 0; n < num_frame; ++n) {
        mag = 0;
        fftx = mask_bf_net->fftx + n * nbin;
        for (i = 0; i < nbin; ++i) {
            // mag =
            // x_mag_unfold[i+n*nbin]*m_mag[i+n*nbin]+x_mag_unfold[i+nbin+n*nbin]*m_mag[i+nbin+n*nbin]+x_mag_unfold[i+nbin*2+n*nbin]*m_mag[i+nbin*2+n*nbin];
            // mag =
            // x_mag_unfold[i+3*n*nbin]*m_mag[i+3*n*nbin]+x_mag_unfold[i+(3*n+1)*nbin]*m_mag[i+(3*n+1)*nbin]+x_mag_unfold[i+(3*n+2)*nbin]*m_mag[i+(3*n+2)*nbin];
            mag = x_mag_unfold[i + n * nbin] * m_mag[i + n * nbin] +
                  x_mag_unfold[i + (num_frame + n) * nbin] *
                      m_mag[i + (num_frame + n) * nbin] +
                  x_mag_unfold[i + (num_frame * 2 + n) * nbin] *
                      m_mag[i + (num_frame * 2 + n) * nbin];
            // mag =
            // x_mag_unfold[i*3+3*n*nbin]*m_mag[i*3+3*n*nbin]+x_mag_unfold[i*3+3*n*nbin+1]*m_mag[i*3+3*n*nbin+1]+x_mag_unfold[i*3+3*n*nbin+2]*m_mag[i*3+3*n*nbin+2];
            mag_mask = sqrtf(m_real[i + n * nbin] * m_real[i + n * nbin] +
                             m_imag[i + n * nbin] * m_imag[i + n * nbin]);
            phase_mask = atan2f(m_imag[i + n * nbin], m_real[i + n * nbin]);
            // real = mag * tanhf(mag_mask) * cosf(x_phase[i]+phase_mask);
            // imag = mag * tanhf(mag_mask) * sinf(x_phase[i]+phase_mask);
            // fftx[i].a = real;
            // fftx[i].b = imag;

            tmp = x_phase[i + n * nbin] + phase_mask;
            tmp1 = mag * tanhf(mag_mask);
            fftx[i].a = tmp1 * cosf(tmp);
            fftx[i].b = tmp1 * sinf(tmp);
        }
    }
}

void wtk_mask_bf_net_ccm2(wtk_mask_bf_net_t *mask_bf_net, float *x_mag_unfold_0,
                          float *x_mag_unfold_1, float *x_mag_unfold_2,
                          float *x_phase, float *m_mag_0, float *m_mag_1,
                          float *m_mag_2, float *m_real, float *m_imag) {
    int nbin = mask_bf_net->nbin;
    int i, n;
    float mag;
    float mag_mask;
    float phase_mask;
    float tmp, tmp1;
    int num_frame = mask_bf_net->cfg->num_frame;
    wtk_complex_t *fftx;

    for (n = 0; n < num_frame; ++n) {
        mag = 0;
        fftx = mask_bf_net->fftx + n * nbin;
        for (i = 0; i < nbin; ++i) {
            mag = x_mag_unfold_0[i + n * nbin] * m_mag_0[i + n * nbin] +
                  x_mag_unfold_1[i + n * nbin] * m_mag_1[i + n * nbin] +
                  x_mag_unfold_2[i + n * nbin] * m_mag_2[i + n * nbin];
            mag_mask = sqrtf(m_real[i + n * nbin] * m_real[i + n * nbin] +
                             m_imag[i + n * nbin] * m_imag[i + n * nbin]);
            phase_mask = atan2f(m_imag[i + n * nbin], m_real[i + n * nbin]);
            // real = mag * tanhf(mag_mask) * cosf(x_phase[i]+phase_mask);
            // imag = mag * tanhf(mag_mask) * sinf(x_phase[i]+phase_mask);
            // fftx[i].a = real*scale;
            // fftx[i].b = imag*scale;

            tmp = x_phase[i + n * nbin] + phase_mask;
            tmp1 = mag * tanhf(mag_mask);
            fftx[i].a = tmp1 * cosf(tmp);
            fftx[i].b = tmp1 * sinf(tmp);
        }
    }
}

void wtk_mask_bf_net_dr_feed_onnx(wtk_mask_bf_net_t *mask_bf_net,
                                  wtk_complex_t *fftx, wtk_complex_t *ffty) {
    int i, j;
    int nbin = mask_bf_net->nbin;
    float *out = NULL;
    float *x_mag_unfold = NULL;
    float *m_mag = NULL;
    float *m_real = NULL;
    float *m_imag = NULL;
    float *x_phase = mask_bf_net->x_phase;
    int num_frame = mask_bf_net->cfg->num_frame;
    int outer_in_num = ffty ? 2 : 1;
    int outer_out_num = mask_bf_net->cfg->use_ccm ? 4 : 1;
    float *err = qtk_nnrt_value_get_data(mask_bf_net->stage2_rt,
                                         mask_bf_net->stage2_inputs[0]);

    if (x_phase) {
        for (i = 0; i < nbin * num_frame; ++i) {
            x_phase[i] = atan2f(fftx[i].b, fftx[i].a);
        }
    }

    if (mask_bf_net->stage2_rt->cfg->use_ss_ipu) {
        for (i = 0; i < nbin * num_frame; ++i) {
            err[i * 2 + 0] = fftx[i].a;
            err[i * 2 + 1] = fftx[i].b;
        }
    } else {
        for (i = 0; i < nbin * num_frame; ++i) {
            err[i] = fftx[i].a;
            err[i + nbin * num_frame] = fftx[i].b;
        }
    }

    if (ffty) {
        float *ee = qtk_nnrt_value_get_data(mask_bf_net->stage2_rt,
                                            mask_bf_net->stage2_inputs[1]);
        if (mask_bf_net->stage2_rt->cfg->use_ss_ipu) {
            for (i = 0; i < nbin * num_frame; ++i) {
                ee[i * 2 + 0] = ffty[i].a;
                ee[i * 2 + 1] = ffty[i].b;
            }
        } else {
            for (i = 0; i < nbin * num_frame; ++i) {
                ee[i] = ffty[i].a;
                ee[i + nbin * num_frame] = ffty[i].b;
            }
        }
    }

    for (i = 0; i < mask_bf_net->stage2_rt->num_in; i++) {
        qtk_nnrt_feed(mask_bf_net->stage2_rt, mask_bf_net->stage2_inputs[i], i);
    }

    qtk_nnrt_run(mask_bf_net->stage2_rt);

    if (mask_bf_net->cfg->use_ccm) {
        qtk_nnrt_value_t x_mag_unfold_val, m_mag_val, m_real_val, m_imag_val;
        qtk_nnrt_get_output(mask_bf_net->stage2_rt, &x_mag_unfold_val, 0);
        qtk_nnrt_get_output(mask_bf_net->stage2_rt, &m_mag_val, 1);
        qtk_nnrt_get_output(mask_bf_net->stage2_rt, &m_real_val, 2);
        qtk_nnrt_get_output(mask_bf_net->stage2_rt, &m_imag_val, 3);
        x_mag_unfold =
            qtk_nnrt_value_get_data(mask_bf_net->stage2_rt, x_mag_unfold_val);
        m_mag = qtk_nnrt_value_get_data(mask_bf_net->stage2_rt, m_mag_val);
        m_real = qtk_nnrt_value_get_data(mask_bf_net->stage2_rt, m_real_val);
        m_imag = qtk_nnrt_value_get_data(mask_bf_net->stage2_rt, m_imag_val);
        wtk_mask_bf_net_ccm(mask_bf_net, x_mag_unfold, x_phase, m_mag, m_real,
                            m_imag);
        qtk_nnrt_value_release(mask_bf_net->stage2_rt, x_mag_unfold_val);
        qtk_nnrt_value_release(mask_bf_net->stage2_rt, m_mag_val);
        qtk_nnrt_value_release(mask_bf_net->stage2_rt, m_real_val);
        qtk_nnrt_value_release(mask_bf_net->stage2_rt, m_imag_val);
    } else {
        qtk_nnrt_value_t out_val;
        qtk_nnrt_get_output(mask_bf_net->stage2_rt, &out_val, 0);
        out = qtk_nnrt_value_get_data(mask_bf_net->stage2_rt, out_val);
        for (i = 0; i < nbin * num_frame; ++i) {
            fftx[i].a = out[i];
            fftx[i].b = out[i + nbin * num_frame];
        }
        qtk_nnrt_value_release(mask_bf_net->stage2_rt, out_val);
    }
    for (i = outer_in_num, j = outer_out_num;
         i < mask_bf_net->stage2_rt->num_in; ++i, ++j) {
        if (mask_bf_net->stage2_rt->cfg->use_ss_ipu) {
            qtk_nnrt_value_t out_val;
            void *src, *dst;
            qtk_nnrt_get_output(mask_bf_net->stage2_rt, &out_val, j);
            src = qtk_nnrt_value_get_data(mask_bf_net->stage2_rt, out_val);
            dst = qtk_nnrt_value_get_data(mask_bf_net->stage2_rt, i);
            memcpy(dst, src,
                   qtk_nnrt_get_input_nbytes(mask_bf_net->stage2_rt, i));
            qtk_nnrt_value_release(mask_bf_net->stage2_rt, out_val);
        } else {
            qtk_nnrt_value_release(mask_bf_net->stage2_rt,
                                   mask_bf_net->stage2_inputs[i]);
            qtk_nnrt_get_output(mask_bf_net->stage2_rt,
                                &mask_bf_net->stage2_inputs[i], j);
        }
    }
    qtk_nnrt_reset(mask_bf_net->stage2_rt);
}

void wtk_mask_bf_net_dr_feed_qnn(wtk_mask_bf_net_t *mask_bf_net,
                                 wtk_complex_t *fftx) {
    int i, j;
    int nbin = mask_bf_net->nbin;
    float *err = mask_bf_net->err;
    float *model_mask = mask_bf_net->model_mask;
    int num_frame = mask_bf_net->cfg->num_frame;
    qtk_nn_vm_t *nv = &mask_bf_net->nv3;
    float model2_scale = mask_bf_net->cfg->model2_scale;

    for (i = 0; i < nbin * num_frame; ++i) {
        err[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
        err[i] *= model2_scale;
    }

    int nout = 30, ndim, nelem;

    if (mask_bf_net->nv3_idx != 0) {
        qtk_nn_vm_get_input(nv, &mask_bf_net->nv3_num_in,
                            (void **)mask_bf_net->nv3_input);
        for (i = 1; i < mask_bf_net->nv3_num_in; i++) {
            memcpy(mask_bf_net->nv3_input[i], mask_bf_net->nv3_cache[i],
                   mask_bf_net->nv3_out_sizes[i]);
        }
    }
    memcpy(mask_bf_net->nv3_input[0], err, mask_bf_net->nv3_in_sizes[0]);
    qtk_nn_vm_run(nv);
    qtk_nn_vm_get_output(nv, &nout, (void **)mask_bf_net->nv3_output);

    if (!mask_bf_net->nv3_out_sizes) {
        mask_bf_net->nv3_out_sizes = wtk_malloc(sizeof(int) * nout);
        for (i = 0; i < nout; i++) {
            nelem = 1;
            uint32_t *shape = qtk_nn_vm_get_output_shape(nv, i, &ndim);
            for (j = 0; j < ndim; j++) {
                nelem *= shape[j];
            }
            mask_bf_net->nv3_out_sizes[i] = nelem * sizeof(float);
            if (i > 0) {
                mask_bf_net->nv3_cache[i] =
                    wtk_malloc(mask_bf_net->nv3_out_sizes[i]);
            }
        }
    }
    memcpy(model_mask, mask_bf_net->nv3_output[0],
           mask_bf_net->nv3_out_sizes[0]);
    for (i = 1; i < nout; i++) {
        memcpy(mask_bf_net->nv3_cache[i], mask_bf_net->nv3_output[i],
               mask_bf_net->nv3_out_sizes[i]);
    }

    mask_bf_net->nv3_idx++;
    qtk_nn_vm_reset(nv);
    // memcpy(item->val, err, item->bytes * item->in_dim);

    // for (i = 0; i < nbin * num_frame; ++i) {
    //     model_mask[i] = out[i];
    // }
}

void wtk_mask_bf_net_edr_feed_qnn(wtk_mask_bf_net_t *mask_bf_net,
                                  wtk_complex_t *fftx, wtk_complex_t *ffty) {
    int i, j;
    int nbin = mask_bf_net->nbin;
    float *err = mask_bf_net->err;
    float *ee = mask_bf_net->ee;
    float *model_mask = mask_bf_net->model_mask;
    int num_frame = mask_bf_net->cfg->num_frame;
    qtk_nn_vm_t *nv = &mask_bf_net->nv2;
    float model2_scale = mask_bf_net->cfg->model2_scale;
    float model2_sp_scale = mask_bf_net->cfg->model2_sp_scale;

    for (i = 0; i < nbin * num_frame; ++i) {
        err[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
        ee[i] = sqrtf(ffty[i].a * ffty[i].a + ffty[i].b * ffty[i].b);
        err[i] *= model2_scale;
        ee[i] *= model2_sp_scale;
    }

    int nout = 30, ndim, nelem;

    if (mask_bf_net->nv2_idx != 0) {
        qtk_nn_vm_get_input(nv, &mask_bf_net->nv2_num_in,
                            (void **)mask_bf_net->nv2_input);
        for (i = 2; i < mask_bf_net->nv2_num_in; i++) {
            memcpy(mask_bf_net->nv2_input[i], mask_bf_net->nv2_cache[i - 1],
                   mask_bf_net->nv2_out_sizes[i - 1]);
        }
    }
    memcpy(mask_bf_net->nv2_input[0], err, mask_bf_net->nv2_in_sizes[0]);
    memcpy(mask_bf_net->nv2_input[1], ee, mask_bf_net->nv2_in_sizes[1]);
    qtk_nn_vm_run(nv);
    qtk_nn_vm_get_output(nv, &nout, (void **)mask_bf_net->nv2_output);

    if (!mask_bf_net->nv2_out_sizes) {
        mask_bf_net->nv2_out_sizes = wtk_malloc(sizeof(int) * nout);
        for (i = 0; i < nout; i++) {
            nelem = 1;
            uint32_t *shape = qtk_nn_vm_get_output_shape(nv, i, &ndim);
            for (j = 0; j < ndim; j++) {
                nelem *= shape[j];
            }
            mask_bf_net->nv2_out_sizes[i] = nelem * sizeof(float);
            if (i > 0) {
                mask_bf_net->nv2_cache[i] =
                    wtk_malloc(mask_bf_net->nv2_out_sizes[i]);
            }
        }
    }
    memcpy(model_mask, mask_bf_net->nv2_output[0],
           mask_bf_net->nv2_out_sizes[0]);
    for (i = 1; i < nout; i++) {
        memcpy(mask_bf_net->nv2_cache[i], mask_bf_net->nv2_output[i],
               mask_bf_net->nv2_out_sizes[i]);
    }

    mask_bf_net->nv2_idx++;
    qtk_nn_vm_reset(nv);

    // memcpy(item->val, err, item->bytes * item->in_dim);
    // memcpy(item->val, ee, item->bytes * item->in_dim);
    //  for (i = 0; i < nbin * num_frame; ++i) {
    //      model_mask[i] = out[i];
    //  }
}

void wtk_mask_bf_net_edr_feed_mask_onnx(wtk_mask_bf_net_t *mask_bf_net,
                                        wtk_complex_t *fftx,
                                        wtk_complex_t *ffty) {
    int i;
    int nbin = mask_bf_net->nbin;
    float *model_mask = mask_bf_net->model_mask;
    int num_frame = mask_bf_net->cfg->num_frame;
    int outer_in_num = ffty ? 2 : 1;
    int outer_out_num = 1;
    qtk_nnrt_t *rt = ffty ? mask_bf_net->stage2_rt : mask_bf_net->dr_stage2_rt;
    qtk_nnrt_value_t *rt_inputs =
        ffty ? mask_bf_net->stage2_inputs : mask_bf_net->dr_stage2_inputs;
    float *err = qtk_nnrt_value_get_data(rt, rt_inputs[0]);
    float model2_scale = mask_bf_net->cfg->model2_scale;
    float model2_sp_scale = mask_bf_net->cfg->model2_sp_scale;

    for (i = 0; i < nbin * num_frame; ++i) {
        err[i] = sqrtf(fftx[i].a * fftx[i].a + fftx[i].b * fftx[i].b);
        err[i] *= model2_scale;
    }
    if (ffty) {
        float *ee = qtk_nnrt_value_get_data(rt, rt_inputs[1]);
        for (i = 0; i < nbin * num_frame; ++i) {
            ee[i] = sqrtf(ffty[i].a * ffty[i].a + ffty[i].b * ffty[i].b);
            ee[i] *= model2_sp_scale;
        }
    }

    for (i = 0; i < rt->num_in; i++) {
        qtk_nnrt_feed(rt, rt_inputs[i], i);
    }

    qtk_nnrt_run(rt);

    {
        for (i = outer_out_num; i < rt->num_out; i++) {
            int in_idx = i - outer_out_num + outer_in_num;
            if (rt->cfg->use_ss_ipu) {
                qtk_nnrt_value_t out_val;
                void *src, *dst;
                qtk_nnrt_get_output(rt, &out_val, i);
                src = qtk_nnrt_value_get_data(rt, out_val);
                dst = qtk_nnrt_value_get_data(rt, rt_inputs[in_idx]);
                memcpy(dst, src, qtk_nnrt_get_input_nbytes(rt, in_idx));
                qtk_nnrt_value_release(rt, out_val);
            } else {
                qtk_nnrt_value_release(rt, rt_inputs[in_idx]);
                qtk_nnrt_get_output(rt, &rt_inputs[in_idx], i);
            }
        }

        qtk_nnrt_value_t model_mask_val;
        qtk_nnrt_get_output(rt, &model_mask_val, 0);
        memcpy(model_mask, qtk_nnrt_value_get_data(rt, model_mask_val),
               sizeof(float) * nbin * num_frame);
        qtk_nnrt_value_release(rt, model_mask_val);

        qtk_nnrt_reset(rt);
    }
}

void wtk_mask_bf_net_feed_model2(wtk_mask_bf_net_t *mask_bf_net) {
    int nbin = mask_bf_net->nbin;
    int num_frame = mask_bf_net->cfg->num_frame;
    wtk_complex_t *fftx;
    wtk_complex_t *ffty;
    float *model_mask = mask_bf_net->model_mask;
    float gf;
    int k, n;
    float scale = 32768.0 / mask_bf_net->cfg->wins;
    float scale_1 = 1.0 / scale;
    float *gc_mask = mask_bf_net->gc_mask;
    wtk_complex_t *fft_tmp = mask_bf_net->fft_tmp;
    float mask = 1.0;
    float max_mask;
    float mask_peak;
    float *mask_mu = mask_bf_net->mask_mu;
    float *mask_mu2 = mask_bf_net->mask_mu2;
    float *mask_tmp = mask_bf_net->mask_tmp;
    int n_mask_mu = mask_bf_net->cfg->n_mask_mu;

    wtk_qmmse_t *qmmse = mask_bf_net->qmmse;
    float *qmmse_gain;

    // if (mask_bf_net->cfg->use_freq_preemph) {
    //     int pre_clip_s = mask_bf_net->cfg->pre_clip_s;
    //     int pre_clip_e = mask_bf_net->cfg->pre_clip_e;
    //     float pre_pow_ratio = mask_bf_net->cfg->pre_pow_ratio;
    //     float pre_mul_ratio = mask_bf_net->cfg->pre_mul_ratio;
    //     float alpha;
    //     for (n = 0; n < num_frame; ++n, fftx += nbin) {
    //         for (int i = pre_clip_s; i < pre_clip_e; ++i) {
    //             alpha = (pre_mul_ratio - 1) *
    //                         (pow(i - pre_clip_s, pre_pow_ratio)) /
    //                         pow((nbin - pre_clip_s), pre_pow_ratio) +
    //                     1;
    //             fftx[i].a *= alpha;
    //             fftx[i].b *= alpha;
    //         }
    //     }
    // }
    if (mask_bf_net->sum_sp_sil == num_frame) {
        max_mask = mask_bf_net->cfg->max_mask;
        mask_peak = mask_bf_net->cfg->mask_peak;
    } else {
        max_mask = mask_bf_net->cfg->sp_max_mask;
        mask_peak = mask_bf_net->cfg->sp_mask_peak;
    }

    fftx = mask_bf_net->fftx;
    ffty = mask_bf_net->ffty;
    memcpy(fft_tmp, fftx, sizeof(wtk_complex_t) * nbin * num_frame);
    if (mask_bf_net->cfg->nspchannel > 0) {
        memcpy(ffty, mask_bf_net->fft_sp[0],
               sizeof(wtk_complex_t) * nbin * num_frame);
    } else {
        memset(ffty, 0, sizeof(wtk_complex_t) * nbin * num_frame);
    }

    if (!mask_bf_net->cfg->use_gainnet2) {
        if (qmmse) {
            fftx = mask_bf_net->fftx;
            ffty = mask_bf_net->ffty;
            qmmse_gain = mask_bf_net->qmmse_gain;
            for (n = 0; n < num_frame;
                 ++n, qmmse_gain += nbin, fftx += nbin, ffty += nbin) {
                wtk_qmmse_flush_echo_mask(qmmse, fftx, ffty,
                                          mask_bf_net->sp_state[n]);
                memcpy(qmmse_gain, qmmse->gain, sizeof(float) * nbin);
            }
        }
    }

    fftx = mask_bf_net->fftx;
    ffty = mask_bf_net->ffty;
    if (mask_bf_net->cfg->use_gainnet2) {
        for (n = 0; n < num_frame; ++n, fftx += nbin, ffty += nbin) {
            for (k = 0; k < nbin; ++k) {
                fftx[k].a *= scale;
                fftx[k].b *= scale;
                ffty[k].a *= scale;
                ffty[k].b *= scale;
            }
            wtk_mask_bf_net_edr_feed_gainnet2(mask_bf_net, fftx, ffty);
            gc_mask[n] = wtk_float_abs_mean(mask_bf_net->vdr->gf, nbin);
            for (k = 0; k < nbin; ++k) {
                gf = mask_bf_net->vdr->gf[k];
                ffty[k].a = fftx[k].a * (1.0 - gf);
                ffty[k].a = fftx[k].b * (1.0 - gf);
                fftx[k].a *= gf;
                fftx[k].b *= gf;
                fftx[k].a *= scale_1;
                fftx[k].b *= scale_1;
                ffty[k].a *= scale_1;
                ffty[k].b *= scale_1;
            }
            if (mask_mu) {
                memcpy(mask_mu, mask_mu + nbin,
                       sizeof(float) * nbin * (n_mask_mu + num_frame - 2));
                memcpy(mask_mu + nbin * (n_mask_mu + num_frame - 2),
                       mask_bf_net->vdr->gf, sizeof(float) * nbin);
            }
        }
    } else if (((mask_bf_net->cfg->use_stage2_rt ||
                 mask_bf_net->cfg->nspchannel == 0) &&
                mask_bf_net->cfg->use_dr_stage2_rt) ||
               (mask_bf_net->cfg->use_qnn && mask_bf_net->cfg->qnn3_buf)) {
        if (mask_bf_net->cfg->use_mask_model) {
            if (mask_bf_net->sum_sp_sil == num_frame) {
                if (mask_bf_net->cfg->use_dr_stage2_rt) {
                    wtk_mask_bf_net_edr_feed_mask_onnx(mask_bf_net, fftx, NULL);
                } else if (mask_bf_net->cfg->use_qnn &&
                           mask_bf_net->cfg->qnn3_buf) {
                    wtk_mask_bf_net_dr_feed_qnn(mask_bf_net, fftx);
                }
            } else {
                if (mask_bf_net->cfg->use_stage2_rt) {
                    wtk_mask_bf_net_edr_feed_mask_onnx(mask_bf_net, fftx, ffty);
                } else if (mask_bf_net->cfg->use_qnn &&
                           mask_bf_net->cfg->qnn2_buf) {
                    wtk_mask_bf_net_edr_feed_qnn(mask_bf_net, fftx, ffty);
                }
            }
            if (qmmse) {
                qmmse_gain = mask_bf_net->qmmse_gain;
                for (k = 0; k < num_frame * nbin; ++k) {
                    model_mask[k] = min(model_mask[k], qmmse_gain[k]);
                }
            }
            for (n = 0; n < num_frame;
                 ++n, fftx += nbin, ffty += nbin, fft_tmp += nbin) {
                gf = 0;
                for (k = 0; k < nbin; ++k) {
                    mask = model_mask[k + n * nbin];
                    mask = max(0.0, min(1.0, mask));
                    // fftx[k].a = fft_tmp[k].a * mask;
                    // fftx[k].b = fft_tmp[k].b * mask;
                    gf += mask;
                    if (mask_peak > 0) {
                        if (mask < mask_peak) {
                            mask = (max_mask / (mask_peak * mask_peak) -
                                    1 / mask_peak) *
                                       mask * mask +
                                   mask;
                        } else {
                            mask = max_mask;
                        }
                    }
                    mask = max(0.0, min(1.0, mask));
                    fftx[k].a = fft_tmp[k].a * mask;
                    fftx[k].b = fft_tmp[k].b * mask;
                    ffty[k].a = fft_tmp[k].a * (1.0 - mask);
                    ffty[k].b = fft_tmp[k].b * (1.0 - mask);
                    if (mask_tmp) {
                        mask_tmp[k] = mask;
                    }
                }
                gc_mask[n] = gf / nbin;
                if (mask_mu) {
                    memmove(mask_mu, mask_mu + nbin,
                            sizeof(float) * nbin * (n_mask_mu + num_frame - 2));
                    memcpy(mask_mu + nbin * (n_mask_mu + num_frame - 2),
                           mask_tmp, sizeof(float) * nbin);
                }
            }
        } else {
            if (mask_bf_net->sum_sp_sil == num_frame) {
                wtk_mask_bf_net_dr_feed_onnx(mask_bf_net, fftx, NULL);
            } else {
                wtk_mask_bf_net_dr_feed_onnx(mask_bf_net, fftx, ffty);
            }
            for (n = 0; n < num_frame;
                 ++n, fftx += nbin, ffty += nbin, fft_tmp += nbin) {
                gf = 0;
                for (k = 0; k < nbin; ++k) {
                    mask =
                        sqrtf(fftx[k].a * fftx[k].a + fftx[k].b * fftx[k].b) /
                        sqrtf(fft_tmp[k].a * fft_tmp[k].a +
                              fft_tmp[k].b * fft_tmp[k].b);
                    mask = max(0.0, min(1.0, mask));
                    gf += mask;
                    if (mask_peak > 0) {
                        if (mask < mask_peak) {
                            mask = (max_mask / (mask_peak * mask_peak) -
                                    1 / mask_peak) *
                                       mask * mask +
                                   mask;
                        } else {
                            mask = max_mask;
                        }
                    }
                    mask = max(0.0, min(1.0, mask));
                    // fftx[k].a = fft_tmp[k].a * mask;
                    // fftx[k].b = fft_tmp[k].b * mask;
                    ffty[k].a = fft_tmp[k].a * (1.0 - mask);
                    ffty[k].b = fft_tmp[k].b * (1.0 - mask);
                    if (mask_tmp) {
                        mask_tmp[k] = mask;
                    }
                }
                gc_mask[n] = gf / nbin;
                if (mask_mu) {
                    memcpy(mask_mu, mask_mu + nbin,
                           sizeof(float) * nbin * (n_mask_mu + num_frame - 2));
                    memcpy(mask_mu + nbin * (n_mask_mu + num_frame - 2),
                           mask_tmp, sizeof(float) * nbin);
                }
            }
        }
    } else if (mask_bf_net->cfg->use_stage2_rt ||
               (mask_bf_net->cfg->use_qnn && mask_bf_net->cfg->qnn2_buf)) {
        if (mask_bf_net->cfg->use_mask_model) {
            if (mask_bf_net->cfg->use_stage2_rt) {
                wtk_mask_bf_net_edr_feed_mask_onnx(mask_bf_net, fftx, ffty);
            } else if (mask_bf_net->cfg->use_qnn &&
                       mask_bf_net->cfg->qnn2_buf) {
                wtk_mask_bf_net_edr_feed_qnn(mask_bf_net, fftx, ffty);
            }
            if (qmmse) {
                qmmse_gain = mask_bf_net->qmmse_gain;
                for (k = 0; k < num_frame * nbin; ++k) {
                    model_mask[k] = min(model_mask[k], qmmse_gain[k]);
                }
            }
            for (n = 0; n < num_frame;
                 ++n, fftx += nbin, ffty += nbin, fft_tmp += nbin) {
                gf = 0;
                for (k = 0; k < nbin; ++k) {
                    mask = model_mask[k + n * nbin];
                    mask = max(0.0, min(1.0, mask));
                    // fftx[k].a = fft_tmp[k].a * mask;
                    // fftx[k].b = fft_tmp[k].b * mask;
                    gf += mask;
                    if (mask_peak > 0) {
                        if (mask < mask_peak) {
                            mask = (max_mask / (mask_peak * mask_peak) -
                                    1 / mask_peak) *
                                       mask * mask +
                                   mask;
                        } else {
                            mask = max_mask;
                        }
                    }
                    mask = max(0.0, min(1.0, mask));
                    fftx[k].a = fft_tmp[k].a * mask;
                    fftx[k].b = fft_tmp[k].b * mask;
                    ffty[k].a = fft_tmp[k].a * (1.0 - mask);
                    ffty[k].b = fft_tmp[k].b * (1.0 - mask);
                    if (mask_tmp) {
                        mask_tmp[k] = mask;
                    }
                }
                gc_mask[n] = gf / nbin;
                if (mask_mu) {
                    memmove(mask_mu, mask_mu + nbin,
                            sizeof(float) * nbin * (n_mask_mu + num_frame - 2));
                    memcpy(mask_mu + nbin * (n_mask_mu + num_frame - 2),
                           mask_tmp, sizeof(float) * nbin);
                }
            }
        } else {
            wtk_mask_bf_net_dr_feed_onnx(mask_bf_net, fftx, ffty);
            for (n = 0; n < num_frame;
                 ++n, fftx += nbin, ffty += nbin, fft_tmp += nbin) {
                gf = 0;
                for (k = 0; k < nbin; ++k) {
                    mask =
                        sqrtf(fftx[k].a * fftx[k].a + fftx[k].b * fftx[k].b) /
                        sqrtf(fft_tmp[k].a * fft_tmp[k].a +
                              fft_tmp[k].b * fft_tmp[k].b);
                    mask = max(0.0, min(1.0, mask));
                    gf += mask;
                    if (mask_peak > 0) {
                        if (mask < mask_peak) {
                            mask = (max_mask / (mask_peak * mask_peak) -
                                    1 / mask_peak) *
                                       mask * mask +
                                   mask;
                        } else {
                            mask = max_mask;
                        }
                    }
                    mask = max(0.0, min(1.0, mask));
                    // fftx[k].a = fft_tmp[k].a * mask;
                    // fftx[k].b = fft_tmp[k].b * mask;
                    ffty[k].a = fft_tmp[k].a * (1.0 - mask);
                    ffty[k].b = fft_tmp[k].b * (1.0 - mask);
                    if (mask_tmp) {
                        mask_tmp[k] = mask;
                    }
                }
                gc_mask[n] = gf / nbin;
                if (mask_mu) {
                    memcpy(mask_mu, mask_mu + nbin,
                           sizeof(float) * nbin * (n_mask_mu + num_frame - 2));
                    memcpy(mask_mu + nbin * (n_mask_mu + num_frame - 2),
                           mask_tmp, sizeof(float) * nbin);
                }
            }
        }
    }

    // if (mask_mu) {
    //     float alpha = mask_bf_net->cfg->mu_t_alpha;
    //     float alpha_1 = 1.0 - alpha;
    //     float alpha2 = mask_bf_net->cfg->mu_f_alpha;
    //     float alpha2_1 = 1.0 - alpha2;
    //     int base_fs = mask_bf_net->cfg->mu_mask_s;
    //     int base_fe = mask_bf_net->cfg->mu_mask_e;
    //     int idx, idx2, idx3;
    //     int i, j;

    //     for (k = 0; k < nbin; ++k) {
    //         for (i = n_mask_mu - 1; i < n_mask_mu + num_frame - 1; ++i) {
    //             idx = i * nbin + k;
    //             mask_mu2[idx] = mask_mu[idx];
    //         }
    //     }
    //     for (k = base_fs; k < base_fe; ++k) {
    //         for (i = n_mask_mu - 1; i < n_mask_mu + num_frame - 1; ++i) {
    //             idx = i * nbin + k;
    //             idx3 = idx;
    //             mask_mu2[idx] = mask_mu[idx];
    //             for (j = i - 1; j > i - n_mask_mu; --j) {
    //                 idx2 = j * nbin + k;
    //                 if (mask_mu[idx3] < mask_mu[idx2]) {
    //                     mask_mu2[idx] =
    //                         mask_mu[idx2] * alpha + mask_mu[idx] * alpha_1;
    //                     idx3 = idx2;
    //                 }
    //             }
    //         }
    //     }
    //     for (i = n_mask_mu - 1; i < n_mask_mu + num_frame - 1; ++i) {
    //         for (k = base_fe; k < nbin; ++k) {
    //             idx = i * nbin + k;
    //             idx3 = idx;
    //             mask_mu2[idx] = mask_mu[idx];
    //             for (j = k; j > 0; --j) {
    //                 idx2 = i * nbin + j;
    //                 if (mask_mu2[idx3] < mask_mu2[idx2]) {
    //                     mask_mu2[idx] =
    //                         mask_mu2[idx2] * alpha2 + mask_mu[idx] * alpha2_1;
    //                     idx3 = idx2;
    //                 }
    //             }
    //         }
    //     }
    // }

    if (mask_mu) {
        float alpha = mask_bf_net->cfg->mu_t_alpha;
        float alpha_1 = 1.0 - alpha;
        float alpha2 = mask_bf_net->cfg->mu_f_alpha;
        float alpha2_1 = 1.0 - alpha2;
        int base_fs = mask_bf_net->cfg->mu_mask_s;
        int base_fe = mask_bf_net->cfg->mu_mask_e;
        int i, j;
        const int start_frame = n_mask_mu - 1;
        const int end_frame = start_frame + num_frame - 1;
        const size_t frame_size = nbin * sizeof(float);
        // 第一部分：批量初始化
        memcpy(&mask_mu2[start_frame * nbin], &mask_mu[start_frame * nbin], 
            (end_frame - start_frame + 1) * frame_size);
    
        // 第二部分：时间方向优化
        for (k = base_fs; k < base_fe; ++k) {
            for (i = start_frame; i <= end_frame; ++i) {
                const int idx = i * nbin + k;
                const float current_val = mask_mu[idx];
                float max_val = current_val;
                int max_index = i;
                
                // 计算搜索边界
                const int j_start = i - 1;
                int j_end = i - n_mask_mu;
                if (j_end < 0) j_end = -1;
                
                // 指针优化搜索
                const float* ptr = &mask_mu[j_start * nbin + k];
                for (j = j_start; j > j_end; --j, ptr -= nbin) {
                    if (*ptr > max_val) {
                        max_val = *ptr;
                        max_index = j;
                    }
                }
                
                // 条件更新
                if (max_index != i) {
                    mask_mu2[idx] = max_val * alpha + current_val * alpha_1;
                }
            }
        }
        
        // 第三部分：频率方向优化
        for (i = start_frame; i <= end_frame; ++i) {
            // 使用原始数据副本
            float frame_copy[nbin];
            memcpy(frame_copy, &mask_mu2[i * nbin], frame_size);
            
            // 预计算低频最大值
            float max_val = frame_copy[0];
            int max_index = 0;
            
            for (k = 1; k < base_fe; ++k) {
                if (frame_copy[k] > max_val) {
                    max_val = frame_copy[k];
                    max_index = k;
                }
            }
            
            // 处理高频部分
            for (k = base_fe; k < nbin; ++k) {
                const int idx = i * nbin + k;
                const float current_val = mask_mu[idx];
                
                // 更新最大值
                if (frame_copy[k] > max_val) {
                    max_val = frame_copy[k];
                    max_index = k;
                }
                
                // 条件更新
                if (max_index != k) {
                    mask_mu2[idx] = mask_mu2[i * nbin + max_index] * alpha2 + 
                                    current_val * alpha2_1;
                }
            }
        }
    }
}

void wtk_mask_bf_net_feed_bf2(wtk_mask_bf_net_t *mask_bf_net) {
    int n, k;
    wtk_complex_t *fftx, *ffty;
    int num_frame = mask_bf_net->cfg->num_frame;
    int nbin = mask_bf_net->nbin;
    float *mask_mu = mask_bf_net->mask_mu;
    float *mask_mu2 = mask_bf_net->mask_mu2;
    int n_mask_mu = mask_bf_net->cfg->n_mask_mu;
    float mu_entropy_thresh = mask_bf_net->cfg->mu_entropy_thresh;
    float bf2_alpha;
    float bf2_alpha_1;
    float echo_bf2_alpha = mask_bf_net->cfg->echo_bf2_alpha;
    float echo_bf2_alpha_1 = 1.0 - echo_bf2_alpha;
    float bf_change_frame = mask_bf_net->cfg->bf_change_frame;
    float bf2_cfg_alpha = mask_bf_net->cfg->bf2_alpha;
    float alpha;

    float *scov;
    float *ncov;
    int *cnt_sum;
    float scov_alpha;
    float ncov_alpha;
    float scov_alpha_1;
    float ncov_alpha_1;
    int init_covnf;
    float w;
    float mu;

    fftx = mask_bf_net->fftx;
    ffty = mask_bf_net->ffty;
    for (n = 0; n < num_frame; ++n, fftx += nbin, ffty += nbin) {
        if (mask_bf_net->cfg->use_echocovm && mask_bf_net->sp_state[n] == 0) {
            scov = mask_bf_net->sim_echo_scov;
            ncov = mask_bf_net->sim_echo_ncov;
            cnt_sum = mask_bf_net->sim_echo_cnt_sum;
        } else {
            scov = mask_bf_net->sim_scov;
            ncov = mask_bf_net->sim_ncov;
            cnt_sum = mask_bf_net->sim_cnt_sum;
        }
        // scov = mask_bf_net->sim_scov;
        ncov = mask_bf_net->sim_ncov;

        scov_alpha = mask_bf_net->cfg->scov_alpha;
        ncov_alpha = mask_bf_net->cfg->ncov_alpha;
        init_covnf = mask_bf_net->cfg->init_covnf;
        scov_alpha_1 = 1.0 - scov_alpha;
        ncov_alpha_1 = 1.0 - ncov_alpha;

        if (mask_bf_net->sp_state[n] == 0) {
            mask_bf_net->echo_out = 0;
        } else {
            if (mask_bf_net->echo_out == 0) {
                mask_bf_net->bf2_alpha_cnt = bf_change_frame;
                mask_bf_net->echo_out = 1;
            }
        }
        if (mask_bf_net->bf2_alpha_cnt > 0) {
            --mask_bf_net->bf2_alpha_cnt;
        }
        alpha = (bf_change_frame - mask_bf_net->bf2_alpha_cnt) * 1.0 /
                bf_change_frame;
        alpha = powf(alpha, 2.0);
        bf2_alpha = bf2_cfg_alpha * alpha + (1.0 - alpha) * echo_bf2_alpha;
        bf2_alpha_1 = 1.0 - bf2_alpha;

        float entropy = wtk_mask_bf_net_entropy(mask_bf_net, fftx, 200, 3500);
        // printf("%f\n", entropy);
        int idx;

        for (k = 0; k < nbin; ++k) {
            if (mask_bf_net->cfg->use_echocovm &&
                mask_bf_net->sp_state[n] == 0) {
                mu = mask_bf_net->cfg->echo_bfmu;
            } else {
                mu = mask_bf_net->cfg->bfmu;
            }
            if (entropy < mu_entropy_thresh) {
                idx = (n + n_mask_mu - 1) * nbin + k;
                if (mask_mu) {
                    mu = (1.0 - mask_mu2[idx]) * mask_bf_net->cfg->bfmu;
                }
            }
            // printf("%f\n", mu);
            if (cnt_sum[k] < init_covnf) {
                scov[k] += fftx[k].a * fftx[k].a + fftx[k].b * fftx[k].b;
            } else if (cnt_sum[k] == init_covnf) {
                scov[k] /= cnt_sum[k];
            } else {
                scov[k] = scov_alpha_1 * scov[k] +
                          scov_alpha *
                              (fftx[k].a * fftx[k].a + fftx[k].b * fftx[k].b);
            }
            if (cnt_sum[k] < init_covnf) {
                ncov[k] += ffty[k].a * ffty[k].a + ffty[k].b * ffty[k].b;
            } else if (cnt_sum[k] == init_covnf) {
                ncov[k] /= cnt_sum[k];
            } else {
                ncov[k] = ncov_alpha_1 * ncov[k] +
                          ncov_alpha *
                              (ffty[k].a * ffty[k].a + ffty[k].b * ffty[k].b);
            }
            cnt_sum[k]++;
            if (ncov[k] < 1e-10) {
                ncov[k] = 1e-10;
            }
            w = scov[k] / ncov[k];
            w = w / (mu + w);
            if (mask_bf_net->sp_state[n] == 0) {
                w = w * echo_bf2_alpha + echo_bf2_alpha_1;
            } else {
                w = w * bf2_alpha + bf2_alpha_1;
            }
            fftx[k].a = fftx[k].a * w;
            fftx[k].b = fftx[k].b * w;
        }
    }
    // static int cnt = 100;
    // --cnt;
    // if (cnt < 0) {
    //     exit(0);
    // }
}

void wtk_mask_bf_net_feed_post_process(wtk_mask_bf_net_t *mask_bf_net) {
    int i, n;
    int num_frame = mask_bf_net->cfg->num_frame;
    int nbin = mask_bf_net->nbin;
    wtk_complex_t *fftx;
    wtk_complex_t *ffty;
    float *gc_mask = mask_bf_net->gc_mask;

    wtk_drft_t *rfft = mask_bf_net->rfft;
    float *rfft_in = mask_bf_net->rfft_in;
    float *synthesis_mem = mask_bf_net->synthesis_mem;
    float *synthesis_window = mask_bf_net->synthesis_window;
    float *out = mask_bf_net->out;
    short *pv = (short *)out;
    int wins = mask_bf_net->cfg->wins;
    int fsize = wins / 2;

    float micenr;
    float micenr_thresh = mask_bf_net->cfg->micenr_thresh;
    int micenr_cnt = mask_bf_net->cfg->micenr_cnt;

    float *model_mask = mask_bf_net->model_mask;
    float qmmse2_mask_thresh = mask_bf_net->cfg->qmmse2_mask_thresh;

    int clip_s = mask_bf_net->cfg->clip_s;
    int clip_e = mask_bf_net->cfg->clip_e;

    fftx = mask_bf_net->fftx;
    ffty = mask_bf_net->ffty;
    for (n = 0; n < num_frame;
         ++n, model_mask += nbin, fftx += nbin, ffty += nbin) {
        if (gc_mask[n] > mask_bf_net->cfg->gc_min_thresh) {
            mask_bf_net->gc_cnt = mask_bf_net->cfg->gc_cnt;
        } else {
            --mask_bf_net->gc_cnt;
        }

        if (mask_bf_net->gc_cnt >= 0 && mask_bf_net->denoise_enable) {
            if (mask_bf_net->qmmse2) {
                wtk_qmmse_set_sp_sil(mask_bf_net->qmmse2,
                                     mask_bf_net->sp_state[n]);
                if (model_mask) {
                    float mean_mask = wtk_float_abs_mean(model_mask, nbin);
                    if (mean_mask < qmmse2_mask_thresh) {
                        wtk_qmmse_feed_mask(mask_bf_net->qmmse2, fftx,
                                            model_mask);
                    } else {
                        wtk_qmmse_denoise(mask_bf_net->qmmse2, fftx);
                    }
                } else {
                    wtk_qmmse_denoise(mask_bf_net->qmmse2, fftx);
                }
            }
            if (mask_bf_net->gc && mask_bf_net->agc_enable) {
                float gc_scale = 32768.0 / mask_bf_net->cfg->wins;
                float gc_scale_1 = 1.0 / gc_scale;
                for (i = 0; i < nbin; ++i) {
                    fftx[i].a *= gc_scale;
                    fftx[i].b *= gc_scale;
                }
                qtk_gain_controller_run(mask_bf_net->gc, fftx, nbin, NULL,
                                        gc_mask[n]);
                for (i = 0; i < nbin; ++i) {
                    fftx[i].a *= gc_scale_1;
                    fftx[i].b *= gc_scale_1;
                }
            }
        }
        if (mask_bf_net->cfg->use_raw_add && mask_bf_net->denoise_enable) {
            for (i = 0; i < nbin; ++i) {
                fftx[i].a += mask_bf_net->fft[0][i].a;
                fftx[i].b += mask_bf_net->fft[0][i].b;
            }
        }

        // static int cnt=0;
        // cnt++;
        micenr = wtk_mask_bf_net_fft_energy(fftx, nbin);
        // printf("%f\n", micenr);
        if (micenr > micenr_thresh) {
            // if(mask_bf_net->mic_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),micenr,micenr_thresh);
            // }
            mask_bf_net->mic_sil = 0;
            mask_bf_net->mic_silcnt = micenr_cnt;
        } else if (mask_bf_net->mic_sil == 0) {
            mask_bf_net->mic_silcnt -= 1;
            if (mask_bf_net->mic_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                mask_bf_net->mic_sil = 1;
            }
        }

        if (mask_bf_net->cfg->use_cnon) {
            wtk_mask_bf_net_feed_cnon(mask_bf_net, fftx);
        }

        for (i = 0; i < clip_s; ++i) {
            fftx[i].a = 0;
            fftx[i].b = 0;
        }
        for (i = clip_e; i < nbin; ++i) {
            fftx[i].a = 0;
            fftx[i].b = 0;
        }

        if (mask_bf_net->cfg->use_pffft) {
            wtk_drft_istft2(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                            synthesis_window);
        } else {
            wtk_drft_istft(rfft, rfft_in, synthesis_mem, fftx, out, wins,
                           synthesis_window);
        }

        for (i = 0; i < fsize; ++i) {
            out[i] *= 32768.0;
        }
        if (mask_bf_net->eq) {
            wtk_equalizer_feed_float(mask_bf_net->eq, out, fsize);
        }
        if (mask_bf_net->limiter) {
            wtk_limiter_feed(mask_bf_net->limiter, out, fsize);
        } else {
            wtk_mask_bf_net_control_bs(mask_bf_net, out, fsize);
        }
        for (i = 0; i < fsize; ++i) {
            pv[i] = floorf(out[i] + 0.5);
        }
        if (mask_bf_net->notify) {
            mask_bf_net->notify(mask_bf_net->ths, pv, fsize);
        }
    }
}

void wtk_mask_bf_net_feed(wtk_mask_bf_net_t *mask_bf_net, short *data, int len,
                          int is_end) {
    int i, j;
    int nbin = mask_bf_net->nbin;
    int nmicchannel = mask_bf_net->cfg->nmicchannel;
    int *mic_channel = mask_bf_net->cfg->mic_channel;
    int nspchannel = mask_bf_net->cfg->nspchannel;
    int *sp_channel = mask_bf_net->cfg->sp_channel;
    int sp_main_chn = mask_bf_net->cfg->sp_main_chn;
    int channel = mask_bf_net->cfg->channel;
    int wins = mask_bf_net->cfg->wins;
    int fsize = wins / 2;
    int num_frame = mask_bf_net->cfg->num_frame;
    int nfsize = fsize * num_frame;
    int offset = nfsize - fsize;
    int f_offset = 0;
    wtk_drft_t *rfft = mask_bf_net->rfft;
    float *rfft_in = mask_bf_net->rfft_in;
    wtk_complex_t **fft = mask_bf_net->fft;
    wtk_complex_t **fft_sp = mask_bf_net->fft_sp;
    wtk_complex_t *fftx = mask_bf_net->fftx;
    // wtk_complex_t *ffty = mask_bf_net->ffty;
    float **analysis_mem = mask_bf_net->analysis_mem;
    float **analysis_mem_sp = mask_bf_net->analysis_mem_sp;
    // float *synthesis_mem = mask_bf_net->synthesis_mem;
    // float *synthesis_window = mask_bf_net->synthesis_window;
    float *analysis_window = mask_bf_net->analysis_window;
    float *out = mask_bf_net->out;
    short *pv = (short *)out;
    wtk_strbuf_t **mic = mask_bf_net->mic;
    wtk_strbuf_t **sp = mask_bf_net->sp;
    float fv;
    int length;
    float spenr;
    float spenr_thresh = mask_bf_net->cfg->spenr_thresh;
    int spenr_cnt = mask_bf_net->cfg->spenr_cnt;
    float mic_scale = mask_bf_net->mic_scale;
    float sp_scale = mask_bf_net->sp_scale;
    float *fv1;
    float *eng = mask_bf_net->eng;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[j]]) * mic_scale;
            wtk_strbuf_push(mic[j], (char *)&(fv), sizeof(float));
        }
        for (j = 0; j < nspchannel; ++j) {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[sp_channel[j]]) * sp_scale;
            wtk_strbuf_push(sp[j], (char *)&(fv), sizeof(float));
        }
        data += channel;
    }
    length = mic[0]->pos / sizeof(float);
    while (length >= nfsize) {
        ++mask_bf_net->nframe;
        f_offset = (mask_bf_net->nframe - 1) * mask_bf_net->nbin;
        for (i = 0; i < nmicchannel; ++i) {
            fv1 = (float *)(mic[i]->data);
            // {
            //     int ii;
            //     int n = wins;

            //     for (ii = 0; ii < n; ++ii) {
            //         analysis_window[ii] =
            //             sqrtf(0.5 * (1 - cos(2 * PI * (ii) / (n - 1))));
            //     }
            // }
            if (mask_bf_net->cfg->use_pffft) {
                wtk_drft_stft2(rfft, rfft_in, analysis_mem[i],
                               fft[i] + f_offset, fv1 + offset, wins,
                               analysis_window);
            } else {
                wtk_drft_stft(rfft, rfft_in, analysis_mem[i], fft[i] + f_offset,
                              fv1 + offset, wins, analysis_window);
            }
            if (eng) {
                eng[i] = eng[i] * 0.9 +
                         wtk_mask_bf_net_fft_energy(fft[i] + f_offset,
                                                    mask_bf_net->nbin) *
                             0.1;
                // eng[i] = wtk_mask_bf_net_sp_energy(fv1 + offset, fsize);
            }
        }
        if (mask_bf_net->echo_enable) {
            for (i = 0; i < nspchannel; ++i) {
                fv1 = (float *)(sp[i]->data);
                if (mask_bf_net->cfg->use_pffft) {
                    wtk_drft_stft2(rfft, rfft_in, analysis_mem_sp[i],
                                   fft_sp[i] + f_offset, fv1 + offset, wins,
                                   analysis_window);
                } else {
                    wtk_drft_stft(rfft, rfft_in, analysis_mem_sp[i],
                                  fft_sp[i] + f_offset, fv1 + offset, wins,
                                  analysis_window);
                }
            }
        } else {
            for (i = 0; i < nspchannel; ++i) {
                memset(fft_sp[i], 0, sizeof(wtk_complex_t) * nbin * num_frame);
            }
        }

        if (nspchannel > 0) {
            fv1 = (float *)(sp[0]->data);
            spenr = wtk_mask_bf_net_sp_energy(fv1 + offset, fsize);
        } else {
            spenr = 0;
        }
        // printf("%f\n", spenr);
        // static int cnt=0;
        // cnt++;
        if (spenr > spenr_thresh) {
            // if(mask_bf_net->sp_sil==1)
            // {
            // 	printf("sp start %f %f
            // %f\n", 1.0/16000*cnt*(nbin-1),spenr,spenr_thresh);
            // }
            mask_bf_net->sp_sil = 0;
            mask_bf_net->sp_silcnt = spenr_cnt;
        } else if (mask_bf_net->sp_sil == 0) {
            mask_bf_net->sp_silcnt -= 1;
            if (mask_bf_net->sp_silcnt <= 0) {
                // printf("sp end %f\n", 1.0/16000*cnt*(nbin-1));
                mask_bf_net->sp_sil = 1;
            }
        }
        // printf("%d\n", mask_bf_net->sp_sil);

        mask_bf_net->sum_sp_sil += mask_bf_net->sp_sil;
        mask_bf_net->sp_state[mask_bf_net->nframe - 1] = mask_bf_net->sp_sil;

        if (mask_bf_net->cfg->use_bf && mask_bf_net->denoise_enable) {
            if (mask_bf_net->nframe == num_frame) {
                if (mask_bf_net->cfg->use_echo_bf) {
                    if (mask_bf_net->sum_sp_sil != num_frame) {
                        wtk_mask_bf_net_feed_edr(mask_bf_net);
                    }
                    if (mask_bf_net->cfg->use_gainnet2_2 ||
                        mask_bf_net->cfg->use_stage1_rt ||
                        mask_bf_net->cfg->use_qnn) {
                        wtk_mask_bf_net_feed_model1(mask_bf_net);
                        if (mask_bf_net->cfg->use_bf_v2) {
                            wtk_mask_bf_net_feed_bf_v2(mask_bf_net);
                        } else {
                            wtk_mask_bf_net_feed_bf(mask_bf_net);
                        }
                    } else {
                        memcpy(fftx, fft[sp_main_chn],
                               sizeof(wtk_complex_t) * nbin * num_frame);
                    }
                } else {
                    if (mask_bf_net->sum_sp_sil == num_frame) {
                        if (mask_bf_net->cfg->use_gainnet2_2 ||
                            mask_bf_net->cfg->use_stage1_rt ||
                            mask_bf_net->cfg->use_qnn) {
                            wtk_mask_bf_net_feed_model1(mask_bf_net);
                            if (mask_bf_net->cfg->use_bf_v2) {
                                wtk_mask_bf_net_feed_bf_v2(mask_bf_net);
                            } else {
                                wtk_mask_bf_net_feed_bf(mask_bf_net);
                            }
                        } else {
                            memcpy(fftx, fft[sp_main_chn],
                                   sizeof(wtk_complex_t) * nbin * num_frame);
                        }
                    } else {
                        memcpy(fftx, fft[sp_main_chn],
                               sizeof(wtk_complex_t) * nbin * num_frame);
                        wtk_mask_bf_net_feed_edr(mask_bf_net);
                    }
                }
                // mask_bf_net->sum_sp_sil = 0;
            }
        } else {
            if (mask_bf_net->cfg->use_freq_preemph) {
                int pre_clip_s = mask_bf_net->cfg->pre_clip_s;
                int pre_clip_e = mask_bf_net->cfg->pre_clip_e;
                float alpha;
                float *pre_alpha = mask_bf_net->pre_alpha;
                int k, n, idx;
                for (n = 0; n < num_frame; ++n) {
                    for (k = pre_clip_s; k < pre_clip_e; ++k) {
                        idx = k + n * nbin;
                        alpha = pre_alpha[k];
                        for (i = 0; i < nmicchannel; ++i) {
                            fft[i][idx].a *= alpha;
                            fft[i][idx].b *= alpha;
                        }
                    }
                }
            }
            memcpy(fftx, fft[sp_main_chn],
                   sizeof(wtk_complex_t) * nbin * num_frame);
            wtk_mask_bf_net_feed_edr(mask_bf_net);
        }

        if (mask_bf_net->cfg->use_debug) {
            if (mask_bf_net->nframe == num_frame) {
                wtk_mask_bf_net_debug(mask_bf_net);
            }
        }

        if (mask_bf_net->nframe == num_frame) {
            if (mask_bf_net->cfg->use_debug == 0) {
                if (mask_bf_net->denoise_enable) {
                    wtk_mask_bf_net_feed_model2(mask_bf_net); // 0.1
                    if (mask_bf_net->cfg->use_bf2) {
                        wtk_mask_bf_net_feed_bf2(mask_bf_net);
                    }
                }
                wtk_mask_bf_net_feed_post_process(mask_bf_net);
            }
            mask_bf_net->sum_sp_sil = 0;
            mask_bf_net->nframe = 0;
        }
        wtk_strbufs_pop(mic, nmicchannel, fsize * sizeof(float));
        wtk_strbufs_pop(sp, nspchannel, fsize * sizeof(float));
        length = mic[0]->pos / sizeof(float);
    }
    if (is_end && length > 0) {
        if (mask_bf_net->notify) {
            out = (float *)mic[0]->data;
            pv = (short *)out;
            for (i = 0; i < length; ++i) {
                pv[i] = WTK_WAV_FLOAT_TO_SHORT(out[i]);
            }
            mask_bf_net->notify(mask_bf_net->ths, pv, length);
        }
    }
}

void wtk_mask_bf_net_set_micscale(wtk_mask_bf_net_t *mask_bf_net, float scale) {
    mask_bf_net->mic_scale = scale;
}
void wtk_mask_bf_net_set_agcenable(wtk_mask_bf_net_t *mask_bf_net, int enable) {
    mask_bf_net->agc_enable = enable;
    if (mask_bf_net->qmmse2) {
        if (enable) {
            mask_bf_net->qmmse2->cfg->use_agc = 1;
        } else {
            mask_bf_net->qmmse2->cfg->use_agc = 0;
        }
    }
}
void wtk_mask_bf_net_set_agclevel(wtk_mask_bf_net_t *mask_bf_net, int level) {
    int i;
    int n_agc_level = mask_bf_net->cfg->n_agc_level;
    float *qmmse2_agc_level = mask_bf_net->cfg->qmmse2_agc_level;
    float *qmmse2_max_gain = mask_bf_net->cfg->qmmse2_max_gain;
    float *gc_gain_level = mask_bf_net->cfg->gc_gain_level;
    level = min(level, n_agc_level);
    if (mask_bf_net->qmmse2) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                mask_bf_net->qmmse2->cfg->agc_level = qmmse2_agc_level[i];
                mask_bf_net->qmmse2->cfg->max_gain = qmmse2_max_gain[i];
                break;
            }
        }
    }
    if (mask_bf_net->gc) {
        for (i = 0; i < n_agc_level; ++i) {
            if (level == i + 1) {
                mask_bf_net->gc->kalman.Z_k = gc_gain_level[i];
                break;
            }
        }
    }
}
void wtk_mask_bf_net_set_echoenable(wtk_mask_bf_net_t *mask_bf_net,
                                    int enable) {
    mask_bf_net->echo_enable = enable;
}
void wtk_mask_bf_net_set_denoiseenable(wtk_mask_bf_net_t *mask_bf_net,
                                       int enable) {
    mask_bf_net->denoise_enable = enable;
}
void wtk_mask_bf_net_set_denoiselevel(wtk_mask_bf_net_t *mask_bf_net,
                                      int level) {
    int i;
    int n_ans_level = mask_bf_net->cfg->n_ans_level;
    float *qmmse2_noise_suppress = mask_bf_net->cfg->qmmse2_noise_suppress;
    level = min(level, n_ans_level);
    if (mask_bf_net->qmmse2) {
        for (i = 0; i < n_ans_level; ++i) {
            if (level == i + 1) {
                mask_bf_net->qmmse2->cfg->noise_suppress =
                    qmmse2_noise_suppress[i];
                break;
            }
        }
    }
}
void wtk_mask_bf_net_set_denoisesuppress(wtk_mask_bf_net_t *mask_bf_net,
                                         float suppress) {
    if (mask_bf_net->qmmse2) {
        mask_bf_net->qmmse2->cfg->noise_suppress = suppress;
    }
}
