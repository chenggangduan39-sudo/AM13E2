#include "qtk_jwt.h"

static void base64url_encode(const uint8_t *input, uint64_t len, char *result) {
    base64_encode(input, len, result);
    int i;
    for (i = strlen(result) - 1; i >= 0 && result[i] == '='; i--) {
        result[i] = '\0';
    }

    for (i = strlen(result); i >= 0; i--) {
        if (result[i] == '+') {
            result[i] = '-';
        }
        if (result[i] == '/') {
            result[i] = '_';
        }
    }
}

void qtk_jwt_token_get(wtk_strbuf_t *token, const char *header, int hlen,
                       const char *payload, int plen, const char *key,
                       int klen) {
    char hdr_base64[512] = {0};
    char pid_base64[512] = {0};
    char sign_base64[512] = {0};

    uint8_t hmac[SHA256_DIGEST_LENGTH] = {0};

    wtk_strbuf_reset(token);
    base64url_encode((const uint8_t *)header, hlen, hdr_base64);
    base64url_encode((const uint8_t *)payload, plen, pid_base64);
    wtk_strbuf_push_f(token, "%s.%s", hdr_base64, pid_base64);
    hmac_sha256(hmac, key, klen, token->data, token->pos);
    base64url_encode((const uint8_t *)hmac, SHA256_DIGEST_LENGTH, sign_base64);
    wtk_strbuf_push_f(token, ".%s", sign_base64);
    wtk_strbuf_push_c(token, 0);
    --token->pos;
}