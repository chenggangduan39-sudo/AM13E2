#include "wtk_strsrc.h"
#include "wtk_svmlight.h"

/* similar as fgets, read a line, return 0 on success, and EOF on end of source */
int wtk_source_gets(wtk_source_t *src, wtk_strbuf_t *sb)
{
    int c;
    wtk_strbuf_reset(sb);

    for (;;)
    {
        c=wtk_source_get(src);
        if (c == '\n' || c == EOF) break;

        wtk_strbuf_push_c(sb, c);
    }

    wtk_strbuf_push_c(sb, '\0');

    return c==EOF ? EOF : 0;
}

static MODEL *_read_model(wtk_source_t *src, int dim)
{
    int ret = -1;

    MODEL *model;
    SWORD  *words;

    wtk_strbuf_t *sb;

    long i, queryid, slackid, wpos;
    double costfactor;
    char *comment, version[100];

    sb = wtk_strbuf_new(512, 1.5f);

    model = (MODEL *)wtk_malloc(sizeof(MODEL));
    words = (SWORD  *)wtk_malloc(sizeof(SWORD) * (dim + 1));

    wtk_source_gets(src, sb); sscanf(sb->data,"SVM-light Version %s\n",version);

    if (strcmp(version, SVMLIGHT_VERSION))
    {
        perror ("Version of model-file does not match version of svm_classify!"); 
        ret = -1;
        goto end;
    }

    wtk_source_gets(src, sb); sscanf(sb->data, "%ld%*[^\n]\n", &model->kernel_parm.kernel_type);  
    wtk_source_gets(src, sb); sscanf(sb->data, "%ld%*[^\n]\n", &model->kernel_parm.poly_degree);
    wtk_source_gets(src, sb); sscanf(sb->data, "%lf%*[^\n]\n", &model->kernel_parm.rbf_gamma);
    wtk_source_gets(src, sb); sscanf(sb->data, "%lf%*[^\n]\n", &model->kernel_parm.coef_lin);
    wtk_source_gets(src, sb); sscanf(sb->data, "%lf%*[^\n]\n", &model->kernel_parm.coef_const);
    wtk_source_gets(src, sb); sscanf(sb->data, "%[^#]%*[^\n]\n", model->kernel_parm.custom);

    wtk_source_gets(src, sb); sscanf(sb->data, "%ld%*[^\n]\n", &model->totwords);
    wtk_source_gets(src, sb); sscanf(sb->data, "%ld%*[^\n]\n", &model->totdoc);
    wtk_source_gets(src, sb); sscanf(sb->data, "%ld%*[^\n]\n", &model->sv_num);
    wtk_source_gets(src, sb); sscanf(sb->data, "%lf%*[^\n]\n", &model->b);

    model->supvec = (DOC **)my_malloc(sizeof(DOC *)*model->sv_num);
    model->alpha  = (double *)my_malloc(sizeof(double)*model->sv_num);
    model->index  = NULL;
    model->lin_weights = NULL;

    for (i = 1; i < model->sv_num; i++)
    {
        wtk_source_gets(src, sb);
        if((ret == parse_document(sb->data, words, &(model->alpha[i]), &queryid, &slackid,
                    &costfactor, &wpos, dim + 1, &comment)))
        {
            printf("\nParsing error while reading model file in SV %ld!\n%s", i, sb->data);
            goto end;
        }

        model->supvec[i] = create_example(-1, 0,0, 0.0, create_svector(words, comment, 1.0));
    }
    ret = 0;

end:
    if (sb)    wtk_strbuf_delete(sb);
    if (words) free(words);
    if (ret && model) {wtk_svmlight_model_delete(model); model = NULL;}
    return model;
}

wtk_svmlight_model_t *wtk_svmlight_model_new(wtk_source_t *source, int dim)
{
    wtk_svmlight_model_t *model;

    if(!(model = _read_model(source, dim))) goto end;

    if(model->kernel_parm.kernel_type == 0)       /* linear kernel */
        add_weight_vector_to_linear_model(model); /* compute weight vector */

end:
    return model;
}

wtk_svmlight_model_t* wtk_svmlight_model_new2(const char *fn, int dim)
{
    wtk_svmlight_model_t *model;
    wtk_source_t source;

    if ((wtk_source_init_file(&source, (char*)fn))) return NULL;

    model = wtk_svmlight_model_new(&source, dim);
    wtk_source_clean_file(&source);

    return model;
}

wtk_svmlight_model_t* wtk_svmlight_model_new3(const char *data, int dim)
{
    wtk_string_t str;
    wtk_source_t source;
    wtk_strsrc_data_t strdata;

    wtk_string_set(&str, (char *)data, strlen(data));

    if ((wtk_strsrc_init(&source, &strdata, &str))) return NULL;

    return wtk_svmlight_model_new(&source, dim);
}

int wtk_svmlight_model_delete(wtk_svmlight_model_t *model)
{
    free_model(model, 1);
    return 0;
}

void wtk_svmlight_nodes_init(wtk_svmlight_node_t *nodes, int dim)
{
    int i;
    for ( i = 0; i < dim; i++)
    {
        nodes[i].index = i + 1;
        nodes[i].value = 0.0f;
    }

    nodes[dim].index = 0; /* end */
    nodes[dim].value = 0.0f;
}

float wtk_svmlight_predict(wtk_svmlight_model_t *model, const wtk_svmlight_node_t *nodes)
{
    float dist;

    DOC *doc = NULL;
    SWORD *words = (SWORD*) nodes;

    if (model->kernel_parm.kernel_type == 0)
    {   /* linear kernel */
        int j;
        for(j=0;(words[j]).wnum != 0;j++)
        {
            if((words[j]).wnum>model->totwords)
                (words[j]).wnum=0;            
        } 

        doc  = create_example(-1,0,0,0.0,create_svector(words,"",1.0));
        dist = classify_example_linear(model,doc);
    }
    else
    {   /* non-linear kernel */
        doc  = create_example(-1,0,0,0.0,create_svector(words,"",1.0));
        dist = classify_example(model,doc);
    }

    if (doc) free_example(doc,1);

    return dist;
}
