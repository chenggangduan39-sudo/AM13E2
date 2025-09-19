
#ifndef QTK_API_CSRSC_QTK_CSRSC
#define QTK_API_CSRSC_QTK_CSRSC

#include "wtk/asr/vad/wtk_vad.h"

#include "qtk_csrsc_cfg.h"
#include "sdk/api_1/asr/qtk_asr.h"
#include "wtk/asr/kws/qtk_sond_cluster.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_csrsc qtk_csrsc_t;
typedef void (*qtk_csrsc_notify_f)(void *ths, qtk_var_t *var);

typedef struct
{
	wtk_queue_node_t qn;
	wtk_queue_node_t hoard_on;
	long count;
	double start_time;
    double end_time;
}qtk_csrsc_msg_node_t;

struct qtk_csrsc {
    qtk_csrsc_cfg_t *cfg;
    qtk_session_t *session;
    qtk_asr_t *asr;
    wtk_vad_t *vad;
    qtk_sond_cluster_t *sc;
    wtk_lockhoard_t msg_hoard;
    wtk_blockqueue_t timequeue;
    //wtk_queue_t vad_q;
    qtk_csrsc_notify_f notify;
    void *notify_ths;
    wtk_strbuf_t *tmpbuf;
    wtk_strbuf_t *vadbuf;
    double start_time;
    double end_time;
    double postime;
    unsigned sil : 1;
    unsigned valid : 1;
    unsigned cancel : 1;
};

qtk_csrsc_t *qtk_csrsc_new(qtk_csrsc_cfg_t *cfg, qtk_session_t *session);
void qtk_csrsc_delete(qtk_csrsc_t *c);
void qtk_csrsc_set_notify(qtk_csrsc_t *c, void *ths, qtk_csrsc_notify_f notify);
/**
 * 2.仅注册使用的接口,每注册一个人,这两个接口就都需要调用一次
*/
int qtk_csrsc_set_enroll(qtk_csrsc_t *qk, char *name, int len, int is_end);
/**
 * 3.注册的声纹文件相关接口
 * 清除注册好的声纹,不会删除文件,调用后 原来注册的所有声纹都不会生效
 */
void qtk_csrsc_clean(qtk_csrsc_t *qk);
/**
 * 注册声纹文件重新加载 重新读取并加载文件里的声纹
*/
void qtk_csrsc_reload(qtk_csrsc_t *qk);
/**
 * 设置声纹文件   vp.bin.idx  vp.bin.data  fn传入"vp.bin",接口里面会调用qtk_kws_reload,不需要在外面重新加载
*/
int qtk_csrsc_set_enroll_fn(qtk_csrsc_t *qk, char *fn, int len);
/**
 * 获取当前的声纹文件名
*/
char* qtk_csrsc_get_fn(qtk_csrsc_t *qk);
int qtk_csrsc_set_vad_time(qtk_csrsc_t *qk, float vs, float ve);
int qtk_csrsc_set_spk_nums(qtk_csrsc_t *qk, int num);

int qtk_csrsc_start(qtk_csrsc_t *c, int left, int right);
int qtk_csrsc_feed(qtk_csrsc_t *c, char *data, int bytes, int is_end);
void qtk_csrsc_reset(qtk_csrsc_t *c);
void qtk_csrsc_cancel(qtk_csrsc_t *c);

#ifdef __cplusplus
};
#endif
#endif
