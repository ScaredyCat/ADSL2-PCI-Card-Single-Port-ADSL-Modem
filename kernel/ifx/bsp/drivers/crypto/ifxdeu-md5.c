/******************************************************************************
**
** FILE NAME    : ifxdeu-md5.c
** PROJECT      : Danube
** MODULES     	: crypto
**
** DATE         : 23 Oct 2006
** AUTHOR       : Lee Yao Chye
** DESCRIPTION  : MD5 Message Digest Algorithm (RFC1321).
** COPYRIGHT    : 	Copyright (c) 2006
**			Infineon Technologies AG
**			Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Date        $Author  $Comment
*******************************************************************************/

/*
 * Cryptographic API.
 *
 * Support for Infineon DEU hardware crypto engine.
 *
 * Copyright (c) 2005  Johannes Doering <info@com-style.de>, INFINEON
 *
 * MD5 Message Digest Algorithm (RFC1321).
 *
 * Derived from cryptoapi implementation, originally based on the
 * public domain implementation written by Colin Plumb in 1993.
 *
 * Copyright (c) Cryptoapi developers.
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * ---------------------------------------------------------------------------
 * Change Log:
 * yclee 15 Jun 2006: tidy code; add local_irq_save() & local_irq_restore()
 * yclee 17 Aug 2006: dma_device_reserve() can only be called once
 * yclee 18 Aug 2006: comment out "controlr.SM = 0" since function needs to support repeated calls
 * tcchen10 Nov 2006: improve DMA performance
 * ---------------------------------------------------------------------------
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/crypto.h>
#include <asm/byteorder.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/sched.h>	/* pulls in jiffies.h */

#ifdef CONFIG_CRYPTO_DEV_INCAIP1_MD5
#include <asm/incaip/inca-ip.h>
#include <asm/incaip/inca-ip-deu-structs.h>
#define HASH_START INCA_IP_AES_AES
#endif

#ifdef CONFIG_CRYPTO_DEV_INCAIP2_MD5
#include <asm/incaip2/incaip2-deu.h>
#include <asm/incaip2/incaip2-deu-structs.h>
// address of hash_con starting
#define HASH_START	HASH_CON
#endif

#ifdef CONFIG_CRYPTO_DEV_DANUBE_MD5
#include <asm/danube/danube.h>
#include <asm/danube/danube_deu.h>
#include <asm/danube/danube_deu_structs.h>
// address of hash_con starting
#define HASH_START	DANUBE_HASH_CON
#endif

#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
#include <asm/danube/danube_dma.h>
#include "ifxdeu-dma.h"
#endif

//#define ONLY_IN_MEM
#define MD5_DIGEST_SIZE					16
#define MD5_HMAC_BLOCK_SIZE				64
#define MD5_BLOCK_WORDS					16	/* 16-word block of data per digest */
#define MD5_HASH_WORDS					4	/* 128-bit digest output */

#define F1(x, y, z)						(z ^ (x & (y ^ z)))
#define F2(x, y, z)						F1(z, x, y)
#define F3(x, y, z)						(x ^ y ^ z)
#define F4(x, y, z)						(y ^ (x | ~z))
#define MD5STEP(f, w, x, y, z, in, s)	(w += f(x, y, z) + in, w = (w << s | w >> (32 - s)) + x)

extern int disable_multiblock;
extern int disable_deudma;

#undef DEBUG_MD5
struct md5_ctx {
	u32 hash[MD5_HASH_WORDS];
	u32 block[MD5_BLOCK_WORDS];
	u64 byte_count;
};
#define MD5_IDLE	0
#define MD5_BUSY	1
static int md5_status;
static int md5_initialized = 0;

void
md5_test (void)
{
	u8 *cptr;
	u32 i, cnt, j1, j2, stamp1, flag;
	u32 key[4];
	struct md5_ctx ctx;
	volatile struct deu_hash_t *hashs = (struct hash_t *) HASH_START;
	volatile struct deu_dma_t *dma = (struct deu_dma_t *) DMA_CON;
	struct dma_device_info *dma_device = ifx_deu[0].dma_device;
	_ifx_deu_device *pDev = ifx_deu;

	/* This is for testing purposes. md5_init() is called in the proper md5sum() system call stack */
	if (!md5_initialized)
		md5_init (NULL);

	printk (KERN_INFO "md5_test(): sizeof(ctx.block) = %d block @0x%x\n",
		sizeof (ctx.block), ctx.block);
	memset (ctx.block, 0xab, sizeof (ctx.block));
#if 0
	cptr = (u8 *) ctx.block;
	for (i = 0; i < 64; i++)
		*cptr++ = i & 0xff;
#endif
#if 0
	for (i = 0; i < 16; i++)
		printk (KERN_INFO "0x%08x\n", ctx.block[i]);
#endif
	ctx.byte_count = 64;

	md5_status = MD5_BUSY;
	j1 = jiffies;
	stamp1 = j1 + HZ;
	for (cnt = 0; !time_after_eq (jiffies, stamp1); cnt++) {
		local_irq_save (flag);
		for (i = 0; i < MD5_BLOCK_WORDS; i++)
			hashs->MR = ctx.block[i];
		//wait for processing
		while (hashs->controlr.BSY) {
			// this will not take long
		}
		local_irq_restore (flag);
	}
	j2 = jiffies;
	md5_status = MD5_IDLE;

	key[0] = hashs->D1R;
	key[1] = hashs->D2R;
	key[2] = hashs->D3R;
	key[3] = hashs->D4R;
	printk (KERN_INFO
		"md5_test(): ============= MD5 Hash Throughput via FPI test result =====================\n");
	printk (KERN_INFO
		"md5_test(): cnt = %d, j1 = %d, j2 = %d, diff = %d\n", cnt,
		j1, j2, j2 - j1);
	printk (KERN_INFO
		"md5_test(): D1R = 0x%08x, D2R = 0x%08x, D3R = 0x%08x, D4R = 0x%08x\n",
		key[0], key[1], key[2], key[3]);
	printk (KERN_INFO
		"md5_test(): ===========================================================================\n");

	printk (KERN_INFO, "\n");

	/* Via DMA */
	pDev->len = sizeof (ctx.block);
	pDev->src = ctx.block;
	*DANUBE_DMA_PS = 1;
	*DANUBE_DMA_PCTRL &= ~(0x3 << 10);	//Don't care about byte swapping
//      *DANUBE_DMA_PCTRL |= (0x3 << 10);       //byte swapping
	while (dma->controlr.BSY) {
		// this will not take long
	}

	md5_status = MD5_BUSY;
	j1 = jiffies;
	stamp1 = j1 + HZ;
	for (cnt = 0; !time_after_eq (jiffies, stamp1); cnt++) {
		local_irq_save (flag);
		while (dma->controlr.BSY) {
			// this will not take long
		}
		if ((i = dma_device_write (dma_device, (u8 *) ctx.block,
				       MD5_HMAC_BLOCK_SIZE,
				       NULL)) != MD5_HMAC_BLOCK_SIZE) {
			printk (KERN_INFO
				"%s %d: %d bytes written: dma_device_write fail! cnt = %d, DMA_CON = 0x%08x\n",
				__func__, __LINE__, i, cnt, dma->controlr);
			local_irq_restore (flag);
			md5_status = MD5_IDLE;
			return -EINVAL;
		}
		while (dma->controlr.BSY) {
			// this will not take long
		}
		local_irq_restore (flag);
	}
	j2 = jiffies;
	md5_status = MD5_IDLE;

	key[0] = hashs->D1R;
	key[1] = hashs->D2R;
	key[2] = hashs->D3R;
	key[3] = hashs->D4R;
	printk (KERN_INFO
		"md5_test(): ============= MD5 Hash Throughput via DMA test result =====================\n");
	printk (KERN_INFO
		"md5_test(): cnt = %d, j1 = %d, j2 = %d, diff = %d\n", cnt,
		j1, j2, j2 - j1);
	printk (KERN_INFO
		"md5_test(): D1R = 0x%08x, D2R = 0x%08x, D3R = 0x%08x, D4R = 0x%08x\n",
		key[0], key[1], key[2], key[3]);
	printk (KERN_INFO
		"md5_test(): ===========================================================================\n");
}

int
md5_stat (void)
{
	return md5_status;
}

static void
md5_transform (u32 * hash, u32 const *in)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	int i = 0;
	volatile register u32 tmp;
	unsigned long flag;
#ifndef ONLY_IN_MEM
	volatile struct deu_hash_t *hashs = (struct hash_t *) HASH_START;
	volatile struct deu_dma_t *dma = (struct deu_dma_t *) DMA_CON;
#else
	unsigned char *prin = NULL;
	struct deu_hash_t *hashs = NULL;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	hashs = (struct deu_hash_t *) kmalloc (sizeof (*hashs), GFP_KERNEL);
	memset (hashs, 0, sizeof (*hashs));
	prin = (unsigned char *) hashs;
#endif
#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	struct dma_device_info *dma_device = ifx_deu[0].dma_device;
	_ifx_deu_device *pDev = ifx_deu;
	int wlen = 0;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#endif

	//printk("md5_transform running\n");

	local_irq_save (flag);

#ifndef CONFIG_CRYPTO_DEV_DANUBE_DMA
	/* Via FPI bus */
	asm ("sync");
	for (i = 0; i < 16; i++) {
#ifdef DEBUG_MD5
		//printk("%u:%x    ",i,in[i]);
		printk ("%u:%x\n", i, cpu_to_le32 (in[i]));
#endif //DEBUG_MD5
		hashs->MR = cpu_to_le32 (in[i]);
		asm ("sync");
	};

	//wait for processing
	while (hashs->controlr.BSY) {
		// this will not take long
	}

#else
	/* Via DMA */
	pDev->len = sizeof (in);
	pDev->src = in;
	*DANUBE_DMA_PS = 1;
	*DANUBE_DMA_PCTRL |= (0x3 << 10);	//byte swapping
	while (dma->controlr.BSY) {
		// this will not take long
	}

	wlen = dma_device_write (dma_device, (u8 *) in, 64, NULL);
	if (wlen != 64) {
		printk ("%s %d: dma_device_write fail!\n", __func__,
			__LINE__);
		local_irq_restore (flag);
		return -EINVAL;
	}

	while (dma->controlr.BSY) {
		// this will not take long
	}

	*DANUBE_DMA_PS = 1;
	*DANUBE_DMA_PCTRL &= ~(0x3 << 10);	//byte swapping deactivate
#endif

	local_irq_restore (flag);
}

/* XXX: this stuff can be optimized */
static inline void
le32_to_cpu_array (u32 * buf, unsigned int words)
{
	while (words--) {
		__le32_to_cpus (buf);
		buf++;
	}
}

static inline void
cpu_to_le32_array (u32 * buf, unsigned int words)
{
	while (words--) {
		__cpu_to_le32s (buf);
		buf++;
	}
}

static inline void
md5_transform_helper (struct md5_ctx *ctx)
{
#ifdef DEBUG_MD5
	printk ("md5_transform_helper running\n");
#endif //DEBUG_MD5

	//le32_to_cpu_array(ctx->block, sizeof(ctx->block) / sizeof(u32));
	md5_transform (ctx->hash, ctx->block);
}

static void
md5_init (void *ctx)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	struct md5_ctx *mctx = ctx;
	unsigned long flag;
#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
	volatile struct deu_dma_t *dma = (struct deu_dma_t *) DMA_CON;
#endif
#ifndef ONLY_IN_MEM
	volatile struct deu_hash_t *hash = (struct hash_t *) HASH_START;
#else
	struct deu_hash_t *hash = NULL;
	unsigned char *prin = NULL;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	hash = (struct deu_hash_t *) kmalloc (sizeof (*hash), GFP_KERNEL);
	memset (hash, 0, sizeof (*hash));
	prin = (unsigned char *) hash;
#endif
	local_irq_save (flag);

	//hexdump(mctx,sizeof(*mctx));

	/*mctx->hash[0] = 0x67452301;
	   mctx->hash[1] = 0xefcdab89;
	   mctx->hash[2] = 0x98badcfe;
	   mctx->hash[3] = 0x10325476;
	 */
	if (mctx)
		mctx->byte_count = 0;

#ifdef DEBUG_MD5
	printk ("md5_init running\n");
	hexdump (hash, sizeof (*hash));
#endif //DEBUG_MD5
	hash->controlr.SM = 1;
	hash->controlr.ALGO = 1;	// 1 = md5  0 = sha1
	hash->controlr.INIT = 1;	// Initialize the hash operation by writing a '1' to the INIT bit.

	printk (KERN_INFO
		"md5_init(): D1R = 0x%x, D2R = 0x%x, D3R = 0x%x, D4R = 0x%x\n",
		hash->D1R, hash->D2R, hash->D3R, hash->D4R);
#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
	if (deu_dma_reserve () != 0) {
		printk ("%s %d: reserve DMA channel fail!\n", __func__,
			__LINE__);
		local_irq_restore (flag);
		return -EINVAL;
	}
	dma->controlr.ALGO = 2;	//10

//	dma->controlr.BS = 1;
	dma->controlr.EN = 1;

#endif // CONFIG_CRYPTO_DEV_DANUBE_DMA
#ifdef DEBUG_MD5
	hexdump (hash, sizeof (*hash));
#endif //DEBUG_MD5

	local_irq_restore (flag);
	md5_initialized = 1;
}

static void
md5_update (void *ctx, const u8 * data, unsigned int len)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	struct md5_ctx *mctx = ctx;
	const u32 avail = sizeof (mctx->block) - (mctx->byte_count & 0x3f);
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef DEBUG_MD5
	hexdump (mctx, sizeof (*mctx));
	printk ("md5_update running\n");
#endif //DEBUG_MD5
	mctx->byte_count += len;

	if (avail > len) {
		memcpy ((char *) mctx->block + (sizeof (mctx->block) - avail),
			data, len);
		return;
	}

	memcpy ((char *) mctx->block + (sizeof (mctx->block) - avail), data,
		avail);

	md5_transform_helper (mctx);
	data += avail;
	len -= avail;

	while (len >= sizeof (mctx->block)) {
		memcpy (mctx->block, data, sizeof (mctx->block));
		md5_transform_helper (mctx);
		data += sizeof (mctx->block);
		len -= sizeof (mctx->block);
	}

	memcpy (mctx->block, data, len);
}

static void
md5_final (void *ctx, u8 * out)
{
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	struct md5_ctx *mctx = ctx;
	const unsigned int offset = mctx->byte_count & 0x3f;
	char *p = (char *) mctx->block + offset;
	int padding = 56 - (offset + 1);
	unsigned long flag;
#ifndef ONLY_IN_MEM
	volatile struct deu_hash_t *hashs = (struct hash_t *) HASH_START;
#else
	unsigned char *prin = NULL;
	struct deu_hash_t *hashs = NULL;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	hashs = (struct deu_hash_t *) kmalloc (sizeof (*hashs), GFP_KERNEL);
	memset (hashs, 0, sizeof (*hashs));
	prin = (unsigned char *) hashs;
#endif
#ifdef DEBUG_MD5
	printk ("md5_final running\n");

	//hexdump(mctx,sizeof(*mctx));
	printk ("block before and after transform\n");
	hexdump (mctx->block, sizeof (mctx->block));
#endif //DEBUG_MD5
	*p++ = 0x80;

	if (padding < 0) {
		memset (p, 0x00, padding + sizeof (u64));
		md5_transform_helper (mctx);
		p = (char *) mctx->block;
		padding = 56;
	}

	memset (p, 0, padding);

#ifdef DEBUG_MD5
	hexdump (&mctx->byte_count, sizeof (mctx->byte_count));
#endif //DEBUG_MD5
	mctx->block[14] = cpu_to_le32 (mctx->byte_count << 3);
	mctx->block[15] = cpu_to_le32 (mctx->byte_count >> 29);

#ifdef DEBUG_MD5
	hexdump (mctx->block, sizeof (mctx->block));
#endif //DEBUG_MD5

	//le32_to_cpu_array(mctx->block, (sizeof(mctx->block) -
	//                  sizeof(u64)) / sizeof(u32));
	md5_transform (mctx->hash, mctx->block);

	local_irq_save (flag);

#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
	//wait for processing
	while (hashs->controlr.BSY) {
		// this will not take long
	}
#endif
	*((u32 *) out + 0) = le32_to_cpu (hashs->D1R);
	*((u32 *) out + 1) = le32_to_cpu (hashs->D2R);
	*((u32 *) out + 2) = le32_to_cpu (hashs->D3R);
	*((u32 *) out + 3) = le32_to_cpu (hashs->D4R);

	//hashs->controlr.SM = 0; // switch off again for next dma transfer

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
	deu_dma_release ();
#endif
	//cpu_to_le32_array(mctx->hash, sizeof(mctx->hash) / sizeof(u32));
	//memcpy(out, mctx->hash, sizeof(mctx->hash));

	local_irq_restore (flag);

	// Wipe context
	memset (mctx, 0, sizeof (*mctx));
}

static struct crypto_alg alg = {
	.cra_name = "md5",
	.cra_preference = CRYPTO_PREF_HARDWARE,
	.cra_flags = CRYPTO_ALG_TYPE_DIGEST,
	.cra_blocksize = MD5_HMAC_BLOCK_SIZE,
	.cra_ctxsize = sizeof (struct md5_ctx),
	.cra_module = THIS_MODULE,
	.cra_list = LIST_HEAD_INIT (alg.cra_list),
	.cra_u = {
		  .digest = {
			     .dia_digestsize = MD5_DIGEST_SIZE,
			     .dia_init = md5_init,
			     .dia_update = md5_update,
			     .dia_final = md5_final}
		  }
};

int __init
ifxdeu_init_md5 (void)
{
	printk (KERN_NOTICE "Using Infineon DEU for MD5 algorithm%s.\n",
		disable_multiblock ? "" : " (multiblock)",
		disable_deudma ? "" : " (DMA)");

	return crypto_register_alg (&alg);
}

void __exit
ifxdeu_fini_md5 (void)
{
	crypto_unregister_alg (&alg);
}
