/******************************************************************************
**
** FILE NAME    : danube_gptu.c
** PROJECT      : Danube
** MODULES     	: GPTU
**
** DATE         : 26 JUL 2005
** AUTHOR       : Xu Liang
** DESCRIPTION  : General Purpose Timer Unit (GPTU) Driver
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
** $Date        $Author         $Comment
** 26 JUL 2005  Xu Liang        Initiate Version
** 30 AUG 2006  Xu Liang        Refine function "set_counter".
** 23 OCT 2006  Xu Liang        Add GPL header.
*******************************************************************************/



/*
 * ####################################
 *              Head File
 * ####################################
 */

/*
 *  Common Head File
 */
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/unistd.h>
#include <asm/irq.h>
#include <linux/errno.h>

/*
 *  Chip Specific Head File
 */

#include <asm/danube/irq.h>
#include <asm/danube/danube_cgu.h>
#include <asm/danube/danube_gptu.h>


/*
 * ####################################
 *              Definition
 * ####################################
 */

#define DEBUG_ON_AMAZON                 0

#define TEST_TIMER_XULIANG              0

#define MAX_NUM_OF_32BIT_TIMER_BLOCKS   6

#ifdef TIMER1A
  #define FIRST_TIMER                   TIMER1A
#else
  #define FIRST_TIMER                   2
#endif

/*
 *  GPTC divider is set or not.
 */
#define GPTU_CLC_RMC_IS_SET             0

/*
 *  Timer Interrupt (IRQ)
 */
#define TIMER_INTERRUPT                 INT_NUM_IM3_IRL22   //  Must be adjusted when ICU driver is available

/*
 *  Bits Operation
 */
#define GET_BITS(x, msb, lsb)           (((x) & ((1 << ((msb) + 1)) - 1)) >> (lsb))
#define SET_BITS(x, msb, lsb, value)    (((x) & ~(((1 << ((msb) + 1)) - 1) ^ ((1 << (lsb)) - 1))) | (((value) & ((1 << (1 + (msb) - (lsb))) - 1)) << (lsb)))

/*
 *  GPTU Register Mapping
 */
#define DANUBE_GPTU                     (KSEG1 + 0x1E100A00)
#define DANUBE_GPTU_CLC                 ((volatile u32*)(DANUBE_GPTU + 0x0000))
#define DANUBE_GPTU_ID                  ((volatile u32*)(DANUBE_GPTU + 0x0008))
#define DANUBE_GPTU_CON(n, X)           ((volatile u32*)(DANUBE_GPTU + 0x0010 + ((X) * 4) + ((n) - 1) * 0x0020))    //  X must be either A or B
#define DANUBE_GPTU_RUN(n, X)           ((volatile u32*)(DANUBE_GPTU + 0x0018 + ((X) * 4) + ((n) - 1) * 0x0020))    //  X must be either A or B
#define DANUBE_GPTU_RELOAD(n, X)        ((volatile u32*)(DANUBE_GPTU + 0x0020 + ((X) * 4) + ((n) - 1) * 0x0020))    //  X must be either A or B
#define DANUBE_GPTU_COUNT(n, X)         ((volatile u32*)(DANUBE_GPTU + 0x0028 + ((X) * 4) + ((n) - 1) * 0x0020))    //  X must be either A or B
#define DANUBE_GPTU_IRNEN               ((volatile u32*)(DANUBE_GPTU + 0x00F4))
#define DANUBE_GPTU_IRNICR              ((volatile u32*)(DANUBE_GPTU + 0x00F8))
#define DANUBE_GPTU_IRNCR               ((volatile u32*)(DANUBE_GPTU + 0x00FC))

/*
 *  Clock Control Register
 */
#define GPTU_CLC_SMC                    GET_BITS(*DANUBE_GPTU_CLC, 23, 16)
#define GPTU_CLC_RMC                    GET_BITS(*DANUBE_GPTU_CLC, 15, 8)
#define GPTU_CLC_FSOE                   (*DANUBE_GPTU_CLC & (1 << 5))
#define GPTU_CLC_EDIS                   (*DANUBE_GPTU_CLC & (1 << 3))
#define GPTU_CLC_SPEN                   (*DANUBE_GPTU_CLC & (1 << 2))
#define GPTU_CLC_DISS                   (*DANUBE_GPTU_CLC & (1 << 1))
#define GPTU_CLC_DISR                   (*DANUBE_GPTU_CLC & (1 << 0))

#define GPTU_CLC_SMC_SET(value)         SET_BITS(0, 23, 16, (value))
#define GPTU_CLC_RMC_SET(value)         SET_BITS(0, 15, 8, (value))
#define GPTU_CLC_FSOE_SET(value)        ((value) ? (1 << 5) : 0)
#define GPTU_CLC_SBWE_SET(value)        ((value) ? (1 << 4) : 0)
#define GPTU_CLC_EDIS_SET(value)        ((value) ? (1 << 3) : 0)
#define GPTU_CLC_SPEN_SET(value)        ((value) ? (1 << 2) : 0)
#define GPTU_CLC_DISR_SET(value)        ((value) ? (1 << 0) : 0)

/*
 *  ID Register
 */
#define GPTU_ID_ID                      GET_BITS(*DANUBE_GPTU_ID, 15, 8)
#define GPTU_ID_CFG                     GET_BITS(*DANUBE_GPTU_ID, 7, 5)
#define GPTU_ID_REV                     GET_BITS(*DANUBE_GPTU_ID, 4, 0)

/*
 *  Control Register of Timer/Counter nX
 *    n is the index of block (1 based index)
 *    X is either A or B
 */
#define GPTU_CON_SRC_EG(n, X)           (*DANUBE_GPTU_CON(n, X) & (1 << 10))
#define GPTU_CON_SRC_EXT(n, X)          (*DANUBE_GPTU_CON(n, X) & (1 << 9))
#define GPTU_CON_SYNC(n, X)             (*DANUBE_GPTU_CON(n, X) & (1 << 8))
#define GPTU_CON_EDGE(n, X)             GET_BITS(*DANUBE_GPTU_CON(n, X), 7, 6)
#define GPTU_CON_INV(n, X)              (*DANUBE_GPTU_CON(n, X) & (1 << 5))
#define GPTU_CON_EXT(n, X)              (*DANUBE_GPTU_CON(n, A) & (1 << 4)) //  Timer/Counter B does not have this bit
#define GPTU_CON_STP(n, X)              (*DANUBE_GPTU_CON(n, X) & (1 << 3))
#define GPTU_CON_CNT(n, X)              (*DANUBE_GPTU_CON(n, X) & (1 << 2))
#define GPTU_CON_DIR(n, X)              (*DANUBE_GPTU_CON(n, X) & (1 << 1))
#define GPTU_CON_EN(n, X)               (*DANUBE_GPTU_CON(n, X) & (1 << 0))

#define GPTU_CON_SRC_EG_SET(value)      ((value) ? 0 : (1 << 10))
#define GPTU_CON_SRC_EXT_SET(value)     ((value) ? (1 << 9) : 0)
#define GPTU_CON_SYNC_SET(value)        ((value) ? (1 << 8) : 0)
#define GPTU_CON_EDGE_SET(value)        SET_BITS(0, 7, 6, (value))
#define GPTU_CON_INV_SET(value)         ((value) ? (1 << 5) : 0)
#define GPTU_CON_EXT_SET(value)         ((value) ? (1 << 4) : 0)
#define GPTU_CON_STP_SET(value)         ((value) ? (1 << 3) : 0)
#define GPTU_CON_CNT_SET(value)         ((value) ? (1 << 2) : 0)
#define GPTU_CON_DIR_SET(value)         ((value) ? (1 << 1) : 0)

/*
 *  Run Register of Timer/Counter nX
 */
#define GPTU_RUN_RL_SET(value)          ((value) ? (1 << 2) : 0)
#define GPTU_RUN_CEN_SET(value)         ((value) ? (1 << 1) : 0)
#define GPTU_RUN_SEN_SET(value)         ((value) ? (1 << 0) : 0)

/*
 *  Reload Register of Timer/Counter nX
 *    n is the index of block (1 based index)
 *    X is either A or B
 */
#define GPTU_RELOAD_VALUE(n, X)         (X == 0 && GPTU_CON_EXT(n, X) ? *DANUBE_GPTU_RELOAD(n, X) : *DANUBE_GPTU_RELOAD(n, X) & 0xFFFF)

/*
 *  Count Register of Timer/Counter nX
 *    n is the index of block (1 based index)
 *    X is either A or B
 */
#define GPTU_COUNT_VALUE(n, X)          (X == A && GPTU_CON_EXT(n, X) ? *DANUBE_GPTU_COUNT(n, X) : *DANUBE_GPTU_COUNT(n, X) & 0xFFFF)

/*
 *  Interrupt Node Enable Register
 */
#define GPTU_IRNEN_TC(n, X)             (*DANUBE_GPTU_IRNEN & (1 << (((n) - 1) * 2 + (X))))

#define GPTU_IRNEN_TC_SET(n, X, value)  ((value) ? (1 << (((n) - 1) * 2 + (X))) : 0)

/*
 *  Interrupt Capture Register
 */
#define GPTU_IRNICR_TC(n, X)            (*DANUBE_GPTU_IRNICR & (1 << (((n) - 1) * 2 + (X))))

#define GPTU_IRNICR_TC_SET(n, X, value) ((value) ? (1 << (((n) - 1) * 2 + (X))) : 0)

/*
 *  Interrupt Node Control Register
 */
#define GPTU_IRNCR_TC(n, X)             (*DANUBE_GPTU_IRNCR & (1 << (((n) - 1) * 2 + (X))))

#define GPTU_IRNCR_TC_SET(n, X, value)  ((value) ? (1 << (((n) - 1) * 2 + (X))) : 0)

/*
 *  Mask of timer flags and some reserved flags.
 */
#define TIMER_FLAG_MASK_SIZE(x)         (x & 0x0001)
#define TIMER_FLAG_MASK_TYPE(x)         (x & 0x0002)
#define TIMER_FLAG_MASK_STOP(x)         (x & 0x0004)
#define TIMER_FLAG_MASK_DIR(x)          (x & 0x0008)
#define TIMER_FLAG_NONE_EDGE            0x0000
#define TIMER_FLAG_MASK_EDGE(x)         (x & 0x0030)
#define TIMER_FLAG_REAL                 0x0000
#define TIMER_FLAG_INVERT               0x0040
#define TIMER_FLAG_MASK_INVERT(x)       (x & 0x0040)
#define TIMER_FLAG_MASK_TRIGGER(x)      (x & 0x0070)
#define TIMER_FLAG_MASK_SYNC(x)         (x & 0x0080)
#define TIMER_FLAG_CALLBACK_IN_HB       0x0200
#define TIMER_FLAG_MASK_HANDLE(x)       (x & 0x0300)
#define TIMER_FLAG_MASK_SRC(x)          (x & 0x1000)


/*
 * ####################################
 * Preparation of Debug on Amazon Chip
 * ####################################
 */

/*
 *  If try module on Amazon chip, prepare some tricks to prevent invalid memory write.
 */
#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    u32 g_pFakeRegisters[0x0100];

    #undef  DANUBE_GPTU
    #define DANUBE_GPTU                 ((u32)g_pFakeRegisters)
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON


/*
 * ####################################
 *              Data Type
 * ####################################
 */

struct timer_dev_timer {
    unsigned int                        f_irq_on;
    unsigned int                        irq;
    unsigned int                        flag;
    unsigned long                       arg1;
    unsigned long                       arg2;
};

struct timer_dev {
    struct semaphore                    sem;
    unsigned int                        number_of_timers;
    unsigned int                        occupation;
    unsigned int                        f_gptu_on;
    struct timer_dev_timer              timer[MAX_NUM_OF_32BIT_TIMER_BLOCKS * 2];
};


/*
 * ####################################
 *             Declaration
 * ####################################
 */

/*
 *  File Operations
 */
static int gptu_ioctl(struct inode *, struct file *, unsigned int, unsigned long);
static int gptu_open(struct inode *, struct file *);
static int gptu_release(struct inode *, struct file *);

/*
 *  Interrupt Handler
 */
static void timer_irq_handler(int, struct timer_dev_timer *, struct pt_regs *);

/*
 *  GPTU Register Operation
 */
static inline void enable_gptu(void);
static inline void disable_gptu(void);

/*
 *  Pre-declaration of 64-bit Unsigned Integer Operation
 */
static inline void uint64_multiply(unsigned int, unsigned int, unsigned int *);
static inline void uint64_divide(unsigned int *, unsigned int, unsigned int *, unsigned int *);

#if defined(TEST_TIMER_XULIANG) && TEST_TIMER_XULIANG
  static void callback(unsigned long);
  void test_timer(void);
#endif

/*
 *  Export Functions
 */
int request_timer(unsigned int, unsigned int, unsigned long, unsigned long, unsigned long);
int free_timer(unsigned int);
int start_timer(unsigned int, int);
int stop_timer(unsigned int);
int get_count_value(unsigned int, unsigned long *);

unsigned long cal_divider(unsigned long);

int set_timer(unsigned int, unsigned int, int, int, unsigned int, unsigned long, unsigned long);
int set_counter(unsigned int, int, int, int, unsigned int, unsigned int, unsigned long, unsigned long);


/*
 * ####################################
 *            Local Variable
 * ####################################
 */

static struct file_operations gptu_fops = {
    owner:      THIS_MODULE,
    ioctl:      gptu_ioctl,
    open:       gptu_open,
    release:    gptu_release
};

static struct miscdevice gptu_miscdev = {
    MISC_DYNAMIC_MINOR,
    "gptu",
    &gptu_fops,
    NULL,
    NULL,
    NULL
};

/*
 *  Used for Interrupt Handler
 */
static struct timer_dev timer_dev;

#if defined(TEST_TIMER_XULIANG) && TEST_TIMER_XULIANG
  static int timer;
#endif


/*
 * ####################################
 *           Global Variable
 * ####################################
 */


/*
 * ####################################
 *            Local Function
 * ####################################
 */

/*
 *  Description:
 *    Handle all ioctl command. This is the only way, which user level could
 *    use to access GPTU driver.
 *  Input:
 *    inode --- struct inode *, file descriptor on drive
 *    file  --- struct file *, file descriptor of virtual file system
 *    cmd   --- unsigned int, device specific commands
 *              1. GPTU_REQUEST_TIMER     - General method to setup
 *                                          timer/counter.
 *              2. GPTU_FREE_TIMER        - Free allocated timer/counter.
 *              3. GPTU_START_TIMER       - Start or resume timer/counter.
 *              4. GPTU_STOP_TIMER        - Suspend timer/counter.
 *              5. GPTU_GET_COUNT_VALUE   - Get current count value.
 *              6. GPTU_CALCULATE_DIVIDER - Calculate timer divider from given
 *                                          frequency (0.001Hz).
 *              7. GPTU_SET_TIMER         - Simplified method to setup timer
 *                                          with given frequency (0.001Hz).
 *              8. GPTU_SET_COUNTER       - Simplified method to setup event
 *                                          counter.
 *    arg   --- unsigned long, pointer to a structure gptu_ioctl_param to pass
 *              in arguments or pass out return value. Members are listed below:
 *              1. timer - In command GPTU_REQUEST_TIMER, GPTU_SET_TIMER, and
 *                         GPTU_SET_COUNTER, this field is ID of expected
 *                         timer/counter. If this field is zero, a timer/counter
 *                         would be allocated by driver and ID would be stored
 *                         in this field.
 *                         In command GPTU_GET_COUNT_VALUE, this field is
 *                         ignored.
 *                         In other command, this field is ID of timer/counter,
 *                         which is requested already.
 *              2. flag  - In command GPTU_REQUEST_TIMER, GPTU_SET_TIMER, and
 *                         GPTU_SET_COUNTER, this field contains flags to
 *                         specify how the timer/counter should be configured.
 *                         a) For GPTU_REQUEST_TIMER, all flag could be used.
 *                         b) For GPTU_SET_TIMER, only flag TIMER_FLAG_ONCE,
 *                            TIMER_FLAG_CYCLIC, TIMER_FLAG_NO_HANDLE, and
 *                            TIMER_FLAG_SIGNAL could be used.
 *                         c) For GPTU_SET_COUNTER, only flag TIMER_FLAG_16BIT,
 *                            TIMER_FLAG_32BIT, TIMER_FLAG_UNSYNC,
 *                            TIMER_FLAG_SYNC, TIMER_FLAG_HIGH_LEVEL_SENSITIVE,
 *                            TIMER_FLAG_LOW_LEVEL_SENSITIVE,
 *                            TIMER_FLAG_RISE_EDGE, TIMER_FLAG_FALL_EDGE, and
 *                            TIMER_FLAG_ANY_EDGE could be used.
 *                         In command GPTU_START_TIMER, zero indicates start
 *                         and non-zero indicates resume. Start means an init
 *                         value would be loaded, however, resume would continue
 *                         counter with the current count value.
 *                         In other command, this field is ignored.
 *              3. value - In command GPTU_REQUEST_TIMER, this field contains
 *                         init/reload value.
 *                         In command GPTU_SET_TIMER, this field contains
 *                         frequency (0.001Hz) of timer.
 *                         In command GPTU_GET_COUNT_VALUE, current count value
 *                         would be stored in this field as output.
 *                         In command GPTU_CALCULATE_DIVIDER, this field
 *                         contains frequency (0.001Hz) as input, and after
 *                         calculation, divider would be stored in this field to
 *                         overwrite the frequency as output.
 *                         In other command, this field is ignored.
 *              4. pid   - In command GPTU_REQUEST_TIMER and GPTU_SET_TIMER, if
 *                         signal is required, this field contains process ID to
 *                         which signal would be sent when overflow/underflow.
 *                         In other command, this field is ignored.
 *              5. sig   - In command GPTU_REQUEST_TIMER and GPTU_SET_TIMER, if
 *                         signal is required, this field contains signal number
 *                         which would be sent when overflow/underflow.
 *                         In other command, this field is ignored.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */
static int gptu_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret;
    struct gptu_ioctl_param param;

    if ( !access_ok(VERIFY_READ, arg, sizeof(struct gptu_ioctl_param)) )
        return -EFAULT;
    copy_from_user(&param, (void*)arg, sizeof(param));

    if ( (((cmd == GPTU_REQUEST_TIMER || cmd == GPTU_SET_TIMER || GPTU_SET_COUNTER) && param.timer < 2)
        || cmd == GPTU_GET_COUNT_VALUE || cmd == GPTU_CALCULATE_DIVIDER)
        && !access_ok(VERIFY_WRITE, arg, sizeof(struct gptu_ioctl_param)) )
        return -EFAULT;

    switch ( cmd )
    {
    case GPTU_REQUEST_TIMER:
        ret = request_timer(param.timer, param.flag, param.value, (unsigned long)param.pid, (unsigned long)param.sig);
        if ( ret > 0 )
        {
            copy_to_user(&((struct gptu_ioctl_param *)arg)->timer, &ret, sizeof(&ret));
            ret = 0;
        }
        break;
    case GPTU_FREE_TIMER:
        ret = free_timer(param.timer);
        break;
    case GPTU_START_TIMER:
        ret = start_timer(param.timer, param.flag);
        break;
    case GPTU_STOP_TIMER:
        ret = stop_timer(param.timer);
        break;
    case GPTU_GET_COUNT_VALUE:
        ret = get_count_value(param.timer, &param.value);
        if ( !ret )
            copy_to_user(&((struct gptu_ioctl_param *)arg)->value, &param.value, sizeof(param.value));
        break;
    case GPTU_CALCULATE_DIVIDER:
        param.value = cal_divider(param.value);
        if ( param.value == 0 )
            ret = -EINVAL;
        else
        {
            copy_to_user(&((struct gptu_ioctl_param *)arg)->value, &param.value, sizeof(param.value));
            ret = 0;
        }
        break;
    case GPTU_SET_TIMER:
        ret = set_timer(param.timer, param.value,
                        TIMER_FLAG_MASK_STOP(param.flag) != TIMER_FLAG_ONCE ? 1 : 0,
                        TIMER_FLAG_MASK_SRC(param.flag) == TIMER_FLAG_EXT_SRC ? 1 : 0,
                        TIMER_FLAG_MASK_HANDLE(param.flag) == TIMER_FLAG_SIGNAL ? TIMER_FLAG_SIGNAL : TIMER_FLAG_NO_HANDLE,
                        (unsigned long)param.pid, (unsigned long)param.sig);
        if ( ret > 0 )
        {
            copy_to_user(&((struct gptu_ioctl_param *)arg)->timer, &ret, sizeof(&ret));
            ret = 0;
        }
        break;
    case GPTU_SET_COUNTER:
        ret = set_counter(param.timer,
                          TIMER_FLAG_MASK_SIZE(param.flag) != TIMER_FLAG_16BIT ? 1 : 0,
                          TIMER_FLAG_MASK_SYNC(param.flag) != TIMER_FLAG_UNSYNC ? 1 : 0,
                          TIMER_FLAG_MASK_SRC(param.flag) == TIMER_FLAG_EXT_SRC ? 1 : 0,
                          TIMER_FLAG_MASK_TRIGGER(param.flag),
                          TIMER_FLAG_NO_HANDLE,
                          0, 0);
        if ( ret > 0 )
        {
            copy_to_user(&((struct gptu_ioctl_param *)arg)->timer, &ret, sizeof(&ret));
            ret = 0;
        }
        break;
    default:
        ret = -ENOTTY;
    }

    return ret;
}

static int gptu_open(struct inode *inode, struct file *file)
{
    return 0;
}

static int gptu_release(struct inode *inode, struct file *file)
{
    return 0;
}

/*
 *  Description:
 *    This is a central timer interrupt handler, which dispatches interrupt to
 *    corresponding handler or send signal according to timer/counter flag.
 *  Input:
 *    irq       --- int, interrupt number (IRQ).
 *    dev_timer --- struct timer_dev_id*, a local structure contains specific
 *                  info.
 *    regs      --- struct pt_regs*, registers before jumping to interrupt
 *                  handler.
 *  Output:
 *    none
 */
static void timer_irq_handler(int irq, struct timer_dev_timer *dev_timer, struct pt_regs *regs)
{
    unsigned int timer;
    unsigned int flag;

    timer = irq - TIMER_INTERRUPT;
    if ( timer < timer_dev.number_of_timers && dev_timer == &timer_dev.timer[timer] )
    {
        /*  Clear interrupt.    */
        *DANUBE_GPTU_IRNCR = 1 << timer;

        /*  Call user hanler or signal. */
        flag = dev_timer->flag;
        if ( !(timer & 0x01) || TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT )    /* 16-bit timer or timer A of 32-bit timer  */
            switch ( TIMER_FLAG_MASK_HANDLE(flag) )
            {
            case TIMER_FLAG_CALLBACK_IN_IRQ:
            case TIMER_FLAG_CALLBACK_IN_HB:
                if ( dev_timer->arg1 )
                    (*(timer_callback)dev_timer->arg1)(dev_timer->arg2);
                break;
            case TIMER_FLAG_SIGNAL:
                send_sig((int)dev_timer->arg2, (struct task_struct *)dev_timer->arg1, 0);
                break;
            }
    }
}

/*
 *  Description:
 *    Set clock control register to enable GPTU module. The default unit core
 *    frequency is same as FPI bus.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static inline void enable_gptu(void)
{
    /*  Activate GPTU module in PMU.    */
    int i = 1000000;

    *(unsigned long*)0xBF10201C &= ~(1 << 12);
    while ( --i && (*(unsigned long*)0xBF102020 & (1 << 12)) );
    if ( !i )
        panic("Activating GPT in PMU failed!");

    /*  Set divider as 1, disable write protection for SPEN, enable module. */
    *DANUBE_GPTU_CLC = GPTU_CLC_SMC_SET(0x00) | GPTU_CLC_RMC_SET(0x01) | GPTU_CLC_FSOE_SET(0)
                     | GPTU_CLC_SBWE_SET(1) | GPTU_CLC_EDIS_SET(0) | GPTU_CLC_SPEN_SET(0)
                     | GPTU_CLC_DISR_SET(0);
}

/*
 *  Description:
 *    Clear and mask off all interrupts and set clock control register to
 *    disable GPTU module.
 *  Input:
 *    none
 *  Output:
 *    none
 */
static inline void disable_gptu(void)
{
    /*  Clear interrupt.    */
    *DANUBE_GPTU_IRNEN = 0x0000;
    *DANUBE_GPTU_IRNCR = 0x0FFF;
#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    *DANUBE_GPTU_IRNCR = 0x0000;
    *DANUBE_GPTU_IRNICR = 0x0000;
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

    /*  Set divider as 0, enable write protection for SPEN, disable module. */
    *DANUBE_GPTU_CLC = GPTU_CLC_SMC_SET(0x00) | GPTU_CLC_RMC_SET(0x00) | GPTU_CLC_FSOE_SET(0)
                    | GPTU_CLC_SBWE_SET(0) | GPTU_CLC_EDIS_SET(0) | GPTU_CLC_SPEN_SET(0)
                     | GPTU_CLC_DISR_SET(1);

    /*  Inactivating GPTU module in PMU.    */
    *(unsigned long*)0xBF10201C |= 1 << 12;
}

/*
 *  Description:
 *    calculate 64-bit multiplication result of two 32-bit unsigned integer
 *  Input:
 *    u32Multiplier1 --- u32 (32-bit), one of the multipliers
 *    u32Multiplier2 --- u32 (32-bit), the other multiplier
 *    u32Result      --- u32[2], array to retrieve the multiplication result,
 *                       index 0 is high word, index 1 is low word
 *  Output:
 *    none
 */
static inline void uint64_multiply(u32 u32Multiplier1, u32 u32Multiplier2, u32 u32Result[2])
{
	u32 u32Multiplier1LowWord = u32Multiplier1 & 0xFFFF;
	u32 u32Multiplier1HighWord = u32Multiplier1 >> 16;
	u32 u32Multiplier2LowWord = u32Multiplier2 & 0xFFFF;
	u32 u32Multiplier2HighWord = u32Multiplier2 >> 16;
	u32 u32Combo1, u32Combo2, u32Combo3, u32Combo4;
	u32 u32Word1, u32Word2, u32Word3, u32Word4;

	u32Combo1 = u32Multiplier1LowWord * u32Multiplier2LowWord;
	u32Combo2 = u32Multiplier1HighWord * u32Multiplier2LowWord;
	u32Combo3 = u32Multiplier1LowWord * u32Multiplier2HighWord;
	u32Combo4 = u32Multiplier1HighWord * u32Multiplier2HighWord;

	u32Word1 = u32Combo1 & 0xFFFF;
	u32Word2 = (u32Combo1 >> 16) + (u32Combo2 & 0xFFFF) + (u32Combo3 & 0xFFFF);
	u32Word3 = (u32Combo2 >> 16) + (u32Combo3 >> 16) + (u32Combo4 & 0xFFFF) + (u32Word2 >> 16);
	u32Word4 = (u32Combo4 >> 16) + (u32Word3 >> 16);

	u32Result[0] = (u32Word4 << 16) | u32Word3;
	u32Result[1] = (u32Word2 << 16) | u32Word1;
}

/*
 *  Description:
 *    divide 64-bit unsigned integer with 32-bit unsigned integer
 *  Input:
 *    u32Numerator   --- u32[2], index 0 is high word of numerator, while
 *                       index 1 is low word of numerator
 *    u32Denominator --- u32 (32-bit), the denominator in division, this
 *                       parameter can not be zero, or lead to unpredictable
 *                       result
 *    pu32Quotient   --- u32 *, the pointer to retrieve 32-bit quotient, null
 *                       pointer means ignore quotient
 *    pu32Residue    --- u32 *, the pointer to retrieve 32-bit residue null
 *                       pointer means ignore residue
 *  Output:
 *    none
 */
static inline void uint64_divide(u32 u32Numerator[2], u32 u32Denominator, u32 *pu32Quotient, u32 *pu32Residue)
{
	u32 u32DWord1, u32DWord2, u32DWord3;
	u32 u32Quotient;
	int i;

	u32DWord3 = 0;
	u32DWord2 = u32Numerator[0];
	u32DWord1 = u32Numerator[1];

	u32Quotient = 0;

	for ( i = 0; i < 64; i++ )
	{
		u32DWord3 = (u32DWord3 << 1) | (u32DWord2 >> 31);
		u32DWord2 = (u32DWord2 << 1) | (u32DWord1 >> 31);
		u32DWord1 <<= 1;
		u32Quotient <<= 1;
		if ( u32DWord3 >= u32Denominator )
		{
			u32DWord3 -= u32Denominator;
			u32Quotient |= 1;
		}
	}
	if ( pu32Quotient )
	    *pu32Quotient = u32Quotient;
	if ( pu32Residue )
	    *pu32Residue = u32DWord3;
}

#if defined(TEST_TIMER_XULIANG) && TEST_TIMER_XULIANG

static void callback(unsigned long arg)
{
    static int counter = 0;

    printk("+++ timer callback (%d) +++\n", counter++);
//    free_timer(timer);
}

void test_timer(void)
{
    int ret;

    timer = TIMER2A;
    if ( timer == TIMER_ANY )
    {
        timer = set_timer(TIMER_ANY, 100, 1, 0, TIMER_FLAG_CALLBACK_IN_IRQ, callback, 0);
        printk("set_timer = %d\n", timer);
    }
    else
    {
        ret = set_timer(timer, 100, 1, 0, TIMER_FLAG_CALLBACK_IN_IRQ, callback, 0);
        printk("set_timer -> %d\n", ret);
    }
    ret = start_timer(timer, 0);
    printk("start_timer -> %d\n", ret);
}

#endif


/*
 * ####################################
 *           Global Function
 * ####################################
 */

/*
 *  Description:
 *    Request a timer/counter device and do configuration.
 *  Input:
 *    timer --- unsigned int, this is index of timer/counter device, which is
 *              1-based index. If this argument is zero, driver will try to
 *              allocate a timer/counter, and return the index.
 *    flag  --- unsigned int, specify the way to configure timer/counter. Valid
 *              flags are listed below (For every group, only one flag could be
 *              used at one time, however, flags from different groups could be
 *              mixed.):
 *              1. a) TIMER_FLAG_16BIT   - This is a 16-bit timer/counter.
 *                 b) TIMER_FLAG_32BIT   - This is a 32-bit timer/counter.
 *              2. a) TIMER_FLAG_TIMER   - This is a timer.
 *                 b) TIMER_FLAG_COUNTER - This is a event counter.
 *              3. a) TIMER_FLAG_ONCE    - Stop when overflow/underflow.
 *                 b) TIMER_FLAG_CYCLIC  - Continue when overflow/underflow.
 *              4. a) TIMER_FLAG_UP      - Count in increment mode.
 *                 b) TIMER_FLAG_DOWN    - Count in decrement mode.
 *              5. a) TIMER_FLAG_HIGH_LEVEL_SENSITIVE - Count high level signal.
 *                 b) TIMER_FLAG_LOW_LEVEL_SENSITIVE  - Count low level signal.
 *                 c) TIMER_FLAG_RISE_EDGE            - Count rising edge.
 *                 d) TIMER_FLAG_FALL_EDGE            - Count falling edge.
 *                 e) TIMER_FLAG_ANY_EDGE             - Count either edge.
 *                 f) TIMER_FLAG_NONE_EDGE            - Reserved flag. This flag
 *                                                      means to count specific
 *                                                      level.
 *                 g) TIMER_FLAG_REAL                 - Reserved flag. This flag
 *                                                      means to count the
 *                                                      original signal.
 *                 h) TIMER_FLAG_INVERT               - Reserved flag. This flag
 *                                                      means to count the
 *                                                      inverted signal.
 *              6. a) TIMER_FLAG_UNSYNC  - No synchronization of signal is
 *                                         performed. The input signal must be
 *                                         synchronous to the clock of counter.
 *                 b) TIMER_FLAG_SYNC    - Perform synchronization of signal.
 *                                         The synchronized signal has a delay
 *                                         of two clock cycles.
 *              7. a) TIMER_FLAG_NO_HANDLE       - Mask off interrupt.
 *                 b) TIMER_FLAG_CALLBACK_IN_IRQ - Invoke callback in interrupt
 *                                                 handler. This flag only be
 *                                                 used in kernel level.
 *                 c) TIMER_FLAG_CALLBACK_IN_BH  - Reserved flag.
 *                 d) TIMER_FLAG_SIGNAL          - Send a signal in interrupt
 *                                                 handler. This flag is useful
 *                                                 for user level application.
 *    value --- unsigned long, this value would be put into reload register, and
 *              works as a init value or reload value.
 *    arg1  --- unsigned long, for flag TIMER_FLAG_NO_HANDLE, this argument is
 *              ignored. For flag TIMER_FLAG_CALLBACK_IN_IRQ, this argument is a
 *              callback function pointer (timer_callback). For flag
 *              TIMER_FLAG_SIGNAL, this argument is PID of receiver process.
 *    arg2  --- unsigned long, for flag TIMER_FLAG_NO_HANDLE, this argument is
 *              ignored. For flag TIMER_FLAG_CALLBACK_IN_IRQ, this argument is
 *              argument would be passed to callback function. For flag
 *              TIMER_FLAG_SIGNAL, this argument is signal number to be sent.
 *  Output:
 *    int   --- 0:              Success
 *              positive value: Index of Timer/Counter Allocated by Driver
 *              negative value: Error Code
 */
int request_timer(unsigned int timer, unsigned int flag, unsigned long value, unsigned long arg1, unsigned long arg2)
{
    int ret;
    unsigned int con_reg, irnen_reg;
    int n, X;

    if ( timer >= FIRST_TIMER + timer_dev.number_of_timers )
        return -EINVAL;

//    printk("request_timer(%d, 0x%08X, %d)\n", (u32)timer, (u32)flag, value);

    if ( TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT )
        value &= 0xFFFF;
    else
        timer &= ~0x01;

    down(&timer_dev.sem);

    /*
     *  Allocate timer.
     */
    if ( timer < FIRST_TIMER )
    {
        unsigned int mask;
        unsigned int shift;

        /*
         *  Pick up a free timer.
         */
        if ( TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT )
        {
            mask = 1 << FIRST_TIMER;
            shift = 1;
        }
        else
        {
            mask = 3 << FIRST_TIMER;
            shift = 2;
        }
        for ( timer = FIRST_TIMER; timer < FIRST_TIMER + timer_dev.number_of_timers; timer += shift, mask <<= shift )
            if ( !(timer_dev.occupation & mask) )
            {
                timer_dev.occupation |= mask;
                break;
            }
        if ( timer >= FIRST_TIMER + timer_dev.number_of_timers )
        {
            up(&timer_dev.sem);
            return -EBUSY;
        }
        else
            ret = timer;
    }
    else
    {
        register unsigned int mask;

        /*
         *  Check if the requested timer is free.
         */
        mask = (TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT ? 1 : 3) << timer;
        if ( (timer_dev.occupation & mask) )
        {
            up(&timer_dev.sem);
            return -EBUSY;
        }
        else
        {
            timer_dev.occupation |= mask;
            ret = 0;
        }
    }

    /*
     *  Prepare control register value.
     */
    switch ( TIMER_FLAG_MASK_EDGE(flag) )
    {
    default:
    case TIMER_FLAG_NONE_EDGE:  con_reg = GPTU_CON_EDGE_SET(0x00); break;
    case TIMER_FLAG_RISE_EDGE:  con_reg = GPTU_CON_EDGE_SET(0x01); break;
    case TIMER_FLAG_FALL_EDGE:  con_reg = GPTU_CON_EDGE_SET(0x02); break;
    case TIMER_FLAG_ANY_EDGE:   con_reg = GPTU_CON_EDGE_SET(0x03); break;
    }
    if ( TIMER_FLAG_MASK_TYPE(flag) == TIMER_FLAG_TIMER )
        con_reg |= TIMER_FLAG_MASK_SRC(flag) == TIMER_FLAG_EXT_SRC ? GPTU_CON_SRC_EXT_SET(1) : GPTU_CON_SRC_EXT_SET(0);
    else
        con_reg |= TIMER_FLAG_MASK_SRC(flag) == TIMER_FLAG_EXT_SRC ? GPTU_CON_SRC_EG_SET(1) : GPTU_CON_SRC_EG_SET(0);
    con_reg |= TIMER_FLAG_MASK_SYNC(flag) == TIMER_FLAG_UNSYNC ? GPTU_CON_SYNC_SET(0) : GPTU_CON_SYNC_SET(1);
    con_reg |= TIMER_FLAG_MASK_INVERT(flag) == TIMER_FLAG_REAL ? GPTU_CON_INV_SET(0) : GPTU_CON_INV_SET(1);
    con_reg |= TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT ? GPTU_CON_EXT_SET(0) : GPTU_CON_EXT_SET(1);
    con_reg |= TIMER_FLAG_MASK_STOP(flag) == TIMER_FLAG_ONCE ? GPTU_CON_STP_SET(1) : GPTU_CON_STP_SET(0);
    con_reg |= TIMER_FLAG_MASK_TYPE(flag) == TIMER_FLAG_TIMER ? GPTU_CON_CNT_SET(0) : GPTU_CON_CNT_SET(1);
    con_reg |= TIMER_FLAG_MASK_DIR(flag) == TIMER_FLAG_UP ? GPTU_CON_DIR_SET(1) : GPTU_CON_DIR_SET(0);

    /*
     *  Fill up running data.
     */
    timer_dev.timer[timer - FIRST_TIMER].flag = flag;
    timer_dev.timer[timer - FIRST_TIMER].arg1 = arg1;
    timer_dev.timer[timer - FIRST_TIMER].arg2 = arg2;
    if ( TIMER_FLAG_MASK_SIZE(flag) != TIMER_FLAG_16BIT )
        timer_dev.timer[timer - FIRST_TIMER + 1].flag = flag;

    /*
     *  Enable GPTU module.
     */
    if ( !timer_dev.f_gptu_on )
    {
        enable_gptu();
        timer_dev.f_gptu_on = 1;
    }

    /*
     *  Enable IRQ.
     */
    if ( TIMER_FLAG_MASK_HANDLE(flag) != TIMER_FLAG_NO_HANDLE )
    {
        if ( TIMER_FLAG_MASK_HANDLE(flag) == TIMER_FLAG_SIGNAL )
            timer_dev.timer[timer - FIRST_TIMER].arg1 = (unsigned long)find_task_by_pid((int)arg1);

        irnen_reg = 1 << (timer - FIRST_TIMER);

#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
        if ( TIMER_FLAG_MASK_HANDLE(flag) == TIMER_FLAG_SIGNAL
             || (TIMER_FLAG_MASK_HANDLE(flag) == TIMER_FLAG_CALLBACK_IN_IRQ && timer_dev.timer[timer - FIRST_TIMER].arg1) )
        {
            enable_irq(timer_dev.timer[timer - FIRST_TIMER].irq);
            timer_dev.timer[timer - FIRST_TIMER].f_irq_on = 1;
        }
#endif
    }
    else
        irnen_reg = 0;

    /*
     *  Write config register, reload value and enable interrupt.
     */
    n = timer >> 1;
    X = timer & 0x01;
    *DANUBE_GPTU_CON(n, X) = con_reg;
    *DANUBE_GPTU_RELOAD(n, X) = value;
//    printk("reload value = %d\n", (u32)value);
    *DANUBE_GPTU_IRNEN |= irnen_reg;

    up(&timer_dev.sem);

    return ret;
}

/*
 *  Description:
 *    Free the timer/counter allocated in function request_timer.
 *  Input:
 *    timer --- unsigned int, this is index of timer/counter device, which is
 *              1-based index. Before calling this function, user must call
 *              request_timer to get timer/counter.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */
int free_timer(unsigned int timer)
{
    unsigned int flag;
    unsigned int mask;
    int n, X;

    if ( !timer_dev.f_gptu_on )
        return -EINVAL;

    if ( timer < FIRST_TIMER || timer >= FIRST_TIMER + timer_dev.number_of_timers )
        return -EINVAL;

    down(&timer_dev.sem);

    flag = timer_dev.timer[timer - FIRST_TIMER].flag;
    if ( TIMER_FLAG_MASK_SIZE(flag) != TIMER_FLAG_16BIT )
        timer &= ~0x01;

    mask = (TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT ? 1 : 3) << timer;
    if ( ((timer_dev.occupation & mask) ^ mask) )
    {
        up(&timer_dev.sem);
        return -EINVAL;
    }

    n = timer >> 1;
    X = timer & 0x01;

    /*
     *  Stop running timer/counter;
     */
    if ( GPTU_CON_EN(n, X) )
        *DANUBE_GPTU_RUN(n, X) = GPTU_RUN_CEN_SET(1);

    /*
     *  Clear interrupt and disable interrupt.
     */
    *DANUBE_GPTU_IRNEN &= ~GPTU_IRNEN_TC_SET(n, X, 1);
    *DANUBE_GPTU_IRNCR |= GPTU_IRNCR_TC_SET(n, X, 1);

    /*
     *  Disable IRQ.
     */
    if ( timer_dev.timer[timer - FIRST_TIMER].f_irq_on )
    {
#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
        disable_irq(timer_dev.timer[timer - FIRST_TIMER].irq);
        timer_dev.timer[timer - FIRST_TIMER].f_irq_on = 0;
#endif
    }

    /*
     *  If all timer are released, disable GPTU module
     */
    timer_dev.occupation &= ~mask;
    if ( !timer_dev.occupation && timer_dev.f_gptu_on )
    {
        disable_gptu();
        timer_dev.f_gptu_on = 0;
    }

    up(&timer_dev.sem);

    return 0;
}

/*
 *  Description:
 *    Start or resume the timer/counter, which is allocated in function
 *    request_timer.
 *  Input:
 *    timer     --- unsigned int, this is index of timer/counter device, which
 *                  is 1-based index. Before calling this function, user must
 *                  call request_timer to get timer/counter.
 *    is_resume --- int, zero indicates start, and non-zero indicates resume.
 *                  Start counting from a given init value, however resume
 *                  counting from current count value.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */
int start_timer(unsigned int timer, int is_resume)
{
    unsigned int flag;
    unsigned int mask;
    int n, X;

    if ( !timer_dev.f_gptu_on )
        return -EINVAL;

    if ( timer < FIRST_TIMER || timer >= FIRST_TIMER + timer_dev.number_of_timers )
        return -EINVAL;

    down(&timer_dev.sem);

    flag = timer_dev.timer[timer - FIRST_TIMER].flag;
    if ( TIMER_FLAG_MASK_SIZE(flag) != TIMER_FLAG_16BIT )
        timer &= ~0x01;

    mask = (TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT ? 1 : 3) << timer;
    if ( ((timer_dev.occupation & mask) ^ mask) )
    {
        up(&timer_dev.sem);
        return -EINVAL;
    }

    n = timer >> 1;
    X = timer & 0x01;

    *DANUBE_GPTU_RUN(n, X) = GPTU_RUN_RL_SET(!is_resume) | GPTU_RUN_SEN_SET(1);

    up(&timer_dev.sem);

    return 0;
}

/*
 *  Description:
 *    Suspend the timer/counter, which is allocated in function request_timer.
 *  Input:
 *    timer --- unsigned int, this is index of timer/counter device, which is
 *              1-based index. Before calling this function, user must call
 *              request_timer to get timer/counter.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */
int stop_timer(unsigned int timer)
{
    unsigned int flag;
    unsigned int mask;
    int n, X;

    if ( !timer_dev.f_gptu_on )
        return -EINVAL;

    if ( timer < FIRST_TIMER || timer >= FIRST_TIMER + timer_dev.number_of_timers )
        return -EINVAL;

    down(&timer_dev.sem);

    flag = timer_dev.timer[timer - FIRST_TIMER].flag;
    if ( TIMER_FLAG_MASK_SIZE(flag) != TIMER_FLAG_16BIT )
        timer &= ~0x01;

    mask = (TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT ? 1 : 3) << timer;
    if ( ((timer_dev.occupation & mask) ^ mask) )
    {
        up(&timer_dev.sem);
        return -EINVAL;
    }

    n = timer >> 1;
    X = timer & 0x01;

    *DANUBE_GPTU_RUN(n, X) = GPTU_RUN_CEN_SET(1);

    up(&timer_dev.sem);

    return 0;
}

/*
 *  Description:
 *    Get current count value of the timer/counter allocated in function
 *    request_timer.
 *  Input:
 *    timer --- unsigned int, this is index of timer/counter device, which is
 *              1-based index. Before calling this function, user must call
 *              request_timer to get timer/counter.
 *    value --- unsigned long*, the current count value would be put in variable
 *              referred to by this pointer.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */
int get_count_value(unsigned int timer, unsigned long *value)
{

    unsigned int flag;
    unsigned int mask;
    int n, X;

    if ( !timer_dev.f_gptu_on )
        return -EINVAL;

    if ( timer < FIRST_TIMER || timer >= FIRST_TIMER + timer_dev.number_of_timers )
        return -EINVAL;

    down(&timer_dev.sem);

    flag = timer_dev.timer[timer - FIRST_TIMER].flag;
    if ( TIMER_FLAG_MASK_SIZE(flag) != TIMER_FLAG_16BIT )
        timer &= ~0x01;

    mask = (TIMER_FLAG_MASK_SIZE(flag) == TIMER_FLAG_16BIT ? 1 : 3) << timer;
    if ( ((timer_dev.occupation & mask) ^ mask) )
    {
        up(&timer_dev.sem);
        return -EINVAL;
    }

    n = timer >> 1;
    X = timer & 0x01;

    *value = *DANUBE_GPTU_COUNT(n, X);

    up(&timer_dev.sem);

   return 0;
}

/*
 *  Description:
 *    This is a help function, which gives divider value according to expected
 *    frequency. However, output frequency with this divider is approximate to
 *    expectation rather than equal to it.
 *  Input:
 *    freq  --- unsigned long, expected frequency in 0.001Hz.
 *  Output:
 *    int   --- 0:    Success
 *              else: Error Code
 */
unsigned long cal_divider(unsigned long freq)
{
    u32 ret;
    u32 residue;
    u32 module_freq[2];
    u32 clock_divider;

#if defined(GPTU_CLC_RMC_IS_SET) && GPTU_CLC_RMC_IS_SET
    clock_divider = GPTU_CLC_RMC;
    if ( clock_divider == 0 )
        return 0;
#else
    clock_divider = 1;
#endif

    /*  Slow FPI Bus is attached    */
//    printk("cgu_get_fpi_bus_clock(2) = %d, clock_divider = %d\n", cgu_get_fpi_bus_clock(2), (u32)clock_divider);
#if 1
    uint64_multiply(cgu_get_fpi_bus_clock(2), 1000, module_freq);
    uint64_divide(module_freq, clock_divider * freq, &ret, &residue);
    if ( (residue << 1) < clock_divider * freq && ret > 0 )
        ret--;
//    printk("divider = %u\n", ret);
#else
    module_freq = (cgu_get_fpi_bus_clock(2) + clock_divider / 2) * 1000 / clock_divider;
    ret = (module_freq + freq / 2) / freq;
    if ( ret > 0 )
        ret--;
//    printk("module_freq = %lu, freq = %lu, divider = %lu\n", module_freq, freq, ret);
#endif

    return (unsigned long)ret;
}

/*
 *  Description:
 *    Request a timer device and do configuration.
 *  Input:
 *    timer       --- unsigned int, this is index of timer device, which is
 *                    1-based index. If this argument is zero, driver will try
 *                    to allocate a timer/counter, and return the index.
 *    freq        --- unsigned long, expected frequency in 0.001Hz. This
 *                    function will calculate the divider, which provides a
 *                    proximate frequency compared to the expectation.
 *    is_cyclic   --- int, if this argument is zero, timer/counter would stop
 *                    when overflow/underflow. In contrast, if this argument is
 *                    non-zero, timer/counter would continue counting when
 *                    overflow/underflow.
 *    is_ext_src  --- int, if this argument is zero, timer use external clock
 *                    source from GPIO.
 *    handle_flag --- unsigned int, there are four possible choices:
 *                    1. TIMER_FLAG_NO_HANDLE       - Mask off interrupt.
 *                    2. TIMER_FLAG_CALLBACK_IN_IRQ - Invoke callback in
 *                                                    interrupt handler. This
 *                                                    flag only be used in
 *                                                    kernel level.
 *                    3. TIMER_FLAG_CALLBACK_IN_BH  - Reserved flag.
 *                    4. TIMER_FLAG_SIGNAL          - Send a signal in interrupt
 *                                                    handler. This flag is
 *                                                    useful for user level
 *                                                    application.
 *    arg1        --- unsigned long, for flag TIMER_FLAG_NO_HANDLE, this
 *                    argument is ignored. For flag TIMER_FLAG_CALLBACK_IN_IRQ,
 *                    this argument is a callback function pointer
 *                    (timer_callback). For flag TIMER_FLAG_SIGNAL, this
 *                    argument is PID of receiverprocess.
 *    arg2        --- unsigned long, for flag TIMER_FLAG_NO_HANDLE, this
 *                    argument is ignored. For flag TIMER_FLAG_CALLBACK_IN_IRQ,
 *                    this argument is argument would be passed to callback
 *                    function. For flag TIMER_FLAG_SIGNAL, this argument is
 *                    signal number to be sent.
 *  Output:
 *    int         --- 0:              Success
 *                    positive value: Index of Timer Allocated by Driver
 *                    negative value: Error Code
 */
int set_timer(unsigned int timer, unsigned int freq, int is_cyclic, int is_ext_src, unsigned int handle_flag, unsigned long arg1, unsigned long arg2)
{
    unsigned long divider;
    unsigned int flag;

    divider = cal_divider(freq);
    if ( divider == 0 )
        return -EINVAL;
    flag = ((divider & ~0xFFFF) ? TIMER_FLAG_32BIT : TIMER_FLAG_16BIT)
           | (is_cyclic ? TIMER_FLAG_CYCLIC : TIMER_FLAG_ONCE)
           | (is_ext_src ? TIMER_FLAG_EXT_SRC : TIMER_FLAG_INT_SRC)
           | TIMER_FLAG_TIMER | TIMER_FLAG_DOWN
           | TIMER_FLAG_MASK_HANDLE(handle_flag);

//    printk("set_timer(%d, %d), divider = %d\n", timer, freq, divider);
    return request_timer(timer, flag, divider, arg1, arg2);
}

/*
 *  Description:
 *    Request a counter device and do configuration.
 *  Input:
 *    timer        --- unsigned int, this is index of counter device, which is
 *                     1-based index. If this argument is zero, driver will try
 *                     to allocate a timer/counter, and return the index.
 *    is_32bit     --- int, if this argument is zero, a 16-bit counter is
 *                     requested, else, a 32-bit counter is requested.
 *    is_sync      --- int, if this argument is non-zero, input signal would be
 *                     synchronized to the clock of counter and there is a delay
 *                     of two clock cycles, else, the fed input signal must be
 *                     synchronous to the clock of counter.
 *    trigger_flag --- unsigned int, there are eight possible choices, three
 *                     of which are reserved for internal use only:
 *                     1. TIMER_FLAG_HIGH_LEVEL_SENSITIVE
 *                                              - Count high level signal.
 *                     2. TIMER_FLAG_LOW_LEVEL_SENSITIVE
 *                                              - Count low level signal.
 *                     3. TIMER_FLAG_RISE_EDGE  - Count rising edge.
 *                     4. TIMER_FLAG_FALL_EDGE  - Count falling edge.
 *                     5. TIMER_FLAG_ANY_EDGE   - Count either edge.
 *                     6. TIMER_FLAG_NONE_EDGE  - Reserved flag. This flag means
 *                                                to count specific level.
 *                     7. TIMER_FLAG_REAL       - Reserved flag. This flag means
 *                                                to count the original signal.
 *                     8. TIMER_FLAG_INVERT     - Reserved flag. This flag means
 *                                                to count the inverted signal.
 *    is_ext_src  --- int, if this argument is zero, timer use external clock
 *                    source from GPIO.
 *    handle_flag  --- unsigned int, there are four possible choices:
 *                     1. TIMER_FLAG_NO_HANDLE       - Mask off interrupt.
 *                     2. TIMER_FLAG_CALLBACK_IN_IRQ - Invoke callback in
 *                                                     interrupt handler. This
 *                                                     flag only be used in
 *                                                     kernel level.
 *                     3. TIMER_FLAG_CALLBACK_IN_BH  - Reserved flag.
 *                     4. TIMER_FLAG_SIGNAL          - Send a signal in
 *                                                     interrupt handler. This
 *                                                     flag is useful for user
 *                                                     level application.
 *    arg1         --- unsigned long, for flag TIMER_FLAG_NO_HANDLE, this
 *                     argument is ignored. For flag TIMER_FLAG_CALLBACK_IN_IRQ,
 *                     this argument is a callback function pointer
 *                     (timer_callback). For flag TIMER_FLAG_SIGNAL, this
 *                     argument is PID of receiverprocess.
 *    arg2         --- unsigned long, for flag TIMER_FLAG_NO_HANDLE, this
 *                     argument is ignored. For flag TIMER_FLAG_CALLBACK_IN_IRQ,
 *                     this argument is argument would be passed to callback
 *                     function. For flag TIMER_FLAG_SIGNAL, this argument is
 *                     signal number to be sent.
 *  Output:
 *    int          --- 0:              Success
 *                     positive value: Index of Counter Allocated by Driver
 *                     negative value: Error Code
 */
int set_counter(unsigned int timer, int is_32bit, int is_sync, int is_ext_src, unsigned int trigger_flag, unsigned int handle_flag, unsigned long arg1, unsigned long arg2)
{
    unsigned int flag;

    if ( timer == TIMER1B )
    {
        flag = (is_sync ? TIMER_FLAG_SYNC : TIMER_FLAG_UNSYNC)
               | (is_32bit ? TIMER_FLAG_32BIT : TIMER_FLAG_16BIT)
               | TIMER_FLAG_INT_SRC
               | TIMER_FLAG_CYCLIC | TIMER_FLAG_COUNTER | TIMER_FLAG_DOWN
               | TIMER_FLAG_MASK_TRIGGER(trigger_flag)
               | TIMER_FLAG_MASK_HANDLE(handle_flag);

        return request_timer(timer, flag, 1, arg1, arg2);
    }
    else
    {
        flag = (is_sync ? TIMER_FLAG_SYNC : TIMER_FLAG_UNSYNC)
               | (is_32bit ? TIMER_FLAG_32BIT : TIMER_FLAG_16BIT)
               | (is_ext_src ? TIMER_FLAG_EXT_SRC : TIMER_FLAG_INT_SRC)
               | TIMER_FLAG_ONCE | TIMER_FLAG_COUNTER | TIMER_FLAG_UP
               | TIMER_FLAG_MASK_TRIGGER(trigger_flag)
               | TIMER_FLAG_MASK_HANDLE(handle_flag);

        return request_timer(timer, flag, 0, arg1, arg2);
    }
}


/*
 * ####################################
 *           Init/Cleanup API
 * ####################################
 */

/*
 *  Description:
 *    register device
 *  Input:
 *    none
 *  Output:
 *    0    --- successful
 *    else --- failure, usually it is negative value of error code
 */
int __init danube_gptu_init(void)
{
    int ret;
#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    unsigned int i;
#endif

    /*  Initialize fake registers to do testing on Amazon.  */
#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    *DANUBE_GPTU_CLC            = 0x00000002;
    *DANUBE_GPTU_ID             = 0x00005960;
    *DANUBE_GPTU_CON(1, 0)      = 0x00000000;
    *DANUBE_GPTU_CON(1, 1)      = 0x00000000;
    *DANUBE_GPTU_CON(2, 0)      = 0x00000000;
    *DANUBE_GPTU_CON(2, 1)      = 0x00000000;
    *DANUBE_GPTU_CON(3, 0)      = 0x00000000;
    *DANUBE_GPTU_CON(3, 1)      = 0x00000000;
    *DANUBE_GPTU_RUN(1, 0)      = 0x00000000;
    *DANUBE_GPTU_RUN(1, 1)      = 0x00000000;
    *DANUBE_GPTU_RUN(2, 0)      = 0x00000000;
    *DANUBE_GPTU_RUN(2, 1)      = 0x00000000;
    *DANUBE_GPTU_RUN(3, 0)      = 0x00000000;
    *DANUBE_GPTU_RUN(3, 1)      = 0x00000000;
    *DANUBE_GPTU_RELOAD(1, 0)   = 0x00000000;
    *DANUBE_GPTU_RELOAD(1, 1)   = 0x00000000;
    *DANUBE_GPTU_RELOAD(2, 0)   = 0x00000000;
    *DANUBE_GPTU_RELOAD(2, 1)   = 0x00000000;
    *DANUBE_GPTU_RELOAD(3, 0)   = 0x00000000;
    *DANUBE_GPTU_RELOAD(3, 1)   = 0x00000000;
    *DANUBE_GPTU_COUNT(1, 0)    = 0x00000000;
    *DANUBE_GPTU_COUNT(1, 1)    = 0x00000000;
    *DANUBE_GPTU_COUNT(2, 0)    = 0x00000000;
    *DANUBE_GPTU_COUNT(2, 1)    = 0x00000000;
    *DANUBE_GPTU_COUNT(3, 0)    = 0x00000000;
    *DANUBE_GPTU_COUNT(3, 1)    = 0x00000000;
    *DANUBE_GPTU_IRNEN          = 0x00000000;
    *DANUBE_GPTU_IRNICR         = 0x00000000;
    *DANUBE_GPTU_IRNCR          = 0x00000000;
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

    /*  Clear interrupt.    */
    *DANUBE_GPTU_IRNEN = 0x0000;
    *DANUBE_GPTU_IRNCR = 0x0FFF;
#if defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON
    *DANUBE_GPTU_IRNCR = 0x0000;
    *DANUBE_GPTU_IRNICR = 0x0000;
#endif  //  defined(DEBUG_ON_AMAZON) && DEBUG_ON_AMAZON

    memset(&timer_dev, 0, sizeof(timer_dev));
    sema_init(&timer_dev.sem, 0);

    /*  Enable timer module to read ID, then turn off it.   */
    enable_gptu();
    timer_dev.number_of_timers = GPTU_ID_CFG * 2;
    disable_gptu();
    if ( timer_dev.number_of_timers > MAX_NUM_OF_32BIT_TIMER_BLOCKS * 2 )
        timer_dev.number_of_timers = MAX_NUM_OF_32BIT_TIMER_BLOCKS * 2;
    printk(KERN_INFO "gptu: totally %d 16-bit timers/counters\n", timer_dev.number_of_timers);

    ret = misc_register(&gptu_miscdev);
    if ( ret )
    {
        printk(KERN_ERR "gptu: can't misc_register, get error %d\n", -ret);
        return ret;
    }
    else
#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
        printk(KERN_INFO "gptu: misc_register on minor %d\n", gptu_miscdev.minor);
#else
        printk(KERN_INFO "gptu: misc_register on minor %d, device ID (%02X.%01X.%02X)\n", gptu_miscdev.minor, GPTU_ID_ID, GPTU_ID_CFG, GPTU_ID_REV);
#endif

#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    for ( i = 0; i < timer_dev.number_of_timers; i++ )
    {
        ret = request_irq(TIMER_INTERRUPT + i, (void (*)(int, void *, struct pt_regs *))timer_irq_handler, SA_INTERRUPT, gptu_miscdev.name, &timer_dev.timer[i]);
        if ( ret )
        {
            for ( ; i >= 0; i-- )
                free_irq(TIMER_INTERRUPT + i, &timer_dev.timer[i]);
            misc_deregister(&gptu_miscdev);
            printk(KERN_ERR "gptu: failed in requesting irq (%d), get error %d\n", i, -ret);
            return ret;
        }
        else
        {
            timer_dev.timer[i].irq = TIMER_INTERRUPT + i;
            disable_irq(timer_dev.timer[i].irq);
            printk(KERN_INFO "gptu: succeeded to request irq %d\n", timer_dev.timer[i].irq);
        }
   }
#endif

    up(&timer_dev.sem);
#if defined(TEST_TIMER_XULIANG) && TEST_TIMER_XULIANG
    test_timer();
#endif
    return 0;
}

/*
 *  Description:
 *    deregister device
 *  Input:
 *    none
 *  Output:
 *    none
 */
void __exit danube_gptu_exit(void)
{
    int ret;
#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    unsigned int i;
#endif

    /*
     *  Finish pending IRQ and release IRQ resource.
     */
#if !defined(DEBUG_ON_AMAZON) || !DEBUG_ON_AMAZON
    for ( i = 0; i < timer_dev.number_of_timers; i++ )
    {
        if ( timer_dev.timer[i].f_irq_on )
            disable_irq(timer_dev.timer[i].irq);
        free_irq(timer_dev.timer[i].irq, &timer_dev.timer[i]);
    }
#endif

    /*
     *  Turn off timer module.
     */
    disable_gptu();

    /*
     *  Deregister device.
     */
    ret = misc_deregister(&gptu_miscdev);
    if ( ret )
        printk(KERN_ERR "gptu: can't misc_deregister, get error number %d\n", -ret);
    else
        printk(KERN_INFO "gptu: misc_deregister successfully\n");
}

EXPORT_SYMBOL(request_timer);
EXPORT_SYMBOL(free_timer);
EXPORT_SYMBOL(start_timer);
EXPORT_SYMBOL(stop_timer);
EXPORT_SYMBOL(get_count_value);
EXPORT_SYMBOL(cal_divider);
EXPORT_SYMBOL(set_timer);
EXPORT_SYMBOL(set_counter);

module_init(danube_gptu_init);
module_exit(danube_gptu_exit);
