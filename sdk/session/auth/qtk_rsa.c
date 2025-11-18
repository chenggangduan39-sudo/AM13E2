#include "qtk_rsa.h"

#ifdef USE_MBEDTLS
#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"

qtk_rsa_t* qtk_rsa_new(char  *pubkey,char *passwd)
{
    qtk_rsa_t *r;
    int ret;
    const char *pers = "qdreamer_mbedtls_pk_encrypt";

    r = (qtk_rsa_t *)wtk_malloc(sizeof(qtk_rsa_t));
    mbedtls_pk_init(&r->pk);
    mbedtls_ctr_drbg_init(&r->ctr_drbg);
    mbedtls_entropy_init(&r->entropy);
    mbedtls_pk_parse_public_key(&r->pk, (const unsigned char *)pubkey,
                                strlen(pubkey) + 1);

    if ((ret = mbedtls_ctr_drbg_seed(&r->ctr_drbg, mbedtls_entropy_func,
                                     &r->entropy, (const unsigned char *)pers,
                                     strlen(pers))) != 0) {
        wtk_debug(" failed\n  ! mbedtls_ctr_drbg_seed returned -0x%04x\n",
                  (unsigned int)-ret);
        goto end;
    }

    r->buf = wtk_strbuf_new(256, 0);
    ret = 0;
end:
    if (ret == -1) {
        qtk_rsa_delete(r);
        r = NULL;
    }
    return r;
}

void qtk_rsa_delete(qtk_rsa_t *r)
{
    mbedtls_pk_free(&r->pk);
    mbedtls_ctr_drbg_free(&r->ctr_drbg);
    mbedtls_entropy_free(&r->entropy);
    if(r->buf){
        wtk_strbuf_delete(r->buf);
    }
    wtk_free(r);
}

wtk_strbuf_t * qtk_rsa_encrypt(qtk_rsa_t *r,char *data,int len)
{
	wtk_strbuf_t *buf=r->buf;
	int ret;
        size_t olen = 256;

        ret = mbedtls_pk_encrypt(&r->pk, (const unsigned char *)data, len,
                                 (unsigned char *)buf->data, &olen, buf->length,
                                 mbedtls_ctr_drbg_random, &r->ctr_drbg);
        //wtk_debug("ret=%d errcode=%ld.\n",ret,ERR_get_error());
        if (ret != 0) {
        wtk_debug("failed. %d\n", ret);
        buf->pos = 0;
        } else {
        // wtk_debug("success.\n");
        buf->pos = olen;
        }
        //wtk_debug("internal [%d] [%s].\n",ret,buf->data);
	return buf;
}

/*
 * @func 比较rdata是和
 *	@param	rdata 没有做RSA加密的内容
 *	@param ddata 已经使用RSA私钥加密的内容
 *	@return: 验证成功返回1,失败返回0
 */
int qtk_rsa_verify(qtk_rsa_t *r,char *rdata,int rlen,char *ddata,int dlen)
{
	int ret;
        ret = mbedtls_pk_verify(&r->pk, MBEDTLS_MD_SHA1,
                                (const unsigned char *)rdata, rlen,
                                (const unsigned char *)ddata, dlen);
        return ret == 0;
}
#elif USE_OPENSSL
qtk_rsa_t* qtk_rsa_new(char  *pubkey,char *passwd)
{
	qtk_rsa_t *r;
	BIO *bio=NULL;
	int ret;

	r=(qtk_rsa_t*)wtk_malloc(sizeof(qtk_rsa_t));
	r->rsa=NULL;
	//wtk_debug("%s.\n",pubkey);
	//wtk_debug("%s.\n",passwd);
	OpenSSL_add_all_ciphers();
	bio=BIO_new_mem_buf(pubkey,-1);
	if(bio == NULL)
	{
		ERR_print_errors_fp(stdout);
		ret=-1;goto end;
	}
	r->rsa=RSA_new();
	if(r->rsa == NULL)
	{
		ERR_print_errors_fp(stdout);
		ret=-1;goto end;
	}
	PEM_read_bio_RSAPublicKey(bio,&(r->rsa),NULL,passwd);
//	printf("\n");
//	RSA_print_fp(stdout,r->rsa,16);
//	printf("\n");
//	wtk_debug("%p\n",r->rsa);
	r->rsa_len=RSA_size(r->rsa);
	r->buf=wtk_strbuf_new(r->rsa_len,0);
	ret=0;
end:
	if(bio)
	{
		BIO_free_all(bio);
	}
	if(ret==-1)
	{
		qtk_rsa_delete(r);
		r=NULL;
	}
	return r;
}

void qtk_rsa_delete(qtk_rsa_t *r)
{
    if(r->rsa){
        EVP_cleanup();
        RSA_free(r->rsa);
        ERR_free_strings();
        CRYPTO_cleanup_all_ex_data();
    }
    if(r->buf){
        wtk_strbuf_delete(r->buf);
    }
    wtk_free(r);
}

wtk_strbuf_t * qtk_rsa_encrypt(qtk_rsa_t *r,char *data,int len)
{
	wtk_strbuf_t *buf=r->buf;
	int ret;

	//wtk_debug("%d.\n",len);
	//wtk_debug("%s.\n",data);
	//wtk_debug("rsalen %d  unencrypt %s  %d.\n",r->rsa_len,data,len);

	ret=RSA_public_encrypt(len,(unsigned char*)data,(unsigned char*)buf->data,r->rsa,RSA_PKCS1_PADDING);
	//wtk_debug("ret=%d errcode=%ld.\n",ret,ERR_get_error());
	if(ret<0)
	{
		//wtk_debug("failed.\n");
		buf->pos=0;
	}else{
		//wtk_debug("success.\n");
		buf->pos=ret;
	}
	//wtk_debug("internal [%d] [%s].\n",ret,buf->data);
	return buf;
}

/*
 * @func 比较rdata是和
 *	@param	rdata 没有做RSA加密的内容
 *	@param ddata 已经使用RSA私钥加密的内容
 *	@return: 验证成功返回1,失败返回0
 */
int qtk_rsa_verify(qtk_rsa_t *r,char *rdata,int rlen,char *ddata,int dlen)
{
	int ret;

	ret=RSA_verify(NID_sha1,(const unsigned char*)rdata,rlen,(const unsigned char*)ddata,dlen,r->rsa);
	if(ret==0)
	{
		ERR_print_errors_fp(stdout);
	}
	return ret;
}
#else
qtk_rsa_t* qtk_rsa_new(char *pubkey,char *passwd)
{
    return NULL;
}

wtk_strbuf_t* qtk_rsa_encrypt(qtk_rsa_t *r,char *data,int len)
{
    return NULL;
}

int qtk_rsa_verify(qtk_rsa_t *r,char *rdata,int rlen,char *ddata,int dlen)
{
    return 0;
}

void qtk_rsa_delete(qtk_rsa_t *r)
{

}

#endif