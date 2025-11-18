#ifndef WTK_MODEL_WTK_MACRO_H_
#define WTK_MODEL_WTK_MACRO_H_
#include "wtk/core/wtk_str.h"
#include "wtk/core/wtk_str_hash.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_macro wtk_macro_t;
#define MACRO_TYPE_L 'l'
#define MACRO_TYPE_H 'h'

struct wtk_macro
{
	char type;			//macro type like 'l','h'
	wtk_string_t *name;	//this is shared with other.
	void *hook;
};

int wtk_macro_cmp(wtk_macro_t* m1,wtk_macro_t *m2);
void wtk_macro_print(wtk_macro_t* m);
#ifdef __cplusplus
};
#endif
#endif
