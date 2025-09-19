#ifndef WTK_ASR_WFST_WTK_WFSTDEC_OUTPUT
#define WTK_ASR_WFST_WTK_WFSTDEC_OUTPUT
#include "wtk/core/wtk_type.h" 
#include "wtk_wfstdec_output_cfg.h"
#include "wtk/core/json/wtk_json.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfstdec_output wtk_wfstdec_output_t;

struct wtk_wfstdec;

struct wtk_wfstdec_output
{
	wtk_wfstdec_output_cfg_t *cfg;
	struct wtk_wfstdec *dec;
	wtk_strbuf_t *hint;
	wtk_strbuf_t *result;
	wtk_json_t *json;
};


wtk_wfstdec_output_t* wtk_wfstdec_output_new(wtk_wfstdec_output_cfg_t *cfg,struct wtk_wfstdec *dec);
void wtk_wfstdec_output_delete(wtk_wfstdec_output_t *output);
void wtk_wfstdec_output_reset(wtk_wfstdec_output_t *output);
void wtk_wfstdec_output_start(wtk_wfstdec_output_t *output);
void wtk_wfstdec_output_set_result(wtk_wfstdec_output_t *output,char *result,int len);
void wtk_wfstdec_output_get_result(wtk_wfstdec_output_t *output,wtk_string_t *v);
void wtk_wfstdec_output_get_str_result(wtk_wfstdec_output_t *output,wtk_string_t *v);
#ifdef __cplusplus
};
#endif
#endif
