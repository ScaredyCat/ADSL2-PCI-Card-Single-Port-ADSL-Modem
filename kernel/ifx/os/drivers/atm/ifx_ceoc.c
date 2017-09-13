/******************************************************************************
 *
 * ifx_ceoc.c
 *
 * Device driver supporting Clear EoC transport in MEI driver 
 *
 *
 ******************************************************************************/


/****************** Header files ***************/

#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/atmdev.h>
#include <linux/atm.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/bitops.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include <ifx/ifx_adsl_linux.h>

#define IFX_ATM_CEOC_VERSION "2.00.0"

/********* Debug Messages ***********/
#ifdef GENERAL_DEBUG
#define PRINTK(args...) printk(args)
#else
#define PRINTK(args...)
#endif /* GENERAL_DEBUG */

/********* Function declarations *********/
extern int IFX_ADSL_Ioctls(struct inode *, struct file *, unsigned int, unsigned long);

static int __init ifx_ceoc_init(void);
static int ifx_ceoc_open(struct atm_vcc *vcc, short vpi, int vci);
static void ifx_ceoc_close(struct atm_vcc *vcc);
static void __exit ifx_ceoc_cleanup(void);
static int ifx_ceoc_send(struct atm_vcc *vcc, struct sk_buff *skb);
int ifx_push_ceoc(struct sk_buff *eoc_buff);

/**********  Global variables **********/
//modified by hsumc 
static struct atm_vcc *g_atm_vcc_ptr_server = NULL;
static struct atm_vcc *g_atm_vcc_ptr_trap = NULL;
static struct atm_dev *ifx_ceoc_dev = NULL;
static int itf_number = 100;
static struct atmdev_ops ifx_ceoc_ops = {
   open:	ifx_ceoc_open,
   close:	ifx_ceoc_close,
   send:	ifx_ceoc_send,
   owner:	THIS_MODULE,
};


static int __init ifx_ceoc_init(void)
{
   unsigned error = 0;

   printk("Infineon ATM driver for Clear Eoc channel Version:%s\n",IFX_ATM_CEOC_VERSION);

   if ((ifx_ceoc_dev = kmalloc(sizeof(struct atm_dev), GFP_KERNEL)) == NULL) {
      PRINTK("IFX-EoC%d: can't allocate memory for device structure.\n", itf_number);
      error = 2;
      return error;
   }
      
   /* Register device */
   ifx_ceoc_dev = atm_dev_register("ceoc", &ifx_ceoc_ops, itf_number, NULL);
   if (ifx_ceoc_dev == NULL) {
      PRINTK("IFX-EoC%d: can't register device.\n", itf_number);
      error = 17;
      return error;
   }
   ifx_ceoc_dev->dev_data = (void *) &itf_number;
   ifx_ceoc_dev->ci_range.vpi_bits = 8;
   ifx_ceoc_dev->ci_range.vci_bits = 16;
   ifx_ceoc_dev->phy = NULL;
  
   if (IFX_ADSL_CEOC_RXCBRegister(ifx_push_ceoc) != 0)
   {
   	PRINTK("IFX_ADSL_CEOC_RXCBRegister Fail!\n");
   }
 
   return error;
}


static int ifx_ceoc_open(struct atm_vcc *vcc, short vpi, int vci)
{
   int error;
   vcc->itf = *((int *) vcc->dev->dev_data);

   if ((error = atm_find_ci(vcc, &vpi, &vci))) {
      PRINTK("IFX-EoC%d: error in atm_find_ci().\n", vcc->itf);
      return error;
   }
   vcc->vpi = vpi;
   vcc->vci = vci;

   set_bit(ATM_VF_ADDR,&vcc->flags);
   set_bit(ATM_VF_READY,&vcc->flags);

   /* Store the VCC pointer in a global structure for later usage */
   if (vpi == 0 && vci == 16 )
   	g_atm_vcc_ptr_server = vcc;
   else
	g_atm_vcc_ptr_trap = vcc;	
	

   return 0;
}


static void ifx_ceoc_close(struct atm_vcc *vcc)
{
   clear_bit(ATM_VF_READY,&vcc->flags);

   vcc->dev_data = NULL;
   clear_bit(ATM_VF_PARTIAL,&vcc->flags);
   clear_bit(ATM_VF_ADDR,&vcc->flags);

/*   atomic_set(&vcc->rx_inuse,0); */
   if (vcc->vpi == 0 && vcc->vci == 16) 		
   	{g_atm_vcc_ptr_server = NULL;}
   else
   	{g_atm_vcc_ptr_trap = NULL;}	
}


static void __exit ifx_ceoc_cleanup(void)
{
   if (IFX_ADSL_CEOC_RXCBUnregister(ifx_push_ceoc) != 0)
   {
   	PRINTK("IFX_ADSL_CEOC_RXCBUnregister Fail!\n");
   }
   atm_dev_deregister(ifx_ceoc_dev);
   kfree(ifx_ceoc_dev);
}


static int ifx_ceoc_send(struct atm_vcc *vcc, struct sk_buff *skb)
{
   PRINTK("ceoc:sending packet\n");
	/* Form the HDLC frame and call the mei driver send routine */
	/* Call the MEI driver ioctl function with AMAZON_MEI_EOC_SEND command */
	/* ?? To Do ??
		Implement the HDLC message formation in the MEI driver */
   IFX_ADSL_Ioctls((struct inode *)0,NULL, IFX_ADSL_IOC_CEOC_SEND, (unsigned long) skb);	
   atomic_inc(&vcc->stats->tx);

   return 0;
}


int ifx_push_ceoc(struct sk_buff *ceoc_buff)
{
 /* Simply Call the VCC->push function to hand over
  * the packet to the Upper Layer */
  
  //added by hsumc 
  // attention: we default send our packet to our server,
  // because the trap packet doesn't expect any response back
   
   if (g_atm_vcc_ptr_server)
   {
   	ATM_SKB(ceoc_buff)->vcc = g_atm_vcc_ptr_server;

	atomic_inc(&g_atm_vcc_ptr_server->stats->rx);
   	g_atm_vcc_ptr_server->push(g_atm_vcc_ptr_server, ceoc_buff);
   }else
   { 
   	dev_kfree_skb(ceoc_buff);
   }
   return 0;
}

MODULE_LICENSE("IFX");
MODULE_DESCRIPTION("Clear EOC support in Driver");

module_init(ifx_ceoc_init);
module_exit(ifx_ceoc_cleanup);
EXPORT_SYMBOL(ifx_push_ceoc);
