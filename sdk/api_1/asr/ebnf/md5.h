/**
 *  $Id: md5.h,v 1.2 2006/03/03 15:04:49 tomas Exp $
 *  Cryptographic module for Lua.
 *  @author  Roberto Ierusalimschy
 */

#ifndef md5_h
#define md5_h

#ifdef __cplusplus
extern "C" {
#endif

#define HASHSIZE 16

/**
 *  md5 hash function.
 *  @param message: aribtary string.
 *  @param len: message length.
 *  @param output: buffer to receive the hash value. Its size must be
 *  (at least) HASHSIZE.
 */
void md5(const char *message, long len, char *output);
void md5_hex(const char *message, long len, char *output);

#ifdef __cplusplus
};
#endif
#endif
