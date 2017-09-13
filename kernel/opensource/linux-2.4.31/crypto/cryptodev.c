/*
 * Driver for /dev/crypto device (aka CryptoDev)
 * Backport to 2.4.20 kernel:
 * Copyright (c) 2005 Johannes Doering <info@com-style.de>, INFINEON
 *
 * based on version for 2.6.10 kernel of
 * Copyright (c) 2004 Michal Ludvig <mludvig@suse.cz>, SuSE Labs
 *
 * Device /dev/crypto provides an interface for 
 * accessing kernel CryptoAPI algorithms (ciphers,
 * hashes) from userspace programs.
 *
 * /dev/crypto interface was originally introduced in
 * OpenBSD and this module attempts to keep the API,
 * although a bit extended.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/random.h>
#include <linux/cryptodev.h>
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <asm/scatterlist.h>

MODULE_AUTHOR("Michal Ludvig <mludvig@suse.cz>");
MODULE_DESCRIPTION("CryptoDev driver");
MODULE_LICENSE("Dual BSD/GPL");

/* ====== Compile-time config ====== */

#define CRYPTODEV_STATS

/* ====== Module parameters ====== */

static int verbosity = 0;
MODULE_PARM(verbosity, "i");
MODULE_PARM_DESC(verbosity, "0: normal, 1: verbose, 2: debug");

#ifdef CRYPTODEV_STATS
static int enable_stats = 0;
MODULE_PARM(enable_stats, "i");
MODULE_PARM_DESC(enable_stats, "collect statictics about cryptodev usage");
#endif

//#define offset_in_page(p) ((unsigned long)(p) & ~PAGE_MASK)
/* ====== Debug helpers ====== */

#define PFX //"cryptodev: "
#define dprintk(level,severity,format,a...)	//		\
//	do { 							\
//		if (level <= verbosity)				\
//			printk(severity format, ##a);
//
/* ====== CryptoAPI ====== */

#define FILL_SG(sg,ptr,len)					\
	do {							\
		(sg)->page = virt_to_page(ptr);			\
		(sg)->offset = offset_in_page(ptr);		\
		(sg)->length = len;				\
		(sg)->dma_address = 0;				\
	} while (0)

static char *crypto_cipher_modes[] = {
	[CRYPTO_TFM_MODE_ECB] = "ecb",
	[CRYPTO_TFM_MODE_CBC] = "cbc",
	[CRYPTO_TFM_MODE_CFB] = "cfb",
	[CRYPTO_TFM_MODE_CTR] = "ctr",
#if 0
	[CRYPTO_TFM_MODE_OFB] = "ofb",
#endif
};

struct csession {
	struct list_head entry;
	struct semaphore sem;
	struct crypto_tfm *tfm;
	uint32_t sid;
#ifdef CRYPTODEV_STATS
#if ! ((COP_ENCRYPT < 2) && (COP_DECRYPT < 2))
#error Struct csession.stat uses COP_{ENCRYPT,DECRYPT} as indices. Do something!
#endif
	unsigned long long stat[2];
	size_t stat_max_size, stat_count;
#endif
};

struct fcrypt {
	struct list_head list;
	struct semaphore sem;
};

/* Prepare session for future use. */
static int
crypto_create_session(struct fcrypt *fcr, struct session_op *sop)
{
	struct csession	*ses_new, *ses_ptr;
	struct crypto_tfm *tfm;
	int mode, ret = 0;
	char alg_name[MAX_ALG_NAME_LEN+1];
	char *keyp;

	/* Does the request make sense? */
	if (!sop->cipher == !sop->mac) {
		printk( KERN_DEBUG "Both 'cipher' and 'mac' set or unset.\n");
		return -EINVAL;
	}

	/* Copy-in the algorithm name if necessary. */
	if (!sop->alg_namelen) {
		/* Hmm, compatibility with OpenBSD CRYPTO_* constants...
		   Should we support it? */
	  printk(KERN_DEBUG "OpenBSD constants are not (yet?) supported.\n");
		return -EINVAL;
	}

	if(sop->alg_namelen > MAX_ALG_NAME_LEN) {
		printk(KERN_DEBUG "Algorithm name too long (%uu > %u)\n",
		       sop->alg_namelen, MAX_ALG_NAME_LEN);
		return -EINVAL;
	}

	copy_from_user(alg_name, sop->alg_name, sop->alg_namelen);
	alg_name[sop->alg_namelen] = '\0';

	if(!sop->cipher) {
		printk( KERN_DEBUG "Hashes are not yet supported.\n");
		return -EINVAL;
	}
	
	switch (sop->cipher & CRYPTO_FLAG_MASK) {
		case CRYPTO_FLAG_ECB:	mode = CRYPTO_TFM_MODE_ECB; break;
		case CRYPTO_FLAG_CBC:	mode = CRYPTO_TFM_MODE_CBC; break;
		case CRYPTO_FLAG_CFB:	mode = CRYPTO_TFM_MODE_CFB; break;
		case CRYPTO_FLAG_CTR:	mode = CRYPTO_TFM_MODE_CTR; break;
#if 0
		/* These modes are not yet supported. */
		case CRYPTO_FLAG_OFB:	mode = CRYPTO_TFM_MODE_OFB; break;
#endif
		default:		return -EINVAL;
	}

	/* Set-up crypto transform. */
	tfm = crypto_alloc_tfm(alg_name, mode);
	if (!tfm) {
		dprintk(1, KERN_DEBUG, "Failed to load transform for %s %s\n",
		       alg_name, crypto_cipher_modes[mode]);
		return -EINVAL;
	}

	/* Was correct key length supplied? */
	if ((sop->keylen < crypto_tfm_alg_min_keysize(tfm)) ||
	    (sop->keylen > crypto_tfm_alg_max_keysize(tfm))) {
		dprintk(1, KERN_DEBUG,
			"Wrong keylen '%uu' for algorithm '%s'. Use %u to %u.\n",
		       sop->keylen, alg_name, crypto_tfm_alg_min_keysize(tfm), 
		       crypto_tfm_alg_max_keysize(tfm));
		crypto_free_tfm(tfm);
		return -EINVAL;
	}

	/* Copy the key from user and set to TFM. */
	keyp = kmalloc(sop->keylen, GFP_KERNEL);
	copy_from_user(keyp, sop->key, sop->keylen);
	ret = crypto_cipher_setkey(tfm, keyp, sop->keylen);
	kfree(keyp);
	if (ret) {
		dprintk(2, KERN_DEBUG,
			"Setting key failed for %s-%uu-%s: flags=0x%X\n",
			alg_name, sop->keylen*8, crypto_cipher_modes[mode],
			tfm->crt_flags);
		dprintk(2, KERN_DEBUG,
			"(see CRYPTO_TFM_RES_* in <linux/crypto.h> for details)\n");
		crypto_free_tfm(tfm);
		return -EINVAL;
	}

	/* Create a session and put it to the list. */
	ses_new = kmalloc(sizeof(*ses_new), GFP_KERNEL);
	if(!ses_new)
		return -ENOMEM;

	memset(ses_new, 0, sizeof(*ses_new));
	get_random_bytes(&ses_new->sid, sizeof(ses_new->sid));
	ses_new->tfm = tfm;
	init_MUTEX(&ses_new->sem);

	down(&fcr->sem);
restart:
	list_for_each_entry(ses_ptr, &fcr->list, entry) {
		/* Check for duplicate SID */
		if (unlikely(ses_new->sid == ses_ptr->sid)) {
			get_random_bytes(&ses_new->sid, sizeof(ses_new->sid));
			/* Unless we have a broken RNG this 
			   shouldn't loop forever... ;-) */
			goto restart;
		}
	}

	list_add(&ses_new->entry, &fcr->list);
	up(&fcr->sem);

	dprintk(2, KERN_DEBUG, "Added session 0x%08X (%s-%uu-%s)\n",
		ses_new->sid, alg_name, sop->keylen*8, crypto_cipher_modes[mode]);
	
	/* Fill in some values for the user. */
	sop->ses = ses_new->sid;
	sop->blocksize = crypto_tfm_alg_blocksize(tfm);

	return 0;
}

/* Everything that needs to be done when removing a session. */
static inline void crypto_destroy_session(struct csession *ses_ptr)
{
	if(down_trylock(&ses_ptr->sem)) {
		dprintk(2, KERN_DEBUG, "Waiting for semaphore of sid=0x%08X\n",
			ses_ptr->sid);
		down(&ses_ptr->sem);
	}
	dprintk(2, KERN_DEBUG, "Removed session 0x%08X\n", ses_ptr->sid);
#if defined(CRYPTODEV_STATS)
	if(enable_stats)
		dprintk(2, KERN_DEBUG,
			"Usage in Bytes: enc=%llu, dec=%llu, max=%zu, avg=%lu, cnt=%zu\n",
			ses_ptr->stat[COP_ENCRYPT], ses_ptr->stat[COP_DECRYPT],
			ses_ptr->stat_max_size, ses_ptr->stat_count > 0
				? ((unsigned long)(ses_ptr->stat[COP_ENCRYPT]+
						   ses_ptr->stat[COP_DECRYPT]) / 
				   ses_ptr->stat_count) : 0,
			ses_ptr->stat_count);
#endif
	crypto_free_tfm(ses_ptr->tfm);
	ses_ptr->tfm = NULL;
	up(&ses_ptr->sem);
	kfree(ses_ptr);
}

/* Look up a session by ID and remove. */
static int crypto_finish_session(struct fcrypt *fcr, uint32_t sid)
{
	struct csession *tmp, *ses_ptr;
	struct list_head *head;
	int ret = 0;
	
	down(&fcr->sem);
	head = &fcr->list;
	/*list_for_each_entry_safe(ses_ptr, tmp, head, entry) {
		if(ses_ptr->sid == sid) {
			list_del(&ses_ptr->entry);
			crypto_destroy_session(ses_ptr);
			break;
		}
	}*/

	if (!ses_ptr) {
		printk(KERN_ERR "Session with sid=0x%08X not found!\n", sid);
		ret = -ENOENT;
	}
	up(&fcr->sem);

	return ret;
}

/* Remove all sessions when closing the file */
static int crypto_finish_all_sessions(struct fcrypt *fcr)
{
	struct csession *tmp, *ses_ptr;

	down(&fcr->sem);
/*	list_for_each_entry_safe(ses_ptr, tmp, &fcr->list, entry) {
		list_del(&ses_ptr->entry);
		crypto_destroy_session(ses_ptr);
	}*/
	up(&fcr->sem);

	return 0;
}

/* Look up session by session ID. The returned session is locked. */
static struct csession *
crypto_get_session_by_sid(struct fcrypt *fcr, uint32_t sid)
{
	struct csession *ses_ptr;

	down(&fcr->sem);
	list_for_each_entry(ses_ptr, &fcr->list, entry) {
		if(ses_ptr->sid == sid) {
			down(&ses_ptr->sem);
			break;
		}
	}
	up(&fcr->sem);

	return ses_ptr;
}

/* This is the main crypto function - feed it with plaintext 
   and get a ciphertext (or vice versa :-) */
static int
crypto_run(struct fcrypt *fcr, struct crypt_op *cop)
{
	char *data, *ivp;
	//char __user *src, __user *dst;
	char  *src,  *dst;
	struct scatterlist sg;
	struct csession *ses_ptr;
	unsigned int ivsize;
	size_t nbytes, bufsize;
	int ret = 0;

	nbytes = cop->len;

	if (cop->op != COP_ENCRYPT && cop->op != COP_DECRYPT) {
		dprintk(1, KERN_DEBUG, "invalid operation op=%u\n", cop->op);
		return -EINVAL;
	}

	ses_ptr = crypto_get_session_by_sid(fcr, cop->ses);
	if (!ses_ptr) {
		dprintk(1, KERN_ERR, "invalid session ID=0x%08X\n", cop->ses);
		return -EINVAL;
	}

	if (nbytes % crypto_tfm_alg_blocksize(ses_ptr->tfm)) {
		dprintk(1, KERN_ERR,
			"data size (%zu) isn't a multiple of block size (%u)\n",
			nbytes, crypto_tfm_alg_blocksize(ses_ptr->tfm));
		ret = -EINVAL;
		goto out_unlock;
	}

	bufsize = PAGE_SIZE < nbytes ? PAGE_SIZE : nbytes;
	data = (char*)__get_free_page(GFP_KERNEL);

	if (unlikely(!data)) {
		ret = -ENOMEM;
		goto out_unlock;
	}

	ivsize = crypto_tfm_alg_ivsize(ses_ptr->tfm);

	ivp = kmalloc(ivsize, GFP_KERNEL);
	if (unlikely(!ivp)) {
		free_page((unsigned long)data);
		ret = -ENOMEM;
		goto out_unlock;
	}

	if (cop->iv) {
		copy_from_user(ivp, cop->iv, ivsize);
		crypto_cipher_set_iv(ses_ptr->tfm, ivp, ivsize);
	}

	src = cop->src;
	dst = cop->dst;

	while(nbytes > 0) {
		size_t current_len = nbytes > bufsize ? bufsize : nbytes;

		copy_from_user(data, src, current_len);

		FILL_SG(&sg, data, current_len);

		if (cop->op == COP_DECRYPT)
			ret = crypto_cipher_decrypt(ses_ptr->tfm, &sg, &sg, current_len);
		else
			ret = crypto_cipher_encrypt(ses_ptr->tfm, &sg, &sg, current_len);

		if (unlikely(ret)) {
			dprintk(0, KERN_ERR, "CryptoAPI failure: flags=0x%x\n",
				ses_ptr->tfm->crt_flags);
			goto out;
		}

		copy_to_user(dst, data, current_len);

		nbytes -= current_len;
		src += current_len;
		dst += current_len;
	}

#if defined(CRYPTODEV_STATS)
	if (enable_stats) {
		/* this is safe - we check cop->op at the function entry */
		ses_ptr->stat[cop->op] += cop->len;
		if (ses_ptr->stat_max_size < cop->len)
			ses_ptr->stat_max_size = cop->len;
		ses_ptr->stat_count++;
	}
#endif

out:
	free_page((unsigned long)data);

	kfree(ivp);

out_unlock:
	up(&ses_ptr->sem);

	return ret;
}

/* ====== /dev/crypto ====== */

static int cryptodev_open(struct inode *inode, struct file *filp)
{
	struct fcrypt *fcr;

	fcr = kmalloc(sizeof(*fcr), GFP_KERNEL);
	if(!fcr)
		return -ENOMEM;

	memset(fcr, 0, sizeof(*fcr));
	init_MUTEX(&fcr->sem);
	INIT_LIST_HEAD(&fcr->list);
	filp->private_data = fcr;

	return 0;
}

static int
cryptodev_release(struct inode *inode, struct file *filp)
{
	struct fcrypt *fcr = filp->private_data;

	if(fcr) {
		crypto_finish_all_sessions(fcr);
		kfree(fcr);
		filp->private_data = NULL;
	}
	return 0;
}

static int
clonefd(struct file *filp)
{
	struct files_struct * files = current->files;
	int fd;

	fd = get_unused_fd();
	if (fd >= 0) {
		get_file(filp);
		FD_SET(fd, files->open_fds);
		fd_install(fd, filp);
	}

	return fd;
}

static int
cryptodev_ioctl(struct inode *inode, struct file *filp,
		unsigned int cmd, unsigned long arg)
{
	struct session_op sop;
	struct crypt_op cop;
	struct fcrypt *fcr = filp->private_data;
	uint32_t ses;
	int ret, fd;

	if (!fcr)
		BUG();

	switch (cmd) {
		case CRIOGET:
			fd = clonefd(filp);
			put_user(fd, (int*)arg);
			return 0;
			
		case CIOCGSESSION:
			copy_from_user(&sop, (void*)arg, sizeof(sop));
			ret = crypto_create_session(fcr, &sop);
			if (ret)
				return ret;
			copy_to_user((void*)arg, &sop, sizeof(sop));
			return 0;

		case CIOCFSESSION:
			get_user(ses, (uint32_t*)arg);
			ret = crypto_finish_session(fcr, ses);
			return ret;

		case CIOCCRYPT:
			copy_from_user(&cop, (void*)arg, sizeof(cop));
			ret = crypto_run(fcr, &cop);
			copy_to_user((void*)arg, &cop, sizeof(cop));
			return ret;

		default:
			return -EINVAL;
	}
}

struct file_operations cryptodev_fops = {
	.owner = THIS_MODULE,
	.open = cryptodev_open,
	.release = cryptodev_release,
	.ioctl = cryptodev_ioctl,
};

struct miscdevice cryptodev = {
	.minor = CRYPTODEV_MINOR,
	.name = "crypto",
	.fops = &cryptodev_fops,
};

static int
cryptodev_register(void)
{
	int rc;

	rc = misc_register (&cryptodev);
	if (rc) {
		printk(KERN_ERR PFX "registeration of /dev/crypto failed\n");
		return rc;
	}

	return 0;
}

static void
cryptodev_deregister(void)
{
	misc_deregister(&cryptodev);
}

/* ====== Module init/exit ====== */

int __init
init_cryptodev(void)
{
	int rc;
	
	rc = cryptodev_register();
	if (rc)
		return rc;

	printk(KERN_INFO PFX "driver loaded.\n");

	return 0;
}

void __exit
exit_cryptodev(void)
{
	cryptodev_deregister();
	printk(KERN_INFO PFX "driver unloaded.\n");
}

module_init(init_cryptodev);
module_exit(exit_cryptodev);
