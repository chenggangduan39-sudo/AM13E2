#ifndef QTK_CORE_JWT_QTK_JWT
#define QTK_CORE_JWT_QTK_JWT

#include "../hmac256/sha256.h"
#include "base64encode.h"

#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

void qtk_jwt_token_get(wtk_strbuf_t *token, const char *header, int hlen,
                       const char *payload, int plen, const char *key,
                       int klen);

#ifdef __cplusplus
};
#endif
#endif