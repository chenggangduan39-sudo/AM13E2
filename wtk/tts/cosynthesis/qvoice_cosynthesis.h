#ifndef __PUBLIC_INC_QVOICE_CO_SYN_H__
#define __PUBLIC_INC_QVOICE_CO_SYN_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int state;        //synthesis success(1) or failed(0).
	float loss;
	char* log;
}cosynthesis_info_t;

typedef void (*qvoice_cosynthesis_notify)(void *upval, short *d, int l, short *uid,int lid);

void *qvoice_cosynthesis_new(char*cfn);
void *qvoice_cosynthesis_newbin(char*cfn);
void qvoice_cosynthesis_delete(void *c);
void qvoice_cosynthesis_reset(void *c);
int qvoice_cosynthesis_process(void *c, const char *s, const char *sep);
void qvoice_cosynthesis_set_notify(void *c, qvoice_cosynthesis_notify notify, void *upval);
cosynthesis_info_t* qvoice_cosynthesis_getOutput(void *c);

#ifdef __cplusplus
};
#endif
#endif
