#ifndef QTK_KWDEC2_H_
#define QTK_KWDEC2_H_
#include "qtk_kwdec.h"

#define KW_MIN_NUM 4
#define KW_MAX_NUM 13

//#define KW_ID 1
#define EPS_ID 0
#define DEFAULT_ID -1
extern int qtk_wdec_post_feed(qtk_kwdec_t* dec, wtk_vector_t* f);

typedef struct 
{
	int in_id;
	int out_id;
	int feat_idx;
}label_map_t;
/*
typedef struct
{	
	int *phone;
	float *pdf;
}pdf_map_t;
*/

#endif
