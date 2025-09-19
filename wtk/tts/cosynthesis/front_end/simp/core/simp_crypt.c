/*=============================================================================
#  Project Name: SIMP Interface, Model, Processor (SIMP) 
# -----------------------------------------------------------------------------
#  File Path : /SIMP/core/simp_crypt.c
#  Version   : $Id$
#  Created by Wilson Zhao (pengcheng.zhao@aispeech.com)
#
#  COPYRIGHT (C) AISpeech Ltd.
=============================================================================*/

#include "simp_crypt.h"

#define MAX_CRYPT_TYPE 2

static simp_encrypt_handle_t encrypt_handle[MAX_CRYPT_TYPE] = {
    simp_crypt_normal,
    simp_crypt_reverse,
};

static simp_decrypt_handle_t decrypt_handle[MAX_CRYPT_TYPE] = {
    simp_crypt_normal,
    simp_crypt_reverse,
};

char*
simp_crypt(char *input, int len, int n_crypt_type, BOOL b_encrypt)
{
    if (n_crypt_type >= MAX_CRYPT_TYPE && n_crypt_type < 0)
    {
        goto end;
    }

    if (TRUE == b_encrypt)
    {
        if (NULL == encrypt_handle[n_crypt_type])
        {
            goto end;
        }
        else
        {
            input = encrypt_handle[n_crypt_type](input, len);
        }
    }
    else
    {
        if (NULL == decrypt_handle[n_crypt_type])
        {
            goto end;
        }
        else
        {
            input = decrypt_handle[n_crypt_type](input, len);
        }
    }

end:
    return input;
}

char*
simp_crypt_reverse(char *input, int len)
{
    int idx = 0;
    char c = 0;
    for (idx = 0; idx < len; idx++)
    {
        c = input[idx];
        input[idx] = ~(char)c;
    }

    return input;
}

char*
simp_crypt_normal(char *input, int len)
{
    return input;
}