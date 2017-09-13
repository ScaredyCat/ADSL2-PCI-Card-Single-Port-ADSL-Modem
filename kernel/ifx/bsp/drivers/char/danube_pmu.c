/* Danube Power Management Unit driver
 * Copyright (c) 2006 Huang Xiaogang <xiaogang.huang@infineon.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include<linux/module.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/kdev_t.h>
#include<asm/atomic.h>
#include<linux/fs.h>

#include<asm/danube/danube.h>
#include<asm/danube/danube_pmu.h>
#include<asm/uaccess.h>

atomic_t danube_pmu_in_use;
static pmu_dev *danube_pmu_dev;

int pmu_set(int power_unit, int value)
{
	u32 cr = 0;
	
	cr = *DANUBE_PMU_PWDCR;
	
	if(value)
		cr |= 1 << power_unit;
	else
		cr &= ((~(1 << power_unit)) & 0x03FFFFFF);/* mask 0 - 25 */
	
	*DANUBE_PMU_PWDCR = cr;
//	printk("DANUBE_PMU_PWDCR = 0x%08x\n", cr);
	return 1;
}

void pmu_set_pmu_pwdcr(int istatus)
{
	*DANUBE_PMU_PWDCR = istatus;
}

int pmu_open(struct inode *inode, struct file *filp)
{
        if (atomic_dec_and_test(&danube_pmu_in_use)) {
                atomic_inc(&danube_pmu_in_use);
                return -EBUSY;
        }
	return 0;
}

int pmu_release(struct inode *inode, struct file *filp)
{
	atomic_inc(&danube_pmu_in_use);
	return 0;
}

static int pmu_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	int value_c, value_s, en;

        printk("cmd=0x%x\n", cmd);

	switch(cmd)
	{
	case DANUBE_PMU_IOC_SET_PWDCR: 
		__get_user(value_c, (int *)arg);
		pmu_set_pmu_pwdcr(value_c);
		break;
	case DANUBE_PMU_IOC_GET_STATUS:
		__put_user((int)(*DANUBE_PMU_SR), (int*)arg);
		break;
 	case DANUBE_PMU_IOC_SET_USBPHY:
	case DANUBE_PMU_IOC_SET_FPI1:    
	case DANUBE_PMU_IOC_SET_VMIPS: 
	case DANUBE_PMU_IOC_SET_VODEC:  
	case DANUBE_PMU_IOC_SET_PCI:  
	case DANUBE_PMU_IOC_SET_DMA:
	case DANUBE_PMU_IOC_SET_USB:        
	case DANUBE_PMU_IOC_SET_UART0:    
	case DANUBE_PMU_IOC_SET_SPI:    
	case DANUBE_PMU_IOC_SET_DSL:  
	case DANUBE_PMU_IOC_SET_EBU:         
	case DANUBE_PMU_IOC_SET_LEDC:      
	case DANUBE_PMU_IOC_SET_GPTC:    
	case DANUBE_PMU_IOC_SET_PPE:   
	case DANUBE_PMU_IOC_SET_FPI0:     
	case DANUBE_PMU_IOC_SET_AHB:    
	case DANUBE_PMU_IOC_SET_SDIO:  
	case DANUBE_PMU_IOC_SET_UART1:  
	case DANUBE_PMU_IOC_SET_WDT0: 
	case DANUBE_PMU_IOC_SET_WDT1:      
	case DANUBE_PMU_IOC_SET_DEU:     
	case DANUBE_PMU_IOC_SET_PPE_TC:
	case DANUBE_PMU_IOC_SET_PPE_ENET1:
	case DANUBE_PMU_IOC_SET_PPE_ENET0:           
	case DANUBE_PMU_IOC_SET_PPE_UTP:
	case DANUBE_PMU_IOC_SET_PPE_TDM:      
		__get_user(en, (int*)arg);   
		pmu_set((cmd & 0xff), en);
		break;
	default:
		return -ENOTTY;
	}
	
	return 0;
}

static struct file_operations pmu_fops =
{
        owner:    THIS_MODULE,
        open:     pmu_open,
        release:  pmu_release,
        ioctl:    pmu_ioctl,
};

static int danube_pmu_proc_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{
        int len=0;
        len+=sprintf(buf+len,"DANUBE_PMU_PROC_READ\n");

        len+=sprintf(buf+len,"DANUBE_PMU_PWDCR(0x%08x) : 0x%08x\n", DANUBE_PMU_PWDCR, *DANUBE_PMU_PWDCR);
        len+=sprintf(buf+len,"DANUBE_PMU_SR(0x%08x)    : 0x%08x\n", DANUBE_PMU_SR, *DANUBE_PMU_SR);

        *eof = 1;
        return len;
}

int __init danube_pmu_init_module(void)
{
	int result;
	dev_t dev = 0;

	printk(KERN_INFO"Danube PMU driver v0.3\n");

	danube_pmu_dev = (pmu_dev*)kmalloc(sizeof(pmu_dev), GFP_KERNEL);
	if(danube_pmu_dev == NULL)
                return -ENOMEM;
        memset(danube_pmu_dev, 0, sizeof(pmu_dev));

	strcpy(danube_pmu_dev->name, "danube_pmu");
	danube_pmu_dev->minor = 0;
	result = register_chrdev(0, danube_pmu_dev->name, &pmu_fops);
	danube_pmu_dev->major = result;
	
	create_proc_read_entry("danube_pmu", 0, NULL, danube_pmu_proc_read, NULL);
        atomic_set(&danube_pmu_in_use,0);

	return 0;
}

void danube_pmu_cleanup_module(void)
{
        unregister_chrdev(danube_pmu_dev->major, danube_pmu_dev->name);
	remove_proc_entry("danube_pmu", NULL);	
}

MODULE_AUTHOR("Huang Xiaogang <xiaogang.huang@infineon.com>");
MODULE_DESCRIPTION("Danube Power Management Unit driver");
MODULE_LICENSE("GPL");

module_init(danube_pmu_init_module);
module_exit(danube_pmu_cleanup_module);

