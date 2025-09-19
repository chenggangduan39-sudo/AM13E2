/*
 * qtk_cv_frec_api.c
 *
 *  Created on: 2024年4月7日
 *      Author: root
 */
#include <float.h>
#include "qtk/cv/face_rec/qtk_cv_face_rec_embedding.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "qtk_cv_frec_api.h"
#define MAX_NUMBER_FREC 30
#define MAX_LEN_USERNAME 32
#define MAX_LEN_EMBEDDING_DIM 64
#ifdef USE_CNTLIMIT
#define MAX_COUNT_LIMIT 2*60*60*10
static long qtk_count=0;
#endif
/*frec hander*/
typedef struct qtk_cv_frec_api {
	qtk_cv_face_rec_embedding_t *dec;
	wtk_main_cfg_t *main_cfg;
	wtk_mbin_cfg_t *mbin_cfg;
	qtk_cv_face_rec_embedding_cfg_t *cfg;

	//save register info
	int nface;
	int embedding_dim;
	float *embedding;
	char *user_name;

	//rec result
	int *rst_ids;
	int rst_nid;
	qtk_cv_frec_api_box_t box[MAX_NUMBER_FREC];

	//target rec result
	int *trst_ids;
	int trst_nid;
	qtk_cv_frec_api_box_t tbox[MAX_NUMBER_FREC];

	float thresh;
} qtk_cv_frec_api_t;

void *qtk_cv_frec_api_new(char* cfn)
{
	qtk_cv_frec_api_t* h;
	int use_bin=1;

    h = wtk_calloc(1, sizeof(qtk_cv_frec_api_t));
    h->dec = NULL;

    if(use_bin)
    {
    	h->mbin_cfg = wtk_mbin_cfg_new_type(qtk_cv_face_rec_embedding_cfg, cfn,
                                         (char *)"./main.cfg");
    	h->dec = qtk_cv_face_rec_embedding_new(
            (qtk_cv_face_rec_embedding_cfg_t *)h->mbin_cfg->cfg);
    }else
    {
        h->main_cfg = wtk_main_cfg_new_type(qtk_cv_face_rec_embedding_cfg, cfn);
        if (h->main_cfg)
        {
            h->dec = qtk_cv_face_rec_embedding_new(
                (qtk_cv_face_rec_embedding_cfg_t *)h->main_cfg->cfg);
        }
    }

    if (h->dec)
    {
    	h->embedding = wtk_malloc(sizeof(float) * MAX_LEN_EMBEDDING_DIM * MAX_NUMBER_FREC);
    	h->rst_ids = wtk_malloc(sizeof(int) * MAX_NUMBER_FREC);
    	h->trst_ids = wtk_malloc(sizeof(int) * MAX_NUMBER_FREC);
    	h->user_name = wtk_malloc(MAX_LEN_USERNAME * MAX_NUMBER_FREC);
    }else{
		wtk_debug("[Error]: build engine failed\n");
                qtk_cv_frec_api_delete(h);
                h = NULL;
    }

	return (void*)h;
}

void qtk_cv_frec_api_delete(void *h)
{
	qtk_cv_frec_api_t* api;

	api = (qtk_cv_frec_api_t*)h;

	if(api->dec)
	{
		wtk_free(api->embedding);
		wtk_free(api->rst_ids);
		wtk_free(api->trst_ids);
		wtk_free(api->user_name);
		qtk_cv_face_rec_embedding_delete(api->dec);
	}
	if (api->main_cfg)
	{
		wtk_main_cfg_delete(api->main_cfg);
	}else if(api->mbin_cfg)
	{
		wtk_mbin_cfg_delete(api->mbin_cfg);
	}

	wtk_free(api);
}

void qtk_cv_frec_api_setThresh(void* h, float thresh)
{
	qtk_cv_frec_api_t* api;

	api = (qtk_cv_frec_api_t*)h;

	api->thresh = thresh;
}

int qtk_cv_frec_api_register(void *h, uint8_t *data, char* user, int user_size)
{
	qtk_cv_frec_api_t* api;
	qtk_cv_bbox_t *bbox;
    qtk_cv_face_rec_embedding_result_t *result;
    int nresult, i, ret;

    api = (qtk_cv_frec_api_t*)h;
    nresult = qtk_cv_face_rec_embedding_process(api->dec, data, &result);
    if (nresult != 1) {
        wtk_debug("[Error]: register face [%d] > 1, please protect one face\n", nresult);
        return 0;
    }
    bbox = &result[0].box;

    //draw box
    api->box[0].x1 = bbox->x1;
    api->box[0].y1 = bbox->y1;
    api->box[0].x2 = bbox->x2;
    api->box[0].y2 = bbox->y2;


    //update register result
    ret=-1;
	for(i=0; i < api->nface; i++)
	{
		ret=strncmp(api->user_name+i*MAX_LEN_USERNAME, user, user_size);
		if (ret==0)
			continue;
	}

	if (ret==0)
	{
		//update
        float *embedding = api->embedding + i*MAX_LEN_EMBEDDING_DIM;
        for (i = 0; i < api->embedding_dim; i++) {
            embedding[i] = (embedding[i] + result[0].embedding[i]) / 2;
        }
	}else
	{
		//new register
		if (api->nface <= MAX_NUMBER_FREC)
		{
			memset(api->embedding + api->nface * MAX_LEN_EMBEDDING_DIM, 0, MAX_LEN_EMBEDDING_DIM);
			memcpy(api->embedding + api->nface * MAX_LEN_EMBEDDING_DIM, result[0].embedding, result[0].embedding_dim);
			memset(api->user_name + api->nface * MAX_LEN_USERNAME, 0, MAX_LEN_USERNAME);
			if (user_size >= MAX_LEN_USERNAME)
				memcpy(api->user_name + api->nface * MAX_LEN_USERNAME, user, MAX_LEN_USERNAME-1);
			else
				memcpy(api->user_name + api->nface * MAX_LEN_USERNAME, user, user_size);

		    //draw box
		    api->tbox[0].x1 = bbox->x1;
		    api->tbox[0].y1 = bbox->y1;
		    api->tbox[0].x2 = bbox->x2;
		    api->tbox[0].y2 = bbox->y2;
		    memcpy(api->tbox[0].name, api->user_name + api->nface * MAX_LEN_USERNAME, MAX_LEN_USERNAME);
			api->nface++;
		}else
		{
			wtk_debug("[Error]: overcome support number of persons\n");
			nresult=0;
		}
	}

    return nresult;
}

static double calc_dist_(float *a, float *b, int len) {
    double res = 0;
    for (int i = 0; i < len; i++) {
        res += (a[i] - b[i]) * (a[i] - b[i]);
    }
    return res;
}

int qtk_cv_frec_api_rec(void *h, uint8_t *data)
{
	qtk_cv_frec_api_t* api;
	qtk_cv_bbox_t *bbox;
    qtk_cv_face_rec_embedding_result_t *result;
    int nresult, i,j, idx, tidx, pos;
    float min_dist;

#ifdef USE_CNTLIMIT
	if  (qtk_count++ > MAX_COUNT_LIMIT)
	{
		wtk_debug("[Error]: overcome maximum use times\n");
		return -1;
	}
#endif
    api = (qtk_cv_frec_api_t*)h;
    nresult = qtk_cv_face_rec_embedding_process(api->dec, data, &result);
    if (nresult != 1) {
        wtk_debug("register face > 1\n");
        exit(-1);
    }
    //printf("==============nresult:%d\n", nresult);
    for (i=0, idx=0, tidx=0, pos=-1; i < nresult; i++)
    {
    	min_dist = FLT_MAX;
    	//printf("==============api->nface:%d\n", api->nface);
    	for(j=0; j < api->nface; j++)
    	{
    		double dist =
    		            calc_dist_(api->embedding+j*api->embedding_dim, result[i].embedding, api->embedding_dim);
            if (dist < min_dist) {
                min_dist = dist;
                pos = j;
            }
    	}

		bbox = &result[i].box;

		//rec result
		api->rst_ids[idx] = pos;
	    api->box[idx].x1 = bbox->x1;
	    api->box[idx].y1 = bbox->y1;
	    api->box[idx].x2 = bbox->x2;
	    api->box[idx].y2 = bbox->y2;
	    idx++;
	    api->rst_nid = idx;
	    //target rec result
    	if (min_dist < api->thresh)
    	{
    		api->trst_ids[tidx] = pos;
    	    api->tbox[tidx].x1 = bbox->x1;
    	    api->tbox[tidx].y1 = bbox->y1;
    	    api->tbox[tidx].x2 = bbox->x2;
    	    api->tbox[tidx].y2 = bbox->y2;
    	    memcpy(api->tbox[tidx].name, api->user_name + pos * MAX_LEN_USERNAME, MAX_LEN_USERNAME);
    		tidx++;
    		api->trst_nid = tidx;
    	}
    }

    return nresult;
}

int qtk_cv_frec_api_rstValue(void* h, qtk_cv_frec_api_box_t **box, int target)
{
	qtk_cv_frec_api_t* api;

	api = (qtk_cv_frec_api_t*)h;

	if (target)
	{
		*box = api->tbox;
		return api->trst_nid;
	}else
	{
		*box = api->box;
		return api->rst_nid;
	}
}
