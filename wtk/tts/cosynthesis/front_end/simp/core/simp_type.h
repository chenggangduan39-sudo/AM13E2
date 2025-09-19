/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP)
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_type.h
#  Version   : $Id$
#  Created by Wilson Zhao (pengcheng.zhao@aispeech.com)
#
#  COPYRIGHT (C) AISpeech Ltd.
=============================================================================*/

#ifndef _SIMP_CORE_TYPE_H_INCLUDED_
#define _SIMP_CORE_TYPE_H_INCLUDED_

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef TRUE
#define TRUE (1==1)
#endif

#ifndef FALSE
#define FALSE (1==0)
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

#ifndef BOOL
typedef int BOOL;
#endif

typedef struct simp_argv            simp_argv_t;
typedef simp_argv_t*                simp_argv_tp;
struct simp_argv{
    int                             argc;
    void**                          argv;
};

typedef void   (* simp_void_handle_t)(void* obj, ...);
typedef BOOL   (* simp_bool_handle_t)(void* obj, ...);
typedef int    (* simp_int_handle_t)(void* obj, ...);
typedef char*  (* simp_string_handle_t)(void* obj, ...);
typedef void*  (* simp_block_handle_t)(void* obj, ...);

#endif//_SIMP_CORE_TYPE_H_INCLUDED_
