/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP) 
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_hash_file.h
#  Version   : $Id$
#  Created by Wilson Zhao (pengcheng.zhao@aispeech.com)
#
#  COPYRIGHT (C) AISpeech Ltd.
=============================================================================*/

#ifndef _SIMP_CORE_HASH_FILE_H_INCLUDED_
#define _SIMP_CORE_HASH_FILE_H_INCLUDED_

#include "simp_type.h"
#include "simp_hash.h"
#include "simp_crypt.h"

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef _DEBUG
#define SIMP_HASH_FILE_ERR(S, ...)          fprintf(stderr, "[%d]", time(NULL));fprintf(stderr, S, __VA_ARGS__)
#define SIMP_HASH_FILE_LOG(S, ...)          printf(S, __VA_ARGS__)
#else
#define SIMP_HASH_FILE_ERR
#define SIMP_HASH_FILE_LOG
#endif//_DEBUG

#define simp_hash_bin_file_handler_find     simp_block_handle_t
#define simp_hash_bin_file_handler_extract  simp_bool_handle_t
#define simp_hash_bin_file_handler_list     simp_void_handle_t
#define simp_hash_bin_file_handler_add      simp_bool_handle_t
#define simp_hash_bin_file_handler_dump     simp_bool_handle_t

#define simp_hash_file_handler_encrypt      simp_block_handle_t
#define simp_hash_file_handler_decrypt      simp_block_handle_t

typedef struct simp_bin_file_item   simp_bin_file_item_t;
typedef simp_bin_file_item_t*       simp_bin_file_item_tp;
struct  simp_bin_file_item {
    int                             n_file_name_len;
    char                            p_file_name[260];
    unsigned int                    n_file_size;
    char*                           p_data;
};

typedef struct simp_bin_file        simp_bin_file_t;
typedef simp_bin_file_t*            simp_bin_file_tp;
struct  simp_bin_file {
    int                             n_hash_bin_version;
    int                             b_crypted;
    short                           n_crypted_type;
    int                             n_file_cnt;

    // file list in bin source
    simp_bin_file_item_t**          pp_file_list;
};

typedef struct simp_hash_file       simp_hash_file_t;
typedef simp_hash_file_t*           simp_hash_file_tp;
struct  simp_hash_file {
    simp_hash_table_t*              p_table;
    simp_bin_file_t*                p_bin_file;
};

simp_hash_file_t*   simp_hash_file_new(int n_file_cnt);
BOOL                simp_hash_file_init(simp_hash_file_t* p_hash_file, char* p_bin_file_name);
BOOL                simp_hash_file_destroy(simp_hash_file_t* p_hash_file);
BOOL                simp_hash_file_add(simp_hash_file_t* p_hash_file, char* p_file_name);
BOOL                simp_hash_file_delete(simp_hash_file_t* p_hash_file, char* p_file_name); // TODO 删除bin文件中的某个文件
BOOL                simp_hash_file_dump_to_bin(simp_hash_file_t* p_hash_file, char* p_file_name); // TODO 导出为bin文件
void                simp_hash_file_list(simp_hash_file_t* p_hash_file);
char*               simp_hash_file_find(simp_hash_file_t* p_hash_file, char* p_file_name);

#ifdef __cplusplus
};
#endif
#endif//_SIMP_CORE_HASH_FILE_H_INCLUDED_