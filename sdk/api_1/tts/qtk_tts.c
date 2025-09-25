#include "qtk_tts.h"
void qtk_tts_init(qtk_tts_t *tts) {
    tts->cfg = NULL;
    tts->session = NULL;
    tts->ins.cldtts = NULL;
    tts->ins.tts = NULL;
}

qtk_tts_t *qtk_tts_new(qtk_tts_cfg_t *cfg, qtk_session_t *session) {
    qtk_tts_t *tts;

    tts = (qtk_tts_t *)wtk_malloc(sizeof(qtk_tts_t));
    qtk_tts_init(tts);

    tts->cfg = cfg;
    tts->session = session;

    wtk_log_log(tts->session->log, "use_cldtts = %d", cfg->use_cldtts);
    if (cfg->use_cldtts) {
        tts->ins.cldtts = qtk_cldtts_new(&(cfg->cldtts), session);
    } else {
        tts->ins.tts = wtk_tts_new(cfg->tts);
    }
    return tts;
}

void qtk_tts_delete(qtk_tts_t *tts) {
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_delete(tts->ins.cldtts);
    } else {
        wtk_tts_delete(tts->ins.tts);
    }
    wtk_free(tts);
}

int qtk_tts_start(qtk_tts_t *tts) {
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_start(tts->ins.cldtts);
    }
    return 0;
}

int qtk_tts_reset(qtk_tts_t *tts) {
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_reset(tts->ins.cldtts);
    } else {
        wtk_tts_reset(tts->ins.tts);
    }
    return 0;
}

int qtk_tts_feed(qtk_tts_t *tts, char *data, int bytes) {
    int ret;

    if (tts->cfg->use_cldtts) {
        ret = qtk_cldtts_process(tts->ins.cldtts, data, bytes);
    } else {
        ret = wtk_tts_process(tts->ins.tts, data, bytes);
    }
    return ret;
}

void qtk_tts_set_notify(qtk_tts_t *tts, void *notify_ths,
                        qtk_cldtts_notify_f notify_f) {
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_set_notify(tts->ins.cldtts, notify_ths, notify_f);
    } else {
        wtk_tts_set_notify(tts->ins.tts, notify_ths,
                           (wtk_tts_notify_f)notify_f);
    }
}

void qtk_tts_set_stop_hint(qtk_tts_t *tts) {
    wtk_log_log0(tts->session->log, "set stop hint");
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_set_stop_hint(tts->ins.cldtts);
    } else {
        wtk_tts_set_stop_hint(tts->ins.tts);
    }
}

void qtk_tts_set_speed(qtk_tts_t *tts, float speed) {
    wtk_log_log(tts->session->log, "set speed = %f", speed);
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_set_speed(tts->ins.cldtts, speed);
    } else {
        wtk_tts_set_speed(tts->ins.tts, speed);
    }
}

void qtk_tts_set_pitch(qtk_tts_t *tts, float pitch) {
    wtk_log_log(tts->session->log, "set pitch = %f", pitch);
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_set_pitch(tts->ins.cldtts, pitch);
    } else {
        wtk_tts_set_pitch(tts->ins.tts, pitch);
    }
}

void qtk_tts_set_volume(qtk_tts_t *tts, float volume) {
    wtk_log_log(tts->session->log, "set volume = %f", volume);
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_set_volume(tts->ins.cldtts, volume);
    } else {
        wtk_tts_set_volume_scale(tts->ins.tts, volume);
    }
}
void qtk_tts_set_coreType(qtk_tts_t *tts, char *data, int len) {
    wtk_log_log(tts->session->log, "set coreType: %.*s", len, data);
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_set_coreType(tts->ins.cldtts, data, len);
    }
}
void qtk_tts_set_res(qtk_tts_t *tts, char *data, int len) {
    wtk_log_log(tts->session->log, "set res: %.*s", len, data);
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_set_res(tts->ins.cldtts, data, len);
    }
}
void qtk_tts_set_useStream(qtk_tts_t *tts, int useStream) {
    wtk_log_log(tts->session->log, "set useStream: %d", useStream);
    if (tts->cfg->use_cldtts) {
        qtk_cldtts_set_useStream(tts->ins.cldtts, useStream);
    }
}
