#ifndef EXPORT_SYMTAB
#define EXPORT_SYMTAB
#endif

#include <linux/config.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/sysctl.h>
#include <linux/proc_fs.h>
#include <linux/ip.h>
#include <linux/tcp.h>

//hsumc 
#include <linux/init.h>
#include <linux/in.h>

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>

#include <asm/uaccess.h>

#include <net80211/if_media.h>
#include <net80211/ieee80211_var.h>
#include <net80211/if_llc.h>

#include "ah.h"
#include "if_athvar.h"
#include "pktlog_fmt.h"
#include "ratectrl.h"
#include "pktlog.h"
#include "pktlog_hal.h"
#include "pktlog_rc.h"

#ifndef NULL
#define NULL 0
#endif

#ifndef __MOD_INC_USE_COUNT
#define	PKTLOG_MOD_INC_USE_COUNT					\
	if (!try_module_get(THIS_MODULE)) {					\
		printk(KERN_WARNING "try_module_get failed\n");		\
	}

#define	PKTLOG_MOD_DEC_USE_COUNT	module_put(THIS_MODULE)
#else
#define	PKTLOG_MOD_INC_USE_COUNT	MOD_INC_USE_COUNT
#define	PKTLOG_MOD_DEC_USE_COUNT	MOD_DEC_USE_COUNT
#endif

typedef enum {
    PKTLOG_PROTO_RX_DESC,
    PKTLOG_PROTO_TX_DESC,
} pktlog_proto_desc_t;

static int __init pktlogmod_init(void);
static void __exit pktlogmod_exit(void);

static int pktlog_attach(struct ath_softc *sc);
static void pktlog_detach(struct ath_softc *sc);

static void pktlog_txctl(struct ath_softc *sc, struct log_tx *log_data,
                      u_int16_t flags);
static void pktlog_txstatus(struct ath_softc *sc, struct log_tx *log_data,
                      u_int16_t flags);
static void pktlog_rx(struct ath_softc *sc, struct log_rx *log_data,
                      u_int16_t flags);

static void pktlog_ani(HAL_SOFTC sc, struct log_ani *log_data,
                       u_int16_t flags);

static void pktlog_rcfind(struct ath_softc *sc,
                          struct log_rcfind *log_data, u_int16_t flags);
static void pktlog_rcupdate(struct ath_softc *sc,
                            struct log_rcupdate *log_data,
                            u_int16_t flags);

static int pktlog_open(struct inode *i, struct file *f);
static int pktlog_release(struct inode *i, struct file *f);
static int pktlog_mmap(struct file *f, struct vm_area_struct *vma);
static ssize_t pktlog_read(struct file *file, char *buf, size_t nbytes,
                           loff_t * ppos);

static void pktlog_release_buf(struct ath_pktlog_info *);

static int pktlog_proto(struct ath_softc *sc, 
                        u_int32_t proto_log[PKTLOG_MAX_PROTO_WORDS], 
                        void *log_data, pktlog_proto_desc_t ds_type, int *plen);

extern struct ath_pktlog_funcs *g_pktlog_funcs;
extern struct ath_pktlog_halfuncs *g_pktlog_halfuncs;
extern struct ath_pktlog_rcfuncs *g_pktlog_rcfuncs;

static struct ath_pktlog_info g_pktlog_info;
static int g_pktlog_mode = PKTLOG_MODE_SYSTEM;

static struct ath_pktlog_funcs g_exported_pktlog_funcs = {
    .pktlog_attach = pktlog_attach,
    .pktlog_detach = pktlog_detach,
    .pktlog_txctl = pktlog_txctl,
    .pktlog_txstatus = pktlog_txstatus,
    .pktlog_rx = pktlog_rx,
};

static struct ath_pktlog_halfuncs g_exported_pktlog_halfuncs = {
    .pktlog_ani = pktlog_ani
};

static struct ath_pktlog_rcfuncs g_exported_pktlog_rcfuncs = {
    .pktlog_rcupdate = pktlog_rcupdate,
    .pktlog_rcfind = pktlog_rcfind
};


#define PKTLOG_WR_LOCK_INIT(_pl_info)
#define PKTLOG_WR_LOCK(_pl_info)
#define PKTLOG_WR_UNLOCK(_pl_info)
#define PKTLOG_WR_LOCK_DESTROY(_pl_info)


#define get_pktlog_state(_sc)  ((_sc)?(_sc)->pl_info->log_state: \
                                   g_pktlog_info.log_state)

#define get_pktlog_bufsize(_sc)  ((_sc)?(_sc)->pl_info->buf_size: \
                                     g_pktlog_info.buf_size)

static struct proc_dir_entry *g_pktlog_pde;

static struct file_operations pktlog_fops = {
  open:pktlog_open,
  release:pktlog_release,
  mmap:pktlog_mmap,
  read:pktlog_read,
};

#ifndef VMALLOC_VMADDR
#define VMALLOC_VMADDR(x) ((unsigned long)(x))
#endif

/* Convert a kernel virtual address to a kernel logical address */
static volatile void *pktlog_virt_to_logical(volatile void *addr)
{
    pgd_t *pgd;
    pmd_t *pmd;
    pte_t *ptep, pte;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
    pud_t *pud;
#endif
    unsigned long vaddr, ret = 0UL;

    vaddr = VMALLOC_VMADDR((unsigned long) addr);

    pgd = pgd_offset_k(vaddr);

    if (!pgd_none(*pgd)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15)
		pud = pud_offset(pgd, vaddr);
        pmd = pmd_offset(pud, vaddr);
#else
        pmd = pmd_offset(pgd, vaddr);
#endif

        if (!pmd_none(*pmd)) {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
            ptep = pte_offset_map(pmd, vaddr);
#else
            ptep = pte_offset(pmd, vaddr);
#endif
            pte = *ptep;

            if (pte_present(pte)) {
                ret = (unsigned long) page_address(pte_page(pte));
                ret |= (vaddr & (PAGE_SIZE - 1));
            }
        }
    }
    return ((volatile void *) ret);
}

static int pktlog_setsize(struct ath_softc *sc, int32_t size)
{
    struct ath_pktlog_info *pl_info = (sc) ? sc->pl_info : &g_pktlog_info;

    if (size < 0)
        return -EINVAL;

    if (size == pl_info->buf_size)
        return 0;

    if(pl_info->log_state) {
        printk(PKTLOG_TAG "Logging should be disabled before changing bufer size\n");
        return -EBUSY;
    }

    if (pl_info->buf != NULL)
        pktlog_release_buf(pl_info);

    if (size != 0)
        pl_info->buf_size = size;

    return 0;
}

static int pktlog_enable(struct ath_softc *sc, int32_t log_state)
{
    u_int32_t page_cnt, vaddr;
    struct page *vpg;
    char dev_name[PKTLOG_DEVNAME_SIZE];
    struct ath_pktlog_info *pl_info = (sc) ? sc->pl_info : &g_pktlog_info;

    pl_info->log_state = 0;

    if (log_state != 0) {
        if (!sc) {
            if (g_pktlog_mode == PKTLOG_MODE_ADAPTER) {
                struct net_device *dev;
                int i;

                /* Disable any active per adapter logging */
                for (i = 0; i < MAX_WLANDEV; i++) {
                    snprintf(dev_name, sizeof(dev_name),
                             WLANDEV_BASENAME "%d", i);
                    if ((dev = dev_get_by_name(dev_name)) != NULL) {
                        struct ath_softc *dev_sc =
                            (struct ath_softc *) (dev->priv);
                        dev_sc->pl_info->log_state = 0;
                        dev_put(dev);
                    }
                }
                g_pktlog_mode = PKTLOG_MODE_SYSTEM;
            }
        } else {
            if (g_pktlog_mode == PKTLOG_MODE_SYSTEM) {
                g_pktlog_info.log_state = 0;
                g_pktlog_mode = PKTLOG_MODE_ADAPTER;
            }
        }

        if (pl_info->buf == NULL) {
            page_cnt = (sizeof(*(pl_info->buf)) +
                        pl_info->buf_size) / PAGE_SIZE;

            if ((pl_info->buf = vmalloc((page_cnt + 2) * PAGE_SIZE))
                == NULL) {
                printk(PKTLOG_TAG
                       "%s: Unable to allocate buffer"
                       "(%d pages)\n", __FUNCTION__, page_cnt);
                return -ENOMEM;
            }

            pl_info->buf = (struct ath_pktlog_buf *)
                (((unsigned long) (pl_info->buf) + PAGE_SIZE - 1)
                 & PAGE_MASK);

            for (vaddr = (unsigned long) (pl_info->buf);
                 vaddr < ((unsigned long) (pl_info->buf)
                          + (page_cnt * PAGE_SIZE)); vaddr += PAGE_SIZE) {
                vpg = virt_to_page(pktlog_virt_to_logical((void *) vaddr));
                SetPageReserved(vpg);
            }

            pl_info->buf->bufhdr.version = CUR_PKTLOG_VER;
            pl_info->buf->bufhdr.magic_num = PKTLOG_MAGIC_NUM;
            pl_info->buf->wr_offset = 0;
            pl_info->buf->rd_offset = -1;
        }
    }
    pl_info->log_state = log_state;
    return 0;
}

static int
ATH_SYSCTL_DECL(ath_sysctl_pktlog_enable, ctl, write, filp, buffer, lenp,
                ppos)
{
    int ret, enable;
    struct ath_softc *sc = (struct ath_softc *) ctl->extra1;

    ctl->data = &enable;
    ctl->maxlen = sizeof(enable);

    if (write) {
        ret = ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
                                       lenp, ppos);
        if (ret == 0)
            return pktlog_enable(sc, enable);
        else
            printk(PKTLOG_TAG "%s:proc_dointvec failed\n", __FUNCTION__);
    } else {
        enable = get_pktlog_state(sc);
        ret = ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
                                       lenp, ppos);
        if (ret)
            printk(PKTLOG_TAG "%s:proc_dointvec failed\n", __FUNCTION__);
    }
    return ret;
}

static int
ATH_SYSCTL_DECL(ath_sysctl_pktlog_size, ctl, write, filp, buffer, lenp,
                ppos)
{
    int ret, size;
    struct ath_softc *sc = (struct ath_softc *) ctl->extra1;

    ctl->data = &size;
    ctl->maxlen = sizeof(size);

    if (write) {
        ret = ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
                                       lenp, ppos);
        if (ret == 0)
            return pktlog_setsize(sc, size);
    } else {
        size = get_pktlog_bufsize(sc);
        ret = ATH_SYSCTL_PROC_DOINTVEC(ctl, write, filp, buffer,
                                       lenp, ppos);
    }
    return ret;
}


#define CTL_AUTO    -2          /* cannot be CTL_ANY or CTL_NONE */

static int pktlog_sysctl_register(struct ath_softc *sc)
{
    struct ath_pktlog_info *pl_info;
    char *proc_name;

    if (sc) {
        pl_info = sc->pl_info;
        proc_name = sc->sc_dev->name;
    } else {
        pl_info = &g_pktlog_info;
        proc_name = PKTLOG_PROC_SYSTEM;
    }

    /*
     * Setup the sysctl table for creating the following sysctl entries:
     * /proc/sys/PKTLOG_PROC_DIR/<adapter>/enable for enabling/disabling pktlog
     * /proc/sys/PKTLOG_PROC_DIR/<adapter>/size for changing the buffer size
     */
    memset(pl_info->sysctls, 0, sizeof(pl_info->sysctls));
    pl_info->sysctls[0].ctl_name = CTL_AUTO;
    pl_info->sysctls[0].procname = PKTLOG_PROC_DIR;
    pl_info->sysctls[0].mode = PKTLOG_PROCSYS_DIR_PERM;
    pl_info->sysctls[0].child = &pl_info->sysctls[2];
    /* [1] is NULL terminator */
    pl_info->sysctls[2].ctl_name = CTL_AUTO;
    pl_info->sysctls[2].procname = proc_name;
    pl_info->sysctls[2].mode = PKTLOG_PROCSYS_DIR_PERM;
    pl_info->sysctls[2].child = &pl_info->sysctls[4];
    /* [3] is NULL terminator */
    pl_info->sysctls[4].ctl_name = CTL_AUTO,
    pl_info->sysctls[4].procname = "enable",
    pl_info->sysctls[4].mode = PKTLOG_PROCSYS_PERM,
    pl_info->sysctls[4].proc_handler = ath_sysctl_pktlog_enable,
    pl_info->sysctls[4].extra1 = sc;

    pl_info->sysctls[5].ctl_name = CTL_AUTO,
    pl_info->sysctls[5].procname = "size",
    pl_info->sysctls[5].mode = PKTLOG_PROCSYS_PERM,
    pl_info->sysctls[5].proc_handler = ath_sysctl_pktlog_size,
    pl_info->sysctls[5].extra1 = sc;

    pl_info->sysctls[6].ctl_name = CTL_AUTO,
    pl_info->sysctls[6].procname = "options",
    pl_info->sysctls[6].mode = PKTLOG_PROCSYS_PERM,
    pl_info->sysctls[6].proc_handler = proc_dointvec,
    pl_info->sysctls[6].data = &pl_info->options,
    pl_info->sysctls[6].maxlen = sizeof(pl_info->options),
    /* [7] is NULL terminator */

    /* and register everything */
    pl_info->sysctl_header = register_sysctl_table(pl_info->sysctls, 1);
    if (!pl_info->sysctl_header) {
        printk("%s: failed to register sysctls!\n", proc_name);
        return -1;
    }

    return 0;
}

static void pktlog_sysctl_unregister(struct ath_softc *sc)
{
    struct ath_pktlog_info *pl_info = (sc) ? sc->pl_info : &g_pktlog_info;

    if (pl_info->sysctl_header) {
        unregister_sysctl_table(pl_info->sysctl_header);
        pl_info->sysctl_header = NULL;
    }
}

static int __init pktlogmod_init(void)
{
    int i;
    char dev_name[PKTLOG_DEVNAME_SIZE];
    struct net_device *dev;
    int ret;

    /* Init system-wide logging */
    g_pktlog_pde = proc_mkdir(PKTLOG_PROC_DIR, NULL);
    if (g_pktlog_pde == NULL) {
        printk(PKTLOG_TAG "%s: proc_mkdir failed\n", __FUNCTION__);
        return -1;
    }
    if ((ret = pktlog_attach(NULL)))
        goto init_fail1;

    /* Init per-adapter logging */
    for (i = 0; i < MAX_WLANDEV; i++) {
        snprintf(dev_name, sizeof(dev_name), WLANDEV_BASENAME "%d", i);
        if ((dev = dev_get_by_name(dev_name)) != NULL) {
            if (pktlog_attach((struct ath_softc *) (dev->priv)))
                printk(PKTLOG_TAG "%s: pktlog_attach failed for %s\n",
                       __FUNCTION__, dev_name);
            dev_put(dev);
        }
    }

    g_pktlog_funcs = &g_exported_pktlog_funcs;
    g_pktlog_halfuncs = &g_exported_pktlog_halfuncs;
    g_pktlog_rcfuncs = &g_exported_pktlog_rcfuncs;

    return 0;

init_fail1:
    remove_proc_entry(PKTLOG_PROC_DIR, NULL);
    return ret;
}

module_init(pktlogmod_init);

static void __exit pktlogmod_exit(void)
{
    int i;
    char dev_name[PKTLOG_DEVNAME_SIZE];
    struct net_device *dev;

    g_pktlog_funcs = NULL;
    g_pktlog_halfuncs = NULL;
    g_pktlog_rcfuncs = NULL;

    pktlog_detach(NULL);

    for (i = 0; i < MAX_WLANDEV; i++) {
        snprintf(dev_name, sizeof(dev_name), WLANDEV_BASENAME "%d", i);
        if ((dev = dev_get_by_name(dev_name)) != NULL) {
            pktlog_detach((struct ath_softc *) (dev->priv));
            dev_put(dev);
        }
    }

    remove_proc_entry(PKTLOG_PROC_DIR, NULL);
}

module_exit(pktlogmod_exit);

/* 
 * Initialize logging for system or adapter
 * Parameter sc should be NULL for system wide logging 
 */
int pktlog_attach(struct ath_softc *sc)
{
    struct ath_pktlog_info *pl_info;
    char *proc_name;
    struct proc_dir_entry *proc_entry;

    if (sc) {
        pl_info = kmalloc(sizeof(*pl_info), GFP_KERNEL);
        if(pl_info == NULL) {
            printk(PKTLOG_TAG "%s:allocation failed for pl_info\n", __FUNCTION__);
            return -ENOMEM;
        }
        sc->pl_info = pl_info;
        proc_name = sc->sc_dev->name;
    } else {
        pl_info = &g_pktlog_info;
        proc_name = PKTLOG_PROC_SYSTEM;
    }

    PKTLOG_WR_LOCK_INIT(pl_info);

    pl_info->buf_size = PKTLOG_DEFAULT_BUFSIZE;
    pl_info->buf = NULL;
    pl_info->log_state = 0;
    pl_info->proc_entry = NULL;
    pl_info->sysctl_header = NULL;

    proc_entry = create_proc_entry(proc_name, PKTLOG_PROC_PERM, g_pktlog_pde);
    if (proc_entry == NULL) {
        printk(PKTLOG_TAG "%s: create_proc_entry failed for %s\n",
               __FUNCTION__, proc_name);
	goto attach_fail1;
    }

    proc_entry->data = pl_info;
    proc_entry->owner = THIS_MODULE;
    proc_entry->proc_fops = &pktlog_fops;
    pl_info->proc_entry = proc_entry;

    if (pktlog_sysctl_register(sc)) {
        printk(PKTLOG_TAG "%s: sysctl register failed for %s\n",
               __FUNCTION__, proc_name);
	goto attach_fail2;
    }
    return 0;

attach_fail2:
    remove_proc_entry(proc_name, g_pktlog_pde);
attach_fail1:
    if(sc)
        kfree(sc->pl_info);
    return -1;
    
    
}

static void pktlog_release_buf(struct ath_pktlog_info *pl_info)
{
    unsigned long page_cnt;
    unsigned long vaddr;
    struct page *vpg;

    page_cnt =
        ((sizeof(*(pl_info->buf)) + pl_info->buf_size) / PAGE_SIZE) + 1;

    for (vaddr = (unsigned long) (pl_info->buf); vaddr <
         (unsigned long) (pl_info->buf) + (page_cnt * PAGE_SIZE);
         vaddr += PAGE_SIZE) {
        vpg = virt_to_page(pktlog_virt_to_logical((void *) vaddr));
        ClearPageReserved(vpg);
    }

    vfree(pl_info->buf);
    pl_info->buf = NULL;
}

static void pktlog_detach(struct ath_softc *sc)
{
    struct ath_pktlog_info *pl_info = sc ? sc->pl_info : &g_pktlog_info;

    remove_proc_entry(pl_info->proc_entry->name, g_pktlog_pde);

    pktlog_sysctl_unregister(sc);

    pl_info->log_state = 0;

    if (pl_info->buf)
        pktlog_release_buf(pl_info);

    PKTLOG_WR_LOCK_DESTROY(pl_info);
    if(sc)
        kfree(pl_info);

    return;
}

static int pktlog_open(struct inode *i, struct file *f)
{
    PKTLOG_MOD_INC_USE_COUNT;
    return 0;
}


static int pktlog_release(struct inode *i, struct file *f)
{
    PKTLOG_MOD_DEC_USE_COUNT;
    return 0;
}

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

static ssize_t
pktlog_read(struct file *file, char *buf, size_t nbytes, loff_t * ppos)
{
    size_t bufhdr_size;
    size_t count = 0, ret_val = 0;
    int rem_len;
    int start_offset, end_offset;
    int fold_offset, ppos_data, cur_rd_offset;
    struct proc_dir_entry *proc_entry = PDE(file->f_dentry->d_inode);
    struct ath_pktlog_info *pl_info =
        (struct ath_pktlog_info *) proc_entry->data;
    struct ath_pktlog_buf *log_buf = pl_info->buf;
   
    if (log_buf == NULL)
        return 0;

    if(*ppos == 0 && pl_info->log_state) {
        pl_info->saved_state = pl_info->log_state;
        pl_info->log_state = 0;
    }

    bufhdr_size = sizeof(log_buf->bufhdr);

    /* copy valid log entries from circular buffer into user space */
    rem_len = nbytes;

    count = 0;

    if (*ppos < bufhdr_size) {
        count = MIN((bufhdr_size - *ppos), rem_len);
        copy_to_user(buf, ((char *) &log_buf->bufhdr) + *ppos, count);
        rem_len -= count;
        ret_val += count;
    }

    start_offset = log_buf->rd_offset;

    if ((rem_len == 0) || (start_offset < 0))
        goto rd_done;

    fold_offset = -1;
    cur_rd_offset = start_offset;

    /* Find the last offset and fold-offset if the buffer is folded */
    do {
        struct ath_pktlog_hdr *log_hdr;
        int log_data_offset;

        log_hdr =
            (struct ath_pktlog_hdr *) (log_buf->log_data + cur_rd_offset);

        log_data_offset = cur_rd_offset + sizeof(struct ath_pktlog_hdr);

        if ((fold_offset == -1)
            && ((pl_info->buf_size - log_data_offset) <= log_hdr->size))
            fold_offset = log_data_offset - 1;

        PKTLOG_MOV_RD_IDX(cur_rd_offset, log_buf, pl_info->buf_size);

        if ((fold_offset == -1) && (cur_rd_offset == 0)
            && (cur_rd_offset != log_buf->wr_offset))
            fold_offset = log_data_offset + log_hdr->size - 1;

        end_offset = log_data_offset + log_hdr->size - 1;
    } while (cur_rd_offset != log_buf->wr_offset);

    ppos_data = *ppos + ret_val - bufhdr_size + start_offset;

    if (fold_offset == -1) {
        if (ppos_data > end_offset)
            goto rd_done;

        count = MIN(rem_len, (end_offset - ppos_data + 1));
        copy_to_user(buf + ret_val, log_buf->log_data + ppos_data, count);
        ret_val += count;
        rem_len -= count;
    } else {
        if (ppos_data <= fold_offset) {
            count = MIN(rem_len, (fold_offset - ppos_data + 1));
            copy_to_user(buf + ret_val, log_buf->log_data + ppos_data,
                         count);
            ret_val += count;
            rem_len -= count;
        }

        if (rem_len == 0)
            goto rd_done;

        ppos_data =
            *ppos + ret_val - (bufhdr_size +
                               (fold_offset - start_offset + 1));

        if (ppos_data <= end_offset) {
            count = MIN(rem_len, (end_offset - ppos_data + 1));
            copy_to_user(buf + ret_val, log_buf->log_data + ppos_data,
                         count);
            ret_val += count;
            rem_len -= count;
        }
    }

  rd_done:
    if((ret_val < nbytes) && pl_info->saved_state) {
        pl_info->log_state = pl_info->saved_state;
        pl_info->saved_state = 0;
    }
    *ppos += ret_val;

    return ret_val;
}

/* vma operations for mapping vmalloced area to user space */
void pktlog_vopen(struct vm_area_struct *vma)
{
    PKTLOG_MOD_INC_USE_COUNT;
}

void pktlog_vclose(struct vm_area_struct *vma)
{
    PKTLOG_MOD_DEC_USE_COUNT;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
struct page *pktlog_vmmap(struct vm_area_struct *vma, unsigned long addr,
                          int *type)
#else
struct page *pktlog_vmmap(struct vm_area_struct *vma, unsigned long addr,
                          int write_access)
#endif
{
    unsigned long offset, vaddr;
    struct proc_dir_entry *proc_entry = PDE(vma->vm_file->f_dentry->d_inode);

    struct ath_pktlog_info *pl_info =
        (struct ath_pktlog_info *) proc_entry->data;

    offset = addr - vma->vm_start + (vma->vm_pgoff << PAGE_SHIFT);

    vaddr = (unsigned long) pktlog_virt_to_logical((void *) (pl_info->buf) + offset);

    if (vaddr == 0UL) {
        printk(PKTLOG_TAG "%s: page fault out of range\n", __FUNCTION__);
        return ((struct page *) 0UL);
    }

    /* increment the usage count of the page */
    get_page(virt_to_page(vaddr));
    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if(type) {
        *type = VM_FAULT_MINOR;
    }
#endif

    return (virt_to_page(vaddr));
}

static struct vm_operations_struct pktlog_vmops = {
  open:pktlog_vopen,
  close:pktlog_vclose,
  nopage:pktlog_vmmap,
};

static int pktlog_mmap(struct file *file, struct vm_area_struct *vma)
{
    struct proc_dir_entry *proc_entry = PDE(file->f_dentry->d_inode);

    struct ath_pktlog_info *pl_info =
        (struct ath_pktlog_info *) proc_entry->data;

    if (vma->vm_pgoff != 0) {
        /* Entire buffer should be mapped */
        return -EINVAL;
    }

    if (!pl_info->buf) {
        printk(PKTLOG_TAG "%s: Log buffer unavailable\n", __FUNCTION__);
        return -ENOMEM;
    }

    vma->vm_flags |= VM_LOCKED;
    vma->vm_ops = &pktlog_vmops;
    pktlog_vopen(vma);
    return 0;
}

static char *pktlog_getbuf(struct ath_pktlog_info *pl_info, u_int16_t log_type, u_int16_t log_size, 
                         u_int16_t flags)
{
    struct ath_pktlog_buf *log_buf;
    int32_t buf_size;
    struct ath_pktlog_hdr *log_hdr;
    int32_t cur_wr_offset;
    char *log_ptr;

    log_buf = pl_info->buf;
    buf_size = pl_info->buf_size;

    PKTLOG_WR_LOCK(pl_info);
    cur_wr_offset = log_buf->wr_offset;
    /* Move read offset to the next entry if there is a buffer overlap */
    if (log_buf->rd_offset >= 0) {
        if ((cur_wr_offset <= log_buf->rd_offset)
            && (cur_wr_offset + sizeof(struct ath_pktlog_hdr)) >
            log_buf->rd_offset)
            PKTLOG_MOV_RD_IDX(log_buf->rd_offset, log_buf, buf_size);
    } else {
        log_buf->rd_offset = cur_wr_offset;
    }

    log_hdr =
        (struct ath_pktlog_hdr *) (log_buf->log_data + cur_wr_offset);
    log_hdr->log_type = log_type;
    log_hdr->flags = flags;
    log_hdr->timestamp = OS_GETUPTIME(NULL);
    log_hdr->size = log_size;

    cur_wr_offset += sizeof(*log_hdr);

    if ((buf_size - cur_wr_offset) < log_size) {
        while ((cur_wr_offset <= log_buf->rd_offset)
               && (log_buf->rd_offset < buf_size))
            PKTLOG_MOV_RD_IDX(log_buf->rd_offset, log_buf, buf_size);
        cur_wr_offset = 0;
    }

    while ((cur_wr_offset <= log_buf->rd_offset)
           && (cur_wr_offset + log_size) > log_buf->rd_offset)
        PKTLOG_MOV_RD_IDX(log_buf->rd_offset, log_buf, buf_size);

    log_ptr = &(log_buf->log_data[cur_wr_offset]);

    cur_wr_offset += log_hdr->size;

    log_buf->wr_offset =
        ((buf_size - cur_wr_offset) >=
         sizeof(struct ath_pktlog_hdr)) ? cur_wr_offset : 0;
    PKTLOG_WR_UNLOCK(pl_info);
   
    return log_ptr;
}

/* 
 * Log Tx data - logs into adapter's buffer if sc is not NULL; 
 *               logs into system-wide buffer if sc is NULL.
 */
static void pktlog_txctl(struct ath_softc *sc, struct log_tx *log_data,
                         u_int16_t flags)
{
    struct ath_pktlog_txctl *tx_log;
    int i, proto = PKTLOG_PROTO_NONE, proto_len = 0;
    u_int8_t misc_cnt;
    struct ath_pktlog_info *pl_info;
    struct ieee80211_frame *wh;
    u_int8_t dir;
    u_int32_t *ds_words, proto_hdr[PKTLOG_MAX_PROTO_WORDS];
    HAL_DESC_INFO desc_info;

    if (g_pktlog_mode == PKTLOG_MODE_ADAPTER)
        pl_info = sc->pl_info;
    else
        pl_info = &g_pktlog_info;

    if ((pl_info->log_state & ATH_PKTLOG_TX)== 0 || 
        log_data->firstds->ds_vdata == 0) {
        return;
    }

    misc_cnt = flags & PHFLAGS_MISCCNT_MASK;
    flags |= (sc->sc_ah->ah_macRev << PHFLAGS_MACREV_SFT) & PHFLAGS_MACREV_MASK;

    if (pl_info->options & ATH_PKTLOG_PROTO) {
        proto = pktlog_proto(sc, proto_hdr, log_data, PKTLOG_PROTO_TX_DESC,
                             &proto_len);
        flags |= (proto << PHFLAGS_PROTO_SFT) & PHFLAGS_PROTO_MASK;
    }

    tx_log = (struct ath_pktlog_txctl *)pktlog_getbuf(pl_info, 
              PKTLOG_TYPE_TXCTL, sizeof(*tx_log) + 
              misc_cnt * sizeof(tx_log->misc[0]) + proto_len, flags);

    wh = (struct ieee80211_frame *) (log_data->firstds->ds_vdata);
    tx_log->framectrl = *(u_int16_t *)(wh->i_fc);
    tx_log->seqctrl   = *(u_int16_t *)(wh->i_seq);

    dir = (wh->i_fc[1] & IEEE80211_FC1_DIR_MASK);
    if(dir == IEEE80211_FC1_DIR_TODS) {
        tx_log->bssid_tail = (wh->i_addr1[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr1[IEEE80211_ADDR_LEN-1]);
        tx_log->sa_tail    = (wh->i_addr2[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr2[IEEE80211_ADDR_LEN-1]);
        tx_log->da_tail    = (wh->i_addr3[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr3[IEEE80211_ADDR_LEN-1]);
    }
    else if(dir == IEEE80211_FC1_DIR_FROMDS) {
        tx_log->bssid_tail = (wh->i_addr2[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr2[IEEE80211_ADDR_LEN-1]);
        tx_log->sa_tail    = (wh->i_addr3[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr3[IEEE80211_ADDR_LEN-1]);
        tx_log->da_tail    = (wh->i_addr1[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr1[IEEE80211_ADDR_LEN-1]);
    }
    else {
        tx_log->bssid_tail = (wh->i_addr3[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr3[IEEE80211_ADDR_LEN-1]);
        tx_log->sa_tail    = (wh->i_addr2[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr2[IEEE80211_ADDR_LEN-1]);
        tx_log->da_tail    = (wh->i_addr1[IEEE80211_ADDR_LEN-2] << 8) | 
                             (wh->i_addr1[IEEE80211_ADDR_LEN-1]);
    }       

    ath_hal_getdescinfo(sc->sc_ah, &desc_info);

    ds_words = (u_int32_t *)(log_data->firstds) + desc_info.txctl_offset;
    for(i = 0; i < desc_info.txctl_numwords; i++)
        tx_log->txdesc_ctl[i] = ds_words[i];

    for (i = 0; i < misc_cnt; i++)
        tx_log->misc[i] = log_data->misc[i];

    if (proto != PKTLOG_PROTO_NONE)
        memcpy(tx_log->proto_hdr, proto_hdr, proto_len);
}

static void pktlog_txstatus(struct ath_softc *sc, struct log_tx *log_data,
                      u_int16_t flags)
{
    struct ath_pktlog_txstatus *tx_log;
    int i;
    u_int8_t misc_cnt;
    struct ath_pktlog_info *pl_info;
    u_int32_t *ds_words;
    HAL_DESC_INFO desc_info;

    if (g_pktlog_mode == PKTLOG_MODE_ADAPTER)
        pl_info = sc->pl_info;
    else
        pl_info = &g_pktlog_info;

    if ((pl_info->log_state & ATH_PKTLOG_TX)== 0) 
        return;

    misc_cnt = flags & PHFLAGS_MISCCNT_MASK;
    flags |= (sc->sc_ah->ah_macRev << PHFLAGS_MACREV_SFT) & PHFLAGS_MACREV_MASK;
    tx_log = (struct ath_pktlog_txstatus *)pktlog_getbuf(pl_info, PKTLOG_TYPE_TXSTATUS, 
                  sizeof(*tx_log) + misc_cnt * sizeof(tx_log->misc[0]), flags);

    ath_hal_getdescinfo(sc->sc_ah, &desc_info);

    ds_words = (u_int32_t *)(log_data->lastds) + desc_info.txstatus_offset;

    for(i = 0; i < desc_info.txstatus_numwords; i++)
        tx_log->txdesc_status[i] = ds_words[i];

    for (i = 0; i < misc_cnt; i++)
        tx_log->misc[i] = log_data->misc[i];
}

static void pktlog_rx(struct ath_softc *sc, struct log_rx *log_data,
                      u_int16_t flags)
{
    struct ath_pktlog_rx *rx_log;
    int i, proto = PKTLOG_PROTO_NONE, proto_len = 0;
    u_int8_t misc_cnt;
    struct ath_pktlog_info *pl_info;
    struct ieee80211_frame *wh;
    u_int32_t *ds_words, proto_hdr[PKTLOG_MAX_PROTO_WORDS];
    HAL_DESC_INFO desc_info;
    u_int8_t dir;

    if (g_pktlog_mode == PKTLOG_MODE_ADAPTER)
        pl_info = sc->pl_info;
    else
        pl_info = &g_pktlog_info;

    if ((pl_info->log_state & ATH_PKTLOG_RX) == 0)
        return;

    misc_cnt = flags & PHFLAGS_MISCCNT_MASK;
    flags |= (sc->sc_ah->ah_macRev << PHFLAGS_MACREV_SFT) & PHFLAGS_MACREV_MASK;

    if (pl_info->options & ATH_PKTLOG_PROTO) {
        proto = pktlog_proto(sc, proto_hdr, log_data, PKTLOG_PROTO_RX_DESC,
                             &proto_len);
        flags |= (proto << PHFLAGS_PROTO_SFT) & PHFLAGS_PROTO_MASK;
    }

    rx_log = (struct ath_pktlog_rx *)pktlog_getbuf(pl_info, PKTLOG_TYPE_RX, 
             sizeof(*rx_log) + misc_cnt * sizeof(rx_log->misc[0]) + proto_len, 
             flags);

    if(log_data->status->rs_datalen > sizeof(struct ieee80211_frame)) {
        wh = (struct ieee80211_frame *) (log_data->ds->ds_vdata);
        rx_log->framectrl = *(u_int16_t *)(wh->i_fc);
        rx_log->seqctrl   = *(u_int16_t *)(wh->i_seq);
    
        dir = (wh->i_fc[1] & IEEE80211_FC1_DIR_MASK);
        if(dir == IEEE80211_FC1_DIR_TODS) {
            rx_log->bssid_tail = (wh->i_addr1[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr1[IEEE80211_ADDR_LEN-1]);
            rx_log->sa_tail    = (wh->i_addr2[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr2[IEEE80211_ADDR_LEN-1]);
            rx_log->da_tail    = (wh->i_addr3[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr3[IEEE80211_ADDR_LEN-1]);
        } else if(dir == IEEE80211_FC1_DIR_FROMDS) {
            rx_log->bssid_tail = (wh->i_addr2[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr2[IEEE80211_ADDR_LEN-1]);
            rx_log->sa_tail    = (wh->i_addr3[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr3[IEEE80211_ADDR_LEN-1]);
            rx_log->da_tail    = (wh->i_addr1[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr1[IEEE80211_ADDR_LEN-1]);
        } else {
            rx_log->bssid_tail = (wh->i_addr3[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr3[IEEE80211_ADDR_LEN-1]);
            rx_log->sa_tail    = (wh->i_addr2[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr2[IEEE80211_ADDR_LEN-1]);
            rx_log->da_tail    = (wh->i_addr1[IEEE80211_ADDR_LEN-2] << 8) | 
                                 (wh->i_addr1[IEEE80211_ADDR_LEN-1]);
        } 
    } else {
        rx_log->framectrl = 0xFFFF;
        rx_log->seqctrl   = 0;
        rx_log->bssid_tail = 0;
        rx_log->da_tail = 0;
        rx_log->sa_tail = 0;
    } 

    ath_hal_getdescinfo(sc->sc_ah, &desc_info);

    ds_words = (u_int32_t *)(log_data->ds) + desc_info.rxstatus_offset;

    for(i = 0; i < desc_info.rxstatus_numwords; i++)
        rx_log->rxdesc_status[i] = ds_words[i];

    for (i = 0; i < misc_cnt; i++)
        rx_log->misc[i] = log_data->misc[i];

    if (proto != PKTLOG_PROTO_NONE)
        memcpy(rx_log->proto_hdr, proto_hdr, proto_len);
}

static void pktlog_ani(HAL_SOFTC hal_sc, struct log_ani *log_data,
                       u_int16_t flags)
{
    struct ath_softc *sc = (struct ath_softc *)hal_sc;
    struct ath_pktlog_ani *ani_log;
    int i;
    u_int8_t misc_cnt;
    struct ath_pktlog_info *pl_info;

    if (g_pktlog_mode == PKTLOG_MODE_ADAPTER)
        pl_info = sc->pl_info;
    else
        pl_info = &g_pktlog_info;

    if ((pl_info->log_state & ATH_PKTLOG_ANI) == 0)
        return;

    misc_cnt = flags & PHFLAGS_MISCCNT_MASK;
    flags |= (sc->sc_ah->ah_macRev << PHFLAGS_MACREV_SFT) & PHFLAGS_MACREV_MASK;
    ani_log = (struct ath_pktlog_ani *)pktlog_getbuf(pl_info, PKTLOG_TYPE_ANI, 
                  sizeof(*ani_log) + misc_cnt * sizeof(ani_log->misc[0]), flags);

    ani_log->phyStatsDisable = log_data->phyStatsDisable;
    ani_log->noiseImmunLvl = log_data->noiseImmunLvl;
    ani_log->spurImmunLvl = log_data->spurImmunLvl;
    ani_log->ofdmWeakDet = log_data->ofdmWeakDet;
    ani_log->cckWeakThr = log_data->cckWeakThr;
    ani_log->firLvl = log_data->firLvl;
    ani_log->listenTime = (u_int16_t) (log_data->listenTime);
    ani_log->cycleCount = log_data->cycleCount;
    ani_log->ofdmPhyErrCount = log_data->ofdmPhyErrCount;
    ani_log->cckPhyErrCount = log_data->cckPhyErrCount;
    ani_log->rssi = log_data->rssi;

    for (i = 0; i < misc_cnt; i++)
        ani_log->misc[i] = log_data->misc[i];
}


static void pktlog_rcfind(struct ath_softc *sc,
                          struct log_rcfind *log_data, u_int16_t flags)
{
    struct ath_pktlog_rcfind *rcf_log;
    struct TxRateCtrl_s *pRc = log_data->rc;
    int i;
    u_int8_t misc_cnt;
    struct ath_pktlog_info *pl_info;

    if (g_pktlog_mode == PKTLOG_MODE_ADAPTER)
        pl_info = sc->pl_info;
    else
        pl_info = &g_pktlog_info;

    if ((pl_info->log_state & ATH_PKTLOG_RCFIND) == 0)
        return;

    misc_cnt = flags & PHFLAGS_MISCCNT_MASK;
    flags |= (sc->sc_ah->ah_macRev << PHFLAGS_MACREV_SFT) & PHFLAGS_MACREV_MASK;
    rcf_log = (struct ath_pktlog_rcfind *)pktlog_getbuf(pl_info, PKTLOG_TYPE_RCFIND, 
                  sizeof(*rcf_log) + misc_cnt * sizeof(rcf_log->misc[0]), flags);
    
    rcf_log->rate = log_data->rate;
    rcf_log->rateCode = log_data->rateCode;
    rcf_log->rcRssiLast = pRc->rssiLast;
    rcf_log->rcRssiLastPrev = pRc->rssiLastPrev;
    rcf_log->rcRssiLastPrev2 = pRc->rssiLastPrev2;
    rcf_log->rssiReduce = log_data->rssiReduce;
    rcf_log->rcProbeRate = log_data->isProbing? pRc->probeRate:0;
    rcf_log->isProbing = log_data->isProbing;
    rcf_log->primeInUse = log_data->primeInUse;
    rcf_log->currentPrimeState = log_data->currentPrimeState;
    rcf_log->rcRateTableSize = pRc->rateTableSize;

    for (i = 0; i < misc_cnt; i++)
        rcf_log->misc[i] = log_data->misc[i];
}

static void pktlog_rcupdate(struct ath_softc *sc,
                            struct log_rcupdate *log_data, u_int16_t flags)
{
    struct ath_pktlog_rcupdate *rcu_log;
    struct TxRateCtrl_s *pRc = log_data->rc;
    int i;
    u_int8_t misc_cnt;
    struct ath_pktlog_info *pl_info;

    if (g_pktlog_mode == PKTLOG_MODE_ADAPTER)
        pl_info = sc->pl_info;
    else
        pl_info = &g_pktlog_info;

    if ((pl_info->log_state & ATH_PKTLOG_RCUPDATE) == 0)
        return;

    misc_cnt = flags & PHFLAGS_MISCCNT_MASK;
    flags |= (sc->sc_ah->ah_macRev << PHFLAGS_MACREV_SFT) & PHFLAGS_MACREV_MASK;
    rcu_log = (struct ath_pktlog_rcupdate *)pktlog_getbuf(pl_info, PKTLOG_TYPE_RCUPDATE, 
                  sizeof(*rcu_log) + misc_cnt * sizeof(rcu_log->misc[0]), flags);

    rcu_log->txRate = log_data->txRate;
    rcu_log->rateCode = log_data->rateCode;
    rcu_log->Xretries = log_data->Xretries;
    rcu_log->retries = log_data->retries;
    rcu_log->rssiAck = log_data->rssiAck;
    rcu_log->rcRssiLast = pRc->rssiLast;
    rcu_log->rcRssiLastLkup = pRc->rssiLastLkup;
    rcu_log->rcRssiLastPrev = pRc->rssiLastPrev;
    rcu_log->rcRssiLastPrev2 = pRc->rssiLastPrev2;
    rcu_log->rcProbeRate = pRc->probeRate;
    rcu_log->rcRateMax = pRc->rateMaxPhy;
    rcu_log->useTurboPrime = log_data->useTurboPrime;
    rcu_log->currentBoostState = log_data->currentBoostState;
    rcu_log->rcHwMaxRetryRate = pRc->hwMaxRetryRate;

    for (i = 0; i < MAX_TX_RATE_TBL; i++) {
        rcu_log->rcRssiThres[i] = pRc->state[i].rssiThres;
        rcu_log->rcPer[i] = pRc->state[i].per;
    }

    for (i = 0; i < misc_cnt; i++)
        rcu_log->misc[i] = log_data->misc[i];
}


/*
 * Searches for the presence of a protocol header and returns protocol type.
 * proto_len and proto_log are modified to return length (in bytes) and
 * contents of header (in host byte order), if one is found.
 */ 
static int 
pktlog_proto(struct ath_softc *sc, u_int32_t proto_log[PKTLOG_MAX_PROTO_WORDS],
             void *log_data, pktlog_proto_desc_t ds_type, int *proto_len)
{
    struct ieee80211_frame *wh = NULL;
    struct llc *llc;
    struct iphdr *ip;
    struct tcphdr *tcp;
    u_int32_t *proto_hdr;
    int i, icv_len;
    struct log_tx *tx_log;
    struct log_rx *rx_log;
    struct ieee80211_node *ni = NULL;
    static const int pktlog_proto_min_hlen = sizeof(struct ieee80211_frame) + 
                                             sizeof(struct llc) + 
                                             sizeof(struct iphdr); 

    switch (ds_type) {
        case PKTLOG_PROTO_TX_DESC:
            tx_log = (struct log_tx *)log_data;
            if (tx_log->bf->bf_skb->len < pktlog_proto_min_hlen)
                return PKTLOG_PROTO_NONE;

            wh = (struct ieee80211_frame *)(tx_log->firstds->ds_vdata);
            ni = (((struct ieee80211_cb *)(tx_log->bf->bf_skb)->cb)->ni);
            break;
        case PKTLOG_PROTO_RX_DESC:
            rx_log = (struct log_rx *)log_data;
            if (rx_log->status->rs_datalen < pktlog_proto_min_hlen)
                return PKTLOG_PROTO_NONE;
            /*
             * Positively need to find ni for cipher
             */ 
            if (rx_log->status->rs_keyix == HAL_RXKEYIX_INVALID || 
               ((ni = sc->sc_keyixmap[rx_log->status->rs_keyix]) == NULL)) {
                return PKTLOG_PROTO_NONE;
            }

            wh = (struct ieee80211_frame *)(rx_log->ds->ds_vdata);
            break;
    }

    if ((wh->i_fc[0] & IEEE80211_FC0_TYPE_MASK) != IEEE80211_FC0_TYPE_DATA)
        return PKTLOG_PROTO_NONE;

    if (ni->ni_ucastkey.wk_cipher == &ieee80211_cipher_none) {
        icv_len = 0;
    } else {
        struct ieee80211_key *k = &ni->ni_ucastkey;
        const struct ieee80211_cipher *cip = k->wk_cipher;

        icv_len = cip->ic_header;
    }

    llc = (struct llc *)((u_int8_t *)wh + ieee80211_anyhdrspace(&sc->sc_ic, wh) 
           + icv_len);
    ip = (struct iphdr *)((u_int8_t *)llc + LLC_SNAPFRAMELEN);

    switch (ip->protocol) {
        case IPPROTO_TCP:
            /* 
             * tcp + ip hdr len are in units of 32-bit words
             */ 
            tcp = (struct tcphdr *)((u_int32_t *)ip + ip->ihl);
            proto_hdr = (u_int32_t *)tcp;

            for (i = 0; i < tcp->doff; i++) {
                *proto_log = ntohl(*proto_hdr);
                proto_log++, proto_hdr++;
            }
            *proto_len = tcp->doff * sizeof(u_int32_t);
            return PKTLOG_PROTO_TCP;
        case IPPROTO_UDP:
        case IPPROTO_ICMP:
        default:
            return PKTLOG_PROTO_NONE;
    }
}
