/******************************************************************************
       Copyright (c) 2002, Infineon Technologies.  All rights reserved.

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
******************************************************************************/
/*
 * DANUBE_WDT.C
 * Global DANUBE_WDT driver header file
 * 04-Aug 2005 Jin-Sze.Sow@infineon.com comments edited
 */

#if defined(MODVERSIONS)
#include <linux/modversions.h>
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include <linux/stat.h>
#include <linux/tty.h>
#include <linux/selection.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/kdev_t.h>
#include <linux/ioctl.h>
#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm-mips/danube/danube_wdt.h>
#include <asm-mips/danube/danube.h>

typedef struct wdt_dev{
      char name[16];
      int major;
      int minor;
      int full;
      char buff[10];
}wdt_dev;

const char WDT_VERSION[] = "WDT_VERSION_0.0.1" ;

#define DANUBE_WDT_EMSG(fmt, args...) printk( "%s: " fmt, __FUNCTION__ , ##args)
#define DANUBE_WDT_DMSG(fmt, args...) printk(" %s:" fmt, __FUNCTION__, ##args)

extern unsigned int danube_get_fpi_hz(void);

static struct wdt_dev *danube_wdt_dev;
static int occupied=0;

int wdt_enable(u32 timeout)
{
        u32 wdt_cr=0;
        u32 wdt_reload=0;
	u32 wdt_clkdiv, clkdiv, wdt_pwl, pwl, ffpi;
        
  	/* clock divider & prewarning limit */
	switch(clkdiv = DANUBE_BIU_WDT_CR_CLKDIV_GET(*DANUBE_BIU_WDT_CR)){
	 case 0: wdt_clkdiv = 1; break;
	 case 1: wdt_clkdiv = 64; break;
	 case 2: wdt_clkdiv = 4096; break;
	 case 3: wdt_clkdiv = 262144; break;
	}
	
	switch(pwl = DANUBE_BIU_WDT_CR_PWL_GET(*DANUBE_BIU_WDT_CR)){
	 case 0: wdt_pwl = 0x8000; break;
	 case 1: wdt_pwl = 0x4000; break;
	 case 2: wdt_pwl = 0x2000; break;
	 case 3: wdt_pwl = 0x1000; break;
	}

#ifdef CONFIG_USE_EMULATOR
	ffpi = 1250000;
#else
//	ffpi = danube_get_cpu_hz();
//	ffpi = cgu_get_cpu_clock();
	ffpi = cgu_get_io_region_clock();
        printk("cpu clock = %d\n", ffpi);
#endif

	/* caculate reload value */
	wdt_reload = (timeout * (ffpi / wdt_clkdiv)) + wdt_pwl;

        printk("wdt_pwl=0x%x, wdt_clkdiv=%d, ffpi=%d, wdt_reload = 0x%x\n", wdt_pwl, wdt_clkdiv, ffpi, wdt_reload);

        if (wdt_reload > 0xFFFF){
                DANUBE_WDT_EMSG("timeout too large %d\n", timeout);
                return -EINVAL;
        }
 
	/* Write first part of password access */
        *DANUBE_BIU_WDT_CR = DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW1);

        wdt_cr = *DANUBE_BIU_WDT_CR;
        wdt_cr &= ( !DANUBE_BIU_WDT_CR_PW_SET(0xff) &
                    !DANUBE_BIU_WDT_CR_PWL_SET(0x3) &
                    !DANUBE_BIU_WDT_CR_CLKDIV_SET(0x3) &
                    !DANUBE_BIU_WDT_CR_RELOAD_SET(0xffff));

        wdt_cr |= ( DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW2) |
                    DANUBE_BIU_WDT_CR_PWL_SET(pwl) |
                    DANUBE_BIU_WDT_CR_CLKDIV_SET(clkdiv) |
                    DANUBE_BIU_WDT_CR_RELOAD_SET(wdt_reload) |
		    DANUBE_BIU_WDT_CR_GEN);

        /* Set reload value in second password access */
        *DANUBE_BIU_WDT_CR = wdt_cr;
	printk("enabled\n");
        return 0 ;
}

void wdt_disable(void)
{
        /* Write first part of password access */
	*DANUBE_BIU_WDT_CR = DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW1);
         /* Disable the watchdog in second password access (GEN=0) */
        *DANUBE_BIU_WDT_CR = DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW2);
}

void wdt_low_power(int en)
{
        u32 wdt_cr=0;
	
	*DANUBE_BIU_WDT_CR = DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW1);

        wdt_cr = *DANUBE_BIU_WDT_CR;
	if(en){
	        wdt_cr &= (!DANUBE_BIU_WDT_CR_PW_SET(0xff));
	        wdt_cr |= (DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW2) | DANUBE_BIU_WDT_CR_LPEN);
	}
	else{
                wdt_cr &= (!DANUBE_BIU_WDT_CR_PW_SET(0xff) &
			   !DANUBE_BIU_WDT_CR_LPEN);
                wdt_cr |= DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW2);
	}		

        /* Set reload value in second password access */
        *DANUBE_BIU_WDT_CR = wdt_cr;
}

void wdt_debug_suspend(int en)
{
        u32 wdt_cr=0;

        *DANUBE_BIU_WDT_CR = DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW1);

        wdt_cr = *DANUBE_BIU_WDT_CR;
        if(en){
                wdt_cr &= (!DANUBE_BIU_WDT_CR_PW_SET(0xff));
                wdt_cr |= (DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW2) | DANUBE_BIU_WDT_CR_DSEN);
        }
        else{
                wdt_cr &= (!DANUBE_BIU_WDT_CR_PW_SET(0xff) &
                           !DANUBE_BIU_WDT_CR_DSEN);
                wdt_cr |= DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW2);
        }

        /* Set reload value in second password access */
        *DANUBE_BIU_WDT_CR = wdt_cr; 
}

void wdt_prewarning_limit(int pwl)
{
        u32 wdt_cr=0;
	
        wdt_cr = *DANUBE_BIU_WDT_CR;
        *DANUBE_BIU_WDT_CR = DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW1);

        wdt_cr &= 0xff00ffff;//(!DANUBE_BIU_WDT_CR_PW_SET(0xff));
	wdt_cr &= 0xf3ffffff;//(!DANUBE_BIU_WDT_CR_PWL_SET(3));
        wdt_cr |= (DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW2) | 
		   DANUBE_BIU_WDT_CR_PWL_SET(pwl));

        /* Set reload value in second password access */
        *DANUBE_BIU_WDT_CR = wdt_cr;
}

void wdt_set_clkdiv(int clkdiv)
{
        u32 wdt_cr=0;

        wdt_cr = *DANUBE_BIU_WDT_CR;
        *DANUBE_BIU_WDT_CR = DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW1);

        wdt_cr &= 0xff00ffff; //(!DANUBE_BIU_WDT_CR_PW_SET(0xff));
        wdt_cr &= 0xfcffffff; //(!DANUBE_BIU_WDT_CR_CLKDIV_SET(3));
        wdt_cr |= (DANUBE_BIU_WDT_CR_PW_SET(DANUBE_WDT_PW2) |
                   DANUBE_BIU_WDT_CR_CLKDIV_SET(clkdiv));

        /* Set reload value in second password access */
        *DANUBE_BIU_WDT_CR = wdt_cr;
}

static int wdt_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int result=0;
    int en=0;
    int istatus;
    int pwl, clkdiv;
    static int timeout = -1;
                            
    switch(cmd){
        case DANUBE_WDT_IOC_START:
      		DANUBE_WDT_DMSG("enable watch dog timer!\n");
	        if ( copy_from_user((void*)&timeout, (void*)arg, sizeof (int)) ){
		           DANUBE_WDT_EMSG("invalid argument\n");
	        	   result=-EINVAL;
	    	}else{
			if((result = wdt_enable(timeout)) < 0){
				timeout = -1;
			}
		}
	       break;
       case DANUBE_WDT_IOC_STOP:
	      	DANUBE_WDT_DMSG("disable watch dog timer\n");
		timeout = -1;
	      	wdt_disable();
	       break;
       case DANUBE_WDT_IOC_PING:
	       	if(timeout < 0){
			result = -EIO;
		}else{
			result = wdt_enable(timeout);
		}
	       break;

       case DANUBE_WDT_IOC_SET_PWL:
	       if ( copy_from_user((void*)&pwl, (void*)arg, sizeof (int)) ){
        	   DANUBE_WDT_EMSG("invalid argument\n");
	           result=-EINVAL;
	      }		              
	      wdt_prewarning_limit(pwl);
	      break;
      case DANUBE_WDT_IOC_SET_DSEN:
	      if ( copy_from_user((void*)&en, (void*)arg, sizeof (int)) ){
	           DANUBE_WDT_EMSG("invalid argument\n");
	           result=-EINVAL;
	       }		              
	      wdt_debug_suspend(en);
	      break;
      case DANUBE_WDT_IOC_SET_LPEN:
	          if ( copy_from_user((void*)&en, (void*)arg, sizeof (int)) ){
			           DANUBE_WDT_EMSG("invalid argument\n");
			           result=-EINVAL;
		       }		              
        	   wdt_low_power(en);
	      break;
      case DANUBE_WDT_IOC_SET_CLKDIV:
                  if ( copy_from_user((void*)&clkdiv, (void*)arg, sizeof (int)) ){
                                   DANUBE_WDT_EMSG("invalid argument\n");
                                   result=-EINVAL;
                       }
                   wdt_set_clkdiv(clkdiv);
              break;
      case DANUBE_WDT_IOC_GET_STATUS:
           istatus = DANUBE_WDT_REG32(DANUBE_BIU_WDT_SR);     
           copy_to_user((int *)arg, (int *)&istatus, sizeof(int));
	      break;
      }

      return result;
}

static int wdt_open(struct inode *inode, struct file *file)
{
	DANUBE_WDT_DMSG("wdt_open\n");
	if (occupied == 1) return -EBUSY;
	occupied = 1;
	MOD_INC_USE_COUNT;
	
	return 0;
}

static int wdt_release(struct inode *inode, struct file *file)
{
  	DANUBE_WDT_DMSG("wdt_release\n"); 
	occupied = 0;
	MOD_DEC_USE_COUNT;
	return 0;
}

int wdt_register_proc_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
   	int len=0;
        len+=sprintf(buf+len,"DANUBE_BIU_WDT_PROC_READ\n");
        
   	len+=sprintf(buf+len,"DANUBE_BIU_WDT_CR(0x%08x)	: 0x%08x\n", DANUBE_BIU_WDT_CR, DANUBE_WDT_REG32(DANUBE_BIU_WDT_CR));	
   	len+=sprintf(buf+len,"DANUBE_BIU_WDT_SR(0x%08x)	: 0x%08x\n", DANUBE_BIU_WDT_SR, DANUBE_WDT_REG32(DANUBE_BIU_WDT_SR));

  	*eof = 1;
   	return len;
}

static struct file_operations wdt_fops =
{
        owner:          THIS_MODULE,
        ioctl:          wdt_ioctl,
        open:           wdt_open,
        release:        wdt_release,
};

int __init danube_wdt_init_module(void)
{
        int result =0; 

  	danube_wdt_dev = (wdt_dev*)kmalloc(sizeof(wdt_dev),GFP_KERNEL);
        printk(KERN_INFO , "%s\n", &WDT_VERSION[20]);
        
  	if (danube_wdt_dev == NULL){
   		return -ENOMEM;
  	}
	memset(danube_wdt_dev,0,sizeof(wdt_dev));
	
	strcpy(danube_wdt_dev->name, DEVICE_NAME);
	
	result = register_chrdev(0,danube_wdt_dev->name,&wdt_fops);

  	if (result < 0) {
  
       	  	DANUBE_WDT_EMSG("cannot register device\n");
                kfree(danube_wdt_dev);
		return -EINVAL;		                
        }
         
        danube_wdt_dev->major = result; 

        /* Create proc file */
        create_proc_read_entry("danube_wdt", 0, NULL, wdt_register_proc_read , NULL);
        return 0;
}

void danube_wdt_cleanup_module(void)
{
  	unregister_chrdev(danube_wdt_dev->major,danube_wdt_dev->name);
  	remove_proc_entry("danube_wdt", NULL);
        kfree(danube_wdt_dev); 
	return;
}


module_init(danube_wdt_init_module);
module_exit(danube_wdt_cleanup_module);


