#ifndef WTK_KWDEC2_POST_H_
#define WTK_KWDEC2_POST_H_
#include "wtk_kwdec2.h"

#define KW_MIN_NUM 2
#define KW_MAX_NUM 13

//#define KW_ID 1
#define EPS_ID 0
#define DEFAULT_ID -1
extern int wtk_kwdec2_post_feed(wtk_kwdec2_t* dec, wtk_vector_t* f);

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
