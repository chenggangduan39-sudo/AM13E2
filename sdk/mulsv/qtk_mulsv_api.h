#ifndef __SDK_MULSV_QTK_MULSV_API_H__
#define __SDK_MULSV_QTK_MULSV_API_H__
#ifdef __cplusplus
extern "C" {
#endif

enum qtk_mulsv_api_data_type {
    QTK_MULSV_API_DATA_TYPE_PCM = 0,
    QTK_MULSV_API_DATA_TYPE_ENROLL_START = 1,
    QTK_MULSV_API_DATA_TYPE_ENROLL_END = 2,
    QTK_MULSV_API_DATA_TYPE_VPRINT_THRESH = 3,
    QTK_MULSV_API_DATA_TYPE_NOTIFY_BIAS = 4
};

typedef void (*qtk_mulsv_api_notify_f)(void *ths,float conf,char *data,int len);

typedef struct qtk_mulsv_api qtk_mulsv_api_t;

struct qtk_mulsv_api{
    void *handle;
};

qtk_mulsv_api_t *qtk_mulsv_api_new(char *cfg_fn, int use_bin);
void qtk_mulsv_api_delete(qtk_mulsv_api_t *m, int use_bin);
void qtk_mulsv_api_start(qtk_mulsv_api_t *m);
void qtk_mulsv_api_reset(qtk_mulsv_api_t *m);
int qtk_mulsv_api_feed(qtk_mulsv_api_t *m,char *data,int len,int data_type,int is_end);
void qtk_mulsv_api_set_notify(qtk_mulsv_api_t *m,void *ths,qtk_mulsv_api_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
