#include "wtk_fnn_type.h"

char* wtk_fnn_post_type_str(wtk_fnn_post_type_t type)
{
	static char *ts[]={
			"wtk_dnn_sigmoid",
			"wtk_dnn_softmax",
			"wtk_dnn_linear",
	};
	return ts[type];
}
