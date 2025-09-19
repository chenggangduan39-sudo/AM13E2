#include <stdio.h>
#include <ctype.h>

#include "wtk/core/wtk_strbuf.h"
#include "wtk_strsrc.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk_libsvm.h"

extern int wtk_source_gets(wtk_source_t *src, wtk_strbuf_t *sb);

wtk_libsvm_model_t* wtk_libsvm_model_new(wtk_source_t *source, int dim)
{
    int ret = -1;

    wtk_libsvm_model_t *model;

    if (!(model = wtk_svm_new(dim))) goto end;
    if ((ret = wtk_svm_load((wtk_svm_t *)model,source))) goto end;

end:
    if (ret && model) {wtk_libsvm_model_delete(model); model = NULL;}
    return model;
}

wtk_libsvm_model_t* wtk_libsvm_model_new2(const char *fn, int dim)
{
    wtk_libsvm_model_t *model;
    wtk_source_t source;

    if ((wtk_source_init_file(&source, (char*)fn))) return NULL;

    model = wtk_libsvm_model_new(&source, dim);
    wtk_source_clean_file(&source);

    return model;
}

wtk_libsvm_model_t* wtk_libsvm_model_new3(const char *data, int dim)
{
    wtk_string_t str;
    wtk_source_t source;
    wtk_strsrc_data_t strdata;

    wtk_string_set(&str, (char *)data, strlen(data));

    if ((wtk_strsrc_init(&source, &strdata, &str))) return NULL;

    return  wtk_libsvm_model_new(&source, dim);
}

wtk_libsvm_model_t* wtk_libsvm_model_new4(wtk_string_t *str, int dim, wtk_source_loader_t *sl)
{
    wtk_source_t source;
    wtk_strsrc_data_t strdata;
    wtk_rbin2_t	*rb;
    wtk_rbin2_item_t *item;
    int ret;
    if(sl)
    {
    	rb=(wtk_rbin2_t*)(sl->hook);
        item=wtk_rbin2_get(rb,str->data,str->len);
        if(!item){return NULL;}
       	if(!item->data)
       	{
       		ret=wtk_rbin2_load_item(rb,item,1);
       		if(ret!=0)
       		{
       			wtk_debug("[%.*s] load failed\n",item->fn->len,item->fn->data);
       			return NULL;
       		}
       	}
       	wtk_source_init(&source);
        if ((wtk_strsrc_init(&source, &strdata, item->data))) return NULL;
        return  wtk_libsvm_model_new(&source, dim);
    }else
    	return wtk_libsvm_model_new2(str->data, dim);
}

int wtk_libsvm_model_delete(wtk_libsvm_model_t *model)
{
    return wtk_svm_delete((wtk_svm_t *) model);
}

wtk_libsvm_norm_t*  wtk_libsvm_norm_new(wtk_source_t *src, int dim)
{
    int   i, ret;
    char *p; 

    wtk_strbuf_t *sb;
    wtk_libsvm_norm_t *norm = NULL;

    norm = wtk_malloc((dim + 1) * sizeof(wtk_libsvm_norm_t));

    sb = wtk_strbuf_new(256, 1.5f);
    wtk_source_gets(src, sb);

    i = 0; p = sb->data;
    if (p == strstr(p, "m:"))
    {
        /* hongjie.zhu's formatting */
        p = p + 2;
        for (;; i++)
        {
            while (*p && isspace(*p)) p++;

            if (!(*p) || i == dim) break;

            norm[i].index = i;
            norm[i].min = 0;
            norm[i].max = atof(p);

            while (*p && !isspace(*p)) p++;
        }
    }
    else
    {
        /* weida.zhou's formatting */
        for (;; i++)
        {
            while (*p && isspace(*p)) p++;

            if (!(*p) || i == dim) break;

            norm[i].index = i;

            if (!(p = strchr(p, ':'))) break;
            norm[i].min = atof(p+1);

            if (!(p = strchr(p, '_'))) break;
            norm[i].max = atof(p+1);

            while (*p && !isspace(*p)) p++;
        }
    }

    if ((!(*p) && i != dim) || (*p && i == dim)) {ret = -1; goto end;}

    norm[i].index = -1; 
    norm[i].min = norm[i].max = 0;

    ret = 0;

end:
    if (sb)   wtk_strbuf_delete(sb);

    if (ret) printf("invalid normalization parameters\n");
    if (ret && norm) {wtk_free(norm); norm = NULL;}
    return norm;
}

wtk_libsvm_norm_t* wtk_libsvm_norm_new2(const char *fn, int dim)
{
    wtk_libsvm_norm_t *norm;
    wtk_source_t source;

    if ((wtk_source_init_file(&source, (char*)fn))) return NULL;

    norm = wtk_libsvm_norm_new(&source, dim);
    wtk_source_clean_file(&source);

    return norm;
}

wtk_libsvm_norm_t* wtk_libsvm_norm_new3(const char *data, int dim)
{
    wtk_string_t str;
    wtk_source_t source;
    wtk_strsrc_data_t strdata;

    wtk_string_set(&str, (char *)data, strlen(data));

    if ((wtk_strsrc_init(&source, &strdata, &str))) return NULL;

    return  wtk_libsvm_norm_new(&source, dim);
}

wtk_libsvm_norm_t* wtk_libsvm_norm_new4(wtk_string_t *str, int dim, wtk_source_loader_t *sl)
{
    wtk_source_t source;
    wtk_strsrc_data_t strdata;
    wtk_rbin2_t	*rb;
    wtk_rbin2_item_t *item;
    int ret;
    if(sl)
    {
    	rb=(wtk_rbin2_t*)(sl->hook);
        item=wtk_rbin2_get(rb,str->data,str->len);
        if(!item){return NULL;}
       	if(!item->data)
       	{
       		ret=wtk_rbin2_load_item(rb,item,1);
       		if(ret!=0)
       		{
       			wtk_debug("[%.*s] load failed\n",item->fn->len,item->fn->data);
       			return NULL;
       		}
       	}
       	wtk_source_init(&source);
        if ((wtk_strsrc_init(&source, &strdata, item->data))) return NULL;
        return  wtk_libsvm_norm_new(&source, dim);
    }else
    	return wtk_libsvm_norm_new2(str->data, dim);
}

int wtk_libsvm_norm_delete(wtk_libsvm_norm_t *norm)
{
    wtk_free(norm);
    return 0;
}

void wtk_libsvm_nodes_init(wtk_libsvm_node_t *nodes, int dim)
{
    int i;
    for (i = 0; i < dim; i++)
    {
        nodes[i].index = i + 1;
        nodes[i].value = 0.0;
    }

    nodes[dim].index = -1; /* end */
    nodes[dim].value = 0.0;
}

void wtk_libsvm_nodes_norm(wtk_libsvm_node_t *nodes, const wtk_libsvm_norm_t *norm)
{
    int i;
    for (i = 0; nodes[i].index != -1; i++)
    {
        if (norm[i].min != norm[i].max)
            nodes[i].value = (nodes[i].value - norm[i].min)/(norm[i].max - norm[i].min);
        //printf("nodes[%d]=%f", i,nodes[i]);
    }
}

float wtk_libsvm_predict(wtk_libsvm_model_t *model, const wtk_libsvm_node_t *nodes)
{
    return svm_predict(model->model, nodes);
}
