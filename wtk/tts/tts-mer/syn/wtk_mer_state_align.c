#include "wtk_mer_state_align.h"

void wtk_mer_lab_split_call(wtk_strbuf_t **str_arr, char *item, int len, int index)
{
    // printf("%d \n", index);
    wtk_strbuf_t *str;
    str_arr[0]->pos = index + 1; /* 标记数组长度 */
    str = str_arr[index+1];
    if (index == 2)
    {
        wtk_strbuf_reset(str);
        wtk_strbuf_push(str, item, len);
        str->data[len] = '\0';
        str->pos = len;
    } else {
        /* pos 存放帧数 frame_num */
        char buf[20];
        memcpy(buf, item, len);
        buf[len]='\0';
        str->pos = atoi(buf);
        // printf("str->pos: %d \n", str->pos);
    }
    // wtk_debug("str_plit_f [%.*s] \n", str->len, str->data);
};

static void pattern_match(wtk_stridx_t *s, wtk_vecf_t *vec, const char *lab, const int line)
{
    wtk_string_t *str;
    regex_t reg;
    regmatch_t pmatch[2];
    const int is_disc = line == -1? 0: 1;
    int len = s->used
      , i
      , status = 1
      , rs_len = 0;
    char pattern[100]
       , rs[5];
    
    for (i=0; i<len; i++)
    {
        str = wtk_stridx_get_str(s, i);
        memcpy(pattern, str->data, str->len);
        pattern[str->len] = '\0';
        if(regcomp( &reg, pattern, REG_EXTENDED) !=0)
        {
            printf("%d: , pattern: %s, plen: %d  ----> Cannot regex compile!\n", i, pattern, str->len);
            exit( -1);
        };
        status = regexec( &reg, lab, 2, pmatch, 0);
        // if (line == 1449) { wtk_debug("status: %d \n lab: %s\n pattern: %s \n", status, lab, pattern);exit(1);}
        if (status == 0)
        {
            rs_len = pmatch[1].rm_eo - pmatch[1].rm_so;
            memcpy(rs, lab + pmatch[1].rm_so, rs_len);
            rs[ rs_len] = '\0';
            // wtk_debug("%s:%d 结果: %d, 长度: %d \n", pattern, i, atoi(rs), rs_len);
            if (is_disc)
            {
                vec->p[line] = 1.0;
                regfree(&reg);
                break;
            } else {  
                vec->p[i] = atof(rs);
            }
        } else
        {
            if (is_disc)
            {
                vec->p[line] = 0.0;
            } else { 
                vec->p[i] = -1.0; 
            }
        }
        regfree(&reg);
    }
}


static void pattern_match_bin(wtk_larray_t *larr, wtk_vecf_t *vec, const char *lab)
{
    int i
      , size = larr->nslot;
    wtk_stridx_t **idx = larr->slot;
    for (i=0; i<size; i++)
    {
        pattern_match( idx[i], vec, lab, i);
    }
}

static void pattern_match_continous_pos(wtk_stridx_t *s, wtk_vecf_t *vec, const char *lab)
{
    pattern_match(s, vec, lab, -1);
}

static wtk_matf_t* remove_silence(wtk_matf_t *m, int arr[], int arr_len)
{
#ifdef USE_DEBUG
    if (arr_len == 0)
    {
        printf("\n\n\n WARNING:  没有任何匹配的静音, 可能是静音标识不匹配 \n\n\n");
    }
#endif
    int col = m->col
      , row = m->row
      , flag = 1
      , nrow = 0
      , i
      , j
      , index;
    float *p;
    float *dstp;
    wtk_matf_t *dst = wtk_matf_new(row - arr_len, col);
    for (i=0; i<row; i++)
    {
        for (j=0; j<arr_len; j++)
        {
            index = arr[j];
            if (index == i) {flag=0;break;}
        }
        if (flag == 0) {flag=1;continue;}
        p = wtk_matf_row(m, i);
        dstp = wtk_matf_row(dst, nrow);
        memcpy(dstp, p, sizeof(float)*col);
        nrow++;
    }
    return dst;
}

int wtk_mer_check_sil( char *lab)
{
    int arr_len = 1
      , i
      , is_sil = 0;
    char *parr[] = {"-pau+"};
    // char *parr[] = {"-sil+"};
    char *p = NULL;

    for (i=0; i<arr_len; i++)
    {
        p = strstr(lab, parr[i]);
        if (p!=NULL) {is_sil=1; goto end;}
    }
    end:
        return is_sil;
}

static void calc_dur( wtk_strbuf_t ***lab_arrs, int lab_len, int *total_dur, int *max_frame_num)
{
    int i, total=0, m=0, dur=0;
    wtk_strbuf_t **str_arr;
    for (i=0; i<lab_len; ++i)
    {
        str_arr = lab_arrs[i];
        dur = str_arr[2]->pos - str_arr[1]->pos;
        total += dur;
        if (dur>m) {m=dur;}
    }
    *total_dur = total;
    *max_frame_num = m;
};

wtk_matf_t* wtk_mer_state_align(wtk_mer_syn_qes_t *qes, char *subphn_feat, wtk_strbuf_t ***lab_arrs, int lab_len)
{
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_stridx_t *continous = qes->continous;
    wtk_larray_t *discrete_larr = qes->discrete_larr;
    int discrete_size = discrete_larr->nslot
      , contin_size = continous->used
      , frame_fea_size
      , fea_size
      , dimension
      , total_dur
      , max_frame_num;
    fea_size = discrete_size + contin_size;

    if (wtk_str_equal_s(subphn_feat, strlen(subphn_feat), "none"))
    {
        frame_fea_size = 0;
    } else if (wtk_str_equal_s(subphn_feat, strlen(subphn_feat), "full"))
    {
        frame_fea_size = 9;
    } else
    {
        wtk_debug("未定义的 subphn_feat: %s \n", subphn_feat);
        wtk_exit(1);
    }
    
    dimension = fea_size + frame_fea_size;
    calc_dur(lab_arrs, lab_len, &total_dur, &max_frame_num);
    // wtk_debug("total_dur: %d  max_frame_num: %d \n", total_dur, max_frame_num);

    wtk_matf_t 
        *lab_feature_matf = wtk_matf_heap_new( heap, total_dur, dimension),
        *cur_block_bin_matf = wtk_matf_heap_new( heap, max_frame_num, dimension),
        *lab_feature_matf2;
    wtk_matf_zero( lab_feature_matf);

    wtk_vecf_t 
        *lab_bin_vec,
        *lab_continous_vec,
        *lab_vec = NULL;
    wtk_strbuf_t **str_arr;
    char full_lab[512];
    int i
      , arr_len /* str_arr[0]->pos */
      , line_i /* 行号 */
      , sil_size = 1000
      , sil_arr[sil_size]
      , sil_index = 0
      , is_silence = 0;

    int /* start_time
      , end_time
      ,  */frame_num /* 帧数 */
      , state_index
      , state_number = 5
      , phn_dur = 0
      , state_index_backward
      , state_duration_base = 0
      , base_frame_i = 0
      , lab_feature_i = 0;

    for (line_i=0; line_i<lab_len; line_i++)
    {
        str_arr = lab_arrs[line_i];
        arr_len = str_arr[0]->pos;

        if ( arr_len == 1)
        {
            wtk_debug("lab未标注时间");
            wtk_exit(1);
            // frame_num = 0;
            // state_index = 1;
            // memcpy( full_lab, str_arr[1]->data, str_arr[1]->pos);
            // full_lab[ str_arr[1]->pos] = '\0';
        } else 
        {
            int full_lab_len = str_arr[3]->pos - 3;
            // printf("line_i: %d full_lab_len: %d\n", line_i, full_lab_len);

            // start_time = str_arr[1]->pos;
            // end_time = str_arr[2]->pos;
            frame_num = str_arr[2]->pos - str_arr[1]->pos;
            state_index = str_arr[3]->data[ full_lab_len + 1] - '0' - 1;
            state_index_backward = 6 - state_index;

            memcpy( full_lab, str_arr[3]->data, full_lab_len);
            full_lab[ full_lab_len] = '\0';
        };

        // wtk_debug( "start_time -> [%d] [%d] [%d] [%s] state_index %d \n", start_time, end_time, frame_num, full_lab, state_index_backward);
        if (state_index == 1)
        {
            lab_bin_vec = wtk_vecf_heap_new( heap, discrete_size);
            lab_continous_vec = wtk_vecf_heap_new( heap, contin_size);
            phn_dur = frame_num;
            state_duration_base = 0;

            pattern_match_bin( discrete_larr, lab_bin_vec, full_lab);
            pattern_match_continous_pos( continous, lab_continous_vec, full_lab);
            // wtk_vecf_print( lab_bin_vec);
            // wtk_vecf_print( lab_continous_vec);
            lab_vec = wtk_vecf_heap_new( heap, lab_bin_vec->len+lab_continous_vec->len);
            wtk_vecf_concat( lab_vec, lab_bin_vec, lab_continous_vec);
            // wtk_vecf_print( lab_vec);
            // exit(1);
            if (arr_len == 1)
            {
                state_index = state_number;
            } else 
            {
                for (i=0; i<state_number-1; i++)
                {
                    str_arr = lab_arrs[line_i + i + 1];
                    // printf("%.*s \n", line->len, line->data);
                    phn_dur += str_arr[2]->pos - str_arr[1]->pos;
                }
            }
        }
        // printf("state_index %d  state_number %d line %d \n", state_index, state_number, line_i);

        is_silence = wtk_mer_check_sil( full_lab);
        // printf("lab_feature_i: %d \n", lab_feature_i);
        // printf("frame_num: %d \n", frame_num);
        if ( lab_feature_i > lab_feature_matf->row) 
        {/* safe check */
            wtk_debug("程序异常, lab_feature_i %d 下标超出矩阵 lab_feature_matf 范围 %d \n", lab_feature_i, lab_feature_matf->row);
            wtk_exit(1);
        }
        if (frame_num > cur_block_bin_matf->row)
        {/* safe check
         */
            wtk_debug("frame_num 大小 %d 超出 cur_block_bin_matf 定义 \n", frame_num);
            wtk_exit(1);
        }
        if (wtk_str_equal_s(subphn_feat, strlen(subphn_feat), "full"))
        {/*
            add_frame_features is true
            python if self.subphone_feats == 'full'
        */
            int i, j;
            float *p, *lab_p;

            // cur_block_bin_matf->row = frame_num;
            for (i=0; i<frame_num; ++i)
            {
                if (is_silence)
                {
                    if (sil_index > sil_size) {wtk_debug("sil_size过小,下标溢出\n");wtk_exit(1);}
                    sil_arr[ sil_index] = base_frame_i;
                    sil_index++;
                }
                base_frame_i++;
                p = wtk_matf_row(cur_block_bin_matf, i);
                memcpy(p, lab_vec->p, lab_vec->len*sizeof(float));
                float arr[] = {
                    (i+1.0) / frame_num,
                    (frame_num - i + 0.0) / frame_num,
                    frame_num,
                    state_index,
                    state_index_backward,
                    phn_dur,
                    (frame_num + 0.0) / phn_dur,
                    (phn_dur - i - state_duration_base  + 0.0) / phn_dur,
                    (state_duration_base + i + 1 + 0.0) / phn_dur,
                };
                for (j=0; j<9; ++j)
                {
                    p[ fea_size + j] = arr[j];
                }
            }
            lab_p = wtk_matf_row(lab_feature_matf, lab_feature_i);
            memcpy(lab_p, cur_block_bin_matf->p, dimension*frame_num*sizeof(float));
            lab_feature_i += frame_num;
        }
        else if ( wtk_str_equal_s(subphn_feat, strlen(subphn_feat), "none") && state_index == state_number)
        {/*python  elif self.subphone_feats == 'none' and state_index == state_number: */
            float *p;
            int col = lab_feature_matf -> col
              , i;
            if (is_silence)
            {
                if (sil_index > sil_size) {wtk_debug("sil_size过小,下标溢出\n");wtk_exit(1);}
                sil_arr[ sil_index] = base_frame_i;
                sil_index++;
            }
            base_frame_i++;

            p = wtk_matf_row( lab_feature_matf, lab_feature_i);
            for (i=0; i<col; i++)
            {
                p[i] = lab_vec->p[i];
            }
            lab_feature_i++;
        }
        state_duration_base += frame_num;
    }

    if (wtk_str_equal_s(subphn_feat, strlen(subphn_feat), "none"))
    {
        lab_feature_matf2 = remove_silence(lab_feature_matf, sil_arr, sil_index);
    } else 
    {
        lab_feature_matf2 = wtk_matf_row_slice(lab_feature_matf, 0, lab_feature_matf->row);
    }
    // wtk_mer_matf_write_file(lab_feature_matf2, "56.lab", 0);
    // printf("lab_feature_i: %d \n", lab_feature_i);
    wtk_heap_delete(heap);
    return lab_feature_matf2;
}


void wtk_mer_dur_to_lab(wtk_matf_t *dur_matf, wtk_strbuf_t ***lab_arrs, int lab_len)
{/* 时长预测矩阵转lab */
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_strbuf_t **str_arr;
    int /* i
      ,  */line_i
      , arr_len /* str_arr[0]->pos */
      , is_silence = 0
      , cur_index = 0;

    int start_time
      , end_time
      , state_index
      , state_number = 5
      , cur_state_dur
      , pred_state_dur = 0
      , prev_end_time = 0;

    for (line_i=0; line_i<lab_len; ++line_i)
    {
        str_arr = lab_arrs[line_i];
        arr_len = str_arr[0]->pos;

        if (arr_len == 1)
        {
            wtk_debug("lab未标注时间");
            wtk_exit(1);
            // start_time = 0;
            // end_time = 600000;
            // memcpy( full_lab, str_arr[1]->data, str_arr[1]->pos);
            // full_lab[ str_arr[1]->pos] = '\0';
        } else 
        {/* 这步生成的fulllab 和 labels_with_state_alignment 有一点点区别 */
            int full_lab_len = str_arr[3]->pos - 3;

            start_time = str_arr[1]->pos;
            end_time = str_arr[2]->pos;
            state_index = str_arr[3]->data[ full_lab_len + 1] - '0' - 1;
        }

        is_silence = wtk_mer_check_sil( str_arr[3]->data);

        if (arr_len == 1)
        {
            wtk_debug("lab未包含时间信息");
            wtk_exit(1);
            // for (state_index=1; state_index < state_number+1; ++state_index)
            // {
            //     if (is_silence)
            //     {
            //         cur_state_dur = end_time - start_time;
            //     } else
            //     {
            //         float *p = wtk_matf_row(dur_matf, cur_index);
            //         pred_state_dur = (int)p[state_index-1];
            //         cur_state_dur = pred_state_dur*5*10000;
            //     }
            // }
        } else 
        {
            if (is_silence)
            {
                cur_state_dur = end_time - start_time;
            } else
            {
                float *p = wtk_matf_row(dur_matf, cur_index);
                pred_state_dur = (int)p[state_index-1];
                cur_state_dur = state_index>4?pred_state_dur+1:pred_state_dur;
            }

            str_arr[1]->pos = prev_end_time;
            str_arr[2]->pos = prev_end_time+cur_state_dur;
            prev_end_time += cur_state_dur;
        }
        if ( state_index == state_number && !is_silence) 
        { cur_index++; }
    }
    wtk_heap_delete(heap);
}
