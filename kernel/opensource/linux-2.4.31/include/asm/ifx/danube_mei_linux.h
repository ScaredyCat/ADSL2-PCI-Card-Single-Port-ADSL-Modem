/******************************************************************************
**
** FILE NAME    : danube_mei_linux.h
** PROJECT      : Danube
** MODULES      : MEI
**
** DATE         : 1 Jan 2006
** AUTHOR       : TC Chen
** DESCRIPTION  : MEI Driver
** COPYRIGHT    :       Copyright (c) 2006
**                      Infineon Technologies AG
**                      Am Campeon 1-12, 85579 Neubiberg, Germany
**
**    This program is free software; you can redistribute it and/or modify
**    it under the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or
**    (at your option) any later version.
**
** HISTORY
** $Version $Date      $Author     $Comment
*******************************************************************************/
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <asm/semaphore.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <asm/uaccess.h>

#undef CONFIG_DEVFS_FS		//165204:henryhsu devfs will make mei open file fail.

#ifdef CONFIG_DEVFS_FS
#include <linux/devfs_fs_kernel.h>
#endif
#ifdef CONFIG_PROC_FS
#include <linux/proc_fs.h>
#endif

#include <linux/list.h>
#include <linux/delay.h>
#define __LINUX__

#ifdef CONFIG_PROC_FS
#define PROC_ITEMS 8
#define MEI_DIRNAME     "mei"
#endif

#include <asm/danube/danube.h>
#include <asm/danube/irq.h>
#include <asm/danube/danube_mei.h>
#include <asm/danube/danube_mei_app.h>
#include <asm/danube/danube_mei_ioctl.h>
#include <asm/danube/danube_mei_app_ioctl.h>
#include <asm/danube/port.h>
#include <asm/danube/danube_gpio.h>
#include <asm/danube/danube_led.h>

#ifdef CONFIG_DEVFS_FS
#define DANUBE_DEVNAME  "danube"
#endif //ifdef CONFIG_DEVFS_FS

#define MEI_LOCKINT(var) \
        save_flags(var);\
        cli()
#define MEI_UNLOCKINT(var) \
        restore_flags(var)

#define MEI_MUTEX_INIT(id,flag) \
        sema_init(&id,flag)
#define MEI_MUTEX_LOCK(id) \
        down_interruptible(&id)
#define MEI_MUTEX_UNLOCK(id) \
        up(&id)

#define MEI_MASK_AND_ACK_IRQ \
        mask_and_ack_danube_irq

#define MEI_DISABLE_IRQ \
        disable_irq
#define MEI_ENABLE_IRQ \
        enable_irq

#define REQUEST_IRQ_HANDLER \
        request_irq

#define FREE_IRQ_HANDLER \
        free_irq

#define MEI_WAIT(ms) \
        {\
                set_current_state(TASK_INTERRUPTIBLE);\
                schedule_timeout(ms);\
        }

#define MEI_INIT_WAKELIST(name,queue) \
        init_waitqueue_head(&queue)

#define MEI_WAIT_EVENT_TIMEOUT(ev,timeout)\
        interruptible_sleep_on_timeout(&ev,timeout)

#define MEI_WAIT_EVENT(ev)\
        interruptible_sleep_on(&ev)
#define MEI_WAKEUP_EVENT(ev)\
        wake_up_interruptible(&ev)

typedef unsigned long MEI_intstat_t;
typedef struct semaphore MEI_mutex_t;
typedef struct file MEI_file_t;
typedef struct inode MEI_inode_t;

extern void mask_and_ack_danube_irq (unsigned int irq_nr);
