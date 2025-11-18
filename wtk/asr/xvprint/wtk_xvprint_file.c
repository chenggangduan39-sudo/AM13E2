#include "wtk_xvprint_file.h"
#include "wtk_xvprint_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "stdio.h"
#define WTK_XVPRINT_MAX_NAME_LEN (64)

int wtk_xvprint_file_append(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file_append2(char *fn,wtk_xvprint_cfg_feat_node_t *node);
int wtk_xvprint_file_get_offset(FILE *file,char *cur);

int wtk_xvprint_file_query(char *fn,wtk_string_t *name)
{
    int ret;
    FILE *file=NULL;
    int head_len; 
    char *data=NULL;
    char *p;
    int flag;
    char cnt;
    
    file=fopen(fn,"r");
    ret=fread(&head_len,sizeof(int),1,file);
    if(ret!=1)
    {
        ret=-1;
        goto end;
    }
    data=wtk_malloc(head_len);
    ret=fread(data,head_len,1,file);
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
        p+=WTK_XVPRINT_MAX_NAME_LEN;
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
    return ret;
}

int wtk_xvprint_file_update_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;
    FILE *file=NULL;
    char *data=NULL;
    char *p;
    int head_len; 
    int off;
    int flag;
    char cnt;

    file=fopen(fn,"r+");
    ret = fread(&head_len,sizeof(int),1,file);
    if (ret != 1) {
        ret = -1;
        goto end;
    }
    data=wtk_malloc(head_len);
    if(data==NULL){ret=-1;goto end;}
    ret=fread(data,head_len,1,file);
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
        p+=WTK_XVPRINT_MAX_NAME_LEN;
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
    fseek(file,sizeof(int),SEEK_SET);
    fwrite(data,head_len,1,file);

    off=off+head_len+sizeof(int);
    fseek(file,off,SEEK_SET);
    fwrite(node->v->p,node->v->len*sizeof(float),1,file);


    ret=0;
end:
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

void wtk_xvprint_file_print_head(char *fn)
{
    FILE *file=NULL;
    int head_len; 
    char *data = NULL;
    char *p;
    int head_num;
    char cnt;
    

    file=fopen(fn,"r");
    if (fread(&head_len,sizeof(int),1,file) != 1) {
        goto end;
    }
    data=wtk_malloc(head_len);
    if (fread(data,head_len,1,file) != 1) {
        goto end;
    }
    head_num=0;
    for(p=data;p<data+head_len;)
    {
        cnt=*p;
        p+=1;
        printf("name:%.*s\n",cnt,p);
        p+=WTK_XVPRINT_MAX_NAME_LEN;
        cnt=*p;
        p+=1;
        printf("spk_cnt:%d\n",cnt);
        printf("offset:%d\n",*(int*)p);
        printf("---------------------------------\n");
        p+=sizeof(int);
        head_num++;
    }
    printf("head_len:%d head_num:%d\n",head_len,head_num);

end:
if (data) {
    wtk_free(data);
}
if (file) {
    fclose(file);
}
}

int wtk_xvprint_file_insert_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;

    ret=wtk_xvprint_file_query(fn,node->name);
    if(ret!=0)
    {
        ret=wtk_xvprint_file_append_feat(fn,node);
    }

    return ret;
}

int wtk_xvprint_file_delete_feat(char *fn,wtk_string_t *name)
{
    int ret;
    FILE *file=NULL;
    int head_len; 
    char *data=NULL;
    char *p;
    char cnt;
    char flag;
    

    file=fopen(fn,"r+");

    ret=fread(&head_len,sizeof(int),1,file);
    if(ret!=1){ret=-1;goto end;}
    data=wtk_malloc(head_len);
    ret=fread(data,head_len,1,file);
    flag=0;
    for(p=data;p<data+head_len;)
    {
        cnt=*p;
        p+=1;
        if(wtk_string_cmp(name,p,cnt)==0)
        {
            flag=1;
        }
        p+=WTK_XVPRINT_MAX_NAME_LEN;
        if(flag==1 && *p!=0)
        {
            *p=0;
            fseek(file,sizeof(int),SEEK_SET);
            fwrite(data,head_len,1,file);
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

int wtk_xvprint_file_append_feat2(char *fn,wtk_queue_t  *q)
{
    int ret;
    wtk_xvprint_cfg_feat_node_t *node;
    wtk_queue_node_t *qn;

    for(qn=q->pop;qn;qn=qn->next)
    {
        node=data_offset2(qn,wtk_xvprint_cfg_feat_node_t,qn);
        ret=wtk_xvprint_file_append_feat(fn,node);
        if(ret!=0)
        {
            goto end;
        }
    }

    ret=0;
end:
    return ret;
}

int wtk_xvprint_file_append_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;

    if(wtk_file_exist(fn)==0)
    {
        ret=wtk_xvprint_file_append2(fn,node);
        if(ret==0)
        {
            goto end;
        }
        ret=wtk_xvprint_file_append(fn,node);
    }else
    {
        ret=wtk_xvprint_file_dump_feat(fn,node);
    }

end:
    return ret;
}

int wtk_xvprint_file_get_offset(FILE *file,char *cur)
{
    int ret;

    cur=cur+sizeof(char)+WTK_XVPRINT_MAX_NAME_LEN+sizeof(char);
    ret=*((int*)cur);

    return ret;
}

void wtk_xvprint_file_set_node_head(char *cur,wtk_xvprint_cfg_feat_node_t *node)
{
    char cnt;

    cnt=node->name->len;
    *cur=cnt;
    cur+=sizeof(char);
    memcpy(cur,node->name->data,node->name->len);
    cur+=WTK_XVPRINT_MAX_NAME_LEN;
    cnt=node->num;
    *cur=cnt;
}

int wtk_xvprint_file_append2(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;
    FILE *file=NULL;
    int head_len;
    char *data=NULL;
    char *p;
    int pos;
    int pos2;
    int feat_len=0;
    int offset;
    char flag;

    file=fopen(fn,"r+");
    ret=fread(&head_len,sizeof(int),1,file);
    if(ret!=1){ret=-1;goto end;}
    data=wtk_malloc(head_len);
    if(data==NULL){ret=-1;goto end;}
    ret=fread(data,head_len,1,file);
    if(ret!=1){ret=-1;goto end;}
    flag=0;
    for(p=data;p<data+head_len;)
    {
        p+=1;
        p+=WTK_XVPRINT_MAX_NAME_LEN;
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
            if(p==data+head_len)
            {
                pos=ftell(file);
                fseek(file,offset+head_len+sizeof(int),SEEK_SET);
                pos2=ftell(file);
                fseek(file,0L,SEEK_END);
                feat_len=ftell(file)-pos2;
                fseek(file,pos,SEEK_SET);
                // printf("feat_len0 %d\n",feat_len);
            }else
            {
                feat_len=wtk_xvprint_file_get_offset(file,p);
                feat_len=feat_len-offset;
                // printf("feat_len1 %d\n",feat_len);
            }
            
            if(feat_len/sizeof(float)==node->v->len)
            {
                p=p-(sizeof(char)+WTK_XVPRINT_MAX_NAME_LEN+sizeof(char)+sizeof(int));
                wtk_xvprint_file_set_node_head(p,node);
                fseek(file,sizeof(int),SEEK_SET);
                fwrite(data,head_len,1,file);
                fseek(file,offset+head_len+sizeof(int),SEEK_SET);
                fwrite(node->v->p,node->v->len*sizeof(float),1,file);
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
    if(file)
    {
        fclose(file);
    }
    if(data)
    {
        wtk_free(data);
    }

    return ret;
}

int wtk_xvprint_file_append(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;
    FILE *file=NULL,*file2=NULL;
    int head_len;  
    wtk_strbuf_t *buf=NULL;
    char *data_tmp=NULL;
    int offset;
    int pos;
    wtk_larray_t *a;
    char *p;
    int i;
    int feat_len;
    int last_len;
    char cnt;
    
    a=wtk_larray_new(1024,sizeof(int));

    buf=wtk_strbuf_new(128,1.0);
    wtk_strbuf_push(buf,fn,strlen(fn));
    wtk_strbuf_push(buf,".tmp.",sizeof(".tmp.")); //push 0 


    file=fopen(fn,"rb");
    file2=fopen(buf->data,"wb");

    ret=fread(&head_len,sizeof(int),1,file);
    if(ret!=1){ret=-1;goto end;}
    data_tmp=wtk_malloc(head_len);
    if(data_tmp==NULL){ret=-1;goto end;}
    ret = fread(data_tmp,head_len,1,file);
    if (ret != 1) {ret = -1; goto end;}
    pos=ftell(file);
    fseek(file,0,SEEK_END);
    offset=ftell(file)-head_len-sizeof(int);
    fseek(file,pos,SEEK_SET);
    
    for(p=data_tmp;p<data_tmp+head_len;)
    {
        p+=1;
        p+=WTK_XVPRINT_MAX_NAME_LEN;
        p+=1;
        wtk_larray_push2(a,p);
        p+=sizeof(int);
    }

    wtk_strbuf_reset(buf);
    cnt=node->name->len;
    wtk_strbuf_push_c(buf,cnt);
    wtk_strbuf_push(buf,node->name->data,node->name->len);
    wtk_strbuf_push(buf,0,WTK_XVPRINT_MAX_NAME_LEN-node->name->len);
    cnt=node->num;
    wtk_strbuf_push_c(buf,cnt);
    wtk_strbuf_push(buf,(char*)&offset,sizeof(int));
    head_len+=buf->pos;
    
    fwrite(&head_len,sizeof(int),1,file2);
    fwrite(data_tmp,head_len-buf->pos,1,file2);
    fwrite(buf->data,buf->pos,1,file2);

    wtk_free(data_tmp);

    data_tmp=NULL;
    last_len=0;
    for(i=0;i<a->nslot;++i)
    {
        feat_len=*((int*)wtk_larray_get(a,i));
        if(i+1!=a->nslot)
        {
            feat_len=*((int*)wtk_larray_get(a,i+1))-feat_len;
            if(last_len!=feat_len)
            {
                if(data_tmp)
                {
                    wtk_free(data_tmp);
                }
                data_tmp=wtk_malloc(feat_len);
            }else if(data_tmp==NULL)
            {
                data_tmp=wtk_malloc(feat_len);
            }
            last_len=feat_len;
        }else
        {
            pos=ftell(file);
            fseek(file,0,SEEK_END);
            feat_len=ftell(file)-pos;
            fseek(file,pos,SEEK_SET);
            if(feat_len>0)
            {
                if(data_tmp!=NULL)
                {
                    if(feat_len>last_len)
                    {
                        wtk_free(data_tmp);
                        data_tmp=wtk_malloc(feat_len);
                    }
                }else
                {
                    data_tmp=wtk_malloc(feat_len);
                }
            }
        }
        if (fread(data_tmp,feat_len,1,file) != 1) {
            ret = -1;
            goto end;
        }
        fwrite(data_tmp,feat_len,1,file2);
    }

    fwrite(node->v->p,node->v->len*sizeof(float),1,file2);




    ret=0;
end:
    if(file2)
    {
        fclose(file2);
    }

    if(file)
    {
        fclose(file);
    }

    if(ret==0)
    {
        wtk_strbuf_reset(buf);
        wtk_strbuf_push(buf,fn,strlen(fn));
        wtk_strbuf_push(buf,".tmp.",sizeof(".tmp.")); //push 0 
        remove(fn);
        rename(buf->data,fn);
    }

    if(data_tmp)
    {
        wtk_free(data_tmp);
    }

    if(a)
    {
        wtk_larray_delete(a);
    }

    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    return ret;
}


int wtk_xvprint_file_dump_feat(char *fn,wtk_xvprint_cfg_feat_node_t *node)
{
    int ret;
    FILE *file=NULL;
    int offset=0;
    int head_len;
    wtk_strbuf_t *buf=NULL;    
    char cnt;
    
    
    buf=wtk_strbuf_new(WTK_XVPRINT_MAX_NAME_LEN,1.0);
    memset(buf->data,0,buf->length);
    file=fopen(fn, "wb");
    //|int     |char    | char x n|char|int   |...|     
    //|head_len|name_len|name_data|cnt |offset|...|
    head_len=sizeof(char)+WTK_XVPRINT_MAX_NAME_LEN+sizeof(char)+sizeof(int);  
    fwrite(&head_len,sizeof(int),1,file);  
    cnt=node->name->len;
    fwrite(&cnt,sizeof(char),1,file);
    wtk_strbuf_push(buf,node->name->data,node->name->len);
    fwrite(buf->data,WTK_XVPRINT_MAX_NAME_LEN,1,file);
    cnt=node->num;
    fwrite(&cnt,sizeof(char),1,file);
    fwrite(&offset,sizeof(int),1,file);
    fwrite(node->v->p,node->v->len*sizeof(float),1,file);
    // fflush(file);

    ret=0;
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    if(file)
    {
        fclose(file);
    }
    return ret;
}

int wtk_xvprint_file_dump(char *fn,wtk_queue_t *feat_q)
{
    int ret;
    FILE *file=NULL;
    wtk_queue_node_t *qn;
    wtk_xvprint_cfg_feat_node_t *node;
    int offset;
    int head_len;    
    wtk_strbuf_t *buf=NULL;
    char cnt;

    buf=wtk_strbuf_new(WTK_XVPRINT_MAX_NAME_LEN,1.0);

    file=fopen(fn, "wb");

    // head_len=0;
    // for(qn=feat_q->pop;qn;qn=qn->next)
    // {
    //     node=data_offset(qn,wtk_xvprint_cfg_feat_node_t,qn);
    //     head_len+=node->name->len;
    // }
    head_len=WTK_XVPRINT_MAX_NAME_LEN*feat_q->length;

    //|int     |char    | WTK_XVPRINT_MAX_NAME_LEN|char|int   |...|     
    //|head_len|name_len|        name_data        |cnt |offset|...|
    head_len=head_len+feat_q->length*(sizeof(char)+sizeof(char)+sizeof(int));   

    fwrite(&head_len,sizeof(int),1,file);           //head len
    offset=0;
    for(qn=feat_q->pop;qn;qn=qn->next)
    {
        node=data_offset(qn,wtk_xvprint_cfg_feat_node_t,qn);
        cnt=node->name->len;
        fwrite(&cnt,sizeof(char),1,file);
        wtk_strbuf_reset(buf);
        wtk_strbuf_push(buf,node->name->data,node->name->len);
        wtk_strbuf_push(buf,0,WTK_XVPRINT_MAX_NAME_LEN-node->name->len);
        fwrite(buf->data,WTK_XVPRINT_MAX_NAME_LEN,1,file);
        cnt=node->num;
        fwrite(&cnt,sizeof(char),1,file);
        fwrite(&offset,sizeof(int),1,file);
        offset+=(node->v->len*sizeof(float));
    }

    for(qn=feat_q->pop;qn;qn=qn->next)
    {
        node=data_offset(qn,wtk_xvprint_cfg_feat_node_t,qn);
        fwrite(node->v->p,node->v->len*sizeof(float),1,file);
        // printf("------------------------\n");
        // int ii;
        // for(ii=0;ii<node->v->len;++ii)
        // {
        //     printf("%d %f\n",ii,node->v->p[ii]);
        // }
    }
    // exit(0);

    ret=0;
    if(buf)
    {
        wtk_strbuf_delete(buf);
    }
    if(file)
    {
        fclose(file);
    }
    return ret;    
}

int wtk_xvprint_file_load(char *fn,wtk_queue_t *q)
{
    int ret;
    FILE *file=NULL;
    wtk_queue_node_t *qn,*qn2;
    wtk_xvprint_cfg_feat_node_t *node;
    int head_len;
    char *head=NULL,*p;
    wtk_larray_t *offset;
    int feat_len,i;
    int pos;
    char cnt;
    char flag;

    offset=wtk_larray_new(1024,sizeof(int));
    file=fopen(fn, "rb");
    ret=fread(&head_len,sizeof(int),1,file);
    if(ret!=1){ret=-1;goto end;}
    head=wtk_malloc(head_len);
    if(NULL==head){ret=-1;goto end;}
    ret=fread(head,head_len,1,file);
    if(ret!=1){ret=-1;goto end;}
    flag=0;
    for(p=head;p<head+head_len;)
    {
        cnt=*p;
        p+=1;
        node=wtk_xvprint_cfg_node_new(p,cnt);
        p+=WTK_XVPRINT_MAX_NAME_LEN;
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
            pos=ftell(file);
            fseek(file,0L,SEEK_END);
            feat_len=ftell(file)-pos;
            fseek(file,pos,SEEK_SET);
        }
        node->v=wtk_vecf_new(feat_len/sizeof(float));
        ret = fread(node->v->p,feat_len,1,file);
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
    if(file)
    {
        fclose(file);
    }
    return ret;
}


wtk_xvprint_cfg_feat_node_t* wtk_xvprint_file_load_feat(char *fn,wtk_string_t *name)
{
    int ret;
    int head_len; 
    FILE *file=NULL;
    char *data=NULL;
    char *p;
    int offset;
    wtk_xvprint_cfg_feat_node_t *node=NULL;
    int pos,pos2;
    int feat_len;
    char cnt;
    char flag;

    file=fopen(fn,"r");
    ret=fread(&head_len,sizeof(int),1,file);
    if(ret!=1)
    {
        ret=-1;
        goto end;
    }
    data=wtk_malloc(head_len);
    if(data==NULL){ret=-1;goto end;}
    ret=fread(data,head_len,1,file);
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
        p+=WTK_XVPRINT_MAX_NAME_LEN;
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
                pos=ftell(file);
                fseek(file,offset+head_len+sizeof(int),SEEK_SET);
                pos2=ftell(file);
                fseek(file,0L,SEEK_END);
                feat_len=ftell(file)-pos2;
                fseek(file,pos,SEEK_SET);
                // printf("feat_len0 %d\n",feat_len);
            }else
            {
                feat_len=wtk_xvprint_file_get_offset(file,p);
                feat_len=feat_len-offset;
                // printf("feat_len1 %d\n",feat_len);
            }
            node->v=wtk_vecf_new(feat_len);
            fseek(file,offset+head_len+sizeof(int),SEEK_SET);
            ret = fread(node->v->p,feat_len,1,file);
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
    if(file)
    {
        fclose(file);
    }
    return node;
}
