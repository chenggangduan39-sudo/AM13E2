#include "qtk_wakeup.h"

void qtk_wakeup_on_data(qtk_wakeup_t *wakeup, wtk_kvadwake_cmd_t cmd, float fs, float fe, short *data, int len);
void qtk_wakeup_on_img(qtk_wakeup_t *wakeup, int res, int start, int end);

void qtk_wakeup_init(qtk_wakeup_t *wakeup) {
	wakeup->cfg = NULL;
	wakeup->session = NULL;
	wakeup->img = NULL;
    wakeup->notify = NULL;
    wakeup->wakethis = NULL;
}

qtk_wakeup_t *qtk_wakeup_new(qtk_wakeup_cfg_t *cfg, qtk_session_t *session) {
    qtk_wakeup_t *wakeup;

    wakeup = (qtk_wakeup_t *)wtk_malloc(sizeof(qtk_wakeup_t));
    qtk_wakeup_init(wakeup);

    wakeup->cfg = cfg;
    wakeup->session = session;

    if(wakeup->cfg->use_img){
        wakeup->img = qtk_img_rec_new(cfg->img);
        qtk_img_thresh_cfg_t normal_thresh;
        qtk_img_thresh_cfg_t normal_echo_thresh;

        normal_thresh.av_prob0 = 0.5;
        normal_thresh.avx0 = 0.5;
        normal_thresh.maxx0 = 0.5;
        normal_thresh.max_prob0 = 0.5;
        normal_thresh.av_prob1 = 0.5;
        normal_thresh.avx1 = 0.5;
        normal_thresh.maxx1 = 0.5;
        normal_thresh.max_prob1 = 0.5;
        normal_thresh.speech_dur = 0;

        normal_echo_thresh.av_prob0 = 0.5;
        normal_echo_thresh.avx0 = 0.5;
        normal_echo_thresh.maxx0 = 0.5;
        normal_echo_thresh.max_prob0 = 0.5;
        normal_echo_thresh.av_prob1 = 0.5;
        normal_echo_thresh.avx1 = 0.5;
        normal_echo_thresh.maxx1 = 0.5;
        normal_echo_thresh.max_prob1 = 0.5;
        normal_echo_thresh.speech_dur = 0;

        qtk_img_thresh_set_cfg(wakeup->img, normal_thresh, 0);
        // qtk_img_thresh_set_cfg(wakeup->img, normal_echo_thresh, 1);
        qtk_img_rec_set_notify(wakeup->img,(qtk_img_rec_notify_f)qtk_wakeup_on_img, wakeup);
    }
    if(wakeup->cfg->use_kvadwake){
        wakeup->kvwake = wtk_kvadwake_new(wakeup->cfg->kvwake_cfg);
        wtk_kvadwake_set_notify(wakeup->kvwake, wakeup, (wtk_kvadwake_notify_f)qtk_wakeup_on_data);
        wtk_kvadwake_set_idx(wakeup->kvwake, 0);
    }
    return wakeup;
}

void qtk_wakeup_delete(qtk_wakeup_t *wakeup) {
    if(wakeup->cfg->use_img){
	    qtk_img_rec_delete(wakeup->img);
    }
    if(wakeup->cfg->use_kvadwake){
        wtk_kvadwake_delete(wakeup->kvwake);
    }
    wtk_free(wakeup);
}

int qtk_wakeup_start(qtk_wakeup_t *wakeup) {
    if(wakeup->cfg->use_kvadwake){
        wtk_kvadwake_start(wakeup->kvwake);
    }
    return 0;
}

int qtk_wakeup_reset(qtk_wakeup_t *wakeup) {
    if(wakeup->cfg->use_img){
	    qtk_img_rec_reset(wakeup->img);
    }
    if(wakeup->cfg->use_kvadwake){
        wtk_kvadwake_reset(wakeup->kvwake);
    }
    return 0;
}

int qtk_wakeup_feed(qtk_wakeup_t *wakeup, char *data, int bytes, int is_end) {
    int ret=0;

    if(wakeup->cfg->use_img){
        ret=qtk_img_rec_feed(wakeup->img, data, bytes, is_end);
    }
    if(wakeup->cfg->use_kvadwake){
        wtk_kvadwake_feed(wakeup->kvwake, (short *)data, bytes>>1, is_end);
    }
    return ret;
}

void qtk_wakeup_set_notify(qtk_wakeup_t *wakeup, void *notify_ths,
		qtk_wakeup_notify_f notify_f) {
    wakeup->notify = notify_f;
    wakeup->wakethis = notify_ths;
}

void qtk_wakeup_set_coreType(qtk_wakeup_t *wakeup, char *data, int len) {
//    wtk_log_log(wakeup->session->log, "set coreType: %.*s", len, data);
}
void qtk_wakeup_set_res(qtk_wakeup_t *wakeup, char *data, int len) {
//    wtk_log_log(wakeup->session->log, "set res: %.*s", len, data);
}

void qtk_wakeup_on_data(qtk_wakeup_t *wakeup, wtk_kvadwake_cmd_t cmd, float fs, float fe, short *data, int len)
{
    if(cmd==WTK_KVADWAKE_WAKE)
    {
        if(wakeup->notify){
            wakeup->notify(wakeup->wakethis, 0, 0, (int)(fs*32), (int)(fe*32));
        }
    }
}

void qtk_wakeup_on_img(qtk_wakeup_t *wakeup, int res, int start, int end)
{
    if(wakeup->notify){
        wakeup->notify(wakeup->wakethis, res, 0, start, end);
    }
}