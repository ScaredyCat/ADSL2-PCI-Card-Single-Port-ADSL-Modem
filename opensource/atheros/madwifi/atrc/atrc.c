#include <linux/module.h>
#include <linux/fs.h>

#if !defined(__LINUX_ARM_ARCH__) && !defined(__LINUX_MIPS32_ARCH__)
#include <asm/msr.h>
#else
#define rdtsc(_tsh, _tsl) (_tsl) = jiffies
#endif
//hsumc
#include <linux/init.h>
#include <asm/uaccess.h>

#include "atrc.h"

MODULE_LICENSE("GPL");

/*
 *----------------------------------------------------------------------
 * local definitions
 *----------------------------------------------------------------------
 */
#define ATRC_MAJOR      77
#define ATRC_NAME       "atrc"

#define __atrc_inc(_i) do {     \
        (_i)++;                 \
        if ((_i) >= ATRC_MAX)   \
            (_i) = 0;           \
    } while (0)

struct semaphore    atrc_sem;
atrc_t       atrc;

#define __atrc_lock_init()  sema_init(&atrc_sem, 1)
#define __atrc_lock()       down(&atrc_sem)
#define __atrc_unlock()     up(&atrc_sem)

#define __atrc_tsl(_tsh, _tsl)  rdtsc(_tsh, _tsl)

/*
 *----------------------------------------------------------------------
 * forward declarations
 *----------------------------------------------------------------------
 */
static int __init atrcm_init(void);
static void __exit atrcm_exit(void);
static int atrc_open(struct inode *inode, struct file *file);
static ssize_t atrc_read(struct file * file, char * buf, size_t count,
        loff_t *ppos);

static struct file_operations atrc_fops = {
    owner:    THIS_MODULE,
    open:     atrc_open,
    read:     atrc_read,
};

static const char atrc_name[] = ATRC_NAME;

module_init(atrcm_init);
module_exit(atrcm_exit);

/*
 *----------------------------------------------------------------------
 * exported functions
 *----------------------------------------------------------------------
 */

int
__atrc_file(char *file)
{
    int     nfile = atrc.nfiles;

    strcpy(atrc.files[nfile], file);
    atrc.nfiles ++;

    return nfile;
}

void
__atrc(uint32_t value, int file, int line)
{
    int tail;
    atrc_t *atrcp = &atrc;

    if (!atrc.stopped) {
        __atrc_lock();

        if (atrc.asserted) {
            atrc.trcleft --;
            if (!atrc.trcleft)
                atrc.stopped = 1;
        }

        tail = atrcp->tail;
        tail = atrc.tail;
        __atrc_inc(atrc.tail);
        if (atrc.tail == atrc.head)
            __atrc_inc(atrc.head);

        __atrc_unlock();

        atrc.atrc[tail].value = value;
        atrc.atrc[tail].file  = file;
        atrc.atrc[tail].line  = line;
        __atrc_tsl(atrc.atrc[tail].tsl, atrc.atrc[tail].tsh);
    }
}

void
__atrcl(uint32_t value, int file, int line)
{
    int tail;
    atrc_t *atrcp = &atrc;

    if (!atrc.stopped) {
        if (atrc.asserted) {
            atrc.trcleft --;
            if (!atrc.trcleft)
                atrc.stopped = 1;
        }

        tail = atrcp->tail;
        tail = atrc.tail;
        __atrc_inc(atrc.tail);
        if (atrc.tail == atrc.head)
            __atrc_inc(atrc.head);

        atrc.atrc[tail].value = value;
        atrc.atrc[tail].file  = file;
        atrc.atrc[tail].line  = line;
        __atrc_tsl(atrc.atrc[tail].tsl, atrc.atrc[tail].tsh);
    }
}

void
__atrc_assert(char *cond, int file, int line)
{
    __atrc(0x666, file, line);
    if (!atrc.asserted) {
        /*
         * stop trace 10% after trigger
         */
        atrc.trcleft  = 0; //ATRC_MAX / 10;
        atrc.asserted = 1;
        atrc.stopped  = 1;
    }
}

EXPORT_SYMBOL(__atrc_file);
EXPORT_SYMBOL(__atrc);
EXPORT_SYMBOL(__atrcl);
EXPORT_SYMBOL(__atrc_assert);
EXPORT_SYMBOL(atrc);

/*
 *----------------------------------------------------------------------
 * local functions
 *----------------------------------------------------------------------
 */

static int __init
atrcm_init(void)
{
    static int atrc_major;

    atrc_major = register_chrdev(ATRC_MAJOR, atrc_name, &atrc_fops);

    if (atrc_major < 0) {
        printk(KERN_WARNING "atrc: cannot alloc major number\n");
        return atrc_major;
    }

    printk(KERN_INFO "atrc: major %d ptr 0x%p\n", atrc_major, &atrc);
    __atrc_lock_init();

    return 0;
}

static void __exit
atrcm_exit(void)
{
    unregister_chrdev(ATRC_MAJOR, atrc_name);
}

static int
atrc_open(struct inode *inode, struct file *file)
{
    return 0;
}

static ssize_t
atrc_read(struct file * file, char * buf, size_t count, loff_t *ppos)
{
    uint8_t *abuf = (uint8_t *)&atrc;
    loff_t  pos = *ppos;
    size_t      acount;

    acount = sizeof(atrc) - pos;
    if (count > acount)
        count = acount;

    if (count > 0)
        copy_to_user(buf, abuf + pos, count);

    *ppos += count;

    return count;
}


