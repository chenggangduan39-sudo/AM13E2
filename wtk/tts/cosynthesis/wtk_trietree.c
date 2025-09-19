#include "wtk_trietree.h"

wtk_trieroot_t *wtk_trietree_new()
{
    wtk_trieroot_t *root;
    root = (wtk_trieroot_t*)wtk_malloc(sizeof(wtk_trienode_t));
    root->child_q = wtk_queue_new();
    root->unit_id = NULL;
    root->nunit = 0;
    return root;
}

void wtk_trietree_node_delete(wtk_trienode_t *node)
{
    wtk_trienode_t *next_node;
    wtk_queue_node_t *qn;
    while(1)
    {
        qn=wtk_queue_pop(node->child_q);
        if(!qn)break;
        next_node = data_offset2(qn,wtk_trienode_t,q_n);
        wtk_trietree_node_delete(next_node);
    }
    if(!node->is_leaf)
    {
        wtk_free(node->unit_id);
    }
    wtk_queue_delete(node->child_q);
    wtk_free(node);    
}

void wtk_trietree_delete(wtk_trieroot_t *root)
{
    wtk_queue_node_t *qn;
    wtk_trienode_t *node;
    while(1)
    {
        qn=wtk_queue_pop(root->child_q);
        if(!qn)break;
        node = data_offset2(qn,wtk_trienode_t,q_n);
        wtk_trietree_node_delete(node);
    }
    wtk_queue_delete(root->child_q);
    wtk_free(root->unit_id);
    wtk_free(root);
}

wtk_trienode_t *wtk_trietree_node_create(char* s,int slen,int is_leaf,uint16_t* unit, uint16_t nunit)
{
    wtk_trienode_t *node;
    node = (wtk_trienode_t*)wtk_malloc(sizeof(wtk_trienode_t));
    node->sname = s;
    node->sname_len = slen;
    node->child_q = wtk_queue_new();
    // wtk_debug("%.*s,%d\n",slen,s,slen);
    node->is_leaf = is_leaf;
    if(is_leaf)
    {
        // node->child_q=NULL;
        node->nunit = nunit;
        node->unit_id = unit;
    }else
    {
        // node->child_q = wtk_queue_new();
        node->nunit=0;
        node->unit_id=NULL;
    }
    return node;
}

wtk_trienode_t *wtk_trietree_node_seek(wtk_queue_t *q,char *s,int len)
{
    // int ret;
    wtk_queue_node_t *qn;
    wtk_trienode_t *node;

    //wtk_debug("%.*s\n",len,s);
    for (qn = q->pop; qn; qn=qn->next)
    {
        node = data_offset2(qn,wtk_trienode_t,q_n);
        // wtk_debug("%.*s, %.*s\n",len,s,node->sname_len,node->sname);
        if(node->sname_len == len)
        {
        	if (!wtk_data_cmp(node->sname,node->sname_len,s,len))
            {
                goto end;
            }            
        }
    }
    node = NULL;
end:
    return node;
}

int compare_u16num(const void *x,const void *y )
{
    return (*(uint16_t*)x - *(uint16_t*)y);
}

uint16_t *wtk_trietree_merged_delete_dup(uint16_t* unit,int len,int *l)
{
    uint16_t prev;
    int i,j=0;
    uint16_t *res,*merge;

    res = (uint16_t *)wtk_malloc(sizeof(uint16_t)*(len));
    memset(res,0,sizeof(uint16_t)*(len));
    qsort(unit,len,sizeof(uint16_t),compare_u16num);
    for (i = 0; i < len;)
    {
        if (i == 0) prev = unit[i++];
        res[j++] = prev;
        while (i < len && unit[i] == prev) ++i;
        if (i < len)
        {
            prev = unit[i];
        }
    }
    *l = j;
    merge = (uint16_t *)wtk_malloc(sizeof(uint16_t)*j);
    memcpy(merge,res,sizeof(uint16_t)*j);

    wtk_free(res);
    return merge;
}

int wtk_trietree_units_merge(uint16_t **unit,uint16_t *unit1,int nunit1,uint16_t *unit2,int nunit2)
{
    uint16_t* unit3 = NULL;
    int len;
    int nunit3 = nunit1 + nunit2;
    uint16_t* merge = NULL;

    unit3 = (uint16_t*)wtk_malloc(sizeof(uint16_t)*nunit3);
    memcpy(unit3,unit1,sizeof(uint16_t)*nunit1);
    memcpy(unit3 + nunit1,unit2,sizeof(uint16_t)*nunit2);
    merge = wtk_trietree_merged_delete_dup(unit3,nunit3,&len);
    wtk_free(unit3);
    *unit = merge;

    return len;
}

void wtk_trietree_root_update(wtk_trieroot_t *root)
{
    wtk_queue_t *q = root->child_q;
    wtk_queue_node_t *qn;
    wtk_trienode_t *node;
    uint16_t *tmp_units;

    for (qn = q->pop; qn; qn=qn->next)
    {
        node = data_offset2(qn,wtk_trienode_t,q_n);
        if(root->unit_id)
        {
            root->nunit = wtk_trietree_units_merge(&tmp_units,root->unit_id,root->nunit,node->unit_id,node->nunit);
            wtk_free(root->unit_id);
            root->unit_id = tmp_units;
        }else
        {
            root->nunit = node->nunit;
            root->unit_id  = (uint16_t*)wtk_malloc(root->nunit*sizeof(uint16_t));
            memcpy(root->unit_id,node->unit_id,sizeof(short)*root->nunit);
        }
    }
}

void wtk_trietree_node_update(wtk_trienode_t *node)
{
    wtk_queue_t *q = node->child_q;
    wtk_queue_node_t *qn;
    wtk_trienode_t *next_node;
    uint16_t *tmp_units;

    for (qn = q->pop; qn; qn=qn->next)
    {
        next_node = data_offset2(qn,wtk_trienode_t,q_n);
        if(next_node->is_leaf==1)
        {
            if(node->unit_id)
            {   
                node->nunit = wtk_trietree_units_merge(&tmp_units,node->unit_id,node->nunit,next_node->unit_id,next_node->nunit);
                wtk_free(node->unit_id);
                node->unit_id = tmp_units;
            }else
            {
                node->nunit = next_node->nunit;
                node->unit_id  = (uint16_t*)wtk_malloc(node->nunit*sizeof(uint16_t));
                memcpy(node->unit_id,next_node->unit_id,sizeof(short)*node->nunit);
            }
        }else
        {            
            wtk_trietree_node_update(next_node);
            if(node->unit_id)
            {
                node->nunit = wtk_trietree_units_merge(&tmp_units,node->unit_id,node->nunit,next_node->unit_id,next_node->nunit);
                wtk_free(node->unit_id);
                node->unit_id = tmp_units;
            }else
            {
                node->nunit = next_node->nunit;
                node->unit_id  = (uint16_t*)wtk_malloc(node->nunit*sizeof(uint16_t));
                memcpy(node->unit_id,next_node->unit_id,sizeof(short)*node->nunit);
            }            
        }
    }
}

void wtk_trietree_update(wtk_trieroot_t *root)
{
    wtk_queue_t *q = root->child_q;
    wtk_queue_node_t *qn;
    wtk_trienode_t *node;
    for (qn = q->pop; qn; qn=qn->next)
    {
        node = data_offset2(qn,wtk_trienode_t,q_n);
        wtk_trietree_node_update(node);
    }
    wtk_trietree_root_update(root);
}

uint16_t *wtk_trietree_unit_get_root(wtk_trieroot_t *root,uint16_t *nunit)
{
    *nunit = root->nunit;
    return root->unit_id;
}

uint16_t *wtk_trietree_unit_get_any(wtk_trieroot_t *root,char *s,int *len,uint16_t *nunit,int deep)
{
    int cur=0;
    wtk_trienode_t *node;
    // wtk_queue_node_t *qn;
    // wtk_queue_t *q;
    wtk_trienode_t *next_node;
    char* sname = s;
    uint16_t *unit_ids=NULL;
    int dl = deep;
    node = wtk_trietree_node_seek(root->child_q,sname,len[cur]);
    if(!node)
    {
        //printf("node doesn't exist\n");
        goto end;
    }else
    {
        if(node->is_leaf )//|| deep==1)
        {
            *nunit = node->nunit;
            unit_ids = node->unit_id;
            goto end;
        }else
        {   
            while(dl)
            {
                sname +=len[cur];
                cur++;
                // q = node->child_q;
                next_node = wtk_trietree_node_seek(node->child_q,sname,len[cur]);
                if(!next_node)
                {
                    //printf("node doesn't exist\n");
                    goto end;
                }
                dl--;
                node = next_node;
            }
            if(node)
            {
                *nunit = node->nunit;
                unit_ids = node->unit_id;
                goto end;
            }else
            {
                //printf("node doesn't exist\n");
                goto end;                
            }
        }
    }
end:
    return unit_ids;
}

uint16_t *wtk_trietree_unit_get(wtk_trieroot_t *root,char *s,int *len,int size,uint16_t *nunit)
{
    int cur=0;
    wtk_trienode_t *node;
    // wtk_queue_node_t *qn;
    // wtk_queue_t *q;
    wtk_trienode_t *next_node;
    uint16_t *unit_ids=NULL;
    char *sname=s;

    node = wtk_trietree_node_seek(root->child_q,sname,len[cur]);
    if(!node)
    {
        //printf("node doesn't exist\n");
        goto end;
    }else
    {
        if(node->is_leaf)
        {
            *nunit = node->nunit;
            unit_ids = node->unit_id;
            goto end;
        }else
        {   
        	//wtk_debug("sname=%.*s\n", len[cur], sname);
        	if (cur >= size) goto end;
            sname+=len[cur];
            cur++;
            while(node && !node->is_leaf)
            {
            	if (cur >= size) goto end;
                next_node=wtk_trietree_node_seek(node->child_q,sname,len[cur]);
                if(!node)
                {
                    //printf("node doesn't exist\n");
                    goto end;
                }else
                {
                	//wtk_debug("sname=%.*s\n", len[cur], sname);
                    sname+=len[cur];
                    cur++;
                    node = next_node;
                }

            }
            if (node)
            {
                *nunit = node->nunit;
                unit_ids = node->unit_id;
            }
        }
    }
end:
    return unit_ids;
}

void wtk_trietree_root_insert(wtk_trieroot_t *root,char *s,int *len,uint16_t* unit, uint16_t nunit,int deep, int shift)
{
    wtk_trienode_t *child;
    wtk_trienode_t *next_child;
    int is_leaf=0;
    int cur=0;
    char *sname = s+shift;
    int sname_distance=0;

    if(deep==1)
    {
        is_leaf=1;
    }

    if(root->child_q->length>0)
    {   
        child = wtk_trietree_node_seek(root->child_q,sname,len[cur]);
        if(!child)
        {
            child = wtk_trietree_node_create(sname,len[cur],is_leaf,unit,nunit);
            wtk_queue_push(root->child_q,&(child->q_n));
        }
    }else
    {
        child = wtk_trietree_node_create(sname,len[cur],is_leaf,unit,nunit);
        wtk_queue_push(root->child_q,&(child->q_n));
    }
    deep--;
    while (deep)
    {
        sname_distance = len[cur] + shift;
        sname += sname_distance;
        cur++;
        // wtk_debug("cur=%.*s\n",len[cur],sname);
        if(deep==1)is_leaf=1;
        if(child->child_q->length>0)
        {
            next_child = wtk_trietree_node_seek(child->child_q,sname,len[cur]);
            if(!next_child)
            {
                next_child = wtk_trietree_node_create(sname,len[cur],is_leaf,unit,nunit);
                wtk_queue_push(child->child_q,&(next_child->q_n));
            }else
            {
                child = next_child;
                deep--;
                continue;
            }
        }else
        {
            next_child = wtk_trietree_node_create(sname,len[cur],is_leaf,unit,nunit);
            wtk_queue_push(child->child_q,&(next_child->q_n));
        }
        child = next_child;
        deep--;
    }
}

int wtk_trietree_readitem(wtk_heap_t* heap, FILE *fp, wtk_trieroot_t **root, int wrd_idx, wtk_trieitem_t* item, int update)
{
    uint16_t idx,deep;
    char *s, *res;
    int slen[4];
    int i,j;
    uint16_t nunit;
    uint16_t *units;

	if (NULL == root[wrd_idx])
	{
		root[wrd_idx] = wtk_trietree_new();
	}
    fseek(fp, item->start, SEEK_SET);
    res=(char*)wtk_heap_malloc(heap, item->len);
    fread(res, item->len, 1, fp);
    for (i=0; i< item->num; i++)
    {
        idx = *((uint16_t*)res);
        res+=sizeof(uint16_t);
        deep = *((uint8_t*)res);
        res+=sizeof(uint8_t);
        s=(char*)(res);
        for(j=0;j<deep;++j)
        {
            slen[j] = *((uint8_t*)res);
            res+=sizeof(uint8_t);
            res+=slen[j];
        }
        nunit = *((uint16_t*)res);
        res+=sizeof(uint16_t);
        units = (uint16_t*)res;
        res+=nunit*sizeof(uint16_t);
        wtk_trietree_root_insert(root[idx],s,slen,units,nunit,deep, sizeof(uint8_t)/sizeof(char));
    }
    if (update)
    {
    	wtk_trietree_update(root[idx]);
    }

    return 0;
}
