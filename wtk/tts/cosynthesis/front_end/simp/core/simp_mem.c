/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP)
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_mem.c
#  Version   : $Id$
#  Created by Wilson Zhao (pengcheng.zhao@aispeech.com)
#
#  COPYRIGHT (C) AISpeech Ltd.
=============================================================================*/

#include "simp_type.h"
#include "simp_mem.h"

static int simp_malloc_cnt = 0;

void*
simp_malloc(unsigned long n_mem_size)
{
    simp_malloc_cnt ++;
    // TODO: Make a pool
    return (void *)malloc(n_mem_size);
}

void*
simp_calloc(size_t n_num_mem_elements, size_t n_size_of_mem_element)
{
    simp_malloc_cnt ++;
    // TODO: Make a pool
    return (void *)calloc(n_num_mem_elements, n_size_of_mem_element);
}

void*
simp_realloc(void *p_mem_block, size_t n_size_of_mem_element)
{
    if (NULL == p_mem_block)
    {
        simp_malloc_cnt ++;
    }
    return (void *)realloc(p_mem_block, n_size_of_mem_element);
}

void
simp_free(void* p_mem_block)
{
    // TODO: Make a pool 
    if (NULL != p_mem_block)
    {
        free(p_mem_block);
        simp_malloc_cnt --;
    }
}

void
simp_debug()
{
    // TODO: SIMPLE WAY TO DEBUG MEMORY LEAK
    if (simp_malloc_cnt != 0)
    {
        //SIMP_MEM_ERR("Memory leaks found. %d blocks are leaked.\n", simp_malloc_cnt);
    }
    else
    {
        //SIMP_MEM_LOG("Memory leak checked, it seems all OK.\n");
    }
}
