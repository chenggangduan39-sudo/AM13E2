#include "wtk_macro.h"

int wtk_macro_cmp(wtk_macro_t* m1,wtk_macro_t *m2)
{
	int ret;

	//wtk_macro_print(m1);
	//wtk_macro_print(m2);
	ret=m1->type-m2->type;
	if(ret!=0){return ret;}
	ret=m1->name->len - m2->name->len;
	if(ret!=0){return ret;}
	return strncmp(m1->name->data,m2->name->data,m1->name->len);
}

void wtk_macro_print(wtk_macro_t* m)
{
	//print_data(m->name->data,m->name->len);
	printf("%*.*s(type=%c)\n",m->name->len,m->name->len,m->name->data,m->type);
}
