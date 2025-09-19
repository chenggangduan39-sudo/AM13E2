/* public domain sha256 implementation based on fips180-3 */
#ifndef _QTK_CORE_HMAC256_SHA256_H_
#define _QTK_CORE_HMAC256_SHA256_H_

#include "wtk/core/wtk_type.h"

#ifdef __cplusplus
extern "C" {
#endif
struct sha256 {
    uint64_t len;    /* processed message length */
    uint32_t h[8];   /* hash state */
    uint8_t buf[64]; /* message block buffer */
};

#define SHA256_DIGEST_LENGTH 32
#define SHA256_HEX_DIGEST_LENGTH 64

// enum {
//	SHA256_DIGEST_LENGTH = 32,
//	SHA256_HEX_DIGEST_LENGTH = 64,
// };

/* reset state */
void sha256_init(void *ctx);
/* process message */
void sha256_update(void *ctx, const void *m, unsigned long len);
/* get message digest */
/* state is ruined after sum, keep a copy if multiple sum is needed */
/* part of the message might be left in s, zero it if secrecy is needed */
void sha256_sum(void *ctx, uint8_t md[SHA256_DIGEST_LENGTH]);

void hmac_sha256(uint8_t *hmac, const char *skey, int klen, const char *content,
                 int clen);
void hmac_sha256_hex(char *hmac_hex, char *key, int klen, char *content,
                     int clen);

#ifdef __cplusplus
};
#endif
#endif
