/******************************************************************************
**
** FILE NAME    : ifxdeu-generic.c
** PROJECT      : Danube
** MODULES     	: crypto
**
** DATE         : 23 Oct 2006
** AUTHOR       : Lee Yao Chye
** DESCRIPTION  : Support for DEU driver.
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

/************************************************************************
 * Cryptographic API.
 *
 * Support for Infineon hardware crypto engine (DEU).
 *
 * Copyright (c) 2005  Johannes Doering <info@com-style.de>, INFINEON
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * ---------------------------------------------------------------------------
 * Change Log:
 * yclee 17 Aug 2006: dma_device_reserve() can only be called once
 * ---------------------------------------------------------------------------
 ************************************************************************/

/* Group definitions for Doxygen */
/** \addtogroup API API-Functions */
/** \addtogroup Internal Internally used functions */

#include <linux/config.h>
#include <linux/version.h>
//#if defined(CONFIG_MODVERSIONS)	// && !defined(MODVERSIONS)
//#error CONFIG_MODVERSIONS defined!!!
//#define MODVERSIONS
//#include <linux/modeversions.h>
//#endif
#if defined(CONFIG_MODVERSIONS)
#define MODVERSIONS
#include <linux/modversions.h>
#endif


#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/crypto.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>		/* Stuff about file systems that we need */
#include <asm/byteorder.h>

#ifdef CONFIG_CRYPTO_DEV_INCAIP1
#include <asm/incaip/inca-ip.h>
#include <asm/incaip/inca-ip-deu-structs.h>
//#define CLC_START  INCA_IP_AES_AES
#endif
#ifdef CONFIG_CRYPTO_DEV_INCAIP2
//#include <asm/incaip2/incaip2_dma.h>
#include <asm/incaip2/incaip2.h>
#include <asm/incaip2/incaip2-deu.h>
#include <asm/incaip2/incaip2-deu-structs.h>
#define CLC_START DEU_CON
#endif
#ifdef CONFIG_CRYPTO_DEV_DANUBE
#include <asm/danube/danube.h>
#include <asm/danube/danube_pmu.h>
#include <asm/danube/danube_deu_structs.h>
#define CLC_START DANUBE_DEU_CLK
#endif

#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
#include <asm/danube/danube_dma.h>
#include "ifxdeu-dma.h"
int disable_deudma = 0;
#else
int disable_deudma = 1;
#endif

#include <asm/danube/danube_deu.h>

static unsigned int device_major = 0;
char *driver_name = "ifxdeu";
#define PROCNAME "ifxdeu"
u32 ioctl_cnt = 0;

extern void md5_test (void);
static int deu_ioctl (struct inode *inode, struct file *file,
		      unsigned int cmd, unsigned long arg);
static int deu_open (struct inode *inode, struct file *file);
static int deu_release (struct inode *inode, struct file *file);

static struct file_operations device_fops = {
//        read:           deu_read,
//        write:          deu_write,
//      poll:           deu_poll,
      ioctl:deu_ioctl,
//      mmap:           deu_mmap,
      open:deu_open,
      release:deu_release,
};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
struct proc_dir_entry *my_proc = NULL;
#else
struct proc_dir_entry my_proc = {
	0,			//n.a
	sizeof (PROCNAME) - 1,	// Length of /proc name
	PROCNAME,		//proc name
	S_IFREG | S_IRUGO,	/* Regular file, read by all */
	1,			/* One link only */
	0,			/* Root's UID */
	0,			/* Root's GID */
	80,			/* "ls" reports the size as this */
	NULL,			/* Default all inode operations */
//        get_my_info,    /* Function when user wants info */
	NULL,
	NULL			/* Everything hereafter is NULL */
};
#endif

int
install_proc ()
{
	int rc;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	my_proc = create_proc_entry (PROCNAME, S_IFREG | S_IRUGO, NULL);
	if (my_proc) {
		my_proc->nlink = 1;
//                my_proc->read_proc=mydriver_get_info;
		rc = 0;
	}
	else
		rc = -1;
#else
	rc = proc_register (&proc_root, &my_proc);
#endif
	return rc;
}

void
remove_proc ()
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,4,0)
	remove_proc_entry (PROCNAME, NULL);
#else
	proc_unregister (&proc_root, my_proc.low_ino);
#endif
}

static int
deu_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	if (_IOC_TYPE (cmd) != DANUBE_DEU_IOC_MAGIC) {
		printk (KERN_ERR "Invalid ioctl command\n");
		return -ENOTTY;
	}
	if (_IOC_NR (cmd) > DANUBE_DEU_IOC_MAXNR) {
		printk (KERN_ERR "Invalid ioctl command\n");
		return -ENOTTY;
	}
	ioctl_cnt++;
	switch (cmd) {
#ifdef CONFIG_CRYPTO_DEV_DANUBE_MD5
	case DANUBE_DEU_IOC_MD5_TEST:
		md5_test ();
		break;
	case DANUBE_DEU_IOC_MD5_GET_STAT:
		return md5_stat ();
		break;
#endif
	default:
		return -ENOTTY;
		break;
	}
	return 0;
}
static int
deu_open (struct inode *inode, struct file *file)
{
	/* Increments usage count */
	MOD_INC_USE_COUNT;
	printk ("<1> The current process is \"%s\" (pid %i)\n", current->comm,
		current->pid);
	printk ("<1> Open major: %d minor: %d jiffies:%ld\n",
		MAJOR (inode->i_rdev), MINOR (inode->i_rdev), jiffies);
	return 0;
}
static int
deu_release (struct inode *inode, struct file *file)
{
	MOD_DEC_USE_COUNT;
	printk ("<1> device_release\n");
	return 0;
}

/**
 * Initialization of all build in Infineon Hardware modules is done here
 * This module is linked together with all modules that hae been selected 
 * in the kernel config for ifx crytpo hw support
 * The initialization functions of all modules should be called from here
 *          
 * \return  0        	OK, all compiled in hardware algorithms have been initialized.
 * \return  -ENOSYS   If Infineon DEU was compiled without any algorithm.
 * \return  OTHER			The return vale of any initialization function of any hardware algorithm failed. 
 * \ingroup Internal  
 */

static int __init
deu_init (void)
{
	int ret = -ENOSYS;

	printk ("Infineon DEU initialization.\n");
	install_proc ();
	ret = register_chrdev (device_major, driver_name, &device_fops);
	if (ret < 0) {
		printk ("<1> Can't register device with kernel, ret==%d\n",
			ret);
		return ret;
	}
	else
		device_major = ret;

#if defined(CONFIG_CRYPTO_DEV_INCAIP2)  | defined(CONFIG_CRYPTO_DEV_DANUBE)
	volatile struct clc_controlr_t *clc =
		(struct clc_controlr_t *) CLC_START;
#ifdef CONFIG_CRYPTO_DEV_INCAIP2
	*INCA_IP2_PMS_PMS_GEN |= 0x00040000;
#endif

#ifdef CONFIG_CRYPTO_DEV_DANUBE

	//pmu_set((DANUBE_PMU_IOC_SET_DEU & 0xff), 1);
	*DANUBE_PMU_PWDCR &= ((~(1 << 20)));	//& 0x3fffff

#endif
	clc->FSOE = 0;
	clc->SBWE = 0;
	clc->SPEN = 0;
	clc->SBWE = 0;
	clc->DISS = 0;
	clc->DISR = 0;
#endif

#ifdef CONFIG_CRYPTO_DEV_DANUBE_DMA
	deu_dma_init ();
#endif

#if defined(CONFIG_CRYPTO_DEV_INCAIP1_DES) || defined(CONFIG_CRYPTO_DEV_INCAIP2_DES) || defined(CONFIG_CRYPTO_DEV_DANUBE_DES)
	printk (KERN_INFO "deu_init(): calling ifxdeu_init_des()\n");
	if ((ret = ifxdeu_init_des ())) {
		printk (KERN_ERR "Infineon DES initialization failed.\n");
		return ret;
	}
#endif
#if defined(CONFIG_CRYPTO_DEV_INCAIP1_AES) || defined(CONFIG_CRYPTO_DEV_INCAIP2_AES) || defined(CONFIG_CRYPTO_DEV_DANUBE_AES)
	printk (KERN_INFO "deu_init(): calling ifxdeu_init_aes()\n");
	if ((ret = ifxdeu_init_aes ())) {
		printk (KERN_ERR "Infineon AES initialization failed.\n");
		return ret;
	}

#endif
#if defined(CONFIG_CRYPTO_DEV_INCAIP2_SHA1) || defined(CONFIG_CRYPTO_DEV_DANUBE_SHA1)
	printk (KERN_INFO "deu_init(): calling ifxdeu_init_sha1()\n");
	if ((ret = ifxdeu_init_sha1 ())) {
		printk (KERN_ERR "Infineon SHA1 initialization failed.\n");
		return ret;
	}
#endif
#if defined(CONFIG_CRYPTO_DEV_INCAIP2_MD5) || defined(CONFIG_CRYPTO_DEV_DANUBE_MD5)
	printk (KERN_INFO "deu_init(): calling ifxdeu_init_md5()\n");
	if ((ret = ifxdeu_init_md5 ())) {
		printk (KERN_ERR "Infineon MD5 initialization failed.\n");
		return ret;
	}

#endif
	if (ret == -ENOSYS)
		printk (KERN_ERR
			"Infineon DEU was compiled without any algorithm.\n");

	printk ("<1> %s nitialized OK. Device Major : %d\n", driver_name,
		device_major);

//      return ret;
	return 0;
}

/**
 * Cleanup the DEU module.
 * Clean up all loaded algorithms that have been loaded for unloading.
 *
 * \ingroup Internal  
 */
static void __exit
deu_fini (void)
{
	remove_proc ();
	unregister_chrdev (device_major, driver_name);

#if defined(CONFIG_CRYPTO_DEV_INCAIP1_DES) || defined(CONFIG_CRYPTO_DEV_INCAIP2_DES) || defined(CONFIG_CRYPTO_DEV_DANUBE_DES)
	ifxdeu_fini_des ();
#endif
#if defined(CONFIG_CRYPTO_DEV_INCAIP1_AES) || defined(CONFIG_CRYPTO_DEV_INCAIP2_AES) || defined(CONFIG_CRYPTO_DEV_DANUBE_AES)
	ifxdeu_fini_aes ();
#endif
#if defined(CONFIG_CRYPTO_DEV_INCAIP2_SHA1) || defined(CONFIG_CRYPTO_DEV_DANUBE_SHA1)
	ifxdeu_fini_sha1 ();
#endif
#if defined(CONFIG_CRYPTO_DEV_INCAIP2_MD5) || defined(CONFIG_CRYPTO_DEV_DANUBE_MD5)
	ifxdeu_fini_md5 ();
#endif
}

int disable_multiblock = 0;
MODULE_PARM (disable_multiblock, "i");
MODULE_PARM_DESC (disable_multiblock,
		  "Disable encryption of whole multiblock buffers.");

void
hexdump (unsigned char *buf, unsigned int len)
{
	while (len--)
		printk ("%02x", *buf++);

	printk ("\n");
}

module_init (deu_init);
module_exit (deu_fini);

MODULE_DESCRIPTION ("Infineon DEU crypto engine support.");
MODULE_LICENSE ("GPL");
MODULE_AUTHOR ("Johannes Doering");
