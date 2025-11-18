/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP)
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_hash.h
#  Version   : $Id$
#  Created by Wilson Zhao (pengcheng.zhao@aispeech.com)
#
#  COPYRIGHT (C) AISpeech Ltd.
=============================================================================*/


#ifndef _SIMP_CORE_HASH_H_INCLUDED_
#define _SIMP_CORE_HASH_H_INCLUDED_

#include "simp_type.h"
#include "simp_mem.h"

#ifdef _DEBUG
#define HASH_ERR(S, ...)            fprintf(stderr, "[%d] ", time(NULL));fprintf(stderr, S, __VA_ARGS__)
#define HASH_LOG(S, ...)            printf(S, __VA_ARGS__)
#else
#define HASH_ERR(s)
#define HASH_LOG(s)
#endif//_DEBUG

#define SIMP_HASH_OFFSET            0
#define SIMP_HASH_A                 1
#define SIMP_HASH_B                 2

#define SIMP_COPY_DATA              1
#define SIMP_NON_COPY_DATA          0

#define HASH_P(HASH_TABLE, S)       simp_hash_get_pos(HASH_TABLE, S)

typedef struct simp_hash_table_item simp_hash_table_item_t;
typedef simp_hash_table_item_t*     simp_hash_table_item_tp;
struct  simp_hash_table_item {
    unsigned long                   n_hash_index_a;
    unsigned long                   n_hash_index_b;

    BOOL                            b_is_occupied;

    char*                           p_hash_string;
    void*                           p_data;
    unsigned long                   n_data_len;
};

typedef struct simp_hash_table      simp_hash_table_t;
typedef simp_hash_table_t*          simp_hash_table_tp;
struct  simp_hash_table {
    simp_hash_table_item_tp*        p_hash_items_list;
    unsigned long                   ul_hash_items;
    int                             b_copy;
};

#ifdef __cplusplus
extern "C"
{
#endif

BOOL                simp_hash_set_val(simp_hash_table_tp p_hash_table, char* p_string, void* p_data, unsigned long n_data_size);
BOOL                simp_hash_table_destroy(simp_hash_table_tp p_hash_table);
simp_hash_table_tp  simp_hash_table_new(unsigned long ul_table_size, int b_copy);
unsigned long       simp_hash_get_pos(simp_hash_table_tp p_hash_table, char* p_string);
unsigned long       simp_hash_get_val_by_key(simp_hash_table_tp p_hash_table, char* p_string, void** p_data);
unsigned long       simp_hash_get_val_by_pos(simp_hash_table_tp p_hash_table, unsigned long n_hash_pos, void** p_data);
unsigned long       simp_hash_get_item_by_key(simp_hash_table_tp p_hash_table, char* p_string, simp_hash_table_item_t** pp_item);
unsigned long       simp_hash_get_item_by_pos(simp_hash_table_tp p_hash_table, unsigned long n_hash_pos, simp_hash_table_item_t** pp_item);
unsigned long       simp_hash_string(unsigned long ul_hash_type, char* p_string);
void                simp_hash_print(simp_hash_table_tp p_hash_table);


#ifdef __cplusplus
};
#endif

#endif//_SIMP_CORE_HASH_H_INCLUDED_
