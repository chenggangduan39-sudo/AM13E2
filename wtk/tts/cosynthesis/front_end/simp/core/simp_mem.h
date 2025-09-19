/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP)
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_mem.h
#  Version   : $Id$
#  Created by Wilson Zhao (pengcheng.zhao@aispeech.com)
#
#  COPYRIGHT (C) AISpeech Ltd.
=============================================================================*/


#ifndef _SIMP_CORE_MEM_H_INCLUDED_
#define _SIMP_CORE_MEM_H_INCLUDED_

#ifdef _DEBUG
#define SIMP_MEM_ERR(S, ...)        fprintf(stderr, "[%d] ", time(NULL));fprintf(stderr, S, __VA_ARGS__)
#define SIMP_MEM_LOG(S, ...)        printf(S, __VA_ARGS__)
#else
#define SIMP_MEM_ERR
#define SIMP_MEM_LOG
#endif//_DEBUG

#define simp_memcpy                 memcpy
#define simp_memcpy_s(DIST, SRC)    memcpy(DIST, SRC, strlen(SRC) + 1)

typedef struct simp_mem_pool        simp_mem_pool_t;
typedef simp_mem_pool_t            *simp_mem_pool_tp;
struct  simp_mem_pool{
    void                           *val;
};

#ifdef __cplusplus
extern "C" {
#endif
void*   simp_malloc(unsigned long n_mem_size);
void*   simp_calloc(size_t n_num_mem_elements, size_t n_size_of_mem_element);
void*   simp_realloc(void *p_mem_block, size_t n_size_of_mem_element);
void    simp_free(void* p_mem_block);
void    simp_debug();
#ifdef __cplusplus
};
#endif

#endif//_SIMP_CORE_MEM_H_INCLUDED_
