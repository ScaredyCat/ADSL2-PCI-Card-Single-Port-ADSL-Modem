/******************************************************************************
 *
 *
 *
 */  
// 507271:tc.chen 2005/07/27 fix F4 loopback issue

/* Header files ***************************************************************/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/atmdev.h>
#include <linux/atm.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <linux/netlink.h>
#include "koam.h"
/* Configurable parameters ****************************************************/

#undef GENERAL_DEBUG
#undef EXTRA_DEBUG  

/* Do not touch these *********************************************************/

#ifdef GENERAL_DEBUG
#define PRINTK(args...) printk(args)
#else
#define PRINTK(args...)
#endif /* GENERAL_DEBUG */

#ifdef EXTRA_DEBUG
#define XPRINTK(args...) printk(args)
#else 
#define XPRINTK(args...)
#endif /* EXTRA_DEBUG */

/* Macros *********************************************************************/

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* Function declarations ******************************************************/
void amz_push_oam(unsigned char *cell);
static void oam_krcv(struct sock *sk, int len);
struct atm_vcc *find_atm_vcc( int vpi,int vci );

/* Global variables ***********************************************************/
struct sock *oam_ksock;
unsigned int oam_upid ;

/* Functions *******************************************************************/
extern void ifx_set_oam_rcv_function(ifx_def_pfn_oam pfn_oam);

/* Section for 2.4.XX kernels *************************************************/

int __init koam_init_module(void)
{       

    printk(KERN_ALERT "oamk: init_module() called.\n");
    mdelay(10);

/* create the kernel socket for oam */
    printk(KERN_ALERT "Opening oam kernel socket\n");
    oam_ksock = netlink_kernel_create(NETLINK_USERSOCK, oam_krcv);
    if(!oam_ksock){
        printk("oam kernel socket not open\n");
        return -1;
    }
    netlink_set_nonroot(NETLINK_USERSOCK, NL_NONROOT_RECV);
    ifx_set_oam_rcv_function(amz_push_oam);
    printk(KERN_ALERT "oamk: init_module() returned.\n");
    return 0;
}

static void oam_krcv(struct sock *sk, int len)
{
	struct sk_buff *skb;
#ifdef AMAZON_OAM
#endif
	IFX_CELL *pCell;
	struct amazon_atm_cell_header * cell_header;
    struct atm_vcc  *vcc;
    u8      vpi,pti;
	u16		vci;

	do {
			while((skb = skb_dequeue(&sk->receive_queue)) !=NULL)
			{
				pCell = (IFX_CELL *)skb->data;

				cell_header = (struct amazon_atm_cell_header*)pCell->cell;
				vpi = cell_header->bit.vpi;
				vci = cell_header->bit.vci;
				pti = cell_header->bit.pti;
		
				//printk("\n vpi:%d, vci:%d, pti:%d\n", vpi,vci,pti);
				oam_upid = pCell->pid;
				//printk("Koam upid:%d \n", oam_upid);
				vcc = find_atm_vcc(vpi, vci );
				if(!vcc){
						// 507271:tc.chen : vpi/vci 0/0 is the inform packet from userspace to kernel space.
						if ( !(vpi == 0 && vci == 0))
						printk("oam error: No vcc for vpi=%d, vci=%d\n", vpi, vci);
						kfree_skb(skb);
						continue;
				}
#ifdef AMAZON_OAM
#else
				skb2 = alloc_skb(64, GFP_ATOMIC);
				if (skb2 == NULL) {
					kfree_skb(skb);
					printk("oam error: No SKB\n");
					continue;
				}
				cell =(u8*)skb_put(skb2,52);
				memcpy((cell+4), pCell->cell, 48); /* TBD PATCH FOR NICSTAR*/
#endif

#ifndef AMAZON_OAM
				ATM_SKB(skb2)->vcc = vcc;
				ATM_SKB(skb2)->iovcnt = 0;
				ATM_SKB(skb2)->atm_options = vcc->atm_options;

				vcc->aal_options = pti;
				skb2->nfmark = 0xAB;
				if(vcc->send(vcc,skb2)){
					printk("oam error: send driver fail \n");
				}
#else
				//printk("calling amz driver\n");
				if(vcc->dev->ops->send_oam(vcc, pCell->cell, 0)){
					printk("oam error: send driver fail \n");
				} 
#endif 
				kfree_skb(skb);
			}
	} while (oam_ksock && oam_ksock->receive_queue.qlen);
	//printk("leaving  oam_krcv\n");
}

void amz_push_oam(unsigned char *cell)
{
	struct sk_buff *skb;
	int status=0;

	if(!oam_upid){
//603081:fchang		kfree(cell);
		return;
	}
	if(!find_task_by_pid(oam_upid)){
			//fchang:added, somebody forgot to free the cell buffer!!! Bug!!
//603081:fchang			kfree(cell);
			return;
	}

	skb = alloc_skb(OAM_CELL_SIZE , GFP_ATOMIC);

	skb_put(skb, OAM_CELL_SIZE );
	memcpy(skb->data, (u8 *)cell,OAM_CELL_SIZE  );

	status = netlink_unicast(oam_ksock,skb,oam_upid,MSG_DONTWAIT);
	if (status <0){
		printk(" koam send fail \n");
		kfree_skb(skb);
	}

//603081:fchang		kfree(cell);
	return;
}

EXPORT_SYMBOL_NOVERS(amz_push_oam);

struct atm_vcc *find_atm_vcc( int vpi,int vci )
{
	struct atm_dev *atmdev = NULL;
 	struct atm_vcc *vcc = NULL;
	int dev_id = 0;

	//printk("vpi:%d, vci : %d\n", vpi,vci);
#if 0 /* [ XXX: port to 2.4.31 ATM changes */
	while( (atmdev = atm_find_dev(dev_id)))
	{
		if (atmdev == NULL)
 		{
			printk("atm dev is NULL \n");
			return NULL;
 		}

		vcc = atmdev->vccs;
		while(vcc != NULL)
		{
			//printk("atm dev vpi:%d, vci : %d\n", vcc->vpi,vcc->vci);
			if((vcc->vpi == vpi) && (vcc->vci == vci))
			{
				return vcc;
			}
			// 507271:tc.chen start
			else if ((vcc->vpi == vpi ) && (vci == 0x3 || vci == 0x4))
			{
				return vcc;
			}
			// 507271:tc.chen end
			if(vcc != atmdev->last)
		 		vcc = vcc->next;
			else
				break;	
		} // end of while loop for lor vcc list search
		dev_id++;
	}
#else
	while( (atmdev = atm_dev_lookup(dev_id)))
	{
		struct sock *s;
		if (atmdev == NULL)
 		{
			printk("atm dev is NULL \n");
			return NULL;
 		}

		s = vcc_sklist;
		for (; s != NULL; s = s->next)
		{
			vcc = s->protinfo.af_atm;
			/* Check if VCC is on atmdev device */
			if (vcc->dev != atmdev) {
				continue;
			}
			//printk("atm dev vpi:%d, vci : %d\n", vcc->vpi,vcc->vci);
			if((vcc->vpi == vpi) && (vcc->vci == vci))
			{
				return vcc;
			}
			// 507271:tc.chen start
			else if ((vcc->vpi == vpi ) && (vci == 0x3 || vci == 0x4))
			{
				return vcc;
			}
		} // end of while loop for lor vcc list search
		atm_dev_put(atmdev);
		dev_id++;
	}

#endif
	// No atmvcc found for this vpi-vci
	return NULL;
}



void __exit koam_cleanup_module(void)
{

    XPRINTK("koam: cleanup_module() called.\n");
    sock_release(oam_ksock->socket);
    if (MOD_IN_USE)
        printk(KERN_ALERT "koam: module in use, remove delayed.\n");

   

    printk(KERN_ALERT "koam: cleanup_module() returned.\n");
}


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Neeraj Jain <neeraj.jain@infineon.com>");
MODULE_DESCRIPTION("oam module for kernel");


module_init(koam_init_module);
module_exit(koam_cleanup_module);
