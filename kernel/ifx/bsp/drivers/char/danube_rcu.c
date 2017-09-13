/*
 * ########################################################################

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
 * ########################################################################
 *
 * DANUBE_RCU.H 
 *
 * Global DANUBE_RCU driver header file
 * 09-Aug 2005 Jin-Sze.Sow@infineon.com comments edited 
 *
 */
#include<linux/module.h>
#include<linux/init.h>
#include<linux/proc_fs.h>
#include<linux/kdev_t.h>
#include<asm/atomic.h>
#include<linux/fs.h>

#include <asm/uaccess.h>
#include <asm/system.h>
#include <asm/danube/danube_rcu.h>
#include <asm/danube/danube.h>

typedef struct rcu_dev{
      char name[16];
      int major;
      int minor;
      int occupied;
      int count;
      char buff[10];
}rcu_dev;

#define DANUBE_RCU_EMSG(fmt, args...) 				printk(KERN_DEBUG "%s: " fmt, __FUNCTION__ , ##args)
#define DEVICE_NAME						"danube_rcu"

static struct rcu_dev *danube_rcu_dev;
static struct proc_dir_entry* danube_rcu_dir;

static int rcu_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int result=0;

    danube_rst_req_parm_t rst_req;
    danube_rst_stat_parm_t rst_stat;

    if(_IOC_TYPE(cmd)!= DANUBE_RCU_IOC_MAGIC) return -ENOTTY;
    if(_IOC_NR(cmd) > DANUBE_RCU_IOC_MAXNR ) return -ENOTTY;
    
    switch(cmd){
	case DANUBE_RCU_IOC_SET_RCU_RST_REQ:
	{
          if(copy_from_user(&rst_req, (void*)arg, sizeof(rst_req)) < 0)
          {
		DANUBE_RCU_EMSG("invalid argument \n "); 
                result = -EINVAL;
          }
	  *DANUBE_RCU_RST_REQ = rst_req.danube_rst_req;
	  break;
	}
	case DANUBE_RCU_IOC_GET_RCU_RST_REQ:
	{
	  rst_req.danube_rst_req = *DANUBE_RCU_RST_REQ;
	  if(copy_to_user((void*)arg, &rst_req, sizeof(rst_req)) < 0){
		printk("KERN_ERR: DANUBE_RCU_IOC_GET_RCU_RST_REQ \n");
		result = -EINVAL;
	  }
	  break;
	}
        case DANUBE_RCU_IOC_GET_STATUS:
	{
          rst_stat.danube_rst_stat = *DANUBE_RCU_RST_STAT; 
	  printk("rst_stat.danube_rst_stat = 0x%08x\n", rst_stat.danube_rst_stat);
          if(copy_to_user((void*)arg, &rst_stat, sizeof(rst_stat)) < 0){

                DANUBE_RCU_EMSG("KERN_ERR: DANUBE_RCU_IOC_GET_STATUS \n");
                result = -EINVAL;
          }
          break;
	}
      	default:
          result = -ENOIOCTLCMD;
	  break;
      }
      return result;
}

static int rcu_open(struct inode *inode, struct file *file)
{
	if (danube_rcu_dev->occupied == 1) return -EBUSY;
	danube_rcu_dev->occupied = 1;
	MOD_INC_USE_COUNT;
	return 0;
}

int rcu_release(struct inode *inode, struct file *file)
{
	danube_rcu_dev->occupied = 0;
        MOD_DEC_USE_COUNT;
	return 0;
}

int rcu_register_proc_read(char *buf, char **start, off_t offset, int count, int *eof, void *data)
{

   	int len=0;
        unsigned int rcu_rst_stat;
        unsigned int rcu_rst_req;

   	len+=sprintf(buf+len ,"DANUBE_RCU_REGISTERS\n");
        /* print internals of rst_stat reg */
        rcu_rst_stat = *DANUBE_RCU_RST_STAT;
        len+=sprintf(buf+len,"DANUBE_RCU_STAT	    	    : 0x%08x\n", rcu_rst_stat);

        /* print internals of rst_req reg */
        rcu_rst_req = *DANUBE_RCU_RST_REQ;
        len+=sprintf(buf+len,"DANUBE_RCU_RST_REQ          : 0x%08x\n", rcu_rst_req);
 
  	*eof = 1;
   	return len;
}

static struct file_operations rcu_fops = {
        ioctl    :  rcu_ioctl,
        open     :  rcu_open,
        release  :  rcu_release,
};
static int __init
init_module_danube_rcu(void)
{
	int result=0;	

  	danube_rcu_dev = (rcu_dev*)kmalloc(sizeof(rcu_dev),GFP_KERNEL);

	if (danube_rcu_dev == NULL){
   		return -ENOMEM;
  	}
	memset(danube_rcu_dev,0,sizeof(rcu_dev));
	strcpy(danube_rcu_dev->name, DEVICE_NAME);
        danube_rcu_dev->minor = 0;
	result = register_chrdev(0, danube_rcu_dev->name, &rcu_fops);
  	if (result < 0) {
             DANUBE_RCU_EMSG("cannot register device\n");
	     kfree(danube_rcu_dev); 
             if(result == -EINVAL )
             	DANUBE_RCU_EMSG(" The specified number is not valid\n");                  	
             if(result == -EBUSY)
            	DANUBE_RCU_EMSG(" The major number is busy \n");	
             return -EINVAL;		                
        }
        danube_rcu_dev->major = result; 
   	create_proc_read_entry( "danube_rcu", 0, danube_rcu_dir, rcu_register_proc_read, NULL);  
	danube_rcu_dev->occupied=0;
       return 0;
}
static void __exit
danube_rcu_cleanup_module(void)
{

	unregister_chrdev(danube_rcu_dev->major,danube_rcu_dev->name);
  	kfree(danube_rcu_dev);	
  	remove_proc_entry("danube_rcu",danube_rcu_dir);
}

MODULE_LICENSE ("GPL");
MODULE_AUTHOR("Infineon IFAP DC COM");
MODULE_DESCRIPTION("DANUBE RCU driver");

module_init(init_module_danube_rcu);
module_exit(danube_rcu_cleanup_module);



