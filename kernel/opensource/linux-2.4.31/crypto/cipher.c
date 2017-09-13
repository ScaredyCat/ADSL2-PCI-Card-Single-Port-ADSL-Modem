/*
 * Cryptographic API.
 *
 * Cipher operations.
 *
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/crypto.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <asm/scatterlist.h>
#include "internal.h"
#include "scatterwalk.h"

#define CRA_CIPHER(tfm)	(tfm)->__crt_alg->cra_cipher

#define DEF_TFM_FUNCTION(name,mode,encdec,iv)	\
static int name(struct crypto_tfm *tfm,		\
                struct scatterlist *dst,	\
                struct scatterlist *src,	\
		unsigned int nbytes)		\
{						\
	return crypt(tfm, dst, src, nbytes,	\
		     mode, encdec, iv);		\
}

#define DEF_TFM_FUNCTION_IV(name,mode,encdec,iv)	\
static int name(struct crypto_tfm *tfm,		\
                struct scatterlist *dst,	\
                struct scatterlist *src,	\
		unsigned int nbytes, u8 *iv)	\
{						\
	return crypt(tfm, dst, src, nbytes,	\
		     mode, encdec, iv);		\
}

typedef void (cryptfn_t)(void *, u8 *, const u8 *);
typedef void (cryptblkfn_t)(void *, u8 *, const u8 *, u8 *,
			    size_t, int, int);
typedef void (procfn_t)(struct crypto_tfm *, u8 *,
                        u8*, cryptfn_t, int enc, void *, int);

static inline void xor_64(u8 *a, const u8 *b)
{
	((u32 *)a)[0] ^= ((u32 *)b)[0];
	((u32 *)a)[1] ^= ((u32 *)b)[1];
}

static inline void xor_128(u8 *a, const u8 *b)
{
	((u32 *)a)[0] ^= ((u32 *)b)[0];
	((u32 *)a)[1] ^= ((u32 *)b)[1];
	((u32 *)a)[2] ^= ((u32 *)b)[2];
	((u32 *)a)[3] ^= ((u32 *)b)[3];
}

static void cbc_process(struct crypto_tfm *tfm, u8 *dst, u8 *src,
			cryptfn_t *fn, int enc, void *info, int in_place)
{
	u8 *iv = info;
	
	/* Null encryption */
	if (!iv)
		return;
		
	if (enc) {
		tfm->crt_u.cipher.cit_xor_block(iv, src);
		(*fn)(crypto_tfm_ctx(tfm), dst, iv);
		memcpy(iv, dst, crypto_tfm_alg_blocksize(tfm));
	} else {
		u8 stack[in_place ? crypto_tfm_alg_blocksize(tfm) : 0];
		u8 *buf = in_place ? stack : dst;

		(*fn)(crypto_tfm_ctx(tfm), buf, src);
		tfm->crt_u.cipher.cit_xor_block(buf, iv);
		memcpy(iv, src, crypto_tfm_alg_blocksize(tfm));
		if (buf != dst)
			memcpy(dst, buf, crypto_tfm_alg_blocksize(tfm));
	}
}

static void ecb_process(struct crypto_tfm *tfm, u8 *dst, u8 *src,
			cryptfn_t fn, int enc, void *info, int in_place)
{
	(*fn)(crypto_tfm_ctx(tfm), dst, src);
}

/* 
 * Generic encrypt/decrypt wrapper for ciphers, handles operations across
 * multiple page boundaries by using temporary blocks.  In user context,
 * the kernel is given a chance to schedule us once per block.
 */
static int crypt(struct crypto_tfm *tfm,
		 struct scatterlist *dst,
		 struct scatterlist *src,
		 unsigned int nbytes, 
		 int mode, int enc, void *info)
{
 	cryptfn_t *cryptofn = NULL;
 	procfn_t *processfn = NULL;
 	cryptblkfn_t *cryptomultiblockfn = NULL;
 
 	struct scatter_walk walk_in, walk_out;
 	size_t max_nbytes = crypto_tfm_alg_max_nbytes(tfm);
 	size_t bsize = crypto_tfm_alg_blocksize(tfm);
	u8 tmp_src[bsize];
        u8 tmp_dst[bsize];

	if (!nbytes)
		return 0;

	if (nbytes % bsize) {
		tfm->crt_flags |= CRYPTO_TFM_RES_BAD_BLOCK_LEN;
		return (-EINVAL);
	}

 
 	switch (mode) {
 		case CRYPTO_TFM_MODE_ECB:
 			if (CRA_CIPHER(tfm).cia_ecb)
 				cryptomultiblockfn = CRA_CIPHER(tfm).cia_ecb;
 			else {
 				cryptofn = (enc == CRYPTO_DIR_ENCRYPT) ?
						CRA_CIPHER(tfm).cia_encrypt :
						CRA_CIPHER(tfm).cia_decrypt;
 				processfn = ecb_process;
 			}
 			break;
 
 		case CRYPTO_TFM_MODE_CBC:
 			if (CRA_CIPHER(tfm).cia_cbc)
 				cryptomultiblockfn = CRA_CIPHER(tfm).cia_cbc;
 			else {
 				cryptofn = (enc == CRYPTO_DIR_ENCRYPT) ?
						CRA_CIPHER(tfm).cia_encrypt :
						CRA_CIPHER(tfm).cia_decrypt;
 				processfn = cbc_process;
 			}
 			break;
 
		/* Until we have the appropriate {ofb,cfb,ctr}_process()
		   functions, the following cases will return -ENOSYS if
		   there is no HW support for the mode. */
 		case CRYPTO_TFM_MODE_OFB:
 			if (CRA_CIPHER(tfm).cia_ofb)
 				cryptomultiblockfn = CRA_CIPHER(tfm).cia_ofb;
 			else
 				return -ENOSYS;
 			break;
 
 		case CRYPTO_TFM_MODE_CFB:
 			if (CRA_CIPHER(tfm).cia_cfb)
 				cryptomultiblockfn = CRA_CIPHER(tfm).cia_cfb;
 			else
 				return -ENOSYS;
 			break;
 
 		case CRYPTO_TFM_MODE_CTR:
 			if (CRA_CIPHER(tfm).cia_ctr)
 				cryptomultiblockfn = CRA_CIPHER(tfm).cia_ctr;
 			else
 				return -ENOSYS;
 			break;
 
 		default:
 			BUG();
 	}

//	if (cryptomultiblockfn)
//		bsize = (max_nbytes > nbytes) ? bsize : max_nbytes;

	scatterwalk_start(&walk_in, src);
	scatterwalk_start(&walk_out, dst);

	for(;;) {
    		u8 *src_p, *dst_p;
		int in_place;

		scatterwalk_map(&walk_in, 0);
		scatterwalk_map(&walk_out, 1);
		src_p = scatterwalk_whichbuf(&walk_in, bsize, tmp_src);
		dst_p = scatterwalk_whichbuf(&walk_out, bsize, tmp_dst);
		in_place = scatterwalk_samebuf(&walk_in, &walk_out,
					       src_p, dst_p);

		nbytes -= bsize;

		scatterwalk_copychunks(src_p, &walk_in, bsize, 0);

 		if (cryptomultiblockfn)
 			(*cryptomultiblockfn)(crypto_tfm_ctx(tfm),
					      dst_p, src_p, info,
					      bsize, enc, in_place);
 		else
 			(*processfn)(tfm, dst_p, src_p, cryptofn,
				     enc, info, in_place);

		scatterwalk_done(&walk_in, 0, nbytes);

		scatterwalk_copychunks(dst_p, &walk_out, bsize, 1);
		scatterwalk_done(&walk_out, 1, nbytes);

		if (!nbytes)
			return 0;
		
		crypto_yield(tfm);
	}

}

static int setkey(struct crypto_tfm *tfm, const u8 *key, unsigned int keylen)
{
	struct cipher_alg *cia = &CRA_CIPHER(tfm);
	
	if (keylen < cia->cia_min_keysize || keylen > cia->cia_max_keysize) {
		tfm->crt_flags |= CRYPTO_TFM_RES_BAD_KEY_LEN;
		return -EINVAL;
	} else
		return cia->cia_setkey(crypto_tfm_ctx(tfm), key, keylen,
		                       &tfm->crt_flags);
}

DEF_TFM_FUNCTION(ecb_encrypt, CRYPTO_TFM_MODE_ECB, CRYPTO_DIR_ENCRYPT, NULL);
DEF_TFM_FUNCTION(ecb_decrypt, CRYPTO_TFM_MODE_ECB, CRYPTO_DIR_DECRYPT, NULL);

DEF_TFM_FUNCTION(cbc_encrypt, CRYPTO_TFM_MODE_CBC, CRYPTO_DIR_ENCRYPT, tfm->crt_cipher.cit_iv);
DEF_TFM_FUNCTION_IV(cbc_encrypt_iv, CRYPTO_TFM_MODE_CBC, CRYPTO_DIR_ENCRYPT, iv);
DEF_TFM_FUNCTION(cbc_decrypt, CRYPTO_TFM_MODE_CBC, CRYPTO_DIR_DECRYPT, tfm->crt_cipher.cit_iv);
DEF_TFM_FUNCTION_IV(cbc_decrypt_iv, CRYPTO_TFM_MODE_CBC, CRYPTO_DIR_DECRYPT, iv);

DEF_TFM_FUNCTION(cfb_encrypt, CRYPTO_TFM_MODE_CFB, CRYPTO_DIR_ENCRYPT, tfm->crt_cipher.cit_iv);
DEF_TFM_FUNCTION_IV(cfb_encrypt_iv, CRYPTO_TFM_MODE_CFB, CRYPTO_DIR_ENCRYPT, iv);
DEF_TFM_FUNCTION(cfb_decrypt, CRYPTO_TFM_MODE_CFB, CRYPTO_DIR_DECRYPT, tfm->crt_cipher.cit_iv);
DEF_TFM_FUNCTION_IV(cfb_decrypt_iv, CRYPTO_TFM_MODE_CFB, CRYPTO_DIR_DECRYPT, iv);

DEF_TFM_FUNCTION(ofb_encrypt, CRYPTO_TFM_MODE_OFB, CRYPTO_DIR_ENCRYPT, tfm->crt_cipher.cit_iv);
DEF_TFM_FUNCTION_IV(ofb_encrypt_iv, CRYPTO_TFM_MODE_OFB, CRYPTO_DIR_ENCRYPT, iv);
DEF_TFM_FUNCTION(ofb_decrypt, CRYPTO_TFM_MODE_OFB, CRYPTO_DIR_DECRYPT, tfm->crt_cipher.cit_iv);
DEF_TFM_FUNCTION_IV(ofb_decrypt_iv, CRYPTO_TFM_MODE_OFB, CRYPTO_DIR_DECRYPT, iv);

DEF_TFM_FUNCTION(ctr_encrypt, CRYPTO_TFM_MODE_CTR, CRYPTO_DIR_ENCRYPT, tfm->crt_cipher.cit_iv);
DEF_TFM_FUNCTION_IV(ctr_encrypt_iv, CRYPTO_TFM_MODE_CTR, CRYPTO_DIR_ENCRYPT, iv);
DEF_TFM_FUNCTION(ctr_decrypt, CRYPTO_TFM_MODE_CTR, CRYPTO_DIR_DECRYPT, tfm->crt_cipher.cit_iv);
DEF_TFM_FUNCTION_IV(ctr_decrypt_iv, CRYPTO_TFM_MODE_CTR, CRYPTO_DIR_DECRYPT, iv);

int crypto_init_cipher_flags(struct crypto_tfm *tfm, u32 flags)
{
	u32 mode = flags & CRYPTO_TFM_MODE_MASK;
	
	tfm->crt_cipher.cit_mode = mode ? mode : CRYPTO_TFM_MODE_ECB;
	if (flags & CRYPTO_TFM_REQ_WEAK_KEY)
		tfm->crt_flags = CRYPTO_TFM_REQ_WEAK_KEY;
	
	return 0;
}

int crypto_init_cipher_ops(struct crypto_tfm *tfm)
{
	int ret = 0;
	struct cipher_tfm *ops = &tfm->crt_cipher;

	ops->cit_setkey = setkey;

	switch (tfm->crt_cipher.cit_mode) {
	case CRYPTO_TFM_MODE_ECB:
		ops->cit_encrypt = ecb_encrypt;
		ops->cit_decrypt = ecb_decrypt;
		break;
		
	case CRYPTO_TFM_MODE_CBC:
		ops->cit_encrypt = cbc_encrypt;
		ops->cit_decrypt = cbc_decrypt;
		ops->cit_encrypt_iv = cbc_encrypt_iv;
		ops->cit_decrypt_iv = cbc_decrypt_iv;
		break;
		
	case CRYPTO_TFM_MODE_CFB:
		ops->cit_encrypt = cfb_encrypt;
		ops->cit_decrypt = cfb_decrypt;
		ops->cit_encrypt_iv = cfb_encrypt_iv;
		ops->cit_decrypt_iv = cfb_decrypt_iv;
		break;
	
	case CRYPTO_TFM_MODE_OFB:
		ops->cit_encrypt = ofb_encrypt;
		ops->cit_decrypt = ofb_decrypt;
		ops->cit_encrypt_iv = ofb_encrypt_iv;
		ops->cit_decrypt_iv = ofb_decrypt_iv;
		break;
	
	case CRYPTO_TFM_MODE_CTR:
		ops->cit_encrypt = ctr_encrypt;
		ops->cit_decrypt = ctr_decrypt;
		ops->cit_encrypt_iv = ctr_encrypt_iv;
		ops->cit_decrypt_iv = ctr_decrypt_iv;
		break;

	default:
		BUG();
	}
	
	if (ops->cit_mode == CRYPTO_TFM_MODE_CBC) {
	    	
	    	switch (crypto_tfm_alg_blocksize(tfm)) {
	    	case 8:
	    		ops->cit_xor_block = xor_64;
	    		break;
	    		
	    	case 16:
	    		ops->cit_xor_block = xor_128;
	    		break;
	    		
	    	default:
	    		printk(KERN_WARNING "%s: block size %u not supported\n",
	    		       crypto_tfm_alg_name(tfm),
	    		       crypto_tfm_alg_blocksize(tfm));
	    		ret = -EINVAL;
	    		goto out;
	    	}
	    	
		ops->cit_ivsize = crypto_tfm_alg_blocksize(tfm);
	    	ops->cit_iv = kmalloc(ops->cit_ivsize, GFP_KERNEL);
		if (ops->cit_iv == NULL)
			ret = -ENOMEM;
	}

out:	
	return ret;
}

void crypto_exit_cipher_ops(struct crypto_tfm *tfm)
{
	if (tfm->crt_cipher.cit_iv)
		kfree(tfm->crt_cipher.cit_iv);
}
