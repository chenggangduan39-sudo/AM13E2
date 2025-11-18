#ifndef WTK_VITE_VPRINT_WTK_VPRINT
#define WTK_VITE_VPRINT_WTK_VPRINT
#include "wtk/core/wtk_type.h"
#include "wtk/asr/vprint/parm/wtk_vparm.h"
#include "wtk/asr/vprint/detect/wtk_vdetect.h"
#include "wtk_vprint_cfg.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/json/wtk_json.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vprint wtk_vprint_t;

struct wtk_vprint
{
	wtk_vprint_cfg_t *cfg;
	wtk_vparm_t *parm;
	wtk_vtrain_t *train;
	wtk_vdetect_t *detect;
	wtk_strbuf_t *buf;
	wtk_json_t *json;
	wtk_strbuf_t *last_usr;
	unsigned ask:1;
};

wtk_vprint_t* wtk_vprint_new(wtk_vprint_cfg_t *cfg);
void wtk_vprint_delete(wtk_vprint_t *v);

void wtk_vprint_train_start(wtk_vprint_t *v);
void wtk_vprint_train_reset(wtk_vprint_t *v);
void wtk_vprint_train_feed(wtk_vprint_t *v,char *data,int bytes,int is_end);
void wtk_vprint_train_update_acc(wtk_vprint_t *v);
void wtk_vprint_train_reset_acc(wtk_vprint_t *v);
void wtk_vprint_train_save(wtk_vprint_t *v,char *name,int len);
void wtk_vprint_train_del(wtk_vprint_t *v,char *name,int len);

void wtk_vprint_detect_start(wtk_vprint_t *v);
void wtk_vprint_detect_reset(wtk_vprint_t *v);
void wtk_vprint_detect_feed(wtk_vprint_t *v,char *data,int bytes,int is_end);
void wtk_vprint_detect_print(wtk_vprint_t *v);

void wtk_vprint_start(wtk_vprint_t *v);
void wtk_vprint_reset(wtk_vprint_t *v);
void wtk_vprint_feed(wtk_vprint_t *v,char *data,int bytes,int is_end);
int wtk_vprint_touch(wtk_vprint_t *v);
void wtk_vprint_save(wtk_vprint_t *v,char *name,int len);

void wtk_vprint_print(wtk_vprint_t *v);
void wtk_vprint_get_result(wtk_vprint_t *v,wtk_string_t *result);
void wtk_vprint_get_usr(wtk_vprint_t *v,wtk_string_t *result);
#ifdef __cplusplus
};
#endif
#endif
