/******************************************************************************
**
** FILE NAME    : ifxdeu-aes.c
** PROJECT      : Danube
** MODULES     	: crypto
**
** DATE         : 23 Oct 2006
** AUTHOR       : Lee Yao Chye
** DESCRIPTION  : AES Algorithm.
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
 * Key expansion routine taken from crypto/aes.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ---------------------------------------------------------------------------
 * Copyright (c) 2002, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
 * All rights reserved.
 *
 * LICENSE TERMS
 *
 * The free distribution and use of this software in both source and binary
 * form is allowed (with or without changes) provided that:
 *
 *   1. distributions of this source code include the above copyright
 *      notice, this list of conditions and the following disclaimer;
 *
 *   2. distributions in binary form include the above copyright
 *      notice, this list of conditions and the following disclaimer
 *      in the documentation and/or other associated materials;
 *
 *   3. the copyright holder's name is not used to endorse products
 *      built using this software without specific written permission.
 *
 * ALTERNATIVELY, provided that this notice is retained in full, this product
 * may be distributed under the terms of the GNU General Public License (GPL),
 * in which case the provisions of the GPL apply INSTEAD OF those given above.
 *
 * DISCLAIMER
 *
 * This software is provided 'as is' with no explicit or implied warranties
 * in respect of its properties, including, but not limited to, correctness
 * and/or fitness for purpose.
 * ---------------------------------------------------------------------------
 * Change Log:
 * yclee 15 Jun 2006: tidy code; add local_irq_save() & local_irq_restore()
 * yclee 11 Jul 2006: take __get_free_page() out of alloc function.
 * yclee 21 Jul 2006: fix nbytes greater than 16 using fpi mode.
 * yclee 24 Jul 2006: comment out udelay() since it works alright without them.
 * yclee 17 Aug 2006: dma_device_reserve() can only be called once
 * yclee 18 Aug 2006: comment out "controlr.SM = 0" since function needs to support repeated calls
 * yclee 23 Aug 2006: save and retrieve key from context
 * yclee 18 Sep 2006: max_nbytes = AES_BLOCK_SIZE
 * yclee 25 Sep 2006: dma mode still crashes during kmalloc() with openswan
 * tcchen31 Oct 2006: copy iv_arg back 
 * tcchen10 Nov 2006: improve DMA performance
 * ---------------------------------------------------------------------------
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/crypto.h>
#include <linux/interrupt.h>
#include <asm/byteorder.h>
#include <linux/delay.h>

#define INCA_IP_AES_AES         (KSEG1 + 0x18000880)
#define AES_MIN_KEY_SIZE        16	/* in uint8_t units */
#define AES_MAX_KEY_SIZE        32	/* ditto */
#define AES_BLOCK_SIZE          16	/* ditto */
#define AES_EXTENDED_KEY_SIZE   64	/* in uint32_t units */
#define AES_EXTENDED_KEY_SIZE_B (AES_EXTENDED_KEY_SIZE * sizeof(uint32_t))
#define DEU_AESIR               INT_NUM_IM0_IRL28

//#define CRYPTO_DEBUG
#ifdef CONFIG_CRYPTO_DEV_INCAIP1_AES
#include <asm/incaip/inca-ip.h>
#include <asm/incaip/inca-ip-deu-structs.h>
#define AES_START INCA_IP_AES_AES
#endif
#ifdef CONFIG_CRYPTO_DEV_INCAIP2_AES
#include <asm/incaip2/incaip2-deu.h>
#include <asm/incaip2/incaip2-deu-structs.h>
#define AES_START AES_CON
#endif
#ifdef CONFIG_CRYPTO_DEV_DANUBE_AES
#include <asm/danube/danube.h>
#include <asm/danube/danube_deu.h>
#include <asm/danube/danube_deu_structs.h>
#define AES_START   DANUBE_AES_CON
#endif
#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
#include "ifxdeu-dma.h"
#include <asm/danube/danube_dma.h>
#include <asm/danube/irq.h>
#endif

struct aes_ctx {
	int key_length;
	u32 E[60];
	u32 D[60];
};

extern int disable_multiblock;

void
deu_interrupt (int irq, void *dev_id, struct pt_regs *regs)
{
	// empty function
}

static int
aes_set_key (void *ctx_arg, const uint8_t * in_key, unsigned int key_len,
	     uint32_t * flags)
{
	struct aes_ctx *ctx = ctx_arg;

	if (key_len != 16 && key_len != 24 && key_len != 32) {
		*flags |= CRYPTO_TFM_RES_BAD_KEY_LEN;
		return -EINVAL;
	}

	ctx->key_length = key_len;

	memcpy ((u8 *) (ctx->E), in_key, key_len);

	return 0;
}

static void
aes_ifxdeu (void *ctx_arg, uint8_t * out_arg, const uint8_t * in_arg,
	    uint8_t * iv_arg, size_t nbytes, int encdec, int mode)
{
#if 0				// kmalloc corruption suspected
	u32 incopy[AES_BLOCK_SIZE] = { 0 };	// use stack as precautionary measure
#endif

	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	volatile struct aes_t *aes = (struct aes_t *) AES_START;
	int i = 0;
	int byte_cnt = nbytes;
	unsigned long flag;
	struct aes_ctx *ctx = ctx_arg;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	int key_len = ctx->key_length;
	u32 *in_key = ctx->E;

#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	int x = 0;
	u32 *out_dmaarr = out_arg;
	u32 *in_dmaarr = in_arg;
	u8 *incopy = NULL;
	u32 *outcopy;
	u32 *dword_aligned_mem_in = NULL;
	u32 *dword_aligned_mem_out = NULL;
	u32 y = 0;
	volatile struct deu_dma_t *dma = (struct deu_dma_t *) DMA_CON;
	struct dma_device_info *dma_device = ifx_deu[0].dma_device;
	_ifx_deu_device *pDev = ifx_deu;
	u32 *out_dma = NULL;
	int wlen = 0;
	u32 timeout = 0;
#endif

	local_irq_save (flag);

	/* 128, 192 or 256 bit key length */
	aes->controlr.K = key_len / 8 - 2;

	if (key_len == 128 / 8) {
		aes->K3R = *((u32 *) in_key + 0);
		aes->K2R = *((u32 *) in_key + 1);
		aes->K1R = *((u32 *) in_key + 2);
		aes->K0R = *((u32 *) in_key + 3);
	}
	else if (key_len == 192 / 8) {
		aes->K5R = *((u32 *) in_key + 0);
		aes->K4R = *((u32 *) in_key + 1);
		aes->K3R = *((u32 *) in_key + 2);
		aes->K2R = *((u32 *) in_key + 3);
		aes->K1R = *((u32 *) in_key + 4);
		aes->K0R = *((u32 *) in_key + 5);
	}
	else if (key_len == 256 / 8) {
		aes->K7R = *((u32 *) in_key + 0);
		aes->K6R = *((u32 *) in_key + 1);
		aes->K5R = *((u32 *) in_key + 2);
		aes->K4R = *((u32 *) in_key + 3);
		aes->K3R = *((u32 *) in_key + 4);
		aes->K2R = *((u32 *) in_key + 5);
		aes->K1R = *((u32 *) in_key + 6);
		aes->K0R = *((u32 *) in_key + 7);
	}
	else {
		printk ("key_len = %d\n", key_len);
		return -EINVAL;
	}
	/* let HW pre-process DEcryption key in any case (even if
	   ENcryption is used). Key Valid (KV) bit is then only
	   checked in decryption routine! */
	aes->controlr.PNK = 1;

#if 0
	while (aes->controlr.BUS) {
		// this will not take long
	}
#endif

//#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
#if 0
	if (!aes->controlr.KV) {
		printk ("aes->controlr.KV = %d\n", aes->controlr.KV);
		return -EINVAL;
	}
#endif

#ifdef CRYPTO_DEBUG
	printk ("hardware crypto in aes_ifxdeu\n");
	printk ("hardware is running\n");
	printk ("encdec %x\n", encdec);
	printk ("mode %x\n", mode);
	printk ("nbytes %u\n", nbytes);
#endif
	aes->controlr.E_D = !encdec;	//encryption
	aes->controlr.O = mode - 1;	//0 ECB 1 CBC 2 OFB 3 CFB 4 CTR hexdump(prin,sizeof(*des));

	//aes->controlr.F = 128; //default  maybe needs to be updated to a proper value calculated from nbytes
	if (mode > 1) {
		aes->IV3R = (*(u32 *) iv_arg);
		aes->IV2R = (*((u32 *) iv_arg + 1));
		aes->IV1R = (*((u32 *) iv_arg + 2));
		aes->IV0R = (*((u32 *) iv_arg + 3));
	};

#ifndef CONFIG_CRYPTO_DEV_DANUBE_DMA
	i = 0;
	while (byte_cnt >= 16) {
		aes->ID3R = *((u32 *) in_arg + (i * 4) + 0);
		aes->ID2R = *((u32 *) in_arg + (i * 4) + 1);
		aes->ID1R = *((u32 *) in_arg + (i * 4) + 2);
		aes->ID0R = *((u32 *) in_arg + (i * 4) + 3);	/* start crypto */
		while (aes->controlr.BUS) {
			// this will not take long
		}

		*((u32 *) out_arg + (i * 4) + 0) = aes->OD3R;
		*((u32 *) out_arg + (i * 4) + 1) = aes->OD2R;
		*((u32 *) out_arg + (i * 4) + 2) = aes->OD1R;
		*((u32 *) out_arg + (i * 4) + 3) = aes->OD0R;

		i++;
		byte_cnt -= 16;
	}

#else // dma

#if 0
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	_ifx_deu_device *pDev = ifx_deu;
	/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

	pDev->len = nbytes;
	pDev->packet_size = nbytes;
	pDev->src = in_arg;

	pDev->dst = out_arg;
	pDev->dst_count = 0;
#endif

	// reserve DEU DMA for AES
	if (deu_dma_reserve () != 0) {
		printk ("%s %d: reserve DMA channel fail!\n", __func__,
			__LINE__);
		local_irq_restore (flag);
		return -EINVAL;
	}

	dma->controlr.ALGO = 1;	//AES

	// check memory alignment (16 bytes alignment for DMA)
	if (((uint32_t) in_arg) & 0xF) {

		incopy = kmalloc (nbytes, GFP_ATOMIC);	// incopy is dword-aligned

		if (incopy == NULL) {
			deu_dma_release ();
			local_irq_restore (flag);
			printk ("%s %d: kmalloc memory fail!\n", __func__,
				__LINE__);
			return -EINVAL;
		}
		else {
			dword_aligned_mem_in = (u32 *) incopy;	// need to do u32-based copy
			memcpy (incopy, in_arg, nbytes);
		}
	}
	else {
		dword_aligned_mem_in = (u32 *) in_arg;	// need to do u32-based copy
	}

	//----------------------------------------------------------------------------------------------
	// somehow we need to use dword-aligned incopy instead of in_arg
	//----------------------------------------------------------------------------------------------
	wlen = dma_device_write (dma_device, (u8 *) dword_aligned_mem_in,
				 nbytes, dword_aligned_mem_in);
	if (wlen != nbytes) {
		if (incopy) {
			kfree (incopy);
		}
		deu_dma_release ();
		local_irq_restore (flag);
		printk ("%s %d: dma_device_write fail!\n", __func__,
			__LINE__);
		return -EINVAL;
	}

	if (((uint32_t) out_dmaarr) & 0x3) {
		outcopy = kmalloc (nbytes, GFP_ATOMIC);	// outcopy is dword-aligned
	}
	else {
		outcopy = out_dmaarr;
	}

#if 0
	while (dma->controlr.BSY) {
		// this will not take long
	}

	while (aes->controlr.BUS);
	{
		// this will not take long
	}
#endif

	// polling DMA rx channel
	while ((pDev->recv_count =
		dma_device_read (dma_device, &out_dma, NULL)) == 0) {
		timeout++;
		if (timeout >= 333000) {
			deu_dma_release ();
			if (incopy) {
				kfree (incopy);
			}
			if (outcopy != out_dmaarr) 
				kfree (outcopy);
			local_irq_restore (flag);
			printk ("%s %d: timeout!!\n", __func__, __LINE__);
			return -EINVAL;
		}
	}

	deu_dma_release ();
#if 0
	while (dma->controlr.BSY) {
		// this will not take long
	}
#endif

	for (i = 0; i < (nbytes / 4); i++) {
		x = i ^ 0x3;

		outcopy[i] = out_dma[x];
	}

	if (outcopy != out_dmaarr) {
		memcpy ((u8 *) out_dmaarr, outcopy, nbytes);	// note that incopy points to dword_aligned_mem
		kfree (outcopy);
	}

	kfree (out_dma);
	if (incopy) {
		kfree (incopy);
	}
#endif // dma
	//tc.chen : copy iv_arg back
	if (mode > 1) {
		*((u32 *) iv_arg) = aes->IV3R;
		*((u32 *) iv_arg + 1) = aes->IV2R;
		*((u32 *) iv_arg + 2) = aes->IV1R;
		*((u32 *) iv_arg + 3) = aes->IV0R;
	}

	local_irq_restore (flag);
}

static void
aes_ifxdeu_ecb (void *ctx, uint8_t * dst, const uint8_t * src,
		uint8_t * iv, size_t nbytes, int encdec, int inplace)
{
	aes_ifxdeu (ctx, dst, src, NULL, nbytes, encdec, CRYPTO_TFM_MODE_ECB);
}

static void
aes_ifxdeu_cbc (void *ctx, uint8_t * dst, const uint8_t * src, uint8_t * iv,
		size_t nbytes, int encdec, int inplace)
{
	aes_ifxdeu (ctx, dst, src, iv, nbytes, encdec, CRYPTO_TFM_MODE_CBC);
}

static void
aes_ifxdeu_cfb (void *ctx, uint8_t * dst, const uint8_t * src, uint8_t * iv,
		size_t nbytes, int encdec, int inplace)
{
	aes_ifxdeu (ctx, dst, src, iv, nbytes, encdec, CRYPTO_TFM_MODE_CFB);
}

static void
aes_ifxdeu_ofb (void *ctx, uint8_t * dst, const uint8_t * src, uint8_t * iv,
		size_t nbytes, int encdec, int inplace)
{
	aes_ifxdeu (ctx, dst, src, iv, nbytes, encdec, CRYPTO_TFM_MODE_OFB);
}

static void
aes_encrypt (void *ctx_arg, uint8_t * out, const uint8_t * in)
{
	//printk("aes_encrypt\n");
	aes_ifxdeu (ctx_arg, out, in, NULL, AES_BLOCK_SIZE,
		    CRYPTO_DIR_ENCRYPT, CRYPTO_TFM_MODE_ECB);

}

static void
aes_decrypt (void *ctx_arg, uint8_t * out, const uint8_t * in)
{
	//printk("aes_decrypt\n");
	aes_ifxdeu (ctx_arg, out, in, NULL, AES_BLOCK_SIZE,
		    CRYPTO_DIR_DECRYPT, CRYPTO_TFM_MODE_ECB);
}

static int
aes_chip_init (void)
{
	volatile struct aes_t *aes = (struct aes_t *) AES_START;
	// start crypto engine with write to ILR
	aes->controlr.SM = 1;
	aes->controlr.ARS = 1;
}

static struct crypto_alg aes_alg = {
	.cra_name = "aes",
	.cra_preference = CRYPTO_PREF_HARDWARE,
	.cra_flags = CRYPTO_ALG_TYPE_CIPHER,
	.cra_blocksize = AES_BLOCK_SIZE,
	.cra_ctxsize = sizeof (struct aes_ctx),
	.cra_module = THIS_MODULE,
	.cra_list = LIST_HEAD_INIT (aes_alg.cra_list),
	.cra_u = {
		  .cipher = {
			     .cia_min_keysize = AES_MIN_KEY_SIZE,
			     .cia_max_keysize = AES_MAX_KEY_SIZE,
			     .cia_setkey = aes_set_key,
			     .cia_encrypt = aes_encrypt,
			     .cia_decrypt = aes_decrypt}
		  }
};

int __init
ifxdeu_init_aes (void)
{
	printk ("ifxdeu initialize!\n");
	if (!disable_multiblock) {
		aes_alg.cra_u.cipher.cia_max_nbytes = AES_BLOCK_SIZE;	//(size_t)-1;
		aes_alg.cra_u.cipher.cia_req_align = 16;
		aes_alg.cra_u.cipher.cia_ecb = aes_ifxdeu_ecb;
		aes_alg.cra_u.cipher.cia_cbc = aes_ifxdeu_cbc;
		aes_alg.cra_u.cipher.cia_cfb = aes_ifxdeu_cfb;
		aes_alg.cra_u.cipher.cia_ofb = aes_ifxdeu_ofb;
	}

	printk (KERN_NOTICE
		"Using Infineon DEU for AES algorithm%s.\n",
		disable_multiblock ? "" : " (multiblock)");

	aes_chip_init ();

	//gen_tabs();
	return crypto_register_alg (&aes_alg);
}

void __exit
ifxdeu_fini_aes (void)
{
	crypto_unregister_alg (&aes_alg);
}
