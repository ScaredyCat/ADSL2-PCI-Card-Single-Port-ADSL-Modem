#ifndef _SYS_DRV_LINUX_H
#define _SYS_DRV_LINUX_H
/****************************************************************************
       Copyright (c) 2001, Infineon Technologies.  All rights reserved.

                               No Warranty
   Because the program is licensed free of charge, there is no warranty for
   the program, to the extent permitted by applicable law.  Except when
   otherwise stated in writing the copyright holders and/or other parties
   provide the program "as is" without warranty of any kind, either
   expressed or implied, including, but not limited to, the implied
   warranties of merchantability and fitness for a particular purpose. The
   entire risk as to the quality and performance of the program is with
   you.  should the program prove defective, you assume the cost of all
   necessary servicing, repair or correction.

   In no event unless required by applicable law or agreed to in writing
   will any copyright holder, or any other party who may modify and/or
   redistribute the program as permitted above, be liable to you for
   damages, including any general, special, incidental or consequential
   damages arising out of the use or inability to use the program
   (including but not limited to loss of data or data being rendered
   inaccurate or losses sustained by you or third parties or a failure of
   the program to operate with any other programs), even if such holder or
   other party has been advised of the possibility of such damages.
 ****************************************************************************
   Module      : $RCSfile: sys_drv_linux.h,v $
   Date        : $Date: 2005/08/30 10:43:41 $
   Description : This file contains the includes and the defines
                 specific to the linux OS

   $Log: sys_drv_linux.h,v $
   Revision 1.1  2005/08/30 10:43:41  pliu
   taken from INCA_IP II
   - change names
   - IM5_11 --> IM4_18

   Revision 1.2  2004/05/18 07:51:02  de6wanme
   Fixed problem with Alloc_Mem when in Interrupt

   Revision 1.1  2004/04/01 15:28:27  de6wanme
   Included Inca-IP OAK driver

   Revision 1.1  2004/03/30 10:59:32  thomas
   Working version of common ...

   Revision 1.43  2003/12/19 10:22:32  rah
   Added windows types and system calls for simulation
   customer module as template for new OS's, still not finished -> serves as discussion basis for moving to OS_... calls

   Revision 1.42  2003/12/10 16:56:58  rah
   changed \n to \n\r

   Revision 1.41  2003/12/10 10:23:12  rah
   Changed OS_Int-Related for more device independance
   Added it to sys_drv_<any>.h
   For PSOS and OSE they are between #if 0, but compiler warning apears

   Revision 1.40  2003/11/21 14:12:17  kamdem
   comments adjusted

   Revision 1.39  2003/11/21 11:24:19  kamdem
   endianess support ... still in test !!!

   Revision 1.38  2003/10/13 18:08:31  neumann
   * Added Wake_Down_Queue() for consistence reasons (see sys_drv_vxworks.h)

   Revision 1.37  2003/08/05 10:33:22  neumann
   - fixed Sem_BinaryInit (sema_init(sem,1) already does the job)
   - added FIXME's: need to find an agreed solution for VxWorks and Linux

   Revision 1.36  2003/07/10 12:11:15  kamdem
   C++ comment "//" removed from the file

   Revision 1.35  2003/07/07 09:57:59  rutkowski
   - Implement a lock/unlock mechanism for global interrupts of the system.

   Revision 1.34  2003/06/27 13:25:11  wachendorf
   added semicolon in Sem_MutexInit which was removed accidentally by Stefan...

   Revision 1.33  2003/06/27 13:03:07  rutkowski
   - Implement a filled waitqueue to use as mutex semaphore.



*******************************************************************************/

/** \file
   This file contains the includes and the defines specific to the linux OS
*/

/** \defgroup SYS_DRV_LINUX Definitions for Linux

   These features are base on 2.4.x kernels.
*/


/* ============================= */
/* Global Includes               */
/* ============================= */

/** System includes */

#ifdef __KERNEL__
#include <linux/kernel.h>
#endif
#ifdef MODULE
#include <linux/module.h>
#endif

/*several includes*/
#include <linux/fs.h>
#include <linux/errno.h>
/*proc-file system*/
#include <linux/proc_fs.h>
/*polling */
#include <linux/poll.h>
/* check-request-release region*/
#include <linux/ioport.h>
/*ioremap-kmalloc*/
#include <linux/vmalloc.h>
/* mdelay - udelay */
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/param.h>
#include <linux/interrupt.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/semaphore.h>
/* little / big endian */
#include <asm/byteorder.h>
/* Interrupts */
#include <linux/irq.h>
#include <linux/list.h>


/*real time includes */
#ifdef RTAI
/* causes warnings, but may it not correct, contact Lineo for
   details */
extern int set_dec_cpu6(unsigned int val);
#include <rtai.h>
#include <rtai_sched.h>
#include <rtai_fifos.h>
#include <rtai_shm.h>
#endif

/* ============================= */
/*          Endianess            */
/* ============================= */

/** \defgroup SYS_DRV_LINUX_ENDIAN Endianess
   \ingroup SYS_DRV_LINUX
*/
/*@{*/
/** byte order is little endian */
#if defined ( __LITTLE_ENDIAN )
#define __BYTE_ORDER __LITTLE_ENDIAN
/** byte order is big endian */
#elif defined ( __BIG_ENDIAN )
#define __BYTE_ORDER __BIG_ENDIAN
#endif
/*@}*/

/* ============================= */
/* Compiler features             */
/* ============================= */

/** \defgroup SYS_DRV_LINUX_FEAT Compiler features
   \ingroup SYS_DRV_LINUX
*/
/*@{*/
/** Try to inline function. */
#define INLINE inline
/** Make functions on local. */
#define LOCAL  static
/*@}*/

/* ============================= */
/* Function Macros               */
/* ============================= */

#define GetImmr() IMAP_ADDR

/** \defgroup SYS_DRV_LINUX_DELAY Delay functions
   \ingroup SYS_DRV_LINUX

   This section defines different delay functions
   with active waiting or with enabling the scheduler.
*/
/*@{*/

/** Wait milliseconds with rescheduling. */
#define Wait(time) do {\
		current->state = TASK_INTERRUPTIBLE; \
		schedule_timeout(HZ * (time) / 1000);\
   } while(0)
/** Wait active milliseconds without rescheduling. */
#define Wait_Milli_Sec(time)              mdelay(time)
/** Wait active microseconds without rescheduling. */
#define Wait_Micro_Sec(time)              udelay(time)

/*@}*/

/** \defgroup SYS_DRV_LINUX_MEMORY Memory functions
   \ingroup SYS_DRV_LINUX

   This section defines functions for allocating and freeing memory.
   For debugging some trace functions are available.
*/
/*@{*/

#ifdef DEBUG
/** Allocate memory with debug trace. */
#define Alloc_Mem(size)             \
   sys_dbgMalloc(size, __LINE__, __FILE__)
/** Free memory with debug trace. */
#define Free_Mem(ptr)               \
   sys_dbgFree((void*)ptr, __LINE__, __FILE__)
/* wrapper for os malloc and free in case of debug */
/* Only used in sys_dbg* functions! */
#define sysdebug_malloc(size)             kmalloc(size, GFP_KERNEL)
#define sysdebug_free(ptr)                kfree((void*)ptr)
#else
/** Allocate kernel memory. */
#define Alloc_Mem(size) ({ \
    void* ptr; \
    if(in_interrupt()) { \
      ptr = kmalloc(size, GFP_ATOMIC); \
   } else { \
      ptr = kmalloc(size, GFP_KERNEL); \
   } \
   ptr; \
   })
/** Free kernel memory. */
#define Free_Mem(ptr)                \
   if (ptr != NULL) {kfree((void*)ptr); ptr = NULL;}
#endif
/*@}*/

/** \defgroup SYS_DRV_LINUX_EVENT Event handling functions
   \ingroup SYS_DRV_LINUX

   This section defines functions for event handling.
*/
/*@{*/

/** Initialize an event. */
#define Init_Event(event)						init_waitqueue_head (&(event))

#define Init_Full_Event(queue) \
   init_waitqueue_head (&(event)); \
   wake_up_interruptible(&(event))

/** Signal an event. */
#define Wake_Up_Event(event)              wake_up_interruptible (&(event))
/** Reset event to initial state. */
#define Clear_Event(event)						/* not needed for linux */

/** Wait for an event.
   \attention This is an old definition, it will be removed in future! */
#define Sleep_On(event, timeout)     \
   interruptible_sleep_on_timeout((event), (HZ * timeout / 1000))
/* FIXME: This is incompatible to VxWorks implementation of Sleep_On/Sleep_On_Event */
/** Wait for an event with timeout. */
#define Sleep_On_Event(event, timeout)    (interruptible_sleep_on_timeout(&(event), \
                                          (HZ * timeout / 1000)) == 0 ? 0 : 1)

/*@}*/

/* temporarily same func as appropriate event func */
/* Initialize a queue. */
#define Init_WakeupQueue(queue)           init_waitqueue_head (&(queue))
/** Signal a queue. */
#define Wake_Up_Queue(queue)              wake_up_interruptible(&(queue))
#define Wake_Down_Queue(queue)            wake_up_interruptible(&(queue))

/** \defgroup SYS_DRV_LINUX_MUTEX Mutex functions
   \ingroup SYS_DRV_LINUX

   This section has mutex definitions.
*/
/*@{*/
/** Create a mutex element. */
#define Sem_MutexInit(sem)                {  \
   (sem)=kmalloc(sizeof(struct semaphore), GFP_KERNEL); \
   sema_init((sem), 1);}

#define Sem_BinaryInit(sem) \
   (sem)=kmalloc(sizeof(struct semaphore), GFP_KERNEL); \
   sema_init((sem), 1);

   /** Lock a mutex section. */
//#define Sem_Lock(sem)                     down((sem))
#define Sem_Lock(sem)                     down_interruptible((sem))
/* FIXME: Better should use down_interruptible...
#define Sem_Lock(sem)                     down_interruptible((sem))
*/
/** Unlock a mutex section. */
#define Sem_Unlock(sem)                   up((sem))
/** Delete a mutex element. */
#define Sem_Free(sem) \
   kfree((sem));

/*@}*/

/** \defgroup SYS_DRV_LINUX_CLOCK Time/Clock functions
   \ingroup SYS_DRV_LINUX
*/
/*@{*/

/** Returns system tick in milliseconds. */
#define Get_Tick()                        (u32)(jiffies * 1000 / HZ)
/** Helper define to read clock value. */
#define sys_Cpu_Clk                       ((bd_t*)__res)->bi_intfreq
/** Returns CPU clock in Hz. */
#define Cpu_Clk                           ((sys_Cpu_Clk<1000)?sys_Cpu_Clk*1000000:sys_Cpu_Clk)

/*@}*/


/** \defgroup SYS_DRV_LINUX_INTERRUPT Enable/Disable global interrupts
   \ingroup SYS_DRV_LINUX
*/
/*@{*/

/*----------------------------------------------------------------------------*/
/* Lock mechanism for general processor interrupts                            */
/* Note: Please avoid to use the mechanism, because disabling all interrupts  */
/*       of the system is not a good solution.                                */
/*----------------------------------------------------------------------------*/
#define Lock_Variable                     unsigned long
#define Lock_Interrupts(var)              save_and_cli(var)
#define Unlock_Interrupts(var)            restore_flags(var)
/*----------------------------------------------------------------------------*/

/*@}*/

/* checks if context is interrupt level. So we can not take semaphores,
   cause that would cause a reschedule */
#define In_Interrupt                      in_interrupt


#if PPC

/** function to allocate dualported memory (only PPC) */
#define CPM_Dpalloc(size)                 m8xx_cpm_dpalloc(size)
/** function to allocate uncached memory (only PPC) */
#define Host_Alloc(size)                  m8xx_cpm_hostalloc(size)

#endif /* PPC */


#ifdef DEBUG
#define ASSERT(expr) \
	if(!(expr)) { \
		printk ( "\n\r" __FILE__ ":%d: Assertion " #expr " failed!\n\r",__LINE__); \
	}
#else
#define ASSERT(cond)
#endif

/** \ingroup SYS_DRV_LINUX_EVENT
   Waits for a specified event to occur or timeout.
   Used in WaitEvent.

   \param wq         - wait queue
   \param condition  - condition to occur
   \param ret        - for return value
   \param timeout    - time out in ms
*/
#define __WaitEvent(wq, condition, ret, timeout)   \
do {									                     \
	bool bSched = false;                            \
	wait_queue_t __wait;						            \
	init_waitqueue_entry(&__wait, current);         \
	add_wait_queue(&wq, &__wait);                   \
	for (;;)                                        \
	{                                               \
		set_current_state(TASK_INTERRUPTIBLE);       \
		if (condition)                               \
		{                                            \
			ret = 1;                                  \
			break;                                    \
	   }                                            \
		if (bSched == false)                         \
		{                                            \
   	   schedule_timeout(HZ * timeout / 1000); \
			bSched = true;                            \
			continue;                                 \
		}                                            \
		if (bSched == true)                          \
		{                                            \
   	   ret = 0;                                  \
		   break;                                    \
      }		                                       \
	}                                               \
	current->state = TASK_RUNNING;                  \
	remove_wait_queue(&wq, &__wait);                \
} while (0)


/** \ingroup SYS_DRV_LINUX_EVENT
   Waits for a specified event to occur or timeout.

   \param wq         - wait queue
   \param condition  - condition to occur
   \param timeout    - time out in ms, 0xFFFF for endless
   \return  1 if event occured and condition is true, otherwise 0
*/
#define WaitEvent(wq, condition, timeout)				            \
({									                                    \
	int __ret = 0;							                           \
	if (in_interrupt())                                         \
	{                                                           \
   	while (!(condition));					                     \
	}                                                           \
	else                                                        \
	{                                                           \
   	if (!(condition))	                                       \
   	{                                                        \
         if (timeout == 0xffff)                                \
            __wait_event_interruptible(wq, condition, __ret);  \
         else                                                  \
      		__WaitEvent(wq, condition, __ret, (timeout));      \
   	}					                                          \
	}                                                           \
 	if ((condition))					                              \
 	   __ret = 1;                                               \
 	else                                                        \
 	   __ret = 0;                                               \
	__ret;								                              \
})

/* Poll/Select returns */

#define  SYS_NOSELECT   0
#define  SYS_DATAIN     POLLIN
#define  SYS_EXCEPT     POLLPRI
#define  SYS_DATAOUT    POLLOUT

/* ============================= */
/* typedefs                      */
/* ============================= */
#define TIMER_ELEMENT                     struct timer_list

/* temporarily same as SYS_EVENT */
typedef wait_queue_head_t                 SYS_POLL;
typedef wait_queue_head_t                 SYS_EVENT;
typedef struct semaphore                  *SEM_ID;
/* List macros, using linux/list.h */
#define LIST struct list_head

/****************************************************************************
Description:
   Initializes the list
Arguments:
  *head: list head to add it after, type *LIST
Code:
   #define INIT_LIST_HEAD(ptr) do { \
   	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
   } while (0)
****************************************************************************/
#define LIST_INIT(head) INIT_LIST_HEAD (head)

/****************************************************************************
Description:
   Insert a new entry after the specified head
Arguments:
  *element: new entry to be added, type *LIST
  *head: list head to add it after, type *LIST
Remarks:
 This is good for implementing stacks.
Example:
   typedef {
      // ...
      // list handling struct
      LIST listHead;
      // ...
   }DEVICE;

   typedef {
      // list handling struct
      LIST list;
      WORD pData[250];
      // ...
   }PACKET;
   // ...
   List_Add ((&pData->list), &(pDev->listHead));
****************************************************************************/
#define List_Add(element, head)     list_add(element, head)

/****************************************************************************
Description:
   Insert a new entry before the specified head
Arguments:
  *element: new entry to be added, type *LIST
  *head: list head to add it after, type *LIST
Remarks:
   This is useful for implementing queues.
****************************************************************************/
#define List_Insert(element, head)  list_add_tail(element, head)

/****************************************************************************
Description:
   Delete a list entry
Arguments:
  *element: new entry to be deleted, type *LIST
****************************************************************************/
#define List_Delete(element)        list_del(element)


/****************************************************************************
Description:
   tests whether a list is empty
Arguments:
  *head: the list head to test, , type *LIST
Code:
	return head->next == head;
****************************************************************************/
#define List_Empty(head)            list_empty(head)


/****************************************************************************
Description:
   get the struct for this entry
Arguments:
   ptr - the &list_head pointer.
   type - the type of the struct this is embedded in.
   member - the name of the list_struct within the struct.
Example:
   PACKET* pEl;
   pEl = list_entry (pDev->listHead, PACKET, list);
   pEl->pData[0] = 1;
Code:
   #define list_entry(ptr, type, member) \
	   ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))
****************************************************************************/
#define List_Entry(ptr, type, member) list_entry(ptr, type, member)


/* ============================= */
/* Global function declaration   */
/* ============================= */

/* Disable/Enable Interrupts, defined in drv_<chipname>_linux.c */
extern int Enable_Irq(int irq);
extern void Disable_Irq(int irq);
extern void Unmask_Irq (int irq);
/* Device Init. Called form Board_DevOpen when the device has been opened for
   the first time. The device structure will be cached */
extern int OS_InitDevice (int nDevn);
/* Device is stopped. Called on device close */
extern void OS_StopDevice (void* pDevice);
/* Install VINETIC ISR   */
extern void OS_Install_IRQ_Handler(void* pDevice, int nIrq);
/* Uninstall VINETIC ISR */
extern void OS_UnInstall_IRQ_Handler(void* pDevice);

#endif /* _SYS_DRV_LINUX_H */
