#include "qtk_eaec.h"
#include "sdk/engine/aec/qtk_eaec_cfg.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/qtk_api.h"
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/bfio/aec/wtk_aec_cfg.h"
#include "wtk/core/wtk_strbuf.h"

static void qtk_eaec_init(qtk_eaec_t *bf) {
    bf->notify = NULL;
    bf->aec = NULL;
    bf->session = NULL;
    bf->ths = NULL;
    bf->cfg = NULL;
    bf->input = NULL;
    bf->input_cap = 0;
    bf->cur_channel = 0;
    bf->audio = NULL;
    qtk_engine_param_init(&bf->param);
}

static void qtk_eaec_clean(qtk_eaec_t *bf) {
    if (bf->aec) {
        wtk_aec_delete(bf->aec);
    }
    if (bf->input) {
        int i;
	for (i = 0; i < bf->cfg->input_channel; i++) {
            wtk_free(bf->input[i]);
        }
        wtk_free(bf->input);
    }
    if (bf->audio) {
        wtk_strbuf_delete(bf->audio);
    }
    if (bf->param.use_bin) {
        qtk_eaec_cfg_delete_bin(bf->cfg);
    } else {
        qtk_eaec_cfg_delete(bf->cfg);
    }
}

qtk_eaec_t *qtk_eaec_new(qtk_session_t *session, wtk_local_cfg_t *params) {
    qtk_eaec_t *bf;
    int ret;

    bf = cast(qtk_eaec_t *, wtk_malloc(sizeof(qtk_eaec_t)));
    qtk_eaec_init(bf);
    bf->session = session;
    qtk_engine_param_set_session(&bf->param, session);
    ret = qtk_engine_param_feed(&bf->param, params);
    if (ret != 0) {
        goto err;
    }
    if (bf->param.use_bin) {
        bf->cfg = qtk_eaec_cfg_new_bin(bf->param.cfg);
    } else {
        bf->cfg = qtk_eaec_cfg_new(bf->param.cfg);
    }

    bf->aec = wtk_aec_new(&bf->cfg->aec);
    bf->input = wtk_malloc(sizeof(short *) * bf->cfg->input_channel);
    bf->input_cap = 320;
    int i;
    for (i = 0; i < bf->cfg->input_channel; i++) {
        bf->input[i] = wtk_malloc(sizeof(short) * bf->input_cap);
    }
    bf->audio = wtk_strbuf_new(1024, 1);

    return bf;
err:
    qtk_eaec_clean(bf);
    wtk_free(bf);
    return NULL;
}

int qtk_eaec_delete(qtk_eaec_t *bf) {
    qtk_eaec_clean(bf);
    wtk_free(bf);
    return 0;
}

int qtk_eaec_start(qtk_eaec_t *bf) { return 0; }

int qtk_eaec_reset(qtk_eaec_t *bf) {
    bf->cur_channel = 0;

    wtk_aec_reset(bf->aec);

    return 0;
}

static int process_input_(qtk_eaec_t *bf, int input_pos, int is_end) {
    wtk_aec_feed(bf->aec, bf->input, input_pos, is_end);
    return 0;
}

int qtk_eaec_feed(qtk_eaec_t *bf, char *data, int len, int is_end) {
    short *audio;
    int audio_len;
    int input_pos = 0;

    if (len % 2) {
        goto err;
    }

    audio = cast(short *, data);
    audio_len = len / 2;

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
        if (bf->aec) {
            process_input_(bf, input_pos, 1);
        }
    }
    return 0;
err:
    return -1;
}

static void on_aec_(qtk_eaec_t *bf, short **data, int len, int is_end) {
    qtk_var_t var;

    if (len > 0) {
        wtk_strbuf_reset(bf->audio);
        int i,j;
	for (i = 0; i < len; i++) {
            for (j = 0; j < bf->cfg->input_channel - bf->cfg->sp_channel; j++) {
                wtk_strbuf_push(bf->audio, cast(char *, data[j] + i),
                                sizeof(short));
            }
        }
        var.type = QTK_SPEECH_DATA_PCM;
        var.v.str.data = cast(char *, bf->audio->data);
        var.v.str.len = bf->audio->pos;
        bf->notify(bf->ths, &var);
    }

    if (is_end) {
        var.type = QTK_SPEECH_END;
        bf->notify(bf->ths, &var);
    }
}

int qtk_eaec_set_notify(qtk_eaec_t *bf, void *ths, qtk_engine_notify_f notify) {
    bf->notify = notify;
    bf->ths = ths;

    wtk_aec_set_notify(bf->aec, bf, cast(wtk_aec_notify_f, on_aec_));

    return 0;
}
