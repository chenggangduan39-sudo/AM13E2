#include "wtk_xvprint_file2.h"
#define WTK_XVPRINT_MAX_NAME_LEN2 (64)

int wtk_xvprint_file2_dump_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;
    FILE *fidx=NULL,*fdata=NULL;
    int offset=0;
    int head_num;
    wtk_strbuf_t *buf=NULL;    
    char cnt;
    
    
    buf=wtk_strbuf_new(WTK_XVPRINT_MAX_NAME_LEN2,1.0);

    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    fidx=fopen(buf->data, "wb");
    //|int     |char    | char x n|char|int   |...|     
    //|head_num|name_len|name_data|cnt |offset|...|

    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".data",sizeof(".data"));
    fdata=fopen(buf->data, "wb");

    head_num=1;  
    fwrite(&head_num,sizeof(int),1,fidx);  
    cnt=node->name->len;
    fwrite(&cnt,sizeof(char),1,fidx);
    wtk_strbuf_reset(buf);
    memset(buf->data,0,buf->length);
    wtk_strbuf_push(buf,node->name->data,node->name->len);
    fwrite(buf->data,WTK_XVPRINT_MAX_NAME_LEN2,1,fidx);
    cnt=node->num;
    fwrite(&cnt,sizeof(char),1,fidx);
    fwrite(&offset,sizeof(int),1,fidx);

    //data
    fwrite(node->v->p,node->v->len*sizeof(float),1,fdata);
    // fflush(file);

    ret=0;
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    if(fidx)
    {
        fclose(fidx);
    }
    if(fdata)
    {
        fclose(fdata);
    }
    return ret;
}

int wtk_xvprint_file2_dump(char *fn,wtk_queue_t *feat_q)
{
    int ret;
    FILE *fidx=NULL,*fdata=NULL;
    wtk_queue_node_t *qn;
    wtk_xvprint_cfg_feat_node_t *node;
    int offset;
    int head_len;    
    wtk_strbuf_t *buf=NULL;
    char cnt;

    buf=wtk_strbuf_new(WTK_XVPRINT_MAX_NAME_LEN2,1.0);

    //idx
    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    fidx=fopen(buf->data, "wb");

    //data
    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".data",sizeof(".data"));
    fdata=fopen(buf->data, "wb");

    // head_len=WTK_XVPRINT_MAX_NAME_LEN2*feat_q->length;
    head_len=feat_q->length;

    //|int     |char    | WTK_XVPRINT_MAX_NAME_LEN|char|int   |...|     
    //|head_num|name_len|        name_data        |cnt |offset|...|
    // head_len=head_len+feat_q->length*(sizeof(char)+sizeof(char)+sizeof(int));   

    fwrite(&head_len,sizeof(int),1,fidx);           //head num
    offset=0;
    for(qn=feat_q->pop;qn;qn=qn->next)
    {
        node=data_offset(qn,wtk_xvprint_cfg_feat_node_t,qn);
        cnt=node->name->len;
        fwrite(&cnt,sizeof(char),1,fidx);
        wtk_strbuf_reset(buf);
        wtk_strbuf_push(buf,node->name->data,node->name->len);
        wtk_strbuf_push(buf,0,WTK_XVPRINT_MAX_NAME_LEN2-node->name->len);
        fwrite(buf->data,WTK_XVPRINT_MAX_NAME_LEN2,1,fidx);
        cnt=node->num;
        fwrite(&cnt,sizeof(char),1,fidx);
        fwrite(&offset,sizeof(int),1,fidx);
        offset+=(node->v->len*sizeof(float));
        fwrite(node->v->p,node->v->len*sizeof(float),1,fdata);
    }

    ret=0;
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    if(fidx)
    {
        fclose(fidx);
    }
    if(fdata)
    {
        fclose(fdata);
    }
    return ret;    
}

int wtk_xvprint_file2_insert_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;

    ret=wtk_xvprint_file2_query(fn,node->name);
    if(ret!=0)
    {
        ret=wtk_xvprint_file2_append_feat(fn,node);
    }

    return ret;
}

int wtk_xvprint_file2_query(char *fn,wtk_string_t *name)
{
    int ret;
    FILE *file=NULL;
    int head_num; 
    int file_len;
    char *data=NULL;
    char *p;
    int flag;
    char cnt;
    wtk_strbuf_t *buf=NULL;

    buf=wtk_strbuf_new(128,0);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    file=fopen(buf->data, "rb");

    fseek(file,0,SEEK_END);
    file_len=ftell(file)-sizeof(int);
    fseek(file,0,SEEK_SET);

    ret=fread(&head_num,sizeof(int),1,file);
    if(ret!=1)
    {
        ret=-1;
        goto end;
    }

    data=wtk_malloc(file_len);
    ret=fread(data,file_len,1,file);
    if(ret!=1)
    {
        ret=-1;
        goto end;
    }

    flag=0;
    for(p=data;p<data+file_len;)
    {
        cnt=*p;
        p+=1;
        if(wtk_string_cmp(name,p,cnt)==0)
        {
            flag=1;
        }
        p+=WTK_XVPRINT_MAX_NAME_LEN2;
        cnt=*p;
        p+=1;
        p+=sizeof(int);
        if(flag==1 && cnt!=0)
        {
            //query
            ret=*((int*)(p-sizeof(int)));
            goto end;
        }else
        {
            flag=0;
        }
    }

    ret=-1;
end:
    if(data)
    {
        wtk_free(data);
    }
    if(file)
    {
        fclose(file);
    }
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    return ret;
}

int wtk_xvprint_file2_delete(char *fn)
{
    FILE *file=NULL;
    wtk_strbuf_t *buf=NULL;
    buf=wtk_strbuf_new(128,0);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    file=fopen(buf->data, "wb");
    if(file)
    {
        fclose(file);
    }
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }

    buf=wtk_strbuf_new(128,0);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".data",sizeof(".data"));
    file=fopen(buf->data, "wb");
    if(file)
    {
        fclose(file);
    }
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    return 0;
}

int wtk_xvprint_file2_delete_all(char *fn)
{
    int ret;
    FILE *file=NULL;
    int head_num;
    int file_len;
    char *data=NULL;
    char *p;
    wtk_strbuf_t *buf=NULL;

    buf=wtk_strbuf_new(128,0);
    //idx
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    file=fopen(buf->data, "rb+");
    fseek(file,0,SEEK_END);
    file_len=ftell(file)-sizeof(int);
    fseek(file,0,SEEK_SET);

    ret=fread(&head_num,sizeof(int),1,file);
    if(ret!=1){ret=-1;goto end;}

    data=wtk_malloc(file_len);
    ret=fread(data,file_len,1,file);
    for(p=data;p<data+file_len;)
    {
//        cnt=*p;
        p+=1;
        p+=WTK_XVPRINT_MAX_NAME_LEN2;
        *p=0;
        p+=1;
        p+=sizeof(int);
    }

    fseek(file,sizeof(int),SEEK_SET);
    fwrite(data,file_len,1,file);

    ret=0;
end:
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    if(data)
    {
        wtk_free(data);
    }
    if(file)
    {
        fclose(file);
    }
    return ret;
}

int wtk_xvprint_file2_delete_feat(char *fn,wtk_string_t *name)
{
    int ret;
    FILE *file=NULL;
    int head_num; 
    int file_len;
    char *data=NULL;
    char *p;
    wtk_strbuf_t *buf=NULL;
    char cnt;
    char flag;
    
    buf=wtk_strbuf_new(128,0);
    //idx
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    file=fopen(buf->data, "rb+");
    if(file==NULL)
    {
        ret=0;
        goto end;
    }
    fseek(file,0,SEEK_END);
    file_len=ftell(file)-sizeof(int);
    fseek(file,0,SEEK_SET);

    ret=fread(&head_num,sizeof(int),1,file);
    if(ret!=1){ret=-1;goto end;}

    data=wtk_malloc(file_len);
    ret=fread(data,file_len,1,file);
    flag=0;
    for(p=data;p<data+file_len;)
    {
        cnt=*p;
        p+=1;
        if(wtk_string_cmp(name,p,cnt)==0)
        {
            flag=1;
        }
        p+=WTK_XVPRINT_MAX_NAME_LEN2;
        if(flag==1 && *p!=0)
        {
            *p=0;
            fseek(file,sizeof(int),SEEK_SET);
            fwrite(data,file_len,1,file);
            break;
        }else
        {
            flag=0;
        }
        p+=1;
        p+=sizeof(int);
    }

    ret=0;
end:
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    if(data)
    {
        wtk_free(data);
    }
    if(file)
    {
        fclose(file);
    }
    return ret;    
}

int wtk_xvprint_check_same(FILE *fidx, int head_num,
                           wtk_xvprint_cfg_feat_node_t *node)
{
    char name_len = -1;
    int ret = 0, i = 0, idx_data_len = 0;
    wtk_string_t *idx_data = NULL;

    fseek(fidx, 0, SEEK_END);
    idx_data_len = ftell(fidx) - sizeof(int);
    fseek(fidx, sizeof(int), SEEK_SET);
    idx_data = wtk_string_new(idx_data_len);
    fread(idx_data->data, sizeof(char), idx_data_len, fidx);
    
    for (i = 0; i < head_num; i++) {
        name_len = *(idx_data->data + i * (2 * sizeof(char) + sizeof(int) +
                                           WTK_XVPRINT_MAX_NAME_LEN2));
        ret = wtk_string_cmp(node->name,
                             idx_data->data +
                                 i * (2 * sizeof(char) + sizeof(int) +
                                      WTK_XVPRINT_MAX_NAME_LEN2) +
                                 sizeof(char),
                             name_len);
        if (0 == ret) {
            ret = i;
            goto end;
        }
    }

    ret = -1;
end:
    wtk_string_delete(idx_data);
    return ret;
}

int wtk_xvprint_get_offset(FILE *fidx, int name_id)
{
    int offset = -1;
    fseek(fidx,
          sizeof(int) +
              name_id *
                  (2 * sizeof(char) + sizeof(int) + WTK_XVPRINT_MAX_NAME_LEN2) +
              2 * sizeof(char) + WTK_XVPRINT_MAX_NAME_LEN2,
          SEEK_SET);

    fread(&offset, sizeof(int), 1, fidx);
    fseek(fidx, 0, SEEK_SET);

    return offset;
}

char wtk_xvprint_get_num(FILE *fidx, int name_id)
{
    char num = -1;
    fseek(fidx,
          sizeof(int) +
              name_id *
                  (2 * sizeof(char) + sizeof(int) + WTK_XVPRINT_MAX_NAME_LEN2) +
              sizeof(char) + WTK_XVPRINT_MAX_NAME_LEN2,
          SEEK_SET);

    fread(&num, sizeof(char), 1, fidx);
    fseek(fidx, 0, SEEK_SET);

    return num;
}

int wtk_xvprint_file2_append(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int i = 0, ret, name_id = -1, last_offset = -1;
    FILE *fidx=NULL,*fdata=NULL;
    int head_num; 
    int file_len;
    wtk_strbuf_t *buf=NULL;
    char cnt, last_num = -1;
    float *last_data_p = NULL;


    buf=wtk_strbuf_new(128,0);
    //idx
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    fidx=fopen(buf->data, "rb+");
    ret=fread(&head_num,sizeof(int),1,fidx);
    if(ret!=1)
    {
        ret=-1;
        goto end;
    }

    // merge if vp.bin.idx contains the same idx
    name_id = wtk_xvprint_check_same(fidx, head_num, node);

    if (-1 == name_id) {
        head_num+=1;
    }
    fseek(fidx,0,SEEK_SET);
    fwrite(&head_num,sizeof(int),1,fidx);
    fseek(fidx,0,SEEK_END);

    //data
    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".data",sizeof(".data"));
    fdata=fopen(buf->data, "rb+");

    if (-1 == name_id) {
        fseek(fdata,0,SEEK_END);
        file_len=ftell(fdata);
        wtk_strbuf_reset(buf);
        cnt=node->name->len;
        wtk_strbuf_push_c(buf,cnt);
        wtk_strbuf_push(buf,node->name->data,node->name->len);
        wtk_strbuf_push(buf,0,WTK_XVPRINT_MAX_NAME_LEN2-node->name->len);
        cnt=node->num;
        wtk_strbuf_push_c(buf,cnt);
        wtk_strbuf_push(buf,(char*)&file_len,sizeof(int));
        fwrite(buf->data,buf->pos,1,fidx);
        fwrite(node->v->p,node->v->len*sizeof(float),1,fdata);
    } else {
        wtk_string_t* last_data = NULL;
        last_data = wtk_string_new(node->v->len * sizeof(float));

        last_offset = wtk_xvprint_get_offset(fidx, name_id);
        if (-1 == last_offset) {
            ret=-1;
            goto end;
        }
        fseek(fdata, last_offset * sizeof(float), SEEK_SET);
        fread(last_data->data, sizeof(float), node->v->len, fdata);

        last_num = wtk_xvprint_get_num(fidx, name_id);
        if (-1 == last_num) {
            ret=-1;
            goto end;
        }

        last_data_p = (float *)last_data->data;
        for (i = 0; i < node->v->len; i++) {
            node->v->p[i] = (node->v->p[i]*node->num + last_data_p[i]*
                    last_num) / (node->num + last_num);
        }
        node->num += last_num;

        fseek(fdata, last_offset * sizeof(float), SEEK_SET);
        fwrite(node->v->p, node->v->len * sizeof(float), 1, fdata);

        fseek(fidx,
              sizeof(int) +
                  name_id * (2 * sizeof(char) + sizeof(int) +
                             WTK_XVPRINT_MAX_NAME_LEN2) +
                  sizeof(char) + WTK_XVPRINT_MAX_NAME_LEN2,
              SEEK_SET);
        fwrite(&node->num, sizeof(char), 1, fidx);

        wtk_string_delete(last_data);
    }

    ret=0;
end:
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }

    if(fidx)
    {
        fclose(fidx);
    }

    if(fdata)
    {
        fclose(fdata);
    }

    return ret;
}

int wtk_xvprint_file2_get_offset(char *cur)
{
    int ret;

    cur=cur+sizeof(char)+WTK_XVPRINT_MAX_NAME_LEN2+sizeof(char);
    ret=*((int*)cur);

    return ret;
}

void wtk_xvprint_file2_set_node_head(char *cur,wtk_xvprint_cfg_feat_node_t *node)
{
    char cnt;

    cnt=node->name->len;
    *cur=cnt;
    cur+=sizeof(char);
    memcpy(cur,node->name->data,node->name->len);
    cur+=WTK_XVPRINT_MAX_NAME_LEN2;
    cnt=node->num;
    *cur=cnt;
}

int wtk_xvprint_file2_append2(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;
    FILE *fidx=NULL,*fdata=NULL;
    int file_len;
    int head_num;
    char *data=NULL;
    char *p;
    int pos;
    int feat_len=0;
    int offset;
    wtk_strbuf_t *buf=NULL;
    char flag;

    buf=wtk_strbuf_new(128,0);
    //idx
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    fidx=fopen(buf->data, "rb+");
    fseek(fidx,0,SEEK_END);
    file_len=ftell(fidx)-sizeof(int);
    fseek(fidx,0,SEEK_SET);

    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".data",sizeof(".data"));
    fdata=fopen(buf->data, "rb+");

    ret=fread(&head_num,sizeof(int),1,fidx);
    if(ret!=1){ret=-1;goto end;}
    data=wtk_malloc(file_len);
    if(data==NULL){ret=-1;goto end;}
    ret=fread(data,file_len,1,fidx);
    if(ret!=1){ret=-1;goto end;}
    flag=0;
    for(p=data;p<data+file_len;)
    {
        p+=1;
        p+=WTK_XVPRINT_MAX_NAME_LEN2;
        if(*p==0)
        {
            flag=1;
        }
        p+=1;
        p+=sizeof(int); 

        if(flag)
        {
            offset=*((int*)(p-sizeof(int)));
            // printf("pp %d\n",(int)((data+head_len)-p));
            if(p==data+file_len)
            {
                fseek(fdata,offset,SEEK_SET);
                pos=ftell(fdata);
                fseek(fdata,0L,SEEK_END);
                feat_len=ftell(fdata)-pos;
                // printf("feat_len0 %d\n",feat_len);
            }else
            {
                feat_len=wtk_xvprint_file2_get_offset(p);
                feat_len=feat_len-offset;
                // printf("feat_len1 %d\n",feat_len);
            }
            
            if(feat_len/sizeof(float)==node->v->len)
            {
                p=p-(sizeof(char)+WTK_XVPRINT_MAX_NAME_LEN2+sizeof(char)+sizeof(int));
                wtk_xvprint_file2_set_node_head(p,node);
                fseek(fidx,sizeof(int),SEEK_SET);
                fwrite(data,file_len,1,fidx);


                fseek(fdata,offset,SEEK_SET);
                fwrite(node->v->p,node->v->len*sizeof(float),1,fdata);
                break;
            }
            flag=0;
        }
    }

    if(flag==0)
    {
        ret=-1;
        goto end;
    }

    ret=0;
end:
    if(fidx)
    {
        fclose(fidx);
    }

    if(fdata)
    {
        fclose(fdata);
    }

    if(buf)
    {
        wtk_strbuf_delete(buf);
    }

    if(data)
    {
        wtk_free(data);
    }

    return ret;
}

int wtk_xvprint_file2_append_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;
    wtk_strbuf_t *buf;

    buf=wtk_strbuf_new(128,0);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    if(wtk_file_exist(buf->data)==0)
    {
        ret=wtk_xvprint_file2_append2(fn,node);
        if(ret==0)
        {
            goto end;
        }
        ret=wtk_xvprint_file2_append(fn,node);
    }else
    {
        ret=wtk_xvprint_file2_dump_feat(fn,node);
    }

end:
    wtk_strbuf_delete(buf);
    return ret;
}

int wtk_xvprint_file2_load(char *fn,wtk_queue_t *q)
{
    int ret;
    wtk_queue_node_t *qn,*qn2;
    wtk_xvprint_cfg_feat_node_t *node;
    char *head=NULL,*p;
    int head_len; 
    int head_num;
    FILE *fidx=NULL,*fdata=NULL;
    wtk_larray_t *offset;
    wtk_strbuf_t *buf=NULL;
    int feat_len,i;
    int pos;
    char cnt;
    char flag;


    // wtk_xvprint_file2_print_head(fn);
    // exit(0);

    offset=wtk_larray_new(1024,sizeof(int));

    buf=wtk_strbuf_new(128,0);

    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    fidx=fopen(buf->data, "rb");
    if(fidx==NULL)
    {
        ret=-1;
        goto end;
    }
    fseek(fidx,0,SEEK_END);
    head_len=ftell(fidx)-sizeof(int);
    fseek(fidx,0,SEEK_SET);

    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".data",sizeof(".data"));
    fdata=fopen(buf->data, "rb");


    ret=fread(&head_num,sizeof(int),1,fidx);
    if(ret!=1){ret=-1;goto end;}
    head=wtk_malloc(head_len);
    if(NULL==head){ret=-1;goto end;}
    ret=fread(head,head_len,1,fidx);
    if(ret!=1){ret=-1;goto end;}
    flag=0;
    for(p=head;p<head+head_len;)
    {
        cnt=*p;
        p+=1;
        //wtk_debug("%.*s\n",cnt,p);
        node=wtk_xvprint_cfg_node_new(p,cnt);
        p+=WTK_XVPRINT_MAX_NAME_LEN2;
        cnt=*p;
        p+=1;
        node->num=cnt;
        // printf("cnt %d\n",cnt);
        if(cnt==0)
        {
            flag=1;
        }
        wtk_larray_push2(offset,p);
        p+=sizeof(int);
        wtk_queue_push(q,&node->qn);
    }

    for(i=0,qn=q->pop;qn;qn=qn->next,++i)
    {
        node=data_offset2(qn,wtk_xvprint_cfg_feat_node_t,qn);
        feat_len=*((int*)wtk_larray_get(offset,i));
        if(qn->next)
        {
            feat_len=*((int*)wtk_larray_get(offset,i+1))-feat_len;
        }else
        {
            pos=ftell(fdata);
            fseek(fdata,0L,SEEK_END);
            feat_len=ftell(fdata)-pos;
            fseek(fdata,pos,SEEK_SET);
            // printf("feat_lendd %d\n",feat_len);
        }
        node->v=wtk_vecf_new(feat_len/sizeof(float));
        ret = fread(node->v->p,feat_len,1,fdata);
        if (ret != 1) {
            ret = -1;
            goto end;
        }
        // int j;
        // for(j=0;j<node->v->len;++j)
        // {
        //     printf("%d %f\n",j,node->v->p[j]);
        // }
        // printf("-------------------------\n");
    }
    // exit(0);

    if(flag)
    {
        for(qn=q->pop;qn;qn=qn2)
        {
            qn2=qn->next;
            node=data_offset2(qn,wtk_xvprint_cfg_feat_node_t,qn);
            if(node->num==0)
            {
                wtk_queue_remove(q,qn);
                wtk_xvprint_cfg_node_delete(node);
            }
        }
    }

    ret=0;
end:
    if(offset)
    {
        wtk_larray_delete(offset);
    }
    if(head)
    {
        wtk_free(head);
    }
    if(fidx)
    {
        fclose(fidx);
    }
    if(fdata)
    {
        fclose(fdata);
    }
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    return ret;
}

void wtk_xvprint_file2_delete_queue(wtk_queue_t *q)
{
	wtk_queue_node_t *qn;
	wtk_xvprint_cfg_feat_node_t *node;

	do
	{
		qn=wtk_queue_pop(q);
		if(qn==NULL)
		{
			break;
		}
		node=data_offset2(qn,wtk_xvprint_cfg_feat_node_t,qn);
		wtk_xvprint_cfg_node_delete(node);
	}while(qn);
}

void wtk_xvprint_file2_query_all(char *fn,void *hook,wtk_xvprint_file2_name_notify notify)
{
	wtk_queue_t q;
    wtk_queue_node_t *qn;
    wtk_xvprint_cfg_feat_node_t *node;

    wtk_queue_init(&q);
	wtk_xvprint_file2_load(fn,&q);

	for(qn=q.pop;qn;qn=qn->next)
	{
		node=data_offset2(qn,wtk_xvprint_cfg_feat_node_t,qn);
		notify(hook,node->name);
	}

	wtk_xvprint_file2_delete_queue(&q);
}


wtk_xvprint_cfg_feat_node_t* wtk_xvprint_file2_load_feat(char *fn,wtk_string_t *name)
{
    int ret;
    int head_len; 
    int head_num;
    FILE *fidx=NULL,*fdata=NULL;
    char *data=NULL;
    char *p;
    int offset;
    wtk_xvprint_cfg_feat_node_t *node=NULL;
    int pos;
    int feat_len;
    wtk_strbuf_t *buf=NULL;
    char cnt;
    char flag;

    buf=wtk_strbuf_new(128,0);

    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    fidx=fopen(buf->data, "rb");
    if(fidx==NULL)
    {
        ret=-1;
        goto end;
    }
    fseek(fidx,0,SEEK_END);
    head_len=ftell(fidx)-sizeof(int);
    fseek(fidx,0,SEEK_SET);

    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".data",sizeof(".data"));
    fdata=fopen(buf->data, "rb");

    ret=fread(&head_num,sizeof(int),1,fidx);
    if(ret!=1)
    {
        ret=-1;
        goto end;
    }
    data=wtk_malloc(head_len);
    if(data==NULL){ret=-1;goto end;}
    ret=fread(data,head_len,1,fidx);
    if(ret!=1)
    {
        ret=-1;
        goto end;
    }

    flag=0;
    for(p=data;p<data+head_len;)
    {
        cnt=*p;
        p+=1;
        if(wtk_string_cmp(name,p,cnt)==0)
        {
            flag=1;
        }
        p+=WTK_XVPRINT_MAX_NAME_LEN2;
        cnt=*p;
        p+=1;
        p+=sizeof(int);

        if(flag==1 && cnt!=0)
        {
            node=wtk_xvprint_cfg_node_new(name->data,name->len);
            if(node==NULL)
            {
                ret=-1;
                goto end;
            }
            node->num=cnt;
            
            offset=*((int*)(p-sizeof(int)));
            if(p==data+head_len)
            {
                fseek(fdata,offset,SEEK_SET);
                pos=ftell(fdata);
                fseek(fdata,0L,SEEK_END);
                feat_len=ftell(fdata)-pos;
                // printf("feat_len0 %d\n",feat_len);
            }else
            {
                feat_len=wtk_xvprint_file2_get_offset(p);
                feat_len=feat_len-offset;
                // printf("feat_len1 %d\n",feat_len);
            }

            node->v=wtk_vecf_new(feat_len);
            fseek(fdata,offset,SEEK_SET);
            ret = fread(node->v->p,feat_len,1,fdata);
            if (ret != 1) {
                ret = -1;
                goto end;
            }
        }else
        {
            flag=0;
        }
    }


    ret=0;
end:
    if(ret!=0)
    {
        if(node)
        {
            wtk_xvprint_cfg_node_delete(node);
        }
    }

    if(data)
    {
        wtk_free(data);
    }

    if(buf)
    {
        wtk_strbuf_delete(buf);
    }

    if(fidx)
    {
        fclose(fidx);
    }

    if(fdata)
    {
        fclose(fdata);
    }
    return node;
}

int wtk_xvprint_file2_update_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;
    FILE *fidx=NULL,*fdata=NULL;
    char *data=NULL;
    char *p;
    int head_len; 
    int off;
    int flag;
    wtk_strbuf_t *buf=NULL;
    int head_num;
    char cnt;

    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".idx",sizeof(".idx"));
    fidx=fopen(buf->data, "rb");
    fseek(fidx,0,SEEK_END);
    head_len=ftell(fidx)-sizeof(int);
    fseek(fidx,0,SEEK_SET);

    wtk_strbuf_reset(buf);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".data",sizeof(".data"));
    fdata=fopen(buf->data, "rb");

    ret=fread(&head_num,sizeof(int),1,fidx);
    if(ret!=1){ret=-1;goto end;}

    data=wtk_malloc(head_len);
    if(data==NULL){ret=-1;goto end;}
    ret=fread(data,head_len,1,fidx);
    if(ret!=1)
    {
        ret=-1;
        goto end;
    }

    off=-1;
    flag=0;
    for(p=data;p<data+head_len;)
    {
        cnt=*p;
        p+=1;
        if(wtk_string_cmp(node->name,p,cnt)==0)
        {
            flag=1;
        }
        p+=WTK_XVPRINT_MAX_NAME_LEN2;
        cnt=*p;
        p+=1;
        p+=sizeof(int);
        if(flag==1 && cnt!=0)
        {
            //offset
            off=*((int*)(p-sizeof(int)));
            cnt=node->num;
            *(p-sizeof(int)-1)=cnt;
            break;
        }else
        {
            flag=0;
        }
    }
    if(off==-1)
    {
        ret=-1;
        goto end;
    }
    fseek(fidx,sizeof(int),SEEK_SET);
    fwrite(data,head_len,1,fidx);

    fseek(fdata,off,SEEK_SET);
    fwrite(node->v->p,node->v->len*sizeof(float),1,fdata);


    ret=0;
end:
    if(data)
    {
        wtk_free(data);
    }
    if(fidx)
    {
        fclose(fidx);
    }
    if(fdata)
    {
        fclose(fdata);
    }
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    return ret;
}
