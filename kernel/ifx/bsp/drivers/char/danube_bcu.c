/******************************************************************************
**
** FILE NAME    : danube_bcu.c
** PROJECT      : Danube
** MODULES      : BCU
**
** DATE         : 04 July 2005
** AUTHOR       : Huang Xiaogang
** DESCRIPTION  : Danube Bcu driver
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
** $Date        $Author         $Comment
** 01 Jan 2006  Huang Xiaogang  modification & verification on Danube chip
*******************************************************************************/

#include <linux/module.h>
#include <linux/config.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/stat.h>
#include <linux/kdev_t.h>
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>
#include <linux/swapctl.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>
#include <asm/danube/irq.h>
#include <asm/danube/danube_bcu.h>
#include <asm/danube/danube.h>

typedef struct bcu_dev {
	char name[16];
	int major;
	int minor;
	int full;
	char buff[10];
	int irq;
} bcu_dev;

// bcu_error data control register
/******************************************************************************
Description:
    Register bcu_error data control structure definitions specified with little endian or big endian defines
Arguments:
   bcu_err_econ_cnt	: specifies the number of FPI bus error counter. ERRCNT incremented on each counter. 
                          Value specified to be 0 to 32768.
   bcu_err_econ_tout    : specifies the state of FPI bus time out signal. Value ==1 indicates active high and value == 0
                          indicates active low. 
   bcu_err_econ_rdy 	: specifies the state of FPI bus ready signal. Value ==1 indicates active high and value == 0
                          indicates active low.
   bcu_err_econ_abt 	: specifies the state of FPI bus abort signal. Value ==1 indicates active high and value == 0
                          indicates active low.
   bcu_err_econ_ack 	: specifies the state of FPI bus acknowledge signal. 
   bcu_err_econ_svm 	: specifies the state of FPI bus supervisor mode signal. Value ==1 indicates active high and value == 0 indicates active low.
   bcu_err_econ_wrn 	: specifies the state of FPI bus write signal. Value ==1 indicates active high and value == 0
                          indicates active low.
   bcu_err_econ_rdn 	: specifies the state of FPI bus read signal. Value ==1 indicates active high and value == 0
                          indicates active low.
   bcu_err_econ_tag 	: specifies the state of FPI bus tag number. 
   bcu_err_econ_opc 	: specifies the state of FPI bus operation code. 
Return Value:
   None
Remarks:
   None.
******************************************************************************/
typedef union {
#ifdef CONFIG_CPU_LITTLE_ENDIAN
	struct {
		unsigned int bcu_err_econ_cnt:16;
		unsigned int bcu_err_econ_tout:1;
		unsigned int bcu_err_econ_rdy:1;
		unsigned int bcu_err_econ_abt:1;
		unsigned int bcu_err_econ_ack:2;
		unsigned int bcu_err_econ_svm:1;
		unsigned int bcu_err_econ_wrn:1;
		unsigned int bcu_err_econ_rdn:1;
		unsigned int bcu_err_econ_tag:4;
		unsigned int bcu_err_econ_opc:4;
	} field;
#else
	struct {
		unsigned int bcu_err_econ_opc:4;
		unsigned int bcu_err_econ_tag:4;
		unsigned int bcu_err_econ_rdn:1;
		unsigned int bcu_err_econ_wrn:1;
		unsigned int bcu_err_econ_svm:1;
		unsigned int bcu_err_econ_ack:2;
		unsigned int bcu_err_econ_abt:1;
		unsigned int bcu_err_econ_rdy:1;
		unsigned int bcu_err_econ_tout:1;
		unsigned int bcu_err_econ_cnt:16;
	} field;
#endif
	unsigned int bcu_err_econ_u32;
} bcu_err_econ_t;

// bcu error structure
typedef struct {
	bcu_err_econ_t bcu_err_econ;
	u32 bcu_err_eaddr;
	u32 bcu_err_edat;
} bcu_err_info_t;

#define DANUBE_BCU_EMSG(fmt, args...) printk( "%s: " fmt, __FUNCTION__ , ##args)
#define IR0_MASK  0x00000001

/* forward declarations for fops */
//0.00052425 ms 125MHz per bus cycle = 8*10-9 ms
#define BCU_ENABLE 1
#define DEVICE_NAME "danube_cos_bcu"

static struct bcu_dev *danube_bcu_dev;

/******************************************************************************
Description:
 Danube BCU get bcu error info priority in bcu_cos device driver
Arguments:
   bcu_error_info  : specifies the pointer to a bcu_error_info pointer structure where the data of 
                   : danube_bcu_econ and danube_bcu_eadd and danube_bcu_edat will be stored.
Return Value:
   Returns none in case of no error.
Remarks:
   None.
******************************************************************************/
void
danube_bcu_get_err_info (bcu_err_info_t * bcu_error_info)
{

	u32 out;

	/* Populate data structure */
	out = *DANUBE_BCU_ECON;
	bcu_error_info->bcu_err_econ.bcu_err_econ_u32 = out;

	out = *DANUBE_BCU_EADD;
	bcu_error_info->bcu_err_eaddr = out;

	out = *DANUBE_BCU_EDAT;
	bcu_error_info->bcu_err_edat = out;
}

/******************************************************************************
Description:
   Pass BCU ioctl control parameters to BCU driver
Arguments:
   inode     : 
   file      : specifies the value to be written to the peripheral module units:
             : 1: enable power_unit
             : 0: do not enable power_unit
   cmd       : ioctl controls function calls
 input parms : struct module_id_parm -- input parameters  int module_id selects module_id and int value sets module_id
             : int istatus  -- input parameters init value = pmu_register parms
             : int istatus -- input parameters read 32 bit data register pmu status register
   arg       : other int value: invalid in power_unit module
Return Value:
   Returns '0' in case of no error, otherwise -EINVAL.
Remarks:
   None
******************************************************************************/
static int
bcu_ioctl (struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{
	int result = 0;
	u32 tmp = 0;
	u32 out;

	if (_IOC_TYPE (cmd) != DANUBE_BCU_IOC_MAGIC)
		return -ENOTTY;
	if (_IOC_NR (cmd) > DANUBE_BCU_IOC_MAXNR)
		return -ENOTTY;

	switch (cmd) {
	case DANUBE_BCU_IOC_SET_PS:
		if (copy_from_user
		    ((int *) &tmp, (int *) arg, (sizeof (int)))) {
			printk ("error\n");
			result = -EINVAL;
		}
		out = *DANUBE_BCU_CON;
		if (tmp)
			out |= (tmp << 18);
		else
			out &= 0xFFBFFFF;
		*DANUBE_BCU_CON = out;
		break;

	case DANUBE_BCU_IOC_SET_PM:
		if (copy_from_user
		    ((int *) &tmp, (int *) arg, (sizeof (int)))) {
			printk ("error\n");
			result = -EINVAL;
		}
		out = *DANUBE_BCU_CON;
		if (tmp)
			out |= (tmp << 19);
		else
			out &= 0xFF7FFFF;
		*DANUBE_BCU_CON = out;
		break;

	case DANUBE_BCU_IOC_SET_DBG:
		if (copy_from_user
		    ((int *) &tmp, (int *) arg, (sizeof (int)))) {
			printk ("error\n");
			result = -EINVAL;
		}

		out = *DANUBE_BCU_CON;
		if (tmp)
			out |= (tmp << 16);
		else
			out &= 0xFFEFFFF;	/* writing 1 to reserved bit will cause crash??? */
		*DANUBE_BCU_CON = out;
		break;

	case DANUBE_BCU_IOC_SET_TOUT:
		if (copy_from_user
		    ((u32 *) & tmp, (u32 *) arg, (sizeof (int)))) {
			printk ("error\n");
			result = -EINVAL;
		}
		if (tmp > 65535)
			return -EINVAL;
		out = (*DANUBE_BCU_CON) & 0xFFF0000;
		out |= tmp;
		*DANUBE_BCU_CON = out;
		break;
	case DANUBE_BCU_IOC_GET_PS:
		out = (*DANUBE_BCU_CON) & 0x00040000;
		tmp = out >> 18;
		copy_to_user ((int *) arg, (int *) &tmp, sizeof (int));
		break;

	case DANUBE_BCU_IOC_GET_DBG:
		out = (*DANUBE_BCU_CON) & 0x00010000;
		tmp = out >> 16;
		copy_to_user ((int *) arg, (int *) &tmp, sizeof (int));
		break;

	case DANUBE_BCU_IOC_GET_TOUT:
		out = (*DANUBE_BCU_CON) & 0x0000FFFF;
		copy_to_user ((u32 *) arg, (u32 *) & out, sizeof (int));
		break;

	case DANUBE_BCU_IOC_GET_BCU_ERR:
		{
			bcu_err_info_t bcu_err_info;
			danube_bcu_get_err_info (&bcu_err_info);	/* reference : tty_ioctl.c */
			copy_to_user ((void *) arg, &bcu_err_info,
				      sizeof (bcu_err_info));
			break;
		}
	case DANUBE_BCU_IOC_IRNEN:
		if (copy_from_user
		    ((int *) &tmp, (int *) arg, (sizeof (int)))) {
			printk ("error\n");
			result = -EINVAL;
		}
		if (tmp)
			*DANUBE_BCU_IRNEN = 0x1;
		else
			*DANUBE_BCU_IRNEN = 0x0;
		break;
	default:
		break;
	}

	return result;
}

/******************************************************************************
Description:
  Pass ocupied parms to driver, specifies state of driver. Value has to be 1 if full.
  Enable Interrupt Node in bcu_dev driver.
Arguments:
 inode  : None
 file   : None
Return Value:
   Returns '-EBUSY' otherwise '0' in case of no error 
Remarks:
   None
***********************************************************************************/
static int
bcu_open (struct inode *inode, struct file *file)
{

	if (danube_bcu_dev->full)
		return -EBUSY;

	return 0;
}

/******************************************************************************
Description:
  Pass bcu release to bcu driver. Value has to be 0 if not full.
Arguments:
 inode  : None
 file   : None
Return Value:
   Returns '0' in case of no error
Remarks:
******************************************************************************/
static int
bcu_release (struct inode *inode, struct file *file)
{
	danube_bcu_dev->full = 0;
	return 0;
}

/******************************************************************************
Description:
  Return bcu register proc_read register. return 7 32bit DANUBE_BCU_CON,
  DANUBE_BCU_ECON, DANUBE_BCU_EADD, DANUBE_BCU_EDAT, DANUBE_BCU_IRNEN, DANUBE_BCU_IRNICR, 
  DANUBE_BCU_IRNCR hardware register values.
Arguments:
  char *buf     : specifies the pointer to a buffer where the data of 
                : DANUBE_BCU_CON,DANUBE_BCU_ECON, DANUBE_BCU_EADD, DANUBE_BCU_EDAT, 
                : DANUBE_BCU_IRNEN, DANUBE_BCU_IRNICR, DANUBE_BCU_IRNCR will be copied
  start         : not used
  offset        : not used
  count         : not used
  int *eof      : indicates end of file: Is value 1 if no error occurs.
  data          : not used
Return Value:
   Returns '0' in case of no error
Remarks:
   None
******************************************************************************/
int
bcu_register_proc_read (char *buf, char **start, off_t offset, int count,
			int *eof, void *data)
{
	int len = 0;

	len += sprintf (buf + len, "DANUBE_EBU_BCU_CON	: 0x%08x\n",
			*DANUBE_BCU_CON);
	len += sprintf (buf + len, "DANUBE_EBU_BCU_ECON	: 0x%08x\n",
			*DANUBE_BCU_ECON);
	len += sprintf (buf + len, "DANUBE_EBU_BCU_EADD	: 0x%08x\n",
			*DANUBE_BCU_EADD);
	len += sprintf (buf + len, "DANUBE_EBU_BCU_EDAT	: 0x%08x\n",
			*DANUBE_BCU_EDAT);
	len += sprintf (buf + len, "DANUBE_EBU_BCU_IRNEN	: 0x%08x\n",
			*DANUBE_BCU_IRNEN);
	len += sprintf (buf + len, "DANUBE_EBU_BCU_IRNICR	: 0x%08x\n",
			*DANUBE_BCU_IRNICR);
	len += sprintf (buf + len, "DANUBE_EBU_BCU_IRNCR	: 0x%08x\n",
			*DANUBE_BCU_IRNCR);

	*eof = 1;
	return len;
}

/******************************************************************************
Description:
   Interrupt service routine to bcu driver. 
Arguments:
 irq    : None
 data   : None
 regs   : None
Return Value:
   Returns none in case of no error.
Remarks:
******************************************************************************/
void
danube_bcu_irq (int irq, void *data, struct pt_regs *regs)
{
	printk ("DANUBE_EBU_BCU_ECON   : 0x%08x\n", *DANUBE_BCU_ECON);
	printk ("DANUBE_EBU_BCU_EADD   : 0x%08x\n", *DANUBE_BCU_EADD);
	printk ("DANUBE_EBU_BCU_EDAT   : 0x%08x\n", *DANUBE_BCU_EDAT);

	return;
}

/******************************************************************************
Description:
     DANUBE_BCU_device device driver call and return functionality implementation
Arguments:
     open:   open wdt driver to state machines in bcu driver
     write:  pass 4 byte array parameters to bcu driver
     read :  read 4 byte array paramters  to bcu driver
     release: release memory to bcu driver
     ioctl:  pass parameters to bcu driver
Return Value:
   None.
Remarks:
   None.
******************************************************************************/
static struct file_operations bcu_fops = {
      open:bcu_open,
      release:bcu_release,
      ioctl:bcu_ioctl,
};

/******************************************************************************
Description:
  Initialise danube_bcu device driver.
Arguments:
  void    : None
Return Value:
   Returns '-1' in case of no error, otherwise 0 in case of none.
Remarks:
   None
******************************************************************************/
int __init
danube_bcu_init_module (void)
{
	int irq, result = 0;
	danube_bcu_dev = (bcu_dev *) kmalloc (sizeof (bcu_dev), GFP_KERNEL);
	if (danube_bcu_dev == 0) {
		return -ENOMEM;
	}

	/* dev init */
	memset (danube_bcu_dev, 0, sizeof (bcu_dev));
	/* init danube_bcu_dev */
	strcpy (danube_bcu_dev->name, DEVICE_NAME);

	result = register_chrdev (0, DEVICE_NAME, &bcu_fops);

	if (result < 0) {
		DANUBE_BCU_EMSG ("cannot register device\n");
		kfree (danube_bcu_dev);
		return -EINVAL;
	}

	danube_bcu_dev->major = result;
	create_proc_read_entry ("danube_bcu", 0, NULL, bcu_register_proc_read,
				NULL);

	danube_bcu_dev->irq = DANUBE_COSBCU_INT;	/* Suppose to be change in the future */
	irq = request_irq (danube_bcu_dev->irq, danube_bcu_irq, SA_INTERRUPT,
			   "danube_bcu_interrupt", NULL);

	if (irq) {
		printk ("danube_cos_bcu request_irq() returned: %d \n", irq);
		return -1;
	}

	return 0;
}

/******************************************************************************
Description:
  Return bcu device driver & unregister chrdev in parms char name[16] and int major parameters 
  & free_irq in parms irq
Arguments:
  void    : None
Return Value:
   Returns none in case of no error, otherwise none.
Remarks:
   None
******************************************************************************/
void
danube_bcu_cleanup_module (void)
{
	unregister_chrdev (danube_bcu_dev->major, danube_bcu_dev->name);
	free_irq (danube_bcu_dev->irq, NULL);
	remove_proc_entry ("danube_bcu", NULL);

	/* free driver */
	kfree (danube_bcu_dev);
}

module_init (danube_bcu_init_module);
module_exit (danube_bcu_cleanup_module);

EXPORT_SYMBOL (danube_bcu_get_err_info);
