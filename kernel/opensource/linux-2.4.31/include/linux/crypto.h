/*
 * Scatterlist Cryptographic API.
 *
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 * Copyright (c) 2002 David S. Miller (davem@redhat.com)
 *
 * Portions derived from Cryptoapi, by Alexander Kjeldaas <astor@fast.no>
 * and Nettle, by Niels Möller.
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 */

#ifndef _LINUX_CRYPTO_H
#define _LINUX_CRYPTO_H

#include <linux/autoconf.h>
#ifndef CONFIG_CRYPTO_INCA
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/string.h>
#include <asm/page.h>
#include <asm/errno.h>

/*
 * Algorithm masks and types.
 */
#define CRYPTO_ALG_TYPE_MASK		0x000000ff
#define CRYPTO_ALG_TYPE_CIPHER		0x00000001
#define CRYPTO_ALG_TYPE_DIGEST		0x00000002
#define CRYPTO_ALG_TYPE_COMPRESS	0x00000004

/*
 * Transform masks and values (for crt_flags).
 */
#define CRYPTO_TFM_MODE_MASK		0x000000ff
#define CRYPTO_TFM_REQ_MASK		0x000fff00
#define CRYPTO_TFM_RES_MASK		0xfff00000

#define CRYPTO_TFM_MODE_ECB		0x00000001
#define CRYPTO_TFM_MODE_CBC		0x00000002
#define CRYPTO_TFM_MODE_CFB		0x00000004
#define CRYPTO_TFM_MODE_CTR		0x00000008
#define CRYPTO_TFM_MODE_OFB		0x00000010

#define CRYPTO_TFM_REQ_WEAK_KEY		0x00000100
#define CRYPTO_TFM_RES_WEAK_KEY		0x00100000
#define CRYPTO_TFM_RES_BAD_KEY_LEN   	0x00200000
#define CRYPTO_TFM_RES_BAD_KEY_SCHED 	0x00400000
#define CRYPTO_TFM_RES_BAD_BLOCK_LEN 	0x00800000
#define CRYPTO_TFM_RES_BAD_FLAGS 	0x01000000

/*
 * Miscellaneous stuff.
 */
#define CRYPTO_UNSPEC			0
#define CRYPTO_MAX_ALG_NAME		64

#define CRYPTO_DIR_ENCRYPT		1
#define CRYPTO_DIR_DECRYPT		0

#define CRYPTO_PREF_GENERIC		0
#define CRYPTO_PREF_OPTIMIZED		1
#define CRYPTO_PREF_HARDWARE		2

struct scatterlist;

/*
 * Algorithms: modular crypto algorithm implementations, managed
 * via crypto_register_alg() and crypto_unregister_alg().
 */
struct cipher_alg {
	unsigned int cia_min_keysize;
	unsigned int cia_max_keysize;
	int (*cia_setkey)(void *ctx, const u8 *key,
	                  unsigned int keylen, u32 *flags);
	void (*cia_encrypt)(void *ctx, u8 *dst, const u8 *src);
	void (*cia_decrypt)(void *ctx, u8 *dst, const u8 *src);
	size_t cia_max_nbytes;
	size_t cia_req_align;
	void (*cia_ecb)(void *ctx, u8 *dst, const u8 *src, u8 *iv,
			size_t nbytes, int encdec, int inplace);
	void (*cia_cbc)(void *ctx, u8 *dst, const u8 *src, u8 *iv,
			size_t nbytes, int encdec, int inplace);
	void (*cia_cfb)(void *ctx, u8 *dst, const u8 *src, u8 *iv,
			size_t nbytes, int encdec, int inplace);
	void (*cia_ofb)(void *ctx, u8 *dst, const u8 *src, u8 *iv,
			size_t nbytes, int encdec, int inplace);
	void (*cia_ctr)(void *ctx, u8 *dst, const u8 *src, u8 *iv,
			size_t nbytes, int encdec, int inplace);
};

struct digest_alg {
	unsigned int dia_digestsize;
	void (*dia_init)(void *ctx);
	void (*dia_update)(void *ctx, const u8 *data, unsigned int len);
	void (*dia_final)(void *ctx, u8 *out);
	int (*dia_setkey)(void *ctx, const u8 *key,
	                  unsigned int keylen, u32 *flags);
};

struct compress_alg {
	int (*coa_init)(void *ctx);
	void (*coa_exit)(void *ctx);
	int (*coa_compress)(void *ctx, const u8 *src, unsigned int slen,
	                    u8 *dst, unsigned int *dlen);
	int (*coa_decompress)(void *ctx, const u8 *src, unsigned int slen,
	                      u8 *dst, unsigned int *dlen);
};

#define cra_cipher	cra_u.cipher
#define cra_digest	cra_u.digest
#define cra_compress	cra_u.compress

struct crypto_alg {
	struct list_head cra_list;
	u32 cra_flags;
	unsigned int cra_blocksize;
	unsigned int cra_ctxsize;
	const char cra_name[CRYPTO_MAX_ALG_NAME];
	unsigned int cra_preference;

	union {
		struct cipher_alg cipher;
		struct digest_alg digest;
		struct compress_alg compress;
	} cra_u;
	
	struct module *cra_module;
};

/*
 * Algorithm registration interface.
 */
int crypto_register_alg(struct crypto_alg *alg);
int crypto_unregister_alg(struct crypto_alg *alg);

/*
 * Algorithm query interface.
 */
int crypto_alg_available(const char *name, u32 flags);

/*
 * Helper function.
 */
void *crypto_aligned_kmalloc (size_t size, int mode, size_t alignment, void **index);

/*
 * Transforms: user-instantiated objects which encapsulate algorithms
 * and core processing logic.  Managed via crypto_alloc_tfm() and
 * crypto_free_tfm(), as well as the various helpers below.
 */
struct crypto_tfm;

struct cipher_tfm {
	void *cit_iv;
	unsigned int cit_ivsize;
	u32 cit_mode;
	int (*cit_setkey)(struct crypto_tfm *tfm,
	                  const u8 *key, unsigned int keylen);
	int (*cit_encrypt)(struct crypto_tfm *tfm,
			   struct scatterlist *dst,
			   struct scatterlist *src,
			   unsigned int nbytes);
	int (*cit_encrypt_iv)(struct crypto_tfm *tfm,
	                      struct scatterlist *dst,
	                      struct scatterlist *src,
	                      unsigned int nbytes, u8 *iv);
	int (*cit_decrypt)(struct crypto_tfm *tfm,
			   struct scatterlist *dst,
			   struct scatterlist *src,
			   unsigned int nbytes);
	int (*cit_decrypt_iv)(struct crypto_tfm *tfm,
			   struct scatterlist *dst,
			   struct scatterlist *src,
			   unsigned int nbytes, u8 *iv);
	void (*cit_xor_block)(u8 *dst, const u8 *src);
};

struct digest_tfm {
	void (*dit_init)(struct crypto_tfm *tfm);
	void (*dit_update)(struct crypto_tfm *tfm,
	                   struct scatterlist *sg, unsigned int nsg);
	void (*dit_final)(struct crypto_tfm *tfm, u8 *out);
	void (*dit_digest)(struct crypto_tfm *tfm, struct scatterlist *sg,
	                   unsigned int nsg, u8 *out);
	int (*dit_setkey)(struct crypto_tfm *tfm,
	                  const u8 *key, unsigned int keylen);
#ifdef CONFIG_CRYPTO_HMAC
	void *dit_hmac_block;
#endif
};

struct compress_tfm {
	int (*cot_compress)(struct crypto_tfm *tfm,
	                    const u8 *src, unsigned int slen,
	                    u8 *dst, unsigned int *dlen);
	int (*cot_decompress)(struct crypto_tfm *tfm,
	                      const u8 *src, unsigned int slen,
	                      u8 *dst, unsigned int *dlen);
};

#define crt_cipher	crt_u.cipher
#define crt_digest	crt_u.digest
#define crt_compress	crt_u.compress

struct crypto_tfm {

	u32 crt_flags;
	
	union {
		struct cipher_tfm cipher;
		struct digest_tfm digest;
		struct compress_tfm compress;
	} crt_u;
	
	struct crypto_alg *__crt_alg;
};

/* 
 * Transform user interface.
 */
 
/*
 * crypto_alloc_tfm() will first attempt to locate an already loaded algorithm.
 * If that fails and the kernel supports dynamically loadable modules, it
 * will then attempt to load a module of the same name or alias.  A refcount
 * is grabbed on the algorithm which is then associated with the new transform.
 *
 * crypto_free_tfm() frees up the transform and any associated resources,
 * then drops the refcount on the associated algorithm.
 */
struct crypto_tfm *crypto_alloc_tfm(const char *alg_name, u32 tfm_flags);
void crypto_free_tfm(struct crypto_tfm *tfm);

/*
 * Transform helpers which query the underlying algorithm.
 */
static inline const char *crypto_tfm_alg_name(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_name;
}

static inline const char *crypto_tfm_alg_modname(struct crypto_tfm *tfm)
{
	struct crypto_alg *alg = tfm->__crt_alg;
	
	if (alg->cra_module)
		return alg->cra_module->name;
	else
		return NULL;
}

static inline u32 crypto_tfm_alg_type(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_flags & CRYPTO_ALG_TYPE_MASK;
}

static inline unsigned int crypto_tfm_alg_min_keysize(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->__crt_alg->cra_cipher.cia_min_keysize;
}

static inline unsigned int crypto_tfm_alg_max_keysize(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->__crt_alg->cra_cipher.cia_max_keysize;
}

static inline unsigned int crypto_tfm_alg_ivsize(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_ivsize;
}

static inline unsigned int crypto_tfm_alg_blocksize(struct crypto_tfm *tfm)
{
	return tfm->__crt_alg->cra_blocksize;
}

static inline unsigned int crypto_tfm_alg_digestsize(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	return tfm->__crt_alg->cra_digest.dia_digestsize;
}

static inline unsigned int crypto_tfm_alg_max_nbytes(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->__crt_alg->cra_cipher.cia_max_nbytes;
}

static inline unsigned int crypto_tfm_alg_req_align(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->__crt_alg->cra_cipher.cia_req_align;
}

/*
 * API wrappers.
 */
static inline void crypto_digest_init(struct crypto_tfm *tfm)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->crt_digest.dit_init(tfm);
}

static inline void crypto_digest_update(struct crypto_tfm *tfm,
                                        struct scatterlist *sg,
                                        unsigned int nsg)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->crt_digest.dit_update(tfm, sg, nsg);
}

static inline void crypto_digest_final(struct crypto_tfm *tfm, u8 *out)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->crt_digest.dit_final(tfm, out);
}

static inline void crypto_digest_digest(struct crypto_tfm *tfm,
                                        struct scatterlist *sg,
                                        unsigned int nsg, u8 *out)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	tfm->crt_digest.dit_digest(tfm, sg, nsg, out);
}

static inline int crypto_digest_setkey(struct crypto_tfm *tfm,
                                       const u8 *key, unsigned int keylen)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_DIGEST);
	if (tfm->crt_digest.dit_setkey == NULL)
		return -ENOSYS;
	return tfm->crt_digest.dit_setkey(tfm, key, keylen);
}

static inline int crypto_cipher_setkey(struct crypto_tfm *tfm,
                                       const u8 *key, unsigned int keylen)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_setkey(tfm, key, keylen);
}

static inline int crypto_cipher_encrypt(struct crypto_tfm *tfm,
                                        struct scatterlist *dst,
                                        struct scatterlist *src,
                                        unsigned int nbytes)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_encrypt(tfm, dst, src, nbytes);
}                                        

static inline int crypto_cipher_encrypt_iv(struct crypto_tfm *tfm,
                                           struct scatterlist *dst,
                                           struct scatterlist *src,
                                           unsigned int nbytes, u8 *iv)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	BUG_ON(tfm->crt_cipher.cit_mode == CRYPTO_TFM_MODE_ECB);
	return tfm->crt_cipher.cit_encrypt_iv(tfm, dst, src, nbytes, iv);
}                                        

static inline int crypto_cipher_decrypt(struct crypto_tfm *tfm,
                                        struct scatterlist *dst,
                                        struct scatterlist *src,
                                        unsigned int nbytes)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	return tfm->crt_cipher.cit_decrypt(tfm, dst, src, nbytes);
}

static inline int crypto_cipher_decrypt_iv(struct crypto_tfm *tfm,
                                           struct scatterlist *dst,
                                           struct scatterlist *src,
                                           unsigned int nbytes, u8 *iv)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	BUG_ON(tfm->crt_cipher.cit_mode == CRYPTO_TFM_MODE_ECB);
	return tfm->crt_cipher.cit_decrypt_iv(tfm, dst, src, nbytes, iv);
}

static inline void crypto_cipher_set_iv(struct crypto_tfm *tfm,
                                        const u8 *src, unsigned int len)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	memcpy(tfm->crt_cipher.cit_iv, src, len);
}

static inline void crypto_cipher_get_iv(struct crypto_tfm *tfm,
                                        u8 *dst, unsigned int len)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_CIPHER);
	memcpy(dst, tfm->crt_cipher.cit_iv, len);
}

static inline int crypto_comp_compress(struct crypto_tfm *tfm,
                                       const u8 *src, unsigned int slen,
                                       u8 *dst, unsigned int *dlen)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_COMPRESS);
	return tfm->crt_compress.cot_compress(tfm, src, slen, dst, dlen);
}

static inline int crypto_comp_decompress(struct crypto_tfm *tfm,
                                         const u8 *src, unsigned int slen,
                                         u8 *dst, unsigned int *dlen)
{
	BUG_ON(crypto_tfm_alg_type(tfm) != CRYPTO_ALG_TYPE_COMPRESS);
	return tfm->crt_compress.cot_decompress(tfm, src, slen, dst, dlen);
}

/*
 * HMAC support.
 */
#ifdef CONFIG_CRYPTO_HMAC
void crypto_hmac_init(struct crypto_tfm *tfm, u8 *key, unsigned int *keylen);
void crypto_hmac_update(struct crypto_tfm *tfm,
                        struct scatterlist *sg, unsigned int nsg);
void crypto_hmac_final(struct crypto_tfm *tfm, u8 *key,
                       unsigned int *keylen, u8 *out);
void crypto_hmac(struct crypto_tfm *tfm, u8 *key, unsigned int *keylen,
                 struct scatterlist *sg, unsigned int nsg, u8 *out);
#endif	/* CONFIG_CRYPTO_HMAC */

#endif	/* CONFIG_CRYPTO_STANDARD */

/* else ifdef CONFIG_CRYPTO */
#ifdef CONFIG_CRYPTO_INCA

/*
 * include/linux/crypto.h
 *
 * Written by Alexander Kjeldaas <astor@fast.no> 1998-10-13
 *
 * Copyright 1998 by Alexander Kjeldaas. This code is licensed under
 * an X11-like license.  See LICENSE.crypto for details.
 *
 */

/*
 * API version tag
 */
#define CRYPTO_API_VERSION_CODE 0x000100
#define CRYPTO_API_VERSION(major,minor,micro) \
 (((major) << 16) + ((minor) << 8) + (micro))

/* some id constants */
#define TRANSFORM_DIGEST 0
#define TRANSFORM_CIPHER 1
#define MAX_TRANSFORM    2

#define CIPHER_MODES       0xFFFF0000 /* used to mask the mode *\
                                       * part of the cipher id */
#define CIPHER_MODE_ECB    0x00000000
#define CIPHER_MODE_CBC    0x00010000
#define CIPHER_MODE_CFB    0x00020000
#define CIPHER_MODE_CTR    0x00040000
#define CIPHER_MODE_RTC    0x00080000

/* Allowed keysizes: This is just a set of commonly found values. If
 * you need additional ones, you can place them here. Note that
 * CIPHER_KEY_ANY really means _any_ key length (that is a multiple of
 * 8 bits, just limited by MAX_KEY_SIZE*32. This is not important now,
 * but might become so if we choose to support keylengths greater than
 * 256 bits. There are many ciphers that can take keys that are longer
 * (e.g. blowfish: 448 bits). If you want to say all key lengths up to
 * 256, play safe and use 0xFFFFFFFF-1 as keysize_mask.
 * CIPHER_KEYSIZE_NONE means that the cipher does not expect a key. It
 * is only used for 'none' encryption.
 */

#define CIPHER_KEYSIZE_ANY  0xFFFFFFFF
#define CIPHER_KEYSIZE_NONE 0x00000000

#define CIPHER_KEYSIZE_40   0x00000010
#define CIPHER_KEYSIZE_56   0x00000040
#define CIPHER_KEYSIZE_64   0x00000080
#define CIPHER_KEYSIZE_80   0x00000200
#define CIPHER_KEYSIZE_96   0x00000800
#define CIPHER_KEYSIZE_112  0x00002000
#define CIPHER_KEYSIZE_128  0x00008000
#define CIPHER_KEYSIZE_160  0x00080000
#define CIPHER_KEYSIZE_168  0x00100000
#define CIPHER_KEYSIZE_192  0x00800000
#define CIPHER_KEYSIZE_256  0x80000000

/*
 * ioctl's for cryptoloop.c
 */
#define CRYPTOLOOP_SET_DEBUG      0X4CFD
#define CRYPTOLOOP_SET_BLKSIZE    0X4CFC

#ifdef __KERNEL__

#include <linux/version.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <asm/page.h>
#include <asm/semaphore.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,3,0)
# ifndef __exit
#  define __exit
# endif
#endif

/* A transform group is a group of transforms that behave in a similar
 * fashion */

struct transform_group {
        int tg_id;
        char *tg_name;  /* "cipher" or "digest" */
        rwlock_t tg_lock;
        struct list_head *tg_head;
#ifdef CONFIG_PROC_FS
        struct proc_dir_entry *tg_proc_parent_dir;
        int (*read_proc)(char *page, char **start, off_t off,
                         int count, int *eof, void *data);
#endif
};

/* A transform is something that can be found by id or name. Ciphers
   and digests are types of transforms. */

struct transform_implementation {
        struct list_head t_list;
        int t_flags;
        char *t_name;
        int t_atomicapi;
        struct transform_group *t_group;
#ifdef CONFIG_PROC_FS
        /* keep track of the allocated proc_dir_entry */
        struct proc_dir_entry *t_proc;
#endif
};

/* Cipher data structures */

struct cipher_context;

typedef int (*cipher_trans_proc)(struct cipher_context *cx, const u8 *in, u8 *out,
                                 int size);

typedef int (*cipher_trans_proc_iv)(struct cipher_context *cx, const u8 *in, u8 *out,
                                 int size, const u8 *iv);

struct cipher_implementation {
        struct transform_implementation trans;
        int blocksize;         /* in bytes */
        int ivsize;            /* in bytes */
        int key_schedule_size; /* in bytes. 0 if the schedule size is
                                  variable. */
        u32 key_size_mask;     /* bit 0 set = 8 bit, ... ,
                                * bit 31 set = 256 bit */

        /*
         * Encrypt the plaintext pointed to by "in".  Write output to
         * "out". Size of plaintext is "size".  Output buffer must be
         * able to hold "size" bytes plus padding necessary to make it
         * a multiple of the cipher blocksize. size <= 0 is
         * undefined. Returns 0 on success, non-zero on
         * failure.
         *
         * encrypt        - Function might sleep. This function will
         *                  always exist.

         * encrypt_atomic - Will never sleep + always SW implementation.
         *                  This function will exist if atomicapi==1 was
         *                  set in find_cipher_by_name where this
         *                  cipher_implementation was returned.
         */
        cipher_trans_proc encrypt;
        cipher_trans_proc encrypt_atomic;

        cipher_trans_proc_iv encrypt_iv;
        cipher_trans_proc_iv encrypt_atomic_iv;

        /*
         * Decrypt the ciphertext pointed to by "in".  Write output to
         * "out". Size of plaintext is "size".  Input buffer is "size"
         * bytes plus padding necessary to make it a multiple of the
         * cipher blocksize. size <= 0 is undefined. Returns 0 on
         * success, non-zero on failure.
         *
         */
        cipher_trans_proc decrypt;
        cipher_trans_proc decrypt_atomic;

        cipher_trans_proc_iv decrypt_iv;
        cipher_trans_proc_iv decrypt_atomic_iv;

        /*
         *
         */

        int (*set_key)(struct cipher_context *cx,
                       const u8 *key, int key_len);

        int (*set_key_atomic)(struct cipher_context *cx,
                              const u8 *key, int key_len);

        /* The following functions are optional.  Most ciphers will
         * not need to specify them, and a default implementation will
         * be used.
         */

        /* Realloc a cipher_context that can hold the key schedule for
         * a given key.  If no cipher_context is given, allocate a new
         * cipher_context.
         */

        struct cipher_context *
        (*realloc_context)(struct cipher_context *old_cx,
                           struct cipher_implementation *ci,
                           int max_key_len);
        void (*wipe_context)(struct cipher_context *cx);
        void (*free_context)(struct cipher_context *cx);

        /* lock and unlock manage the module use counts */
        void (*lock)(void);
        void (*unlock)(void);

        /* The following functions are used by software
         * implementations that wish to provide a single function that
         * implements atomic and non-atomic versions of encrypt,
         * decrypt, and set_key. If these functions are set,
         * register_cipher will provide generic implementations of
         * encrypt*, decrypt*, and set_key*.  However these functions
         * should not be called directly by users since they will only
         * exist for software-based cipher implementations.  */

        int (*_encrypt) (struct cipher_context *cx,
                         const u8 *in, u8 *out, int size, int atomic,
                         const u8 *iv);
        int (*_decrypt)(struct cipher_context *cx,
                        const u8 *in, u8 *out, int size, int atomic,
                        const u8 *iv);
        int (*_set_key)(struct cipher_context *cx,
                        const u8 *key, int key_len, int atomic);

};

#define MAX_IV_SIZE  32              /* 32 byte - 256 bit */

struct cipher_context {
        struct cipher_implementation *ci;
        int may_sleep;      /* cipher implementation (sw) might sleep */
        int keyinfo_size;   /* in bytes - usually equal to
                             * ci->key_schedule_size. */
        u32 *keyinfo;
        int key_length;     /* in bytes */
        u8 iv[MAX_IV_SIZE]; /* static IV */
};


/* Digest data structures */

struct digest_context;
struct digest_implementation {
        struct transform_implementation trans;
        int blocksize;           /* in bytes */
        int working_size;        /* in bytes */

        /* Open / initialize / reset the digest */
        int (*open)(struct digest_context *cx);
        int (*open_atomic)(struct digest_context *cx);

        /* Add more data to the digest */
        int (*update)(struct digest_context *cx, const u8 *in, int size);
        int (*update_atomic)(struct digest_context *cx, const u8 *in, int size);

        /* Calculate the digest, but make it possible to add futher data
         * using update(). Normally, you should use close() instead. */
        int (*digest)(struct digest_context *cx, u8 *out);
        int (*digest_atomic)(struct digest_context *cx, u8 *out);

        /* Add any needed padding, and calculate the digest. */
        int (*close)(struct digest_context *cx, u8 *out);
        int (*close_atomic)(struct digest_context *cx, u8 *out);

        /* Calculate the digest HMAC. */
        int (*hmac)(struct digest_context *cx, const u8 *key, int key_len, const u8 *in, int size, u8 *hmac);
        int (*hmac_atomic)(struct digest_context *cx, const u8 *key, int key_len, const u8 *in, int size, u8 *hmac);

        struct digest_context *(*realloc_context)(struct digest_context *old_cx, struct digest_implementation *ci);
        void (*free_context)(struct digest_context *cx);

        /* lock and unlock manage the module use counts */
        void (*lock)(void);
        void (*unlock)(void);

        /* hook for software implemented digests */
        int (*_open)(struct digest_context *cx, int atomic);
        int (*_update)(struct digest_context *cx, const u8 *in, int size, int atomic);
        int (*_digest)(struct digest_context *cx, u8 *out, int atomic);
        int (*_close)(struct digest_context *cx, u8 *out, int atomic);
        int (*_hmac)(struct digest_context *cx, const u8 *key, int key_len, const u8 *in, int size, u8 *hmac, int atomic);
};

struct digest_context {
        struct digest_implementation *di;
        u32 *digest_info;
};


struct transform_implementation *find_transform_by_name(const char *name,
                                                        int tgroup,
                                                        int atomicapi);

static inline struct cipher_implementation *
find_cipher_by_name(const char *name, int atomicapi)
{
        return (struct cipher_implementation *)
                find_transform_by_name(name, TRANSFORM_CIPHER, atomicapi);
}

static inline struct digest_implementation *
find_digest_by_name(const char *name, int atomicapi)
{
        return (struct digest_implementation *)
                find_transform_by_name(name, TRANSFORM_DIGEST, atomicapi);
}

int register_transform(struct transform_implementation *ti, int tgroup);
int register_cipher(struct cipher_implementation *ci);
int register_digest(struct digest_implementation *di);

int unregister_transform(struct transform_implementation *ti);
int unregister_cipher(struct cipher_implementation *ci);


/* Utility macros */

#define INIT_CIPHER_BLKOPS(name)                \
        _encrypt: name##_encrypt,               \
        _decrypt: name##_decrypt

#define INIT_CIPHER_OPS(name)                   \
        _set_key: name##_set_key,               \
        lock:    name##_lock,                   \
        unlock:  name##_unlock

#define INIT_DIGEST_OPS(name)                   \
        _open:   name##_open,                   \
        _update: name##_update,                 \
        _digest: name##_digest,                 \
        _close:  name##_close,                  \
        _hmac:   name##_hmac,                   \
        lock:   name##_lock,                    \
        unlock: name##_unlock

/* inline PKCS padding functions */
static inline int pkcspad_inplace(u8 *buf,
                                  u32 buf_len, u32 payload_len) {
        int i;
        u8 P = (u8)((buf_len - payload_len) & 0xff);

        for (i = payload_len; i < buf_len; i++) {
                buf[i] = P;
        }

        return i;
}

static inline int pkcspad(u8 *out, const u8 *buf,
                          u32 buf_len, u32 payload_len) {
        int i;
        u8 P = (u8)((buf_len - payload_len) & 0xff);

        memcpy(out, buf, payload_len);

        for (i = payload_len; i < buf_len; i++) {
                out[i] = P;
  }

        return i;
}

#endif /* __KERNEL__ */
#endif /* CONFIG_CRYPTO_INCA */
#endif /* _LINUX_CRYPTO_H_ */

/* eof */

