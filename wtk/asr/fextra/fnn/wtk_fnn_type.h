#ifndef WTK_VITE_PARM_POST_DNN_WTK_FNN_TYPE_H_
#define WTK_VITE_PARM_POST_DNN_WTK_FNN_TYPE_H_
#include "wtk/core/wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum
{
	wtk_fnn_sigmoid=0,
	wtk_fnn_softmax,
	wtk_fnn_linear,
	wtk_fnn_relu,
	wtk_fnn_pnorm,
	wtk_fnn_rescale,
	//wtk_fnn_normalize,
	wtk_fnn_sigmoid_normal,
}wtk_fnn_post_type_t;

char* wtk_fnn_post_type_str(wtk_fnn_post_type_t type);
#ifdef __cplusplus
};
#endif
#endif
