#include "qtk_mulsv_api.h"
#include "qtk_mulsv.h"

qtk_mulsv_api_t *qtk_mulsv_api_new(char *cfg_fn, int use_bin) {
    qtk_mulsv_api_t *m;
    qtk_mulsv_cfg_t *cfg;
    qtk_mulsv_t *handle;

    m=wtk_malloc(sizeof(*m));
    if (use_bin) {
        cfg = qtk_mulsv_cfg_new_bin(cfg_fn);
    } else {
        cfg = qtk_mulsv_cfg_new(cfg_fn);
    }
    handle=qtk_mulsv_new(cfg);
    m->handle=(void*)handle;

    return m;
}

void qtk_mulsv_api_delete(qtk_mulsv_api_t *m, int use_bin) {
    qtk_mulsv_cfg_t *cfg;
    qtk_mulsv_t *handle=(qtk_mulsv_t*)(m->handle);
    cfg=handle->cfg;

    qtk_mulsv_delete(m->handle);
    if (use_bin) {
        qtk_mulsv_cfg_delete_bin(cfg);
    } else {
        qtk_mulsv_cfg_delete(cfg);
    }
    wtk_free(m);
}

void qtk_mulsv_api_start(qtk_mulsv_api_t *m)
{
    qtk_mulsv_start(m->handle);
}

void qtk_mulsv_api_reset(qtk_mulsv_api_t *m)
{
    qtk_mulsv_reset(m->handle);
}

int qtk_mulsv_api_feed(qtk_mulsv_api_t *m, char *data, int len, int data_type,
                       int is_end) {
    switch(data_type)
    {
    case QTK_MULSV_API_DATA_TYPE_PCM:
        qtk_mulsv_feed(m->handle,data,len,is_end);
        break;
    case QTK_MULSV_API_DATA_TYPE_ENROLL_START:
        qtk_mulsv_feed2(m->handle,data,len,QTK_MULSV_MSG_ENROLL_START);
        break;
    case QTK_MULSV_API_DATA_TYPE_ENROLL_END:
        qtk_mulsv_feed2(m->handle,data,len,QTK_MULSV_MSG_ENROLL_END);
        break;
    case QTK_MULSV_API_DATA_TYPE_VPRINT_THRESH:
        qtk_mulsv_feed2(m->handle,data,len,QTK_MULSV_MSG_VPRINT_THRESH);
        break;
    case QTK_MULSV_API_DATA_TYPE_NOTIFY_BIAS:
        qtk_mulsv_feed2(m->handle, data, len, QTK_MULSV_MSG_NOTIFY_BIAS);
    default:
        break;
    }

    return 0;
}

void qtk_mulsv_api_set_notify(qtk_mulsv_api_t *m,void *ths,qtk_mulsv_api_notify_f notify)
{
    qtk_mulsv_set_notify(m->handle,ths,(qtk_mulsv_notify_f)notify);
}
