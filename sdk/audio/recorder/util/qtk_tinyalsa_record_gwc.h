#ifndef __QTK_TINYALSA_RECORD_GWC_H__
#define __QTK_TINYALSA_RECORD_GWC_H__
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_tinyalsa_record_gwc qtk_tinyalsa_record_gwc_t;

typedef void(*qtk_tinyalsa_record_gwc_notify_f)(void *ths,short **pv,int len);

typedef enum
{
	QTK_TINYALSA_RECORD_GWC_ALIGN,
	QTK_TINYALSA_RECORD_GWC_READY,
}qtk_tinyalsa_record_gwc_state_t;

struct qtk_tinyalsa_record_gwc
{
	int c;	//raw channel
	int oc; //output channel;
	int sample;
	int buf_size;
	void *ths;
	qtk_tinyalsa_record_gwc_notify_f notify;
	qtk_tinyalsa_record_gwc_state_t state;
	wtk_strbuf_t *buf;
	short **pv;
	unsigned first_flag:1;
};

qtk_tinyalsa_record_gwc_t* qtk_tinyalsa_record_gwc_new();
void qtk_tinyalsa_record_gwc_delete(qtk_tinyalsa_record_gwc_t *trg);
void qtk_tinyalsa_record_gwc_start(qtk_tinyalsa_record_gwc_t *trg);
void qtk_tinyalsa_record_gwc_reset(qtk_tinyalsa_record_gwc_t *trg);
void qtk_tinyalsa_record_gwc_set_notify(qtk_tinyalsa_record_gwc_t *trg,void *ths,qtk_tinyalsa_record_gwc_notify_f notify);
void qtk_tinyalsa_record_gwc_feed(qtk_tinyalsa_record_gwc_t *trg,char *data,int len);

#ifdef __cplusplus
};
#endif
#endif
