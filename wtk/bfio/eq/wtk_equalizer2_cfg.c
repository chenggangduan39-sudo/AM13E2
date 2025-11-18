#include "wtk_equalizer2_cfg.h"

static float _i0f(float x) {
    float af = fabsf(x);
    float cf, bf;
    if (af < 3.75) {
        cf = x / 3.75;
        cf = cf * cf;
        bf = 1.0 + cf * (3.515623 +
                         cf * (3.089943 +
                               cf * (1.20675 +
                                     cf * (0.265973 + cf * (0.0360768 +
                                                            cf * 0.0045813)))));

    } else {
        cf = 3.75 / af;
        bf = (exp(af) / sqrt(af)) *
             (0.3989423 +
              cf * (0.0132859 +
                    cf * (0.0022532 +
                          cf * (-0.0015756 +
                                cf * (0.0091628 +
                                      cf * (-0.0205771 +
                                            cf * (0.0263554 +
                                                  cf * (-0.0164763 +
                                                        cf * 0.0039238))))))));
    }
    return bf;
}

void wtk_equlizer2_cfg_filter_init(wtk_equalizer2_cfg_t *cfg) {
    if (cfg->band_count < 2 || cfg->band_count > 50) {
        wtk_debug("error:equalizer2 band_count error band_count:%d\n",
                  cfg->band_count);
        return;
    }
    if (cfg->nfir < 4 || cfg->nfir > 250 || cfg->nfir % 2 == 0) {
        wtk_debug("error:equalizer2 nfir error nfir:%d\n", cfg->nfir);
        return;
    }
    if (cfg->kdb <= 0) {
        wtk_debug("error:equalizer2 kdb error kdb:%f\n", cfg->kdb);
        return;
    }
    int HalfFIR_idx = (cfg->nfir - 1) / 2;
    int i, j;

    cfg->cf = (float *)wtk_malloc(sizeof(float) * cfg->nfir);
    memset(cfg->cf, 0, sizeof(float) * cfg->nfir);
    double *fNorm = (double *)wtk_malloc(sizeof(double) * cfg->band_count);
    memset(fNorm, 0, sizeof(float) * cfg->band_count);
    float *aVolts = (float *)wtk_malloc(sizeof(float) * cfg->band_count);
    memset(aVolts, 0, sizeof(float) * cfg->band_count);

    // 转换dB到电压比，频率到采样率的归一化值
    for (i = 0; i < cfg->band_count; ++i) {
        fNorm[i] = cfg->rate[i] / cfg->sfreq;
        aVolts[i] = powf(10.0f, cfg->value[i] / 20.0f);
    }

    // 计算中心系数(n=HalfFIR_idx时候，sinc函数的极限值为 2*aVolts * fNorm)
    cfg->cf[HalfFIR_idx] = 2.0f * aVolts[0] * fNorm[0];
    for (i = 1; i < cfg->band_count; ++i) {
        cfg->cf[HalfFIR_idx] += 2.0 * aVolts[i] * (fNorm[i] - fNorm[i - 1]);
    }

    // 计算其他系数(sinc函数的其他点)
    for (i = 1; i < HalfFIR_idx + 1; ++i) {
        double q = M_PI * i;

        // 处理低频段(低通)
        cfg->cf[i + HalfFIR_idx] = aVolts[0] * sin(fNorm[0] * 2.0 * q) / q;

        // 处理其他频段(带通)
        for (j = 1; j < cfg->band_count; ++j) {
            float term1 = sin(fNorm[j] * 2.0 * q) / q;
            float term2 = sin(fNorm[j - 1] * 2.0 * q) / q;
            cfg->cf[i + HalfFIR_idx] += aVolts[j] * (term1 - term2);
        }
    }

    // 计算凯泽窗参数beta
    float beta = 0.0f;
    if (cfg->kdb > 50.0f) {
        beta = 0.1102 * (cfg->kdb - 8.7);
    } else if (cfg->kdb > 20.96) {
        beta = 0.5842 * powf((cfg->kdb - 20.96), 0.4) +
               0.07886 * (cfg->kdb - 20.96);
    }

    float kbes = beta == 0.0f ? 1.0f : 1.0f / _i0f(beta);

    // 应用凯泽窗
    float scaleXj2 = powf(2.0 / (cfg->nfir - 1), 2.0);
    for (i = 0; i < HalfFIR_idx + 1; ++i) {
        float xj2 = scaleXj2 * powf(i + 0.5f, 2.0);
        float WindowWt = 0.0f;
        if (xj2 <= 1.0f) {
            WindowWt = kbes * _i0f(beta * sqrtf(1.0 - xj2));
        }
        cfg->cf[HalfFIR_idx + i] *= WindowWt;
        if (HalfFIR_idx - i >= 0) {
            cfg->cf[HalfFIR_idx - i] = cfg->cf[HalfFIR_idx + i];
        }
    }

    wtk_free(fNorm);
    wtk_free(aVolts);
}

int wtk_equalizer2_cfg_init(wtk_equalizer2_cfg_t *cfg) {
    cfg->sfreq = 16000.0f;
    cfg->value = NULL;
    cfg->rate = NULL;
    cfg->band_count = 0;

    cfg->nfir = 101;
    cfg->kdb = 30.0f;
    cfg->cf = NULL;

    cfg->use_pffft = 1;
    return 0;
}

int wtk_equalizer2_cfg_clean(wtk_equalizer2_cfg_t *cfg) {
    if (cfg->value) {
        wtk_free(cfg->value);
    }
    if (cfg->rate) {
        wtk_free(cfg->rate);
    }
    if (cfg->cf) {
        wtk_free(cfg->cf);
    }

    return 0;
}

int wtk_equalizer2_cfg_update_local(wtk_equalizer2_cfg_t *cfg,
                                    wtk_local_cfg_t *lc) {
    int ret = 0, i = 0;
    wtk_string_t *v = NULL;
    wtk_array_t *array = NULL;

    wtk_local_cfg_update_cfg_f(lc, cfg, sfreq, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_pffft, v);

    array = wtk_local_cfg_find_array_s(lc, "rate");
    if (array) {
        cfg->band_count = array->nslot;
        cfg->rate = wtk_malloc(sizeof(float) * cfg->band_count);
        for (i = 0; i < cfg->band_count; ++i) {
            v = ((wtk_string_t **)array->slot)[i];
            cfg->rate[i] = wtk_str_atof(v->data, v->len);
        }
    }
    array = wtk_local_cfg_find_array_s(lc, "value");
    if (array == NULL) {
        wtk_debug("error:equalizer2 value don't have\n");
        ret = -1;
        goto end;
    }
    if (cfg->band_count != array->nslot) {
        wtk_debug("error:equalizer2 value don't have\n");
        ret = -1;
        goto end;
    }
    cfg->value = wtk_malloc(sizeof(float) * cfg->band_count);
    for (i = 0; i < cfg->band_count; ++i) {
        v = ((wtk_string_t **)array->slot)[i];
        cfg->value[i] = wtk_str_atof(v->data, v->len);
    }
end:
    return ret;
}

int wtk_equalizer2_cfg_update(wtk_equalizer2_cfg_t *cfg) {
    int ret = 0;
    wtk_equlizer2_cfg_filter_init(cfg);
    return ret;
}

int wtk_equalizer2_cfg_update2(wtk_equalizer2_cfg_t *cfg,
                               wtk_source_loader_t *sl) {
    return wtk_equalizer2_cfg_update(cfg);
}
wtk_equalizer2_cfg_t *wtk_equalizer2_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_equalizer2_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_equalizer2_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_equalizer2_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_equalizer2_cfg_delete(wtk_equalizer2_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_equalizer2_cfg_t *wtk_equalizer2_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_equalizer2_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_equalizer2_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_equalizer2_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_equalizer2_cfg_delete_bin(wtk_equalizer2_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
