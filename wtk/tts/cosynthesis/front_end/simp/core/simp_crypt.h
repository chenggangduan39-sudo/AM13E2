/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP) 
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_crypt.h
#  Version   : $Id$
#  Created by Wilson Zhao (pengcheng.zhao@aispeech.com)
#
#  COPYRIGHT (C) AISpeech Ltd.
=============================================================================*/

#ifndef _SIMP_CORE_CRYPT_H_INCLUDED_
#define _SIMP_CORE_CRYPT_H_INCLUDED_

#include "simp_type.h"

typedef enum{
    SIMP_CRYPT_NORMAL = 0,
    SIMP_CRYPT_REVERSE,
} SIMP_CRYPT_TYPE;

typedef char *(*simp_encrypt_handle_t)(char *input, int len);
typedef char *(*simp_decrypt_handle_t)(char *input, int len);

char *simp_crypt(char *input, int len, int n_crypt_type, BOOL b_encrypt);
char *simp_crypt_normal(char *input, int len);
char *simp_crypt_reverse(char *input, int len);

#endif//_SIMP_CORE_CRYPT_H_INCLUDED_
