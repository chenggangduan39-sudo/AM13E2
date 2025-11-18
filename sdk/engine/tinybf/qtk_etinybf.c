#include "qtk_etinybf.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/tinybf/qtk_etinybf_cfg.h"
#include "sdk/qtk_api.h"
#include "wtk/bfio/qform/wtk_qform9.h"
#include "wtk/bfio/qform/wtk_qform9_cfg.h"

static void qtk_etinybf_init(qtk_etinybf_t *bf) {
    bf->notify = NULL;
    bf->qform9 = NULL;
    bf->session = NULL;
    bf->ths = NULL;
    bf->cfg = NULL;
    bf->input = NULL;
    bf->input_cap = 0;
    bf->cur_channel = 0;
    bf->vboxebf = NULL;
    qtk_engine_param_init(&bf->param);
}

static void qtk_etinybf_clean(qtk_etinybf_t *bf) {
    if (bf->qform9) {
        wtk_qform9_delete(bf->qform9);
    } else if (bf->vboxebf) {
        wtk_gainnet_bf_stereo_delete(bf->vboxebf);
    }
    if (bf->input) {
        int i;
	for (i = 0; i < bf->cfg->input_channel; i++) {
            wtk_free(bf->input[i]);
        }
        wtk_free(bf->input);
    }
    if (bf->param.use_bin) {
        qtk_etinybf_cfg_delete_bin(bf->cfg);
    } else {
        qtk_etinybf_cfg_delete(bf->cfg);
    }
    if (bf->buf) {
        wtk_strbuf_delete(bf->buf);
    }
}

qtk_etinybf_t *qtk_etinybf_new(qtk_session_t *session,
                               wtk_local_cfg_t *params) {
    qtk_etinybf_t *bf;
    int ret;

    bf = cast(qtk_etinybf_t *, wtk_malloc(sizeof(qtk_etinybf_t)));
    qtk_etinybf_init(bf);
    bf->session = session;
    qtk_engine_param_set_session(&bf->param, session);
    ret = qtk_engine_param_feed(&bf->param, params);
    if (ret != 0) {
        goto err;
    }
    if (bf->param.use_bin) {
        bf->cfg = qtk_etinybf_cfg_new_bin(bf->param.cfg);
    } else {
        bf->cfg = qtk_etinybf_cfg_new(bf->param.cfg);
    }

    if (bf->cfg->use_qform9) {
        bf->qform9 = wtk_qform9_new(&bf->cfg->qform9);
    } else if (bf->cfg->use_vboxebf) {
        bf->vboxebf = wtk_gainnet_bf_stereo_new(&bf->cfg->vboxebf);
    }
    bf->input = wtk_malloc(sizeof(short *) * bf->cfg->input_channel);
    bf->input_cap = 320;
    int i;
    for (i = 0; i < bf->cfg->input_channel; i++) {
        bf->input[i] = wtk_malloc(sizeof(short) * bf->input_cap);
    }
    bf->buf = wtk_strbuf_new(1024, 1);

    return bf;
err:
    qtk_etinybf_clean(bf);
    wtk_free(bf);
    return NULL;
}

int qtk_etinybf_delete(qtk_etinybf_t *bf) {
    qtk_etinybf_clean(bf);
    wtk_free(bf);
    return 0;
}

int qtk_etinybf_start(qtk_etinybf_t *bf) {
    if (bf->cfg->use_qform9) {
        wtk_qform9_start(bf->qform9, bf->cfg->theta, bf->cfg->phi);
    } else if (bf->cfg->use_vboxebf) {
        wtk_gainnet_bf_stereo_start(bf->vboxebf);
    }
    return 0;
}

int qtk_etinybf_reset(qtk_etinybf_t *bf) {
    bf->cur_channel = 0;
    if (bf->cfg->use_qform9) {
        wtk_qform9_reset(bf->qform9);
    } else if (bf->cfg->use_vboxebf) {
        wtk_gainnet_bf_stereo_reset(bf->vboxebf);
    }
    wtk_strbuf_reset(bf->buf);
    return 0;
}

static int process_input_(qtk_etinybf_t *bf, int input_pos, int is_end) {
    if (bf->cfg->use_qform9) {
        wtk_qform9_feed(bf->qform9, bf->input, input_pos, is_end);
    }
    return 0;
}

int qtk_etinybf_feed(qtk_etinybf_t *bf, char *data, int len, int is_end) {
    short *audio;
    int audio_len;
    int input_pos = 0;

    if (len % 2) {
        goto err;
    }
    audio = cast(short *, data);
    audio_len = len / 2;
    if (bf->cfg->use_qform9) {
        while (audio_len > 0) {
            bf->input[bf->cur_channel++][input_pos] = *audio++;
            audio_len--;
            if (bf->cur_channel == bf->cfg->input_channel) {
                input_pos++;
                bf->cur_channel = 0;
                if (audio_len < bf->cfg->input_channel ||
                    input_pos == bf->input_cap) {
                    process_input_(bf, input_pos, 0);
                    input_pos = 0;
                }
            }
        }
        if (is_end) {
            if (bf->cur_channel) {
                goto err;
            }
            if (bf->qform9)
                process_input_(bf, input_pos, 1);
        }
    } else if (bf->cfg->use_vboxebf) {
        wtk_gainnet_bf_stereo_feed(bf->vboxebf, audio,
                                audio_len / bf->cfg->input_channel, is_end);
    }

    return 0;
err:
    return -1;
}

static void on_qform9_(qtk_etinybf_t *bf, short *data, int len, int is_end) {
    qtk_var_t var;

    if (len > 0) {
        var.type = QTK_SPEECH_DATA_PCM;
        var.v.str.data = cast(char *, data);
        var.v.str.len = len * 2;
        bf->notify(bf->ths, &var);
    }

    if (is_end) {
        var.type = QTK_SPEECH_END;
        bf->notify(bf->ths, &var);
    }
}

static void on_vbox_ebf_(qtk_etinybf_t *bf, short **data, int len) {
    qtk_var_t var;
    int i;
    if (len > 0) {
        wtk_strbuf_reset(bf->buf);
        for (i = 0; i < len; i++) {
            wtk_strbuf_push(bf->buf, cast(char *, &data[0][i]), sizeof(short));
            wtk_strbuf_push(bf->buf, cast(char *, &data[1][i]), sizeof(short));
        }
        var.type = QTK_SPEECH_DATA_PCM;
        var.v.str.data = bf->buf->data;
        var.v.str.len = bf->buf->pos;
        bf->notify(bf->ths, &var);
    }
}

int qtk_etinybf_set_notify(qtk_etinybf_t *bf, void *ths,
                           qtk_engine_notify_f notify) {
    bf->notify = notify;
    bf->ths = ths;
    if (bf->cfg->use_qform9) {
        wtk_qform9_set_notify(bf->qform9, bf,
                              cast(wtk_qform9_notify_f, on_qform9_));
    } else if (bf->cfg->use_vboxebf) {
        //有问题
        wtk_gainnet_bf_stereo_set_notify(
            bf->vboxebf, bf, (wtk_gainnet_bf_stereo_notify_f)on_vbox_ebf_);
    }

    return 0;
}
