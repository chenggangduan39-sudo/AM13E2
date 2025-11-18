#ifndef QTK_AUTH_QTK_RSA
#define QTK_AUTH_QTK_RSA
#ifdef USE_MBEDTLS
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/pk.h"
#endif
#ifdef USE_OPENSSL
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/engine.h>
#include <openssl/evp.h>
#endif
#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_rsa qtk_rsa_t;
struct qtk_rsa
{
#ifdef USE_MBEDTLS
    mbedtls_pk_context pk;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
#endif
#ifdef USE_OPENSSL
    RSA *rsa;
#endif
    wtk_strbuf_t *buf;
    int rsa_len;
};

qtk_rsa_t* qtk_rsa_new(char *pubkey,char *passwd);
wtk_strbuf_t* qtk_rsa_encrypt(qtk_rsa_t *r,char *data,int len);
int qtk_rsa_verify(qtk_rsa_t *r,char *rdata,int rlen,char *ddata,int dlen);
void qtk_rsa_delete(qtk_rsa_t *r);

#ifdef __cplusplus
};
#endif
#endif
