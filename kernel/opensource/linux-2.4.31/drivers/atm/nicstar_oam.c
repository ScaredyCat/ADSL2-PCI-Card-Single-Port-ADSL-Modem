/******************************************************************************
 *
 * nicstar.c   (Nicstar2 driver)
 *
 * Date: 28/052004
 * Version: 2.01
 *
 * LINUX Device driver supporting UBR, CBR, VBR and ABR for PROATM cards based
 * on the IDT 77252 SAR.
 *
 *
 * IMPORTANT:   This code is derived from the Linux nicstar driver for
 *			    IDT 77211 SAR from Rui Prior. Some portions of this
 *			    driver still exist here.
 *
 * Author: Christian Bucari: bucari@prosum.net
 * Copyright (c) 2002, 2003, 2004 Prosum
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation.
 * You should have received a copy of the  GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ******************************************************************************
 * Note on VBR usage:
 *
 * VBR QoS is characterized by using pcr, scr and mbs parameters.
 * So far (we don't know why), the atm_trafprm structure does not provide
 * neither scr nor mbs. We could modify atm_traf_prm but we are a bit shiny in
 * mofifying kernel structures so we prefered to use max_pcr in place of pcr,
 * pcr in place of scr and min_pcr in place of mbs (bt). This is ugly but it
 * makes the job without impacting on other ATM layers and drivers.
 * So when using VBR VC's, enter the pcr value into max_pcr, the scr value into
 * pcr and the the mbs value into min_pcr. This VBR implementation is intended
 * for writing direct atm applications that manage the qos structure directly
 * since the Linux-atm tools do not provide direct VBR support. With a simple
 * modification you may also add the VBR qos support into qos2text.c and
 * text2qos.c and recompile linux-atm.
 *
 ******************************************************************************
 * Note on PROATM card support. (Look at README file for installation details)
 *
 * PROATM-155 cards use the PMC SIERRA SUNI or the IDT77155 physical interface
 * whereas PROATM-25 cards use the IDT77105 physical interface.
 * Defining CONFIG_ATM_NICSTAR_USE_SUNI and CONFIG_ATM_NICSTAR_USE_IDT77105
 * makes Nictar2 support both 25.6 and 155 Mbps PROATM cards.
 *
 *
 */

/* Header files ***************************************************************/
#include <linux/version.h>
#include <linux/module.h>
#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/atmdev.h>
#include <linux/atm.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include "nicstar_oam.h"
#include "suni.h"
#include "idt77105.h"
#include "nicstar_rtbl.c"

#include <linux/netlink.h>

#if BITS_PER_LONG != 32
#  error FIXME: this driver requires a 32-bit platform
#endif

/*
#undef CONFIG_ATM_NICSTAR_USE_IDT77105
#undef CONFIG_ATM_NICSTAR_USE_SUNI
*/

/* Configurable parameters ****************************************************/

#undef PHY_LOOPBACK
// #define PHY_LOOPBACK 1
#undef GENERAL_DEBUG
#undef EXTRA_DEBUG

#undef NS_USE_DESTRUCTORS /* For now keep this undefined unless you know
                                you're going to use only raw ATM */
#undef NS_POLLING

#define GENERAL_DEBUG 1
//#undef GENERAL_DEBUG
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

#define CMD_BUSY(card) (readl((card)->membase + STAT) & NS_STAT_CMDBZ)

#define NS_DELAY mdelay(1)

#define ALIGN_BUS_ADDR(addr, alignment) \
        ((((u32) (addr)) + (((u32) (alignment)) - 1)) & ~(((u32) (alignment)) - 1))
#define ALIGN_ADDRESS(addr, alignment) \
        bus_to_virt(ALIGN_BUS_ADDR(virt_to_bus(addr), alignment))

#undef CEIL

#ifndef ATM_SKB
#define ATM_SKB(s) (&(s)->atm)
#endif

#define ns_grab_int_lock(card,flags)   spin_lock_irqsave(&(card)->int_lock,(flags))
#define ns_grab_res_lock(card,flags)   spin_lock_irqsave(&(card)->res_lock,(flags))
#define ns_grab_scq_lock(card,scq,flags)  spin_lock_irqsave(&(scq)->lock,flags)

void infi_push_oam(unsigned char *);
extern struct sock *oam_ksock;
extern unsigned int oam_upid ;


/* Function prototypes ******************************************************/
static u32 ns_read_sram(ns_dev *card, u32 sram_address);
static void ns_write_sram(ns_dev *card, u32 sram_address, u32 *value, int count);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
static int __init ns_init_card(int i, struct pci_dev *pcidev);
static void __init ns_init_card_error(ns_dev *card, int error);
static void ns_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
static int ns_open(struct atm_vcc *vcc, short vpi, int vci);
#else
static int __devinit ns_init_card(int i, struct pci_dev *pcidev);
static void __devinit ns_init_card_error(ns_dev *card, int error);
static irqreturn_t ns_irq_handler(int irq, void *dev_id, struct pt_regs *regs);
static int ns_open(struct atm_vcc *vcc);
#endif
static void  ns_close(struct atm_vcc *vcc);
static scq_info *get_scq(int type, u32 scd);
static void free_scq(ns_dev *card, scq_info *scq, struct atm_vcc *vcc);
static void push_rxbufs(ns_dev *card, u32 type, u32 handle1, u32 addr1,
                            u32 handle2, u32 addr2);
static void fill_tst(ns_dev *card, int n, vc_map *vc, u32 conn);
static int ns_send(struct atm_vcc *vcc, struct sk_buff *skb);
static int push_scqe(ns_dev *card, vc_map *vc, scq_info *scq, ns_scqe *tbd,
                          struct sk_buff *skb);
static void process_tsq(ns_dev *card);
static void drain_scq(ns_dev *card, scq_info *scq, int pos);
static void process_rsq(ns_dev *card);
static void dequeue_rx(ns_dev *card, ns_rsqe *rsqe);

#ifdef NS_USE_DESTRUCTORS
static void ns_sb_destructor(struct sk_buff *sb);
static void ns_lb_destructor(struct sk_buff *lb);
static void ns_hb_destructor(struct sk_buff *hb);

#endif /* NS_USE_DESTRUCTORS */
static void recycle_rx_buf(ns_dev *card, struct sk_buff *skb);
static void recycle_iovec_rx_bufs(ns_dev *card, struct iovec *iov, int count);
static void recycle_iov_buf(ns_dev *card, struct sk_buff *iovb);
static void dequeue_sm_buf(ns_dev *card, struct sk_buff *sb);
static void dequeue_lg_buf(ns_dev *card, struct sk_buff *lb);
static int ns_proc_read(struct atm_dev *dev, loff_t *pos, char *page);
static int ns_ioctl(struct atm_dev *dev, unsigned int cmd, void *arg);
static void which_list(ns_dev *card, struct sk_buff *skb);
static void ns_eeprom_rd (ns_dev *card, u32 ee_add, u8 *memptr, u32 len);
#ifdef NS_POLLING
static void ns_poll(unsigned long arg);
#endif
static int ns_parse_mac(char *mac, unsigned char *esi);
static short ns_h2i(char c);
static void ns_phy_put(struct atm_dev *dev, unsigned char value,
                           unsigned long addr);
static unsigned char ns_phy_get(struct atm_dev *dev, unsigned long addr);
static void waitfor_notbusy(struct ns_dev *card);
static void ns_push_on_scq (ns_dev *card, scq_info *scq);
static int ns_phys_init (ns_dev *card, u32 *);
inline static int ns_find_conn (ns_dev *card, short vpi, int vci);
static int ns_check_vc (ns_dev *card, short vpi, int vci);
static void ns_process_rcqe (ns_dev *card, ns_rcqe *rawcell);
static void ns_rawc(ns_dev *card);

/* Global variables ***********************************************************/
static struct ns_dev *cards[NS_MAX_CARDS];
static unsigned num_cards = 0;
#if LINUX_VERSION_CODE < KERNEL_VERSION (2,6,0)
static struct atmdev_ops atm_ops =
{
    open:       ns_open,
    close:      ns_close,
    ioctl:      ns_ioctl,
    send:       ns_send,
    phy_put:    ns_phy_put,
    phy_get:    ns_phy_get,
    proc_read:  ns_proc_read
};
#else
static struct atmdev_ops atm_ops =
{
   .open	    = ns_open,
   .close	    = ns_close,
   .ioctl	    = ns_ioctl,
   .send	    = ns_send,
   .phy_put	    = ns_phy_put,
   .phy_get	    = ns_phy_get,
   .proc_read   = ns_proc_read,
   .owner	    = THIS_MODULE,
};
#endif
#ifdef NS_POLLING
static struct timer_list ns_timer;
#endif
static char *mac[NS_MAX_CARDS] = {
    NULL
#if NS_MAX_CARDS > 1
    , NULL
#endif
#if NS_MAX_CARDS > 2
    , NULL
#endif
#if NS_MAX_CARDS > 3
    , NULL
#endif
#if NS_MAX_CARDS > 4
    , NULL
#endif
}
;
static int vpibits = NS_VPIBITS;
static int vpibase = NS_VPIBASE;
static int vcibase = NS_VCIBASE;

#ifdef MODULE
MODULE_PARM(mac, "1-"__MODULE_STRING(NS_MAX_CARDS)"s");
MODULE_PARM(vpibits, "i");
MODULE_PARM(vcibase, "i");
MODULE_PARM(vpibase, "i");
#endif /* MODULE */

/* Functions *******************************************************************/

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
/* Section for 2.4.XX kernels *************************************************/

#ifdef MODULE
int __init init_module(void)
{
    int             i;
    unsigned        error = 0;     /* Initialized to remove compile warning */
    struct pci_dev  *pcidev;

    XPRINTK("nicstar: init_module() called.\n");
    mdelay(10);

    if (!pci_present()) {
        printk("nicstar: no PCI subsystem found.\n");
        return - EIO;
    }

    for (i = 0; i < NS_MAX_CARDS; i++)
             cards[i] = NULL;

    pcidev = NULL;
    for (i = 0; i < NS_MAX_CARDS; i++) {
        if ((pcidev = pci_find_device(PCI_VENDOR_ID_IDT,
                                      PCI_DEVICE_ID_IDT_IDT77252,
                                      pcidev)) == NULL)
                      break;

        error = ns_init_card(i, pcidev);
        if (error)
            cards[i--] = NULL;
       /* Try to find another card but don't increment index */
    }
    if (i == 0) {
        if (!error) {
            printk("nicstar: no card found.\n");
            return - ENXIO;
        }
        else
            return - EIO;
    }
    PRINTK("nicstar: General debug enabled.\n");
    XPRINTK("nicstar: extra debug enabled.\n");
#ifdef PHY_LOOPBACK
    printk("nicstar: using PHY loopback.\n");
#endif /* PHY_LOOPBACK */

#ifdef NS_POLLING
    init_timer(&ns_timer);
    ns_timer.expires = jiffies + NS_POLL_PERIOD;
    ns_timer.data = 0UL;
    ns_timer.function = ns_poll;
    add_timer(&ns_timer);
#endif

    XPRINTK("nicstar: init_module() returned.\n");
    return 0;
}

void cleanup_module(void)
{
    int             i, j;
    unsigned short  pci_command;
    ns_dev          *card;
    struct sk_buff  *hb;
    struct sk_buff  *iovb;
    struct sk_buff  *lb;
    struct sk_buff  *sb;

    XPRINTK("nicstar: cleanup_module() called.\n");
    if (MOD_IN_USE)
        printk("nicstar: module in use, remove delayed.\n");
#ifdef NS_POLLING
    del_timer(&ns_timer);
#endif
    for (i = 0; i < NS_MAX_CARDS; i++) {
        if (cards[i] == NULL)
            continue;

        /* stop Phy */
        card = cards[i];

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,22)
#ifdef CONFIG_ATM_NICSTAR_USE_IDT77105
        if (card->max_pcr == ATM_25_PCR) {
            idt77105_stop(card->atmdev);
        }
#endif
#ifdef CONFIG_ATM_NICSTAR_USE_SUNI
        if (card->max_pcr == ATM_OC3_PCR) {
            (*card->atmdev->phy->stop)(card->atmdev);
        }
#endif
#else
        (*card->atmdev->phy->stop)(card->atmdev);
#endif
        /* Stop everything */
        writel(0x00000000, card->membase + CFG);

        /* De-register device */
        atm_dev_deregister(card->atmdev);

        /* Disable memory mapping and busmastering */
        if (pci_read_config_word(card->pcidev, PCI_COMMAND, &pci_command) != 0) {
            printk("nicstar%d: can't read PCI_COMMAND.\n", i);
        }
        pci_command &= ~(PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
        if (pci_write_config_word(card->pcidev, PCI_COMMAND, pci_command) != 0) {
            printk("nicstar%d: can't write PCI_COMMAND.\n", i);
        }
        /* Free up resources */
        j = 0;
        PRINTK("nicstar%d: freeing %d huge buffers.\n", i, card->hbpool.count);
        while ((hb = skb_dequeue(&card->hbpool.queue)) != NULL) {
            dev_kfree_skb_any(hb);
            j++;
        }
        PRINTK("nicstar%d: %d huge buffers freed.\n", i, j);
        j = 0;
        PRINTK("nicstar%d: freeing %d iovec buffers.\n", i, card->iovpool.count);
        while ((iovb = skb_dequeue(&card->iovpool.queue)) != NULL) {
            dev_kfree_skb_any(iovb);
            j++;
        }
        PRINTK("nicstar%d: %d iovec buffers freed.\n", i, j);
        while ((lb = skb_dequeue(&card->b1pool.queue)) != NULL)
                     dev_kfree_skb_any(lb);

        while ((sb = skb_dequeue(&card->b0pool.queue)) != NULL)
                     dev_kfree_skb_any(sb);

		PRINTK("nicstar%d: freeing SCQ0.\n",i);
        free_scq(card, card->scq0, NULL);

		PRINTK("nicstar%d: freeing SCQs.\n", i);
        for (j = 0; j < NS_SCD_NUM; j++) {
            if (card->scd2vc[j] != NULL)
                free_scq(card, card->scd2vc[j]->scq, card->scd2vc[j]->tx_vcc);
        }
		PRINTK("nicstar%d: freeing card.\n", i);
        kfree(card->vcmap);
        kfree(card->rsq.org);
        kfree(card->tsq.org);
        free_irq(card->pcidev->irq, card);
        iounmap((void *) card->membase);
        kfree(card);
    }
    XPRINTK("nicstar: cleanup_module() returned.\n");
}

#else

int __init nicstar_detect(void)
{
    int             i;
    unsigned        error = 0;
    struct pci_dev  *pcidev;

    if (!pci_present()) {
        printk("nicstar: no PCI subsystem found.\n");
        return - EIO;
    }
    for (i = 0; i < NS_MAX_CARDS; i++)
             cards[i] = NULL;

    pcidev = NULL;
    for (i = 0; i < NS_MAX_CARDS; i++) {
        if ((pcidev = pci_find_device(PCI_VENDOR_ID_IDT,
                                      PCI_DEVICE_ID_IDT_IDT77252,
                                      pcidev)) == NULL)
                      break;

        error = ns_init_card(i, pcidev);
        if (error)
            cards[i--] = NULL;
        /* Try to find another card but don't increment index */
    }
    pcidev = NULL;
    for (; i < NS_MAX_CARDS; i++) {
        if ((pcidev = pci_find_device(PCI_VENDOR_ID_IDT,
                                      PCI_DEVICE_ID_IDT_IDT77V252,
                                      pcidev)) == NULL)
                      break;

        error = ns_init_card(i, pcidev);
        if (error)
            cards[i--] = NULL;
        /* Try to find another card but don't increment index */
    }
    if (i == 0 && error)
        return - EIO;

    XPRINTK("nicstar: Extra debug enabled.\n");
    PRINTK("nicstar: General debug enabled.\n");
#ifdef PHY_LOOPBACK
    printk("nicstar: using PHY loopback.\n");
#endif /* PHY_LOOPBACK */

#ifdef NS_POLLING
    init_timer(&ns_timer);
    ns_timer.expires = jiffies + NS_POLL_PERIOD;
    ns_timer.data = 0UL;
    ns_timer.function = ns_poll;
    add_timer(&ns_timer);
#endif

    return i;
}

#endif /* MODULE */

#else
/* Section for 2.6.XX kernels ****************************************/

static int __devinit nicstar_init_one(struct pci_dev *pcidev,
				      const struct pci_device_id *ent)
{
   static int index = 0;

   PRINTK("nicstar%d: nicstar_init_one() called.\n", index);

   cards[index] = NULL;
   if (ns_init_card (index, pcidev)) {
      cards[index] = NULL;	/* don't increment index */
      return -ENODEV;
   }
   index++;
   return 0;
}

static void __devexit nicstar_remove_one(struct pci_dev *pcidev)
{
   int i, j;
   ns_dev *card = pci_get_drvdata(pcidev);
   struct sk_buff *hb;
   struct sk_buff *iovb;
   struct sk_buff *lb;
   struct sk_buff *sb;

   XPRINTK("nicstar: nicstar_remove_one() called.\n");
   i = card->index;

   if (cards[i] == NULL)
      return;

   /* stop phy */
   if (card->atmdev->phy && card->atmdev->phy->stop)
      (*card->atmdev->phy->stop)(card->atmdev);

   /* Stop everything */
   writel(0x00000000, card->membase + CFG);

   /* De-register device */
   atm_dev_deregister(card->atmdev);

   /* Disable PCI device */
   pci_disable_device(pcidev);

   /* Free up resources */
   j = 0;
   PRINTK("nicstar%d: freeing %d huge buffers.\n", i, card->hbpool.count);
   while ((hb = skb_dequeue(&card->hbpool.queue)) != NULL) {
      dev_kfree_skb_any(hb);
      j++;
   }
   PRINTK("nicstar%d: %d huge buffers freed.\n", i, j);
   j = 0;
   PRINTK("nicstar%d: freeing %d iovec buffers.\n", i, card->iovpool.count);
   while ((iovb = skb_dequeue(&card->iovpool.queue)) != NULL){
      dev_kfree_skb_any(iovb);
      j++;
   }
   PRINTK("nicstar%d: %d iovec buffers freed.\n", i, j);
   while ((lb = skb_dequeue(&card->b1pool.queue)) != NULL)
      dev_kfree_skb_any(lb);

   while ((sb = skb_dequeue(&card->b0pool.queue)) != NULL)
      dev_kfree_skb_any(sb);

   PRINTK("nicstar%d: freeing SCQ0.\n",i);
   free_scq(card, card->scq0, NULL);

   PRINTK("nicstar%d: freeing SCQs.\n", i);
   for (j = 0; j < NS_SCD_NUM; j++) {
        if (card->scd2vc[j] != NULL)
            free_scq(card, card->scd2vc[j]->scq, card->scd2vc[j]->tx_vcc);
   }
   PRINTK("nicstar%d: freeing card.\n", i);
   kfree(card->vcmap);
   kfree(card->rsq.org);
   kfree(card->tsq.org);
   free_irq(card->pcidev->irq, card);
   iounmap((void *) card->membase);
   kfree(card);
   XPRINTK("nicstar: nicstar_remove_one() returned.\n");

}

static struct pci_device_id nicstar_pci_tbl[] __devinitdata =
{
	{ PCI_VENDOR_ID_IDT, PCI_DEVICE_ID_IDT_IDT77252,
	  PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ 0, }
};
MODULE_DEVICE_TABLE(pci, nicstar_pci_tbl);

static struct pci_driver nicstar_driver = {
	.name		= "nicstar",
	.id_table	= nicstar_pci_tbl,
	.probe		= nicstar_init_one,
	.remove		= __devexit_p(nicstar_remove_one),
};

static int __init nicstar_init(void)
{
   unsigned error = 0;	/* Initialized to remove compile warning */

   XPRINTK("nicstar: nicstar_init() called.\n");

   error = pci_module_init (&nicstar_driver);
   if (error)
        return error;

   XPRINTK("nicstar: Extra debug enabled.\n");
   PRINTK("nicstar: General debug enabled.\n");

#ifdef PHY_LOOPBACK
   printk("nicstar: using PHY loopback.\n");
#endif /* PHY_LOOPBACK */

#ifdef NS_POLLING
   init_timer(&ns_timer);
   ns_timer.expires = jiffies + NS_POLL_PERIOD;
   ns_timer.data = 0UL;
   ns_timer.function = ns_poll;
   add_timer(&ns_timer);
#endif

   return 0;
}

static void __exit nicstar_cleanup(void)
{
   XPRINTK("nicstar: nicstar_cleanup() called.\n");

#ifdef NS_POLLING
   del_timer(&ns_timer);
#endif
   pci_unregister_driver(&nicstar_driver);

   XPRINTK("nicstar: nicstar_cleanup() returned.\n");
}

#endif
/* common section ***********************************************************/

inline static void waitfor_notbusy(struct ns_dev *card)
{
    u32             stat;

    stat = readl(card->membase + STAT);
    while (stat & NS_STAT_CMDBZ)
        stat = readl((card)->membase + STAT);
}

static u32 ns_read_sram(ns_dev *card, u32 sram_address)
{
    unsigned long   flags;
    u32             data;
    sram_address    <<= 2;

    sram_address &= 0x0007FFFC;
    sram_address |= 0x50000000;                       /* SRAM read command */
    ns_grab_res_lock(card, flags);
    waitfor_notbusy(card);
    writel(sram_address, card->membase + CMD);
    waitfor_notbusy(card);
    data = readl(card->membase + DR0);
    spin_unlock_irqrestore(&card->res_lock, flags);
    return data;
}

static void ns_write_sram(ns_dev *card, u32 sram_address, u32 *value, int count)
{
    unsigned long   flags;
    int             i, c;

    count--;                        /* count range now is 0..3 instead of 1..4 */
    c = count;
    c <<= 2;                        /* to use increments of 4 */
    ns_grab_res_lock(card, flags);
    waitfor_notbusy(card);
    for (i = 0; i <= c; i += 4)
       writel(* (value++), card->membase + i);

    /* Note: DR# registers are the first 4 dwords in nicstar's memspace,
                so card->membase + DR0 == card->membase */
    sram_address <<= 2;
    sram_address &= 0x0007FFFC;
    sram_address |= (0x40000000 | count);

    writel(sram_address, card->membase + CMD);
    spin_unlock_irqrestore(&card->res_lock, flags);
    waitfor_notbusy(card);
}

static int __devinit ns_phys_init (ns_dev *card, u32 *ns_cfg)
{
    u32 data;

    /* PHY reset */
    writel(0xa, card->membase + GP);
    NS_DELAY;
    writel(0x2, card->membase + GP);
    NS_DELAY;
    waitfor_notbusy(card);
    XPRINTK("nicstar%d: DR0 read (0x%08X).\n", card->index,
                                        readl(card->membase + DR0));
    NS_DELAY;

    /* Detect PHY type */
    printk("nicstar%d: PHY type detection called.\n", card->index);
    waitfor_notbusy(card);
    writel(NS_CMD_READ_UTILITY | 0x00000100, card->membase + CMD);
    waitfor_notbusy(card);
    data = readl(card->membase + DR0);
    XPRINTK("nicstar%d: DR0 read (0x%08X).\n", card->index, data);
    strcpy(card->name, "unknown_card");

    switch (data) {
#ifdef CONFIG_ATM_NICSTAR_USE_IDT77105
      case 0x00000009:
        printk("nicstar%d: PHY seems to be 25 Mbps.\n", card->index);
        strcpy(card->name, "proatm25");
        card->max_pcr = ATM_25_PCR;
        waitfor_notbusy(card);
        writel(0x00000008, card->membase + DR0);
        writel(NS_CMD_WRITE_UTILITY | 0x00000100, card->membase + CMD);
#ifdef PHY_LOOPBACK
        waitfor_notbusy(card);
        writel(0x00000022, card->membase + DR0);
        writel(NS_CMD_WRITE_UTILITY | 0x00000102, card->membase + CMD);
#endif /* PHY_LOOPBACK */
        break;
#endif /* USE_IDT77105 */
#ifdef CONFIG_ATM_NICSTAR_USE_SUNI
      case 0x00000030:
      case 0x00000031:
        printk("nicstar%d: PHY seems to be 155 Mbps.\n", card->index);
        strcpy(card->name, "proatm155");
        card->max_pcr = ATM_OC3_PCR;
#ifdef PHY_LOOPBACK
        waitfor_notbusy(card);
        writel(0x00000002, card->membase + DR0);
        writel(NS_CMD_WRITE_UTILITY | 0x00000105, card->membase + CMD);
#endif /* PHY_LOOPBACK */
        break;
#endif /* USE_SUNI */
      default:
        printk("nicstar%d: unknown PHY type (0x%08X).\n", card->index, data);
        return 8;
    }
    writel(0x00000000, card->membase + GP);

    /* Find SRAM size */
    data = 0x76543210;
    ns_write_sram(card, 0x1C003, &data, 1);
    data = 0x89ABCDEF;
    ns_write_sram(card, 0x14003, &data, 1);

    if (ns_read_sram(card, 0x14003) == 0x89ABCDEF &&
                    ns_read_sram(card, 0x1C003) == 0x76543210) {
        card->sram_size = 128;
        card->rct_size = 4096;
        card->vcibits = 12 - vpibits;
        card->tst_num = 4095;
        *ns_cfg |= NS_CFG_RCTSIZE_4096_ENTRIES;
    }
    else {
        card->sram_size = 32;
        card->rct_size = 1024;
        card->vcibits = 10 - vpibits;
        card->tst_num = 2047;
        *ns_cfg |= NS_CFG_RCTSIZE_1024_ENTRIES;
    }
    printk("nicstar%d: SRAM size = %dK x 32bit\n", card->index, card->sram_size);

    card->vpibits = vpibits;
    card->maxvpi = (1<<card->vpibits) - 1;
    PRINTK ("VPI bits = %d, VPI mask = 0x%x\n", card->vpibits, card->maxvpi);
    card->maxvci = (1<<card->vcibits) - 1;
    PRINTK ("VCI bits = %d, VCI mask = 0x%x\n", card->vcibits, card->maxvci);

    /* dynamic SRAM map */
    card->rct = NS_TCT_ENTRY_SIZE * card->rct_size;
    card->rate = card->rct + NS_RCT_ENTRY_SIZE * card->rct_size + 2*NS_RFBQ_SIZE;
    card->tst =  card->rate + abr_vbr_rate_tables_len;
    card->dtst = card->tst + card->tst_num + 1;
    card->scd = card->dtst + NS_DTST_SIZE ;
    card->rxfifo = card->sram_size * 1024 - NS_RXFIFO_SIZE;
    card->scd_size = ((card->rxfifo - card->scd)/NS_SCD_ENTRY_SIZE) - 1;
    if (card->scd_size >= 4095)
        card->scd_size = 4095;
    card->scd_ubr0 = card->scd + card->scd_size * NS_SCD_ENTRY_SIZE;

    PRINTK ("nicstar%d: SRAM Map for %d connections:\n",
                                        card->index, card->rct_size);
    PRINTK ("TCT =      0x0\n");
    PRINTK ("RCT =      0x%x\n", card->rct);
    PRINTK ("RATE_TBL = 0x%x\n", card->rate);
    PRINTK ("TST =      0x%x\n", card->tst);
    PRINTK ("DTST =     0x%x\n", card->dtst);
    PRINTK ("SCD =      0x%x\n", card->scd);
    PRINTK ("SCD_UBR0 = 0x%x\n", card->scd_ubr0);
    PRINTK ("RXFIFO =   0x%x\n", card->rxfifo);

    PRINTK ("ns_phys_init: SCD number = %d\n", card->scd_size);

    return 0;
}

static int __devinit ns_init_card(int i, struct pci_dev *pcidev)
{
    int             j;
    struct ns_dev   *card = NULL;
    unsigned char   pci_latency;
    unsigned        error = 0;
    u32             data;
    u32             u32d[4];
    u32             ns_cfg = 0;
    u32             *abr_vbr_rate_tables;
    u32             bcount;
    u32             cfg[9] = { NS_CFG_VPIBITS_0, NS_CFG_VPIBITS_1, NS_CFG_VPIBITS_2,
                                            0,0,0,0,0, NS_CFG_VPIBITS_8};

    XPRINTK("nicstar%d: ns_init_card() called.\n", i);

    if (vpibits!=0 && vpibits!=1 && vpibits!=2 && vpibits!=8) {
        printk ("nicstar%d: incorrect number of vpi bits = %d. Should be 0, 1, 2, or 8.\n", card->index, vpibits);
        return 1;
    }
    ns_cfg = cfg[vpibits];

    if ((card = kmalloc(sizeof (ns_dev), GFP_KERNEL)) == NULL) {
        printk("nicstar%d: can't allocate memory for device structure.\n", i);
        error = 2;
        ns_init_card_error(card, error);
        return error;
    }
    /* clear the ns_dev structure */
    memset (card, 0, sizeof (ns_dev));

    cards[i] = card;
    spin_lock_init(&card->int_lock);
    spin_lock_init(&card->res_lock);

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,0)
    pci_set_drvdata(pcidev, card);
#endif

    card->index = i;
    card->atmdev = NULL;
    card->pcidev = pcidev;
    card->membase = pci_resource_start(pcidev, 1);
    card->membase = (unsigned long) ioremap(card->membase, NS_IOREMAP_SIZE);
    if (card->membase == 0) {
        printk("nicstar%d: can't ioremap() membase.\n", card->index);
        error = 3;
        ns_init_card_error(card, error);
        return error;
    }
    PRINTK("nicstar%d: membase at 0x%x.\n", i, (unsigned int) card->membase);

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,0)
    pci_set_master(pcidev);
#else
    {
        unsigned short  pci_command;

        if (pci_read_config_word(pcidev, PCI_COMMAND, &pci_command) != 0) {
            printk("nicstar%d: can't read PCI_COMMAND.\n", i);
            error = 4;
            ns_init_card_error(card, error);
            return error;
        }
        pci_command |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
        if (pci_write_config_word(pcidev, PCI_COMMAND, pci_command) != 0) {
            printk("nicstar%d: can't write PCI_COMMAND.\n", i);
            error = 5;
            ns_init_card_error(card, error);
            return error;
        }
    }
#endif
    if (pci_read_config_byte(pcidev, PCI_LATENCY_TIMER, &pci_latency) != 0) {
        printk("nicstar%d: can't read PCI latency timer.\n", i);
        error = 6;
        ns_init_card_error(card, error);
        return error;
    }
#ifdef NS_PCI_LATENCY
    if (pci_latency < NS_PCI_LATENCY) {
        PRINTK("nicstar%d: setting PCI latency timer to %d.\n", i, NS_PCI_LATENCY);
        for (j = 1; j < 4; j++)  {
            if (pci_write_config_byte(pcidev, PCI_LATENCY_TIMER, NS_PCI_LATENCY) != 0)
                break;
        }
        if (j == 4) {
            printk("nicstar%d: can't set PCI latency timer to %d.\n",                                                             i, NS_PCI_LATENCY);
            error = 7;
            ns_init_card_error(card, error);
            return error;
        }
    }
#endif /* NS_PCI_LATENCY */

    /* Clear timer overflow */
    data = readl(card->membase + STAT);
    if (data & NS_STAT_TMROF)
        writel(NS_STAT_TMROF, card->membase + STAT);

    /* Software reset */
    writel(NS_CFG_SWRST, card->membase + CFG);
    NS_DELAY;
    writel (0x00000000, card->membase + CFG);
    data = readl(card->membase + GP);
    PRINTK("nicstar%d: GP read (0x%08X).\n", i, data);

    /* Card hardware initialization */
    error = ns_phys_init (card, &ns_cfg);
    if (error) {
        ns_init_card_error(card, error);
        return error;
    }

    writel (ns_cfg, card->membase + CFG);   /* init rct and tct sizes */

    if (request_irq(pcidev->irq, &ns_irq_handler,
                        SA_INTERRUPT | SA_SHIRQ, card->name, card) != 0) {
        printk("nicstar%d: can't allocate IRQ %d for %s.\n",
                        card->index, pcidev->irq, card->name);
        error = 9;
        ns_init_card_error(card, error);
        return error;
    }

    /* Set the VPI/VCI MSB mask (must be zero to receive OAM cells) */
    card->vpm = (vcibase>>card->vcibits) |
                 ((vpibase>>card->vpibits) << (16 - card->vcibits));
    card->vpm &= 0x7fff;
    writel(card->vpm, card->membase + VPM);
    printk ("nicstar%d: vpibase=%d, vcibase=%d, VPM=0x%x\n", i,
                                                vcibase, vpibase, card->vpm);

    /* Initialize TSQ */
    card->tsq.org = kmalloc(NS_TSQSIZE + NS_TSQ_ALIGNMENT, GFP_KERNEL);
    if (card->tsq.org == NULL) {
        printk("nicstar%d: can't allocate TSQ.\n", i);
        error = 10;
        ns_init_card_error(card, error);
        return error;
    }
    card->tsq.base = (ns_tsi *) ALIGN_ADDRESS(card->tsq.org, NS_TSQ_ALIGNMENT);
    card->tsq.next = card->tsq.base;
    for (j = 0; j < NS_TSQ_NUM_ENTRIES; j++)
             ns_tsi_init(card->tsq.base + j);

    writel(0x00000000, card->membase + TSQH);
    writel((u32) virt_to_bus(card->tsq.base), card->membase + TSQB);

    /* Initialize RSQ */
    card->rsq.org = kmalloc(NS_RSQSIZE + NS_RSQ_ALIGNMENT, GFP_KERNEL);
    if (card->rsq.org == NULL) {
        printk("nicstar%d: can't allocate RSQ.\n", i);
        error = 11;
        ns_init_card_error(card, error);
        return error;
    }
    card->rsq.base = (ns_rsqe *) ALIGN_ADDRESS(card->rsq.org, NS_RSQ_ALIGNMENT);
    card->rsq.next = card->rsq.base;
    card->rsq.last = card->rsq.base + (NS_RSQ_NUM_ENTRIES - 1);
    for (j = 0; j < NS_RSQ_NUM_ENTRIES; j++)
             ns_rsqe_init(card->rsq.base + j);

    writel(0x00000000, card->membase + RSQH);
    writel((u32) virt_to_bus(card->rsq.base), card->membase + RSQB);

    /* Initialize TCT */
    u32d[0] = 0x00000000;
    u32d[1] = 0x00000000;
    u32d[2] = 0x00000000;
    u32d[3] = 0x00000000;
    for (j = 0; j < (2 *card->rct_size); j++)
             ns_write_sram(card, j *4, u32d, 4);

    /* Initialize RCT. AAL type is set on opening the VC. */
#ifdef NS_RCQ_SUPPORT
    u32d[0] = NS_RCTE_RAWCELLINTEN;
#else
    u32d[0] = 0x00000000;
#endif
    u32d[1] = 0x00000000;
    u32d[2] = 0x00000000;
    u32d[3] = 0xFFFFFFFF;
    for (j = 0; j < card->rct_size; j++)
             ns_write_sram(card, card->rct + (j *4), u32d, 4);

    /* Initialize RAWHND */
    card->raw_hnd[0] = 0;
    writel((u32) virt_to_bus (card->raw_hnd), card->membase + RAWHND);

    /* Initialize the vc map */
    if ((card->vcmap = kmalloc(card->rct_size *sizeof (vc_map),
                                            GFP_KERNEL)) == NULL) {
        printk("nicstar%d: can't allocate memory for VCs map.\n", i);
        error = 12;
        ns_init_card_error(card, error);
        return error;
    }

    memset(card->vcmap, 0, card->rct_size *sizeof (vc_map));
    for (j = 0; j<card->rct_size; j++)
        card->vcmap[j].conn = j;     /* initialize the connection number */

    for (j = 0; j < NS_SCD_NUM; j++)
             card->scd2vc[j] = NULL;


    /* Initialize ABRVBR TABLES */
    writel(NS_ABRVBRSIZE | (card->dtst<<2), card->membase + ABRSTD);
    for (j = 0; j < NS_DTST_SIZE; j++) {
        ns_write_sram(card, card->dtst + j, u32d, 1);
    }

    /* Initialize the Rate Table */
    if (card->max_pcr == ATM_25_PCR)
        abr_vbr_rate_tables = abr_vbr_rate_tables_25Mb;
    else
        abr_vbr_rate_tables = abr_vbr_rate_tables_155Mb;

    for (j = 0; j < abr_vbr_rate_tables_len; j++) {
        ns_write_sram(card, (card->rate + j), &abr_vbr_rate_tables[j], 1);
    }
    writel(card->rate << 2, card->membase + RTBL);

    /* Initialize the TST (use only one) */
    card->tst_free_entries = card->tst_num;
    data = NS_TST_OPCODE_VARIABLE;
    for (j = 0; j < card->tst_num; j++)
             ns_write_sram(card, card->tst + j, &data, 1);

    data = ns_tste_make(NS_TST_OPCODE_END, card->tst<<2);
    ns_write_sram(card, card->tst + card->tst_num, &data, 1);

    writel(card->tst << 2, card->membase + TSTB);

    /* Initialize SCQ0, the UBR SCQ for unspecified PCR */
    card->scq0 = get_scq(UBR_SCQ, card->scd_ubr0);
    if (card->scq0 == (scq_info *) NULL) {
        printk("nicstar%d: can't get SCQ0.\n", i);
        error = 13;
        ns_init_card_error(card, error);
        return error;
    }
    u32d[0] = (u32) virt_to_bus(card->scq0->base);
    u32d[1] = (u32) 0x00000000;
    u32d[2] = (u32) 0xffffffff;
    u32d[3] = (u32) 0x00000000;
    ns_write_sram(card, card->scd_ubr0, u32d, 4);
    for (j = 1; j < 3; j++) {
        u32d[0] = (u32)0x00000000;
        u32d[1] = (u32)0x00000000;
        u32d[2] = (u32)0x00000000;
        u32d[3] = (u32)0x00000000;
        ns_write_sram(card, card->scd_ubr0 + 4 *j, u32d, 4);
    }
    card->scq0->scd = card->scd_ubr0;
    PRINTK("nicstar%d: SCQ0 base at 0x%x.\n", i, (u32) card->scq0->base);

    /* Initialize RxFIFO */
    writel((card->rxfifo << 2) | NS_RXFD_SIZE, card->membase + RXFD);

    /* Initialize free buffer queues registers #34-#45 */
    writel(NS_FBQ0_WP | NS_FBQ0_RP, card->membase + FBQP0);
    writel(NS_FBQ1_WP | NS_FBQ0_RP, card->membase + FBQP1);
    writel(NS_FBQ2_WP | NS_FBQ0_RP, card->membase + FBQP2);
    writel(NS_FBQ3_WP | NS_FBQ0_RP, card->membase + FBQP3);
    writel(NS_B0THLD | NS_B0NITHLD | NS_B0CITHLD | NS_B0SIZE,
                                                    card->membase + FBQS0);
    writel(NS_B1THLD | NS_B1NITHLD | NS_B1CITHLD | NS_B1SIZE,
                                                    card->membase + FBQS1);
    writel(NS_B2THLD | NS_B2NITHLD | NS_B2CITHLD, card->membase + FBQS2);
    writel(NS_B3THLD | NS_B3NITHLD | NS_B3CITHLD, card->membase + FBQS3);

    /* Initialize buffer levels */
    card->b0nr.min = MIN_B0;
    card->b0nr.init = NUM_B0;
    card->b0nr.max = MAX_B0;
    card->b1nr.min = MIN_B1;
    card->b1nr.init = NUM_B1;
    card->b1nr.max = MAX_B1;
    card->iovnr.min = MIN_IOVB;
    card->iovnr.init = NUM_IOVB;
    card->iovnr.max = MAX_IOVB;
    card->hbnr.min = MIN_HB;
    card->hbnr.init = NUM_HB;
    card->hbnr.max = MAX_HB;
    card->b0_handle = 0x00000000;
    card->b0_addr = 0x00000000;
    card->b1_handle = 0x00000000;
    card->b1_addr = 0x00000000;
    card->fbie = 1; /* To prevent push_rxbufs from enabling the interrupt */

    /* Pre-allocate some huge buffers */
    PRINTK("nicstar%d: pre-allocate huge buffers.\n", i);
    skb_queue_head_init(&card->hbpool.queue);
    card->hbpool.count = 0;
    for (j = 0; j < NUM_HB; j++) {
        struct sk_buff  *hb;

        hb = alloc_skb(NS_HBUFSIZE, GFP_KERNEL);
        if (hb == NULL) {
            printk("nicstar%d: can't allocate %dth of %d huge buffers.\n",
                   i, j, NUM_HB);
            break;
        }
        skb_queue_tail(&card->hbpool.queue, hb);
        card->hbpool.count++;
    }

    /* Allocate large buffers : b1 */
    skb_queue_head_init(&card->b1pool.queue);
    card->b1pool.count = 0;                           /* Not used */
    for (j = 0; j < NUM_B1; j++) {
        struct sk_buff  *lb;

        lb = alloc_skb(NS_B1SKBSIZE, GFP_KERNEL);
        if (lb == NULL) {
            printk("nicstar%d: can't allocate %dth of %d large buffers.\n",
                   i, j, NUM_B1);
            break;
        }
        skb_queue_tail(&card->b1pool.queue, lb);
        skb_reserve(lb, NS_B0BUFSIZE);
        push_rxbufs(card, BUF_B1, (u32) lb, (u32) virt_to_bus(lb->data), 0, 0);
        /* Due to the implementation of push_rxbufs() this is 1, not 0 */
        if (j == 1) {
            card->rcbuf = lb;
            card->raw_ch = (ns_rcqe*)lb->data;
        }
    }

    /* Verify that push worked well */
    if ((bcount = ns_fbqc_get(readl(card->membase + FBQP1))) < card->b1nr.min) {
        printk("nicstar%d: Error: allocate %d large buffers but lfbqc = %d only.\n",
               i, j, bcount);
        error = 16;
        ns_init_card_error(card, error);
        return error;
    }

    /* Allocate small buffers : b0 */
    skb_queue_head_init(&card->b0pool.queue);
    card->b0pool.count = 0;                           /* Not used */
    for (j = 0; j < NUM_B0; j++) {
        struct sk_buff  *sb;

        sb = alloc_skb(NS_B0SKBSIZE, GFP_KERNEL);
        if (sb == NULL) {
            printk("nicstar%d: can't allocate %dth of %d small buffers.\n",
                   i, j, NUM_B0);
            break;
        }
        skb_queue_tail(&card->b0pool.queue, sb);
        skb_reserve(sb, NS_AAL0_HEADER);
        push_rxbufs(card, BUF_B0, (u32) sb, (u32) virt_to_bus(sb->data), 0, 0);
    }
    /* Verify that push worked well */
    if ((bcount = ns_fbqc_get(readl(card->membase + FBQP0))) < card->b0nr.min) {
        printk("nicstar%d: Error, Allocate %d small buffers but sfbqc = %d only.\n",
               i, j, bcount);
        error = 17;
        ns_init_card_error(card, error);
        return error;
    }
    /* Allocate iovec buffers */
    skb_queue_head_init(&card->iovpool.queue);
    card->iovpool.count = 0;
    for (j = 0; j < NUM_IOVB; j++) {
        struct sk_buff  *iovb;

        iovb = alloc_skb(NS_IOVBUFSIZE, GFP_KERNEL);
        if (iovb == NULL) {
            printk("nicstar%d: can't allocate %dth of %d iovec buffers.\n",
                   i, j, NUM_IOVB);
            error = 17;
            ns_init_card_error(card, error);
            return error;
        }
        skb_queue_tail(&card->iovpool.queue, iovb);
        card->iovpool.count++;
    }
    card->intcnt = 0;
    card->fbie = 1;

    /* Register device */
    card->atmdev = atm_dev_register(card->name, &atm_ops, - 1, NULL);
    if (card->atmdev == NULL) {
        printk("nicstar%d: can't register device %s\n", i, card->name);
        error = 18;
        ns_init_card_error(card, error);
        return error;
    }
    /* Read EEPROM */
    if (ns_parse_mac (mac[i], card->atmdev->esi)) {
        unsigned char   *esi = card->atmdev->esi;

        ns_eeprom_rd (card, NICSTAR_EPROM_PROSUM_MAC_ADDR_OFFSET, esi, 6);
        if (esi[0] != PROSUM_MAC_0 || esi[1] != PROSUM_MAC_1 || esi[2] != PROSUM_MAC_2)
            ns_eeprom_rd (card, NICSTAR_EPROM_IDT_MAC_ADDR_OFFSET, esi, 6);

        printk("nicstar%d: MAC address %02X:%02X:%02X:%02X:%02X:%02X\n", i,
               esi[0], esi[1], esi[2], esi[3], esi[4], esi[5]);
    }
    card->atmdev->dev_data = card;
    card->atmdev->ci_range.vpi_bits = 8;
    card->atmdev->ci_range.vci_bits = 16;
    card->atmdev->link_rate = card->max_pcr;
    card->atmdev->phy = NULL;

#ifdef CONFIG_ATM_NICSTAR_USE_SUNI
    if (card->max_pcr == ATM_OC3_PCR) {
        suni_init(card->atmdev);
    }
#endif

#ifdef CONFIG_ATM_NICSTAR_USE_IDT77105
    if (card->max_pcr == ATM_25_PCR) {
        idt77105_init(card->atmdev);
    }
#endif

    if (card->atmdev->phy && card->atmdev->phy->start)
        card->atmdev->phy->start(card->atmdev);

    writel(NS_DEFAULT_CFG | ns_cfg, card->membase + CFG);

    PRINTK("nicstar%d: card->vpibits= %d.\n", i, card->vpibits);
    PRINTK("nicstar%d: card->vcibits= %d.\n", i, card->vcibits);
    num_cards++;
    return error;
}

static void __init ns_init_card_error(ns_dev *card, int error)
{
    if (error >= 19) {
        writel(0x00000000, card->membase + CFG);
    }
    if (error >= 18) {
        struct sk_buff  *iovb;

        while ((iovb = skb_dequeue(&card->iovpool.queue)) != NULL)
                       dev_kfree_skb_any(iovb);

    }
    if (error >= 17) {
        struct sk_buff  *sb;

        while ((sb = skb_dequeue(&card->b0pool.queue)) != NULL)
                     dev_kfree_skb_any(sb);

        free_scq(card, card->scq0, NULL);
    }
    if (error >= 16) {
        struct sk_buff  *lb;

        while ((lb = skb_dequeue(&card->b1pool.queue)) != NULL)
                     dev_kfree_skb_any(lb);
    }
    if (error >= 14) {
        struct sk_buff  *hb;
        while ((hb = skb_dequeue(&card->hbpool.queue)) != NULL)
                     dev_kfree_skb_any(hb);
    }
    if (error >= 13) {
        kfree(card->vcmap);
    }
    if (error >= 12) {
        kfree(card->rsq.org);
    }
    if (error >= 11) {
        kfree(card->tsq.org);
    }
    if (error >= 10) {
        free_irq(card->pcidev->irq, card);
    }
    if (error >= 4) {
        iounmap((void *) card->membase);
    }
    if (error >= 3) {

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,0)
    pci_disable_device(card->pcidev);
#endif
        kfree(card);
    }
}

static scq_info *get_scq(int type, u32 scd)
{
    scq_info        *scq;
    int             i;

    scq = (scq_info *) kmalloc(sizeof (scq_info), GFP_KERNEL);
    if (scq == (scq_info *) NULL)
        return (scq_info *) NULL;
                           /* reserve twice the size for alignment purpose*/
    scq->org = kmalloc(2 * SCQ_SIZE, GFP_KERNEL);
        if (scq->org == NULL) {
        kfree(scq);
        return (scq_info *) NULL;
    }                      /* reserve a table of skbuf pointers */
    scq->skb = (struct sk_buff * *) kmalloc(sizeof (struct sk_buff *) * SCQ_NUM_ENTRIES
                                            , GFP_KERNEL);
    if (scq->skb == NULL) {
        kfree(scq->org);
        kfree(scq);
        return (scq_info *) NULL;
    }
    scq->scq_type = type;
    scq->base = (ns_scqe *) ALIGN_ADDRESS(scq->org, SCQ_SIZE);
    scq->next = scq->base;
    scq->last = scq->base + SCQ_NUM_ENTRIES - 1;
    scq->tail = scq->last;
    scq->scd = scd;
    scq->tbd_count = 0;
    skb_queue_head_init(&scq->waiting);
    spin_lock_init(&scq->lock);
    for (i = 0; i < SCQ_NUM_ENTRIES; i++)
             scq->skb[i] = NULL;

    return scq;
}

/* For variable rate SCQ vcc must be NULL */
static void free_scq(ns_dev *card, scq_info *scq, struct atm_vcc *vcc)
{
    int             i;
    struct sk_buff  *skb;

	if (scq==NULL) {
		printk ("nicstar%d: try to free a null SCQ\n", card->index);
		return;
	}

    while ((skb = skb_dequeue(&scq->waiting)))
 		dev_kfree_skb_any (skb);

    if (scq == card->scq0) {
        for (i = 0; i < SCQ_NUM_ENTRIES; i++)
        {
            if (scq->skb[i] != NULL) {
                vcc = ATM_SKB(scq->skb[i])->vcc;
                if (vcc->pop != NULL)
                    vcc->pop(vcc, scq->skb[i]);
                else
                    dev_kfree_skb_any(scq->skb[i]);

            }
        }
    } else {
        /* vcc must be != NULL */
        if (vcc == NULL) {
            printk("nicstar%d: free_scq() called with vcc == NULL.", card->index);
            for (i = 0; i < SCQ_NUM_ENTRIES; i++)
                        dev_kfree_skb_any(scq->skb[i]);
        } else {
            for (i = 0; i < SCQ_NUM_ENTRIES; i++)
            {
                if (scq->skb[i] != NULL) {
                    if (vcc->pop != NULL)
                        vcc->pop(vcc, scq->skb[i]);
                    else
                        dev_kfree_skb_any(scq->skb[i]);

                }
            }
        }
    }

    kfree(scq->skb);
    kfree(scq->org);
    kfree(scq);
}


/* The handles passed must be pointers to the sk_buff containing the small
   or large buffer(s) cast to u32. */
static void push_rxbufs(ns_dev *card, u32 type, u32 handle1, u32 addr1,
                        u32 handle2, u32 addr2)
{
    u32             fbqp0;
    u32             fbqp1;
    unsigned long   flags;

#ifdef GENERAL_DEBUG
    if (!addr1)
        printk("nicstar%d: push_rxbufs called with addr1 = 0.\n", card->index);
#endif /* GENERAL_DEBUG */

    fbqp0 = readl(card->membase + FBQP0);
    fbqp1 = readl(card->membase + FBQP1);
    card->sbfqc = ns_fbqc_get(fbqp0);
    card->lbfqc = ns_fbqc_get(fbqp1);
    if (type == BUF_B0) {
        if (!addr2) {
            if (card->b0_addr) {
                addr2 = card->b0_addr;
                handle2 = card->b0_handle;
                card->b0_addr = 0x00000000;
                card->b0_handle = 0x00000000;
            } else {
            /* (!b0_addr) */
                card->b0_addr = addr1;
                card->b0_handle = handle1;
            }
        }
    } else {
        /* type == BUF_B1 */
        if (!addr2) {
            if (card->b1_addr) {
                addr2 = card->b1_addr;
                handle2 = card->b1_handle;
                card->b1_addr = 0x00000000;
                card->b1_handle = 0x00000000;
            } else {
            /* (!b1_addr) */
                card->b1_addr = addr1;
                card->b1_handle = handle1;
            }
        }
    }
    if (addr2) {
        if (type == BUF_B0) {
            if (card->sbfqc >= card->b0nr.max) {
                PRINTK("nicstar%d: freing small buffers.\n", card->index);
                XPRINTK ("handle1 = 0x%x, handle2 = Ox%x", handle1, handle2);
                skb_unlink((struct sk_buff *) handle1);
                dev_kfree_skb_any((struct sk_buff *) handle1);
                skb_unlink((struct sk_buff *) handle2);
                dev_kfree_skb_any((struct sk_buff *) handle2);
                return;
            }
            else
                card->sbfqc += 2;

        } else {
            /* (type == BUF_B1) */
            if (card->lbfqc >= card->b1nr.max) {
                PRINTK("nicstar%d: freing large buffers.\n", card->index);
                XPRINTK ("handle1 = 0x%x, handle2 = Ox%x", handle1, handle2);
                skb_unlink((struct sk_buff *) handle1);
                dev_kfree_skb_any((struct sk_buff *) handle1);
                skb_unlink((struct sk_buff *) handle2);
                dev_kfree_skb_any((struct sk_buff *) handle2);
                return;
            }
            else
                card->lbfqc += 2;
        }
        ns_grab_res_lock(card, flags);
        PRINTK("nicstar%d: Pushing %s buffers at 0x%x and 0x%x.\n", card->index,
               (type == BUF_B0 ? "small" : "large"), addr1, addr2);
        waitfor_notbusy(card);
        writel(addr2, card->membase + DR3);
        writel(handle2, card->membase + DR2);
        writel(addr1, card->membase + DR1);
        writel(handle1, card->membase + DR0);
        writel(NS_CMD_WRITE_FREEBUFQ | (u32) type, card->membase + CMD);
        spin_unlock_irqrestore(&card->res_lock, flags);
    }
    if (!card->fbie && card->sbfqc >= card->b0nr.min &&
        card->lbfqc >= card->b1nr.min) {
        card->fbie = 1;
        writel((readl(card->membase + CFG) | NS_CFG_FBIE), card->membase + CFG);
    }
    return;
}
#define INT_FLAGS ( NS_STAT_TSIF  | \
                    NS_STAT_TXICP | \
                    NS_STAT_TSQF  | \
                    NS_STAT_TMROF | \
                    NS_STAT_PHYI  | \
                    NS_STAT_RSQF  | \
                    NS_STAT_EOPDU | \
                    NS_STAT_RAWCF | \
                    NS_STAT_FBQ0A | \
                    NS_STAT_FBQ1A | \
                    NS_STAT_RSQAF)

#define  CLEAR_FLAGS ( NS_STAT_TSIF | NS_STAT_TXICP | NS_STAT_TMROF \
                        | NS_STAT_PHYI | NS_STAT_EOPDU | NS_STAT_RAWCF)

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,0)
static irqreturn_t ns_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
#else
static void ns_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
#endif
{
    u32             stat_r;
    ns_dev          *card;
    struct atm_dev  *dev;
    unsigned long   flags;
    volatile unsigned int i = 0 ;

    card = (ns_dev *) dev_id;
    dev = card->atmdev;
    card->intcnt++;
    ns_grab_int_lock(card, flags);
    stat_r = readl(card->membase + STAT);

	PRINTK("nicstar%d: Entering ns_irq_handler, stat= 0x%x\n", card->index, stat_r);

    while (stat_r & INT_FLAGS) {
        if ( i++ > 100) {
            break;    /* avoid infinite loop */
        }

        writel(CLEAR_FLAGS, card->membase + STAT);   /* clear the interrupt */

        /* Transmit Status Indicator has been written to T. S. Queue */
        if (stat_r & NS_STAT_TSIF) {
            PRINTK("nicstar%d: TSI interrupt\n", card->index);
            process_tsq(card);
        }
        /* Transmit Status Queue 7/8 full */
        if (stat_r & NS_STAT_TSQF) {
            PRINTK("nicstar%d: TSQ full.\n", card->index);
            process_tsq(card);
        }
        /* PHY device interrupt signal active */
        if (stat_r & NS_STAT_PHYI) {
            PRINTK("nicstar%d: PHY interrupt.\n", card->index);
            if (dev->phy && dev->phy->interrupt) {
                dev->phy->interrupt(dev);
            }
        }
        /* Receive Status Queue is full */
        if (stat_r & NS_STAT_RSQF) {
            printk("nicstar%d: RSQ full.\n", card->index);
            process_rsq(card);
        }
        /* Complete CS-PDU received */
        if (stat_r & NS_STAT_EOPDU) {
            PRINTK("nicstar%d: End of CS-PDU received.\n", card->index);
            process_rsq(card);
        }
        /* Raw cell received */
        if (stat_r & NS_STAT_RAWCF) {
            PRINTK("nicstar%d: Raw cell received\n", card->index);
            ns_rawc (card);
        }

        /* Small buffer queue attention */
        /* Happens only when too much traffic. try to allocate more buffers */
        if (stat_r & NS_STAT_FBQ0A) {
            int             i;
            struct sk_buff  *sb;
            u32             fbqp0;

            PRINTK("nicstar%d: free buffer queue 0 attention.\n",
                   card->index);
            for (i = 0; i < card->b0nr.min; i++)
            {
                sb = alloc_skb(NS_B0SKBSIZE, GFP_ATOMIC);
                if (sb == NULL) {
                    /* if no more buffers available. Disable Q0A interrupt and give up */
                    /* this interrupt will be enabled again when buffers are recycled */
                    writel(readl(card->membase + CFG)& ~NS_CFG_FBIE, card->membase + CFG);
                    card->fbie = 0;
                    break;
                }
                skb_queue_tail(&card->b0pool.queue, sb);
                skb_reserve(sb, NS_AAL0_HEADER);
                push_rxbufs(card, BUF_B0, (u32) sb, (u32) virt_to_bus(sb->data), 0, 0);
            }
            /*card->sbfqc = i;*/
            fbqp0 = readl(card->membase + FBQP0);
            card->sbfqc = ns_fbqc_get(fbqp0);
            process_rsq(card);
        }

        /* Large buffer queue attention */
        /* Happens only when too much traffic. try to allocate more buffers */
        if (stat_r & NS_STAT_FBQ1A) {
            int             i;
            struct sk_buff  *lb;
            u32             fbqp1;

            PRINTK("nicstar%d: free buffer queue 1 attention.\n",
                   card->index);
            for (i = 0; i < card->b1nr.min; i++)
            {
                lb = alloc_skb(NS_B1SKBSIZE, GFP_ATOMIC);
                if (lb == NULL) {
                    /* if no more buffers available. Disable Q0A interrupt and give up */
                    /* this interrupt will be enabled again when buffers are recycled */
                    writel(readl(card->membase + CFG)& ~NS_CFG_FBIE, card->membase + CFG);
                    card->fbie = 0;
                    break;
                }
                skb_queue_tail(&card->b1pool.queue, lb);
                skb_reserve(lb, NS_B0BUFSIZE);
                push_rxbufs(card, BUF_B1, (u32) lb, (u32) virt_to_bus(lb->data), 0, 0);
            }
            /* card->lbfqc = i;*/
            fbqp1 = readl(card->membase + FBQP1);
            card->lbfqc = ns_fbqc_get(fbqp1);
            process_rsq(card);
        }

        /* Receive Status Queue is 7/8 full */
        if (stat_r & NS_STAT_RSQAF) {
            PRINTK("nicstar%d: RSQ 7/8 full.\n", card->index);
            process_rsq(card);
        }

        /* Incomplete CS-PDU has been transmitted */
        if (stat_r & NS_STAT_TXICP) {
            printk("nicstar%d: Incomplete CS-PDU transmitted.\n", card->index);
        }
        /* Timer overflow */
        if (stat_r & NS_STAT_TMROF) {
            PRINTK("nicstar%d: Timer overflow.\n", card->index);
        }

        stat_r = readl(card->membase + STAT);
    }
    if (i > 100)
        printk ("nicstar%d: long interrupt loop, status reg= 0x%x\n", card->index, stat_r);

    PRINTK("nicstar%d: end of interrupt service\n", card->index);
    spin_unlock_irqrestore(&card->int_lock, flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,0)
    return IRQ_HANDLED;
#endif
}

/* raw cell process */
/* Here you can do something with raw cell data */
static void ns_process_rcqe (ns_dev *card, ns_rcqe *rawcell)
{
	//PRINTK("nicstar%d: Nothing to do with raw cell\n", card->index);

	struct sk_buff *skb;
	int status=0;

	PRINTK("nicstar [%s]: OAM cell recieved\n", __FUNCTION__);
	XPRINTK("nicstar [%s]: upid:%d \n", __FUNCTION__, oam_upid);
	if(!oam_upid){
		printk("nicstar [%s]: Error, start OAM user daemon first\n", __FUNCTION__);
		return;
	}
	if(!find_task_by_pid(oam_upid)){
		printk("nicstar [%s]: Error, restart OAM user daemon\n", __FUNCTION__);
		return;
	}

	PRINTK("nicstar [%s]: Rxed cell from remote end\n", __FUNCTION__);

	skb = alloc_skb(NS_RCQE_SIZE , GFP_ATOMIC);

	skb->len = NS_RCQE_SIZE;
	memcpy(skb->data, (u8 *)rawcell, NS_RCQE_SIZE);
	skb_put(skb, NS_RCQE_SIZE);

	status = netlink_unicast(oam_ksock, skb, oam_upid, MSG_DONTWAIT);
	if (status <0){
		printk("nicstar [%s]: koam send fail\n", __FUNCTION__);
		kfree_skb(skb);
	}
	return;
}

/* raw cell receipt */
static void ns_rawc(ns_dev *card)
{
    ns_rcqe      *rawcell;
    u32          tail;

    if (card->raw_hnd[0] == 0)
        return;

    /*
     * Mars - fix endian problem on nicstar driver for MALTA BE
     * 		caused baffling crashes for OAM
     */
    tail = (le32_to_cpu(card->raw_hnd[0])) + NS_RCQE_SIZE;

    while (virt_to_bus(card->raw_ch) != tail) {
        rawcell = card->raw_ch;
        if (ns_rcqe_islast(rawcell)) {
            struct sk_buff  *oldbuf;
            oldbuf = card->rcbuf;
            card->rcbuf = (struct sk_buff *) ns_rcqe_nextbufhandle(rawcell);
            card->raw_ch = (ns_rcqe*)(card->rcbuf->data);
            recycle_rx_buf(card, oldbuf);
        }
        else {
            /*
             * Mars - fix endian problem of ATM header before further processing.
             */
            rawcell->word_1 = le32_to_cpu(rawcell->word_1);
            ns_process_rcqe (card, rawcell);
            card->raw_ch ++;
        }
    }
}

/* Compute (scr<<16)/pcr */
static u16 scr_on_pcr(u32 scr, u32 pcr)
{
    u32 q = 0;

    scr <<= 8;
    q = scr/pcr;
    scr = (scr % pcr) << 8;
    return  (q << 8) | (scr/pcr) ;
}

/* translate rate to ATM Forum rate
 * 2^e (1+m/512) = rate   => m = rate * 2^(9-e) - 512
 */
static u16 u32_to_afr(u32 rate)
{
	u16 m, e;
	u32 mask;

	if (rate == 0) return 0;
	for (mask = 0x80000000, e = 31; mask; mask >>= 1, e--)
		if (rate & mask) break;
    m = e >= 9 ? rate >> (e-9): rate << (9-e);
    m -= 512;

	return (1<<14 | e<<9 | m);
}
/*
 * First transform rate to ATM fixed point format (afr)
 * Then find the index corresponding to afr into log_to_conv table
 */
static u8 ns_rate2log (ns_dev *card, u32 rate)
{
	u16 afr_rate = u32_to_afr(rate), afr_value;
	u32 data;
	u8 lower =0, upper =255, mid;
    u32 * rate_table;

	PRINTK ("nicstar: afr_rate= 0x%x\n", afr_rate);
    if(card->max_pcr == ATM_OC3_PCR) {
		rate_table = abr_vbr_rate_tables_155Mb;
	}
	else {
		rate_table = abr_vbr_rate_tables_25Mb;
	}

    /*
     * Could use the rate2log table but this content
     * research gives better precision
     */
	while (1) {
		mid = (u8) ((lower + upper) >> 1);
		data = rate_table [mid];
		afr_value = (u16) (data >> 17);  /* extract cps from table */
		if ((afr_rate == afr_value) || (upper <= lower))
			break;
        else if (afr_rate > afr_value)
			lower = mid + 1;
		else
			upper = mid - 1;
	}
	if (afr_value > afr_rate)
		mid--;

	return (mid);
}

static int ns_init_tx_cbr (ns_dev *card, vc_map *vc, struct atm_vcc *vcc, int conn)
{
    scq_info        *scq;
    int             j;
    u32             u32d[8];
    int             tcr, tcra;      /* target cell rate, and absolute value */
    int             n = 0;          /* Number of entries in the TST */
    u32             tmpl, modl;
    int             scdi = 0;     /* SCD Index */

	PRINTK ("pcr= 0x%x, max_pcr= 0x%x, min_pcr= 0x%x\n",
			vcc->qos.txtp.pcr, vcc->qos.txtp.max_pcr, vcc->qos.txtp.min_pcr);

    /* Check requested cell rate and availability of SCD */
    if (vcc->qos.txtp.max_pcr == 0 && vcc->qos.txtp.pcr == 0 &&
        vcc->qos.txtp.min_pcr == 0) {
        printk("nicstar%d: trying to open a CBR vc with cell rate = 0 \n",
                card->index);
        return - EINVAL;
    }
    /* Cell rate computation */
    tcr = atm_pcr_goal(& (vcc->qos.txtp));
    tcra = tcr >= 0 ? tcr : - tcr;
    PRINTK("nicstar%d: target cell rate = %d.\n", card->index, tcr);
    tmpl = (unsigned long) tcra * (unsigned long) card->tst_num;
    modl = tmpl % card->max_pcr;
    n = (int)(tmpl / card->max_pcr);
    if (tcr > 0) {
        if (modl > 0)
            n++;
    }
    else if (tcr == 0) {
        if ((n = (card->tst_free_entries - NS_TST_RESERVED)) <= 0) {
            printk("nicstar%d: no CBR bandwidth free.\n", card->index);
            return - EINVAL;
        }
    }
    if (n == 0) {
        printk ("nicstar%d: selected bandwidth < granularity.\n", card->index);
        return - EINVAL;
    }
    if (n > (card->tst_free_entries - NS_TST_RESERVED)) {
        printk ("nicstar%d: not enough free CBR bandwidth.\n", card->index);
        return - EINVAL;
    }
    else
        card->tst_free_entries -= n;

    PRINTK("nicstar%d: writing %d tst entries.\n", card->index, n);

    /* get SCD */
    for (scdi = 0; scdi < NS_SCD_NUM; scdi++)
    {
        if (card->scd2vc[scdi] == NULL) {
            card->scd2vc[scdi] = vc;
            break;
        }
    }
    if (scdi == NS_SCD_NUM) {
        printk("nicstar%d: no SCD available for CBR channel.\n", card->index);
        card->tst_free_entries += n;
        return - EBUSY;
    }
    vc->adr_scd = card->scd + scdi * NS_SCD_ENTRY_SIZE;

    /* get SCQ */
    scq = get_scq(CBR_SCQ, vc->adr_scd);
    if (scq == (scq_info *) NULL) {
        printk("nicstar%d: can't get Fixed Rate SCQ.\n", card->index);
        card->scd2vc[scdi] = NULL;
        card->tst_free_entries += n;
        return - ENOMEM;
    }
    vc->scq = scq;

    /* set up SCD */
    u32d[0] = (u32)virt_to_bus (scq->base);
    u32d[1] = (u32)0x00000000;
    u32d[2] = (u32)0xffffffff;
    u32d[3] = (u32)0x00000000;
    ns_write_sram(card, vc->adr_scd, u32d, 4);
    for (j = 1; j < 3; j++)                   /* initialize the cache */
    {
        u32d[0] = (u32)0x00000000;
        u32d[1] = (u32)0x00000000;
        u32d[2] = (u32)0x00000000;
        u32d[3] = (u32)0x00000000;
        ns_write_sram(card, vc->adr_scd + 4 *j, u32d, 4);
    }
    PRINTK("nicstar%d: ns_open (), CBR scd written at:0x%x\n", card->index, vc->adr_scd);

    /* set up TCT */
    u32d[0] = NS_TCTE_TYPE_CBR | vc->adr_scd;
    u32d[1] = 0x00000000;
    u32d[2] = 0x00000000;
    u32d[3] = 0x00000000;
    ns_write_sram(card, (conn * 8), u32d, 4);
    u32d[4] = 0x00000000;
    u32d[5] = 0x00000000;
    u32d[6] = 0x00000000;
    u32d[7] = 0x00000000;
    ns_write_sram(card, (conn * 8) + 4, u32d+4, 4);

    PRINTK("nicstar: a (type CBR)TCT entry written: word_1= 0x%x.\n", u32d[0]);

    /* set up TST */
    fill_tst(card, n, vc, conn);

    return 0;
}

static int ns_init_tx_ubr (ns_dev *card, vc_map *vc, struct atm_vcc *vcc, int conn)
{
    scq_info        *scq;
    int             j;
    u32             u32d[8];
    u32             max_pcr = 0;
    int             scdi = 0;     /* SCD Index */
    int             fubr0 = 0;

	PRINTK ("pcr= 0x%x, max_pcr= 0x%x\n", vcc->qos.txtp.pcr, vcc->qos.txtp.max_pcr);

    if (vcc->qos.txtp.pcr == 0 ) {
        PRINTK("nicstar%d: trying to open a UBR vc (unspecified PCR)\n", card->index);
		vc->init_er = 0xff;
		vc->lacr = 0xff;
        fubr0 = 1;
    } else {

	    max_pcr = vcc->qos.txtp.max_pcr;
	    if ( max_pcr <  vcc->qos.txtp.pcr)
	        max_pcr =  vcc->qos.txtp.pcr;

	  	vc->init_er 	= ns_rate2log (card, max_pcr);
		vc->lacr    	= ns_rate2log (card, vcc->qos.txtp.pcr);

		PRINTK ("max_pcr= 0x%x, init_er= 0x%x, lacr= 0x%x\n",
                                        max_pcr, vc->init_er, vc->lacr);
		if (vc->init_er == 0xff && vc->lacr == 0xff)
			fubr0 = 1;
    }
	if (fubr0) {
		vc->adr_scd = card->scd_ubr0;
		vc->scq = card->scq0;	 // no need to update
	}
	else {

	    	/* UBR with specific pcr value. We use a specific SCD */

	    /* get SCD */
	    for (scdi = 0; scdi < NS_SCD_NUM; scdi++) {
	        if (card->scd2vc[scdi] == NULL) {
	            card->scd2vc[scdi] = vc;
	            break;
	        }
	    }
	    if (scdi == NS_SCD_NUM) {
	        printk ("nicstar%d: no SCD available for UBR channel.\n", card->index);
	        return - EBUSY;
	    }
	    vc->adr_scd = card->scd + scdi * NS_SCD_ENTRY_SIZE;
	    scq = get_scq (UBR_SCQ, vc->adr_scd);
	    if (scq == (scq_info *) NULL) {
	        printk ("nicstar%d: can't get UBR SCQ.\n", card->index);
	        card->scd2vc[scdi] = NULL;
	        return - ENOMEM;
	    }
	    vc->scq = scq;

	    /* set up SCD */
	    u32d[0] = (u32) virt_to_bus(scq->base);
	    u32d[1] = (u32)0x00000000;
	    u32d[2] = (u32)0xffffffff;
	    u32d[3] = (u32)0x00000000;
	    ns_write_sram(card, vc->adr_scd, u32d, 4);
	    for (j = 1; j < 3; j++)           /* initialize the cache */
	    {
	        u32d[0] = (u32)0x00000000;
	        u32d[1] = (u32)0x00000000;
	        u32d[2] = (u32)0x00000000;
	        u32d[3] = (u32)0x00000000;
	        ns_write_sram(card, vc->adr_scd + 4 *j, u32d, 4);
	    }
	    XPRINTK("nicstar: UBR scd written at:0x%x\n", (int)vc->adr_scd);
	}

    /* finaly create the UBR entry in the TCT */
    u32d[0] = NS_TCTE_TYPE_UBR | vc->adr_scd;
    u32d[1] = 0x00000000;
    u32d[2] = 0x00000000;
    u32d[3] = NS_TCTE_HALT | NS_TCTE_IDLE;
    ns_write_sram(card, (conn *8), u32d, 4);
    u32d[4] = 0x00000000;
    u32d[5] = 0x00000000;
    u32d[6] = 0x00000000;
    u32d[7] = 0x80000000;
    ns_write_sram(card, (conn *8) + 4, u32d+4, 4);

    XPRINTK ("nicstar: a (type UBR) TCT entry written at 0x%x:\n",
                               (int)(card->membase + (conn * 8)));
	XPRINTK ("0x%x 0x%x 0x%x 0x%x\n", u32d[0], u32d[1], u32d[2], u32d[3]);
	XPRINTK ("0x%x 0x%x 0x%x 0x%x\n", u32d[4], u32d[5], u32d[6], u32d[7]);

    return 0;
}

/*
 * VBR is normally not supported by Linux. A VBR QoS is defined by 3 parameters
 * PCR, SCR and MBS.  To set a VBR connection, pass the VBR parameters into the
 * standard traffic parameters of the atm_trafprm stucture as following:
 *
 *      traffic_class = ATM_VBR (3),
 *      max_pcr = the needed PCR value,
 *      pcr =  the needed SCR value,
 *      min_pcr = the needed MBS value (1 to 255)
 */

static int ns_init_tx_vbr (ns_dev *card, vc_map *vc, struct atm_vcc *vcc, int conn)
{
    scq_info        *scq;
    int             j;
    u32             u32d[8];
    u16             pcr_token;
    int             scdi = 0;     /* SCD Index */
    int             scr, pcr, mbs;

#if 0   // commented by Neeraj for supporting vbr in normal way
	PRINTK ("max_pcr= 0x%x, pcr= 0x%x, min_pcr= 0x%x\n",
			vcc->qos.txtp.max_pcr, vcc->qos.txtp.pcr, vcc->qos.txtp.min_pcr);

   	mbs = vcc->qos.txtp.min_pcr;   /* use min_pcr as MBS */
    scr = vcc->qos.txtp.pcr;      	/* use pcr as SCR */
    pcr = vcc->qos.txtp.max_pcr;     /* use max_pcr as PCR */
#else
    PRINTK("pcr= 0x%x, scr= 0x%x, mbs= 0x%x\n",
            vcc->qos.txtp.pcr, vcc->qos.txtp.scr, vcc->qos.txtp.mbs);

    mbs = vcc->qos.txtp.mbs;   /* use min_pcr as MBS */
    scr = vcc->qos.txtp.scr;      	/* use pcr as SCR */
    pcr = vcc->qos.txtp.pcr;     /* use max_pcr as PCR */

#endif

    if (mbs <= 0 || mbs > 255) {
        printk("nicstar%d: trying to open a VBR vc with invalid MBS: %d\n",
                card->index, mbs);
        return - EINVAL;
    }
    if (scr <= 0 || scr > pcr) {
        printk("nicstar%d: trying to open a VBR vc with SCR null or higher than PCR: %d\n",
                card->index, scr);
        return - EINVAL;
    }
    if (pcr <= 0 || pcr > card->max_pcr) {
        printk("nicstar%d: trying to open a VBR vc with invalid PCR, %d\n",
                card->index, pcr);
        return - EINVAL;
    }

    /* get SCD */
    for (scdi = 0; scdi < NS_SCD_NUM; scdi++) {
        if (card->scd2vc[scdi] == NULL) {
            card->scd2vc[scdi] = vc;
            break;
        }
    }
    if (scdi == NS_SCD_NUM) {
        printk ("nicstar%d: no SCD available for VBR channel.\n", card->index);
        return - EBUSY;
    }
    vc->adr_scd = card->scd + scdi * NS_SCD_ENTRY_SIZE;
    scq = get_scq(VBR_SCQ, vc->adr_scd);
    if (scq == (scq_info *) NULL) {
        printk ("nicstar%d: can't get VBR SCQ.\n", card->index);
        card->scd2vc[scdi] = NULL;
        return - ENOMEM;
    }
    vc->scq = scq;

    /* set up SCQ */
    u32d[0] = (u32) virt_to_bus(scq->base);
    u32d[1] =  u32d[0] & 0x00001FF0;
    if ((u32d[0]& 0x00001FFF) != u32d[1])
        printk("nicstar%d: error in scq->base address \n", card->index );

    u32d[2] = (u32)0xffffffff;
    u32d[3] = (u32)0x00000000;
    ns_write_sram(card, vc->adr_scd, u32d, 4);
    for (j = 1; j < 3; j++)           /* initialize the cache */
    {
        u32d[0] = (u32)0x00000000;
        u32d[1] = (u32)0x00000000;
        u32d[2] = (u32)0x00000000;
        u32d[3] = (u32)0x00000000;
        ns_write_sram(card, vc->adr_scd + 4 *j, u32d, 4);
    }
    XPRINTK("nicstar: VBR scd written at:0x%x\n", vc->adr_scd);

    /* create the VBR entry in the TCT */
    pcr_token = scr_on_pcr (scr, pcr);
    if (pcr_token == 0) pcr_token++;
	vc->init_er = ns_rate2log (card, pcr);
	vc->lacr    = (u32)ns_rate2log (card, scr);

    u32d[0] = NS_TCTE_TYPE_VBR | vc->adr_scd;
    u32d[1] = 0x00000000;
    u32d[2] = NS_TCTE_TSIF;
    u32d[3] = NS_TCTE_HALT | NS_TCTE_IDLE;
    ns_write_sram(card, (conn *8), u32d, 4);

    u32d[4] = 0x7F<<24;      		/* max IdleCount = 0x7f*/
    u32d[5] = 0x01<<24;           	/* token bucket integer portion=1*/
    u32d[6] = (mbs << 16) | pcr_token;   	   /* icr is mbs */
    u32d[7] = 0x00000000;
    ns_write_sram(card, (conn *8) + 4, u32d+4, 4);

    XPRINTK ("nicstar: a (type VBR)TCT entry written at 0x%x:\n",
                                        (int)(card->membase + (conn * 8)));
	XPRINTK ("0x%x 0x%x 0x%x 0x%x\n", u32d[0], u32d[1], u32d[2], u32d[3]);
	XPRINTK ("0x%x 0x%x 0x%x 0x%x\n", u32d[4], u32d[5], u32d[6], u32d[7]);

    return 0;
}

static int ns_init_tx_abr (ns_dev *card, vc_map *vc, struct atm_vcc *vcc, int conn)
{
    scq_info        *scq;
    int             j;
    u32             u32d[8];
    u32             air_table, rdf_table, cdf_table;
    u32             norm_age_count, uili, acrc, lmcr, crm;
    int             scdi = 0;     /* SCD Index */

	PRINTK ("pcr= 0x%x, max_pcr= 0x%x, min_pcr= 0x%x\n",
			vcc->qos.txtp.pcr, vcc->qos.txtp.max_pcr, vcc->qos.txtp.min_pcr);
	PRINTK ("rif= 0x%x, rdf= Ox%x, cdf= 0x%x\n",
			vcc->qos.txtp.rif, vcc->qos.txtp.rdf, vcc->qos.txtp.cdf);

    if ( !((vcc->qos.txtp.max_pcr >= vcc->qos.txtp.pcr== 0) &&
            (vcc->qos.txtp.pcr >= vcc->qos.txtp.min_pcr))) {
        printk("nicstar%d: ABR vc should have max_pcr >= pcr >= min_pcr: %d, %d, %d\n",
                card->index, vcc->qos.txtp.max_pcr, vcc->qos.txtp.pcr, vcc->qos.txtp.min_pcr);
        return - EINVAL;
    }

    /* get SCD */
    for (scdi = 0; scdi < NS_SCD_NUM; scdi++) {
        if (card->scd2vc[scdi] == NULL) {
            card->scd2vc[scdi] = vc;
            break;
        }
    }
    if (scdi == NS_SCD_NUM) {
        printk("nicstar%d: no SCD available for ABR channel.\n", card->index);
        return - EBUSY;
    }
    vc->adr_scd = card->scd + scdi * NS_SCD_ENTRY_SIZE;

    /* get SCQ */
    scq = get_scq(ABR_SCQ, vc->adr_scd);
    if (scq == (scq_info *) NULL) {
        printk("nicstar%d: can't get abr SCQ.\n", card->index);
        card->scd2vc[scdi] = NULL;
        return - ENOMEM;
    }
    vc->scq = scq;

    /* set up SCD */
    u32d[0] = (u32) virt_to_bus(scq->base);
    u32d[1] = (u32) u32d[0]& 0x00001FF0;
    if ((u32d[0]& 0x00001FFF) != u32d[1])
        printk("nicstar: ns_open, error in scq->base address \n");

    u32d[2] = (u32)0xffffffff;
    u32d[3] = (u32)0x00000000;
    ns_write_sram(card, vc->adr_scd, u32d, 4);
    for (j = 1; j < 3; j++)           /* initialize the cache */
    {
        u32d[0] = (u32)0x00000000;
        u32d[1] = (u32)0x00000000;
        u32d[2] = (u32)0x00000000;
        u32d[3] = (u32)0x00000000;
        ns_write_sram(card, vc->adr_scd + 4 *j, u32d, 4);
    }
    XPRINTK("nicstar: ABR SCD written at:0x%x\n", vc->adr_scd);

    /* create ABR entry in the TCT */
    vc->init_er = ns_rate2log (card, vcc->qos.txtp.max_pcr);
	lmcr = (u32) ns_rate2log (card, vcc->qos.txtp.min_pcr);
	vc->lacr = (u32) ns_rate2log (card, vcc->qos.txtp.pcr);
	air_table = 4 + 15 - (vcc->qos.txtp.rif);
    rdf_table = 20 + 15 - (vcc->qos.txtp.rdf);
    if (vcc->qos.txtp.cdf_pres)
	    cdf_table = 20 + 15 - (vcc->qos.txtp.cdf);
    else
        cdf_table = rdf_table;

	norm_age_count = 4;
	uili = 0;                                     /* use it or lose it */
	acrc = (u8)(vcc->qos.txtp.adtf_pres);
	crm = 4;

    u32d[0] = NS_TCTE_TYPE_ABR | acrc<<28 | uili<<26 |  vc->adr_scd ;
	u32d[1] = crm << 29;
    u32d[2] = NS_TCTE_TSIF;
	u32d[3] = (NS_TCTE_HALT | NS_TCTE_IDLE) | (norm_age_count << 21);
    ns_write_sram(card, (conn * 8), u32d, 4);
	u32d[4] = (lmcr << 24);
	u32d[5] = ((cdf_table & 0x3F) << 20) | ((rdf_table & 0x3F) << 14) |
			                ((air_table & 0x3F) << 8);
    u32d[6] = 0x00000000;
    u32d[7] = 0x00000000;

    ns_write_sram(card, (conn * 8) + 4, u32d+4, 4);
	XPRINTK ("0x%x 0x%x 0x%x 0x%x\n", u32d[0], u32d[1], u32d[2], u32d[3]);
	XPRINTK ("0x%x 0x%x 0x%x 0x%x\n", u32d[4], u32d[5], u32d[6], u32d[7]);

    return 0;
}


/*  Check VPI and VCI  */

static int ns_check_vc (ns_dev *card, short vpi, int vci)
{
    u16 vp = (u16)vpi;
    u32 vc = (u32)vci;

    /* vci check */
    if ((vc >> card->vcibits) != (card->vpm & (0xffff >> card->vcibits))) {
        return -EINVAL;
    }
    /* vpi check */
    if ((vp >> card->vpibits) != (card->vpm >> (16 - card->vcibits))) {
        return -EINVAL;
    }
    return 0;
}

inline static int ns_find_conn (ns_dev *card, short vpi, int vci)
{
    return ((vpi & card->maxvpi)<< card->vcibits) | (vci & card->maxvci);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,0)
static int ns_open(struct atm_vcc *vcc)
#else
static int ns_open(struct atm_vcc *vcc, short vpi, int vci)
#endif
{
    ns_dev          *card;
    vc_map          *vc;
    int             error;
    int             inuse;          /* tx or rx vc already in use by another vcc */
    u32             conn;
    unsigned char   traffic_class;

    /* the connection number used to index the TCT */
    card = (ns_dev *) vcc->dev->dev_data;
    if (vcc->qos.aal != ATM_AAL5 && vcc->qos.aal != ATM_AAL0) {
        printk("nicstar%d: unsupported AAL.\n", card->index);
        return - EINVAL;
    }
#if LINUX_VERSION_CODE < KERNEL_VERSION (2,6,0)
    PRINTK("nicstar%d: opening vpi.vci %d.%d \n", card->index, (int) vpi, vci);
    if ((error = ns_check_vc (card, vpi, vci))) {
    	printk ("nicstar%d: vpi.vci %d.%d out of bounds (max vpi= %d, max vci= %d, mask = 0x%x)\n",
        card->index, (int)vpi, vci, card->maxvpi, card->maxvci, (int)card->vpm);
        return error;
    }

    if ((error = atm_find_ci(vcc, &vpi, &vci))) {
        printk("nicstar%d: error in atm_find_ci().\n", card->index);
        return error;
    }

    conn =  ns_find_conn (card, vpi, vci);
    vcc->vpi = vpi;
    vcc->vci = vci;
#else
    PRINTK("nicstar%d: opening vpi.vci %d.%d \n", card->index, (int) vcc->vpi, vcc->vci);
    conn =  ns_find_conn (card, vcc->vpi, vcc->vci);
#endif
    vc = & (card->vcmap[conn]);
    vcc->dev_data = vc;

    inuse = 0;
    if (vcc->qos.txtp.traffic_class != ATM_NONE && vc->tx)
        inuse = 1;

    if (vcc->qos.rxtp.traffic_class != ATM_NONE && vc->rx)
        inuse += 2;

    if (inuse) {
        printk("nicstar%d: %s vci already in use.\n", card->index,
               inuse == 1 ? "tx" : inuse == 2  ? "rx" : "tx and rx");
        return - EINVAL;
    }
    set_bit(ATM_VF_ADDR, &vcc->flags);

    /* NOTE: You are not allowed to modify an open connection's QOS. To change
     * that, remove the ATM_VF_PARTIAL flag checking. There may be other changes
     * needed to do that.
     */


    if (!test_bit(ATM_VF_PARTIAL, &vcc->flags)) {

        set_bit(ATM_VF_PARTIAL, &vcc->flags);

        if (vcc->qos.txtp.traffic_class == ATM_NONE)
            traffic_class = vcc->qos.rxtp.traffic_class;
        else
            traffic_class = vcc->qos.txtp.traffic_class;

        switch (traffic_class) {

        case ATM_CBR:
            PRINTK("nicstar%d: trying to open CBR vc.\n", card->index);
            error = ns_init_tx_cbr (card, vc, vcc, conn);
            break;

        case (ATM_UBR):
            PRINTK("nicstar%d: trying to open UBR vc.\n", card->index);
            error = ns_init_tx_ubr (card, vc, vcc, conn);
            break;

        case (ATM_VBR):
            PRINTK("nicstar%d: trying to open VBR vc.\n", card->index);
            error = ns_init_tx_vbr (card, vc, vcc, conn);
            break;

        case (ATM_ABR):
            PRINTK("nicstar%d: trying to open ABR vc.\n", card->index);
            error = ns_init_tx_abr (card, vc, vcc, conn);
            break;
	// Neeraj
	case (ATM_VBR_RT):
            PRINTK("nicstar%d: trying to open rt-vbr vc.\n", card->index);
            error = ns_init_tx_vbr (card, vc, vcc, conn);
            break;

        default:
            printk ("nicstar%d: Unknown traffic class, %d\n",
                                            card->index, traffic_class);
            return -EPROTONOSUPPORT;

        }   /* switch (traffic_class) */

        if (error) {
            clear_bit(ATM_VF_PARTIAL, &vcc->flags);
            clear_bit(ATM_VF_ADDR, &vcc->flags);
            return error;
        }

        /* start scheduling  for non CBR VC*/
        if (vcc->qos.txtp.traffic_class != ATM_CBR) {
            vc->active = 1;
			PRINTK ("nicstar: start scheduling connection %d\n", conn);
       		writel(NS_UPD_INIT_ER | vc->init_er <<16 | conn, card->membase + TCMDQ);
        	writel(NS_START_UPD_LACR | vc->lacr <<16 | conn, card->membase + TCMDQ);

			mdelay (1);
			XPRINTK ("TCT :\n");
			XPRINTK ("0x%x, 0x%x, 0x%x, 0x%x\n",
				 ns_read_sram(card, conn*8),
				 ns_read_sram(card, conn*8 + 1),
				 ns_read_sram(card, conn*8 + 2),
				 ns_read_sram(card, conn*8 + 3));
			XPRINTK ("0x%x, 0x%x, 0x%x, 0x%x\n",
				 ns_read_sram(card, conn*8 + 4),
				 ns_read_sram(card, conn*8 + 5),
				 ns_read_sram(card, conn*8 + 6),
				 ns_read_sram(card, conn*8 + 7));
        }
    }

    if (vcc->qos.txtp.traffic_class != ATM_NONE) {
        vc->tx = 1;
        vc->tx_vcc = vcc;
        vc->tbd_count = 0;

    }
    if (vcc->qos.rxtp.traffic_class != ATM_NONE) {
        u32             status;

        vc->rx = 1;
        vc->rx_vcc = vcc;
        vc->rx_iov = NULL;

        /* Open the receive connection */
        if (vcc->qos.aal == ATM_AAL5)
		    status = NS_RCTE_AAL5 | NS_RCTE_CONNECTOPEN;
        else
             /*vcc->qos.aal == ATM_AAL0 */
            status = NS_RCTE_AAL0 | NS_RCTE_CONNECTOPEN;
#if 1
#ifdef NS_RCQ_SUPPORT
        status |= NS_RCTE_RAWCELLINTEN;
#endif
        ns_write_sram(card, card->rct + conn * NS_RCT_ENTRY_SIZE, &status, 1);
#else
/* Neeraj ignore ns_write_sram, but WHY? */
#ifdef NS_RCQ_SUPPORT
        status |= NS_RCTE_RAWCELLINTEN;
#else
        ns_write_sram(card, card->rct + conn * NS_RCT_ENTRY_SIZE, &status, 1);
#endif /*OAM*/
#endif
    }
    set_bit(ATM_VF_READY, &vcc->flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION (2,6,0)
    MOD_INC_USE_COUNT;
#endif
    return 0;
}

/* close connection */
static void ns_close(struct atm_vcc *vcc)
{
    vc_map          *vc = vcc->dev_data;
    ns_dev          *card = vcc->dev->dev_data;
    u32             data;
    int             i;
    int             conn = ns_find_conn (card, vcc->vpi, vcc->vci);
    unsigned long   flags;
    ns_scqe         *scqep;
    scq_info        *scq;


    PRINTK("nicstar%d: closing vpi.vci %d.%d \n", card->index,
           (int) vcc->vpi, vcc->vci);

    clear_bit(ATM_VF_READY, &vcc->flags);
    if (vcc->qos.rxtp.traffic_class != ATM_NONE) {
        u32             addr;
        unsigned long   flags;

        addr = card->rct + conn * NS_RCT_ENTRY_SIZE;
        ns_grab_res_lock(card, flags);
        waitfor_notbusy(card);

		/* stop receive part */
        writel(NS_CMD_CLOSE_CONNECTION | addr << 2, card->membase + CMD);
        spin_unlock_irqrestore(&card->res_lock, flags);
        vc->rx = 0;
        if (vc->rx_iov != NULL) {
            struct sk_buff  *iovb;
            u32             stat;
            u32             fbqp0;
            u32             fbqp1;

            stat = readl(card->membase + STAT);
            fbqp0 = readl(card->membase + FBQP0);
            fbqp1 = readl(card->membase + FBQP1);
            card->sbfqc = ns_fbqc_get(fbqp0);
            card->lbfqc = ns_fbqc_get(fbqp1);
            PRINTK("nicstar%d: closing a VC with pending rx buffers.\n",
                   card->index);
            iovb = vc->rx_iov;
            recycle_iovec_rx_bufs(card, (struct iovec *) iovb->data,
                                  NS_SKB(iovb)->iovcnt);
            NS_SKB(iovb)->iovcnt = 0;
            NS_SKB(iovb)->vcc = NULL;
            ns_grab_int_lock(card, flags);
            recycle_iov_buf(card, iovb);
            spin_unlock_irqrestore(&card->int_lock, flags);
            vc->rx_iov = NULL;
        }
    }
    if (vcc->qos.txtp.traffic_class != ATM_NONE) {
        vc->tx = 0;
    }

	/* stop transmit part */
    scq = vc->scq;

    while (scq != card->scq0) {
        ns_grab_scq_lock(card, scq, flags);
        scqep = scq->next;
        if (scqep == scq->base)
            scqep = scq->last;
        else
            scqep--;

        if (scqep == scq->tail) {
            spin_unlock_irqrestore(&scq->lock, flags);
            break;
        }

        /* If the last entry is not a TSR, place one in the SCQ in order to
                    be able to completely drain it and then close. */
        if (!ns_scqe_is_tsr(scqep) && scq->tail != scq->next) {
            ns_scqe         tsr;
            u32             scdi, scqi;
            u32             data;
            int             index;

            tsr.word_1 = ns_tsr_mkword_1(NS_TSR_TSIF);
            scdi = (vc->adr_scd - card->scd) / NS_SCD_ENTRY_SIZE;
            scqi = scq->next - scq->base;
            tsr.word_2 = ns_tsr_mkword_2(scdi, scqi);
            tsr.word_3 = 0x00000000;
            tsr.word_4 = 0x00000000;
            *scq->next = tsr;
            index = (int) scqi;
            scq->skb[index] = NULL;
            if (scq->next == scq->last)
                scq->next = scq->base;
            else
                scq->next++;

            data = (u32) virt_to_bus(scq->next);
            ns_write_sram(card, scq->scd, &data, 1);
        }
        schedule();
        restore_flags(flags);
    }

	if (vcc->qos.txtp.traffic_class == ATM_CBR) {
        /* Free all TST entries */
        data = NS_TST_OPCODE_VARIABLE;
        for (i = 0; i < card->tst_num; i++)
        {
            if (card->tste2vc[i] == vc) {
                ns_write_sram(card, card->tst + i, &data, 1);
                card->tste2vc[i] = NULL;
                card->tst_free_entries++;
            }
        }
	}
	else if (vcc->qos.txtp.traffic_class != ATM_NONE) {
        u32 d;
		d = ns_read_sram (card, conn*8 + 3);
		if (!(d & NS_TCTE_IDLE)) {
            writel(NS_HALT | conn, card->membase + TCMDQ);
		}
	}

	if (vc->scq != card->scq0) {

	    card->scd2vc[ (vc->adr_scd - card->scd) / NS_SCD_ENTRY_SIZE] = NULL;
    	free_scq(card, vc->scq, vcc);
	}

   /* remove all references to vcc before deleting it */
   if (vcc->qos.txtp.traffic_class != ATM_NONE) {
     unsigned long flags;
     scq_info *scq = card->scq0;

     ns_grab_scq_lock(card, scq, flags);

     for(i = 0; i < SCQ_NUM_ENTRIES; i++) {
       if(scq->skb[i] && ATM_SKB(scq->skb[i])->vcc == vcc) {
        ATM_SKB(scq->skb[i])->vcc = NULL;
	    atm_return(vcc, scq->skb[i]->truesize);
        PRINTK("nicstar: deleted pending vcc mapping\n");
       }
     }

     spin_unlock_irqrestore(&scq->lock, flags);
   }

   vcc->dev_data = NULL;
   clear_bit(ATM_VF_PARTIAL,&vcc->flags);
   clear_bit(ATM_VF_ADDR,&vcc->flags);
#if LINUX_VERSION_CODE < KERNEL_VERSION (2,6,0)
   MOD_DEC_USE_COUNT;
#endif

#ifdef EXTRA_DEBUG
    {
        u32             stat, cfg;
        u32             fbqp0;
        u32             fbqp1;

        stat = readl(card->membase + STAT);
        fbqp0 = readl(card->membase + FBQP0);
        fbqp1 = readl(card->membase + FBQP1);
        cfg = readl(card->membase + CFG);
        printk("STAT = 0x%08X  CFG = 0x%08X  \n", stat, cfg);
        printk("TSQ: base = 0x%08X  next = 0x%08X  TSQT = 0x%08X \n",
               (u32) card->tsq.base, (u32) card->tsq.next, readl(card->membase + TSQT));
        printk("RSQ: base = 0x%08X  next = 0x%08X  last = 0x%08X  RSQT = 0x%08X \n",
               (u32) card->rsq.base, (u32) card->rsq.next, (u32) card->rsq.last,
               readl(card->membase + RSQT));
        printk("free buffer queue interrupt %s \n", card->fbie ? "enabled" : "disabled");
        printk("SBCNT = %d  count = %d   LBCNT = %d count = %d \n",
               ns_fbqc_get(fbqp0), card->b0pool.count, ns_fbqc_get(fbqp1), card->b1pool.count);
        printk("hbpool.count = %d  iovpool.count = %d \n", card->hbpool.count, card->iovpool.count);
    }
#endif

    XPRINTK("nicstar : ns_close returned\n");
}

static void fill_tst(ns_dev *card, int num_entries, vc_map *vc, u32 conn)
{
    u32             new_tst;
    unsigned long   cl;
    int             e, r;
    u32             data;

    /* It would be very complicated to keep the two TSTs synchronized while
       assuring that writes are only made to the inactive TST. So, for now I
       will use only one TST. If problems occur, I will change this again */

    new_tst = card->tst;
    /* Fill procedure */
    for (e = 0; e < card->tst_num; e++) {
        if (card->tste2vc[e] == NULL)
            break;
    }
    if (e == card->tst_num) {
        printk("nicstar%d: No free TST entries found. \n", card->index);
        return;
    }
    r = num_entries;
    cl = card->tst_num;
    data = ns_tste_make(NS_TST_OPCODE_FIXED, conn);
    while (r > 0) {
        if (cl >= card->tst_num && card->tste2vc[e] == NULL) {
            card->tste2vc[e] = vc;
            ns_write_sram(card, new_tst + e, &data, 1);
            PRINTK("tst entry: 0x%x, written at 0x%x\n", ns_read_sram(card, new_tst + e),
                new_tst + e);
            cl -= card->tst_num;
            r--;
        }
        if (++e == card->tst_num) {
            e = 0;
        }
        cl += num_entries;
    }

    /* End of fill procedure */

/*   not usefull since we use only the first TST
 *
 *   data = ns_tste_make(NS_TST_OPCODE_END, new_tst<<2);
 *   ns_write_sram(card, new_tst + card->tst_num, &data, 1);
 *   ns_write_sram(card, card->tst_addr + card->tst_num, &data, 1);
 *   card->tst_addr = new_tst;
 */
}



static int ns_send(struct atm_vcc *vcc, struct sk_buff *skb)
{
    ns_dev          *card;
    vc_map          *vc;
    scq_info        *scq;
    unsigned long   sflags;
    unsigned long   flags;

    /* TBD flags, not CPU flags */
    card = vcc->dev->dev_data;
    ns_grab_int_lock(card, sflags);
    PRINTK("nicstar%d: ns_send() called.\n", card->index);
    if ((vc = (vc_map *) vcc->dev_data) == NULL) {
        printk("nicstar%d: vcc->dev_data == NULL on ns_send().\n", card->index);
        atomic_inc(&vcc->stats->tx_err);
        dev_kfree_skb_any(skb);
        PRINTK("nicstar%d: ns_send() return -EINVAL.\n", card->index);
        spin_unlock_irqrestore(&card->int_lock, sflags);
        return - EINVAL;
    }
    if (!vc->tx) {
        printk("nicstar%d: Trying to transmit on a non-tx VC.\n", card->index);
        atomic_inc(&vcc->stats->tx_err);
        dev_kfree_skb_any(skb);
        PRINTK("nicstar%d: ns_send() return -EINVAL.\n", card->index);
        spin_unlock_irqrestore(&card->int_lock, sflags);
        return - EINVAL;
    }
    if (vcc->qos.aal != ATM_AAL5 && vcc->qos.aal != ATM_AAL0) {
        printk("nicstar%d: Only AAL0 and AAL5 are supported.\n", card->index);
        atomic_inc(&vcc->stats->tx_err);
        dev_kfree_skb_any(skb);
        PRINTK("nicstar%d: ns_send() return -EINVAL.\n", card->index);
        spin_unlock_irqrestore(&card->int_lock, sflags);
        return - EINVAL;
    }
    if (skb_shinfo(skb)->nr_frags != 0) {
        printk("nicstar%d: No scatter-gather yet.\n", card->index);
        atomic_inc(&vcc->stats->tx_err);
        dev_kfree_skb_any(skb);
        PRINTK("nicstar%d: ns_send() return -EINVAL.\n", card->index);
        spin_unlock_irqrestore(&card->int_lock, sflags);
        return - EINVAL;
    }

    scq = ((vc_map *) vcc->dev_data)->scq;
    ATM_SKB(skb)->vcc = vcc;
    skb_queue_tail(&scq->waiting, skb);
    ns_grab_scq_lock(card, scq, flags);
    ns_push_on_scq (card, scq);
    spin_unlock_irqrestore(&scq->lock, flags);
    PRINTK("nicstar%d: ns_send() return 0.\n", card->index);
    spin_unlock_irqrestore(&card->int_lock, sflags);
    return 0;
}

/* try to push a maximum number of skbs on SCQ */

static void ns_push_on_scq (ns_dev *card, scq_info *scq) {
    ns_scqe         scqe;
    struct sk_buff  *skb;
    struct atm_vcc  *vcc;
    vc_map          *vc;
    u32             flags;
    u32             buflen;

	while ((skb = skb_dequeue(&scq->waiting))) {

        vcc = ATM_SKB(skb)->vcc;
        vc = (vc_map *) vcc->dev_data;

        if (vcc->qos.aal == ATM_AAL5) {
#if 0
            buflen = cpu_to_le32((u32) skb->len);
#else
    	    /* Bug : Don't convert part of word_1 to correct endianness. Breaks
	         * on big-endian machines, let ns_tbd_mkword_1() do the job -
	         * Ritesh - fixed after a couple of days of debugging!
    	     */
    	    buflen = (u32) skb->len;
#endif
	    if(skb->nfmark == 0xAB){
            	flags = NS_TBD_OAM;
#if 0 /* prosum remove it */
                flags |= NS_TBD_EOPDU;  /* Payload type 1 - end of pdu */
#endif
		PRINTK("nicstar: OAM cell\n");
	    }
	    else {
            	flags = NS_TBD_AAL5;
            }
            scqe.word_2 = cpu_to_le32((u32) virt_to_bus(skb->data));
            scqe.word_3 = cpu_to_le32((u32) skb->len);
#if 1
            scqe.word_4 = ns_tbd_mkword_4(0, (u32) vcc->vpi, (u32) vcc->vci, (u32)vcc->aal_options,
                             ATM_SKB(skb)->atm_options & ATM_ATMOPT_CLP ? 1: 0);
#else
            scqe.word_4 = ns_tbd_mkword_4(0, (u32) vcc->vpi, (u32) vcc->vci, 0,
                             ATM_SKB(skb)->atm_options & ATM_ATMOPT_CLP ? 1: 0);
#endif
            flags |= NS_TBD_EOPDU;      /* support only one-buffer CS-PDUs */
        } else {
            /* (vcc->qos.aal == ATM_AAL0) */
            buflen = ATM_CELL_PAYLOAD;                /* i.e., 48 bytes */
	    if (skb->nfmark == 0xAB){
            	flags = NS_TBD_OAM;
#if 0 /* prosum remove it */
                flags |= NS_TBD_EOPDU;  /* Payload type 1 - end of pdu */
#endif
		PRINTK("nicstar: OAM cell\n");
	    }
	    else {
            	flags = NS_TBD_AAL0;
	    }
            scqe.word_2 = cpu_to_le32((u32) virt_to_bus(skb->data) + NS_AAL0_HEADER);
            scqe.word_3 = cpu_to_le32(0x00000000);
            if (*skb->data & 0x02)
                flags |= NS_TBD_EOPDU;  /* Payload type 1 - end of pdu */
                                        /* support only one-buffer CS-PDUs */
            scqe.word_4 = cpu_to_le32(* ((u32 *) skb->data)& ~NS_TBD_VC_MASK);
            /* Force the VPI/VCI to be the same as in VCC struct */
            scqe.word_4 |= cpu_to_le32(((((u32) vcc->vpi) << NS_TBD_VPI_SHIFT |
                                        ((u32) vcc->vci) << NS_TBD_VCI_SHIFT)&
                                           NS_TBD_VC_MASK) | ((vcc->aal_options <<1) & 0x0E));
        }

	/* Now ns_tbd_mkword_1() corrects endianness as required on BE m/cs,
	 * this is because we have removed cpu_to_le32() on buflen parameter
	 * above for AAL5 case - Ritesh
	 */
        scqe.word_1 = ns_tbd_mkword_1(flags, (u32)0, buflen);/* tsrtag=0 */

        scq = ((vc_map *) vcc->dev_data)->scq;
        XPRINTK("nicstar%d: ns_push_on_scq(): current scq-base= 0x%08x \n",
                        card->index, (u32) scq->base);
        XPRINTK("0x%x, 0x%x, 0x%x, 0x%x\n", scqe.word_1, scqe.word_2, scqe.word_3, scqe.word_4);

        if (push_scqe(card, vc, scq, &scqe, skb) != 0) {
            XPRINTK("nicstar%d: ns_push_on_scq(): reenqueue skb since scq is full \n", card->index);
            skb_queue_head(&scq->waiting, skb);    /* reenqueue skb if scq is full */
			    break;
        }
        atomic_inc(&vcc->stats->tx);
    }
}


static int push_scqe(ns_dev *card, vc_map *vc, scq_info *scq, ns_scqe *tbd, struct sk_buff *skb)
{
    ns_scqe         tsr;
    u32             scdi, scqi, data;
    int             index;
    ns_scqe         *one_ahead;

    /* check if enough room for TBD and TSR */
    if (scq->next == scq->tail) {
        XPRINTK("nicstar%d: push_scqe(): no room for TBD\n", card->index);
        return 1;    /* no room for TBD */
    }
    if (scq->next == scq->last)
        one_ahead = scq->base;
    else
        one_ahead = scq->next + 1;

    if (one_ahead == scq->tail) {
        XPRINTK("nicstar%d: push_scqe(): no room for TSR\n", card->index);
        return 1;  /* no room for TSR */
    }

    /* enough room for TBD and TSR, insert TBD */
    *scq->next = *tbd;
    index = (int)(scq->next - scq->base);
    scq->skb[index] = skb;
    XPRINTK("nicstar%d: push_scqe(): sending skb at 0x%08x (pos %d).\n",
            card->index, (u32) skb, index);

    if (scq->next == scq->last)
        scq->next = scq->base;
    else
        scq->next++;

    /* insert TSR */
#if 0 /* commit by RITESH */
    /* XXX: The below is _WRONG_ as BE (like MIPS) doesn't work correctly while
     * LE works (x86) because cpu_to_le32() is null for the latter */
    tsr.word_1 = ns_tsr_mkword_1(NS_TSR_TSIF);
    if (scq->scq_type == CBR_SCQ)
        tsr.word_1 |= (1 << 20);  	/* tsr_tag not null for cbr tsr */
#else
    /* commit by Mars to prevent unpredict initialized value */
    if (scq->scq_type == CBR_SCQ)
        tsr.word_1 = ns_tsr_mkword_1(NS_TSR_TSIF | (1 << 20));  /* tsr_tag not null for cbr tsr */
    else
        tsr.word_1 = ns_tsr_mkword_1(NS_TSR_TSIF);
#endif

    if (scq == card->scq0)
        scdi = NS_TSR_SCDISUBR0;	 /* scdi=0xFFFF FOR SCQ0*/
    else
        scdi = (vc->adr_scd - card->scd) / NS_SCD_ENTRY_SIZE;

    scqi = scq->next - scq->base;
    tsr.word_2 = ns_tsr_mkword_2(scdi, scqi);
    tsr.word_3 = 0x00000000;
    tsr.word_4 = 0x00000000;
    *scq->next = tsr;
    index = (int) scqi;
    scq->skb[index] = NULL;

    if (scq->next == scq->last)
        scq->next = scq->base;
    else
        scq->next++;

    data = (u32) virt_to_bus(scq->next);
    ns_write_sram(card, scq->scd, &data, 1);          /* update Tail pointer in the SCD*/

    /* restart connection if idle  */
    if (scq->scq_type != CBR_SCQ || scq->scq_type != UBR_SCQ) {
        if (!vc->active)
            writel (NS_START | vc->conn, card->membase + TCMDQ);
    }

    return 0;
}

static void process_tsq(ns_dev *card)
{
    u32             scdi;
    u32             type;
    scq_info        *scq;
    ns_tsi          *previous = NULL;
    ns_tsi          *next = card->tsq.next;
	vc_map			*vc;
    int             conn;

    if (ns_tsi_isempty(next)){
        XPRINTK("nicstar%d: process_tsq(): tsi is empty. returning.\n", card->index);
        return;
    }

    do {
        type = ns_tsi_gettype(next);

        if (type == NS_TSI_TYPE_TSR) {
		    unsigned long   flags;
            /* the tsi is due to a tsr */
            scdi = ns_tsi_getscdindex(next);
			if (scdi == NS_TSR_SCDISUBR0) {
				scq = card->scq0;
			}
			else {
				vc = card->scd2vc[scdi];
	            if (vc == NULL) {
	                printk("nicstar%d: could not find VC from SCD index.\n", card->index);
	                ns_tsi_init(next);
                    card->tsq.next = next;
	                return;
	        	}
	        	scq = vc->scq;
			}
			ns_grab_scq_lock(card, scq, flags);
	        drain_scq (card, scq, ns_tsi_getscqpos(next));
	        ns_push_on_scq (card, scq);
			spin_unlock_irqrestore(&scq->lock, flags);
    	}
        else if (type == NS_TSI_TYPE_IDLE) {
            XPRINTK ("nicstar%d: process_tsq(): tsi type is NS_TSI_TYPE_IDLE\n", card->index);
            conn = (ns_tsi_getword_1 (next)) & 0x3fff;
            if( conn < card->rct_size) {
                vc = &card->vcmap[conn];
                vc->active = 0;      /* record that connection is no longer active*/
            }
        }
        else {
            XPRINTK ("nicstar%d: process_tsq(): unknow tsi type: 0x%x\n", card->index, type);
        }
        ns_tsi_init(next);     /* mark TSI empty */
        previous = next;
        next++ ;
        if (next == (card->tsq.base + NS_TSQ_NUM_ENTRIES))
            next = card->tsq.base;
    }
    while (!ns_tsi_isempty(next));

    card->tsq.next = next;
    writel ((u32)previous - (u32)card->tsq.base, card->membase + TSQH);
}

static void drain_scq(ns_dev *card, scq_info *scq, int pos)
{
    struct atm_vcc  *vcc;
    struct sk_buff  *skb;
    int             i;

    XPRINTK("nicstar%d: drain_scq() called, scq at 0x%x, pos %d.\n",
            card->index, (u32) scq, pos);
    if (pos >= SCQ_NUM_ENTRIES) {
        printk("nicstar%d: Bad index on drain_scq().\n", card->index);
        return;
    }

    i = (int)(scq->tail - scq->base);
    if (++i == SCQ_NUM_ENTRIES)
        i = 0;

    while (i != pos) {
        skb = scq->skb[i];
        XPRINTK("nicstar%d: freeing skb at 0x%x (index %d).\n",
                card->index, (u32) skb, i);
        if (skb != NULL) {
            vcc = ATM_SKB(skb)->vcc;
            if (vcc->pop != NULL)
                vcc->pop(vcc, skb);
            else
                dev_kfree_skb_any(skb);

            scq->skb[i] = NULL;
        }
        if (++i == SCQ_NUM_ENTRIES)
            i = 0;
    }
    scq->tail = scq->base + pos;
}

static void process_rsq(ns_dev *card)
{
    ns_rsqe         *previous;

    if (!ns_rsqe_valid(card->rsq.next))
        return;

    do
    {
        dequeue_rx(card, card->rsq.next);
        ns_rsqe_init(card->rsq.next);
        previous = card->rsq.next;
        if (card->rsq.next == card->rsq.last)
            card->rsq.next = card->rsq.base;
        else
            card->rsq.next++;
    }
    while (ns_rsqe_valid(card->rsq.next));

    writel((((u32) previous) - ((u32) card->rsq.base)),
           card->membase + RSQH);
}

static void dequeue_rx(ns_dev *card, ns_rsqe *rsqe)
{
    u32             vpi, vci;
    vc_map          *vc;
    struct sk_buff  *iovb;
    struct iovec    *iov;
    struct atm_vcc  *vcc;
    struct sk_buff  *skb;
    unsigned short  aal5_len;
    int             len;
    u32             fbqp0;
    u32             fbqp1;
    u32             conn;

    fbqp0 = readl(card->membase + FBQP0);
    fbqp1 = readl(card->membase + FBQP1);
    card->sbfqc = ns_fbqc_get(fbqp0);
    card->lbfqc = ns_fbqc_get(fbqp1);
    skb = (struct sk_buff *) le32_to_cpu(rsqe->buffer_handle);
    vpi = ns_rsqe_vpi(rsqe);
    vci = ns_rsqe_vci(rsqe);
    if (vpi == 0 && vci == 0) {
        printk ("nicstar%d: SDU received for vc 0.0.\n", card->index);
        PRINTK ("rsqe:\nw1= 0x%x, w2= 0x%x, w3= 0x%x, w4= 0x%x\n", 
                        rsqe->word_1, rsqe->buffer_handle, 
                        rsqe->final_aal5_crc32, rsqe->word_4);

        recycle_rx_buf(card, skb);
        return;
    }
    if (ns_check_vc (card, vpi, vci)) {
        printk("nicstar%d: SDU received for out-of-range vc %d.%d.\n",
               card->index, vpi, vci);
        recycle_rx_buf(card, skb);
        return;
    }
    conn = ns_find_conn (card, vpi, vci);
    vc = & (card->vcmap[conn]);
    if (!vc->rx) {
        printk("nicstar%d: SDU received on non-rx vc %d.%d.\n",
                 card->index, vpi, vci);
        recycle_rx_buf(card, skb);
        return;
    }
    vcc = vc->rx_vcc;
    if (vcc->qos.aal == ATM_AAL0) {
        struct sk_buff  *sb;
        unsigned char   *cell;
        int             i;

        cell = skb->data;
        for (i = ns_rsqe_cellcount(rsqe); i; i--)
        {
            if ((sb = alloc_skb(NS_B0SKBSIZE, GFP_ATOMIC)) == NULL) {
                printk("nicstar%d: Can't allocate buffers for aal0.\n",
                       card->index);
                atomic_add(i, &vcc->stats->rx_drop);
                break;
            }
            if (!atm_charge(vcc, sb->truesize)) {
                PRINTK("nicstar%d: atm_charge() dropped aal0 packets.\n",
                         card->index);
                atomic_add(i - 1, &vcc->stats->rx_drop);/* already increased by 1 */
                dev_kfree_skb_any(sb);
                break;
            }
            /* Rebuild the header */
            * ((u32 *) sb->data) = le32_to_cpu(rsqe->word_1) << 4 |
                                   (ns_rsqe_clp(rsqe)? 0x00000001 : 0x00000000);
            if (i == 1 && ns_rsqe_eopdu(rsqe))
                * ((u32 *) sb->data) |= 0x00000002;

            skb_put(sb, NS_AAL0_HEADER);
            memcpy(sb->tail, cell, ATM_CELL_PAYLOAD);
            skb_put(sb, ATM_CELL_PAYLOAD);
            ATM_SKB(sb)->vcc = vcc;
            do_gettimeofday(&sb->stamp);
            vcc->push(vcc, sb);
            atomic_inc(&vcc->stats->rx);
            cell += ATM_CELL_PAYLOAD;
        }
        recycle_rx_buf(card, skb);
        return;
    }
    /* To reach this point, the AAL layer can only be AAL5 */

    if ((iovb = vc->rx_iov) == NULL) {
        iovb = skb_dequeue(&card->iovpool.queue);
        if (iovb == NULL) {
            /* No buffers in the queue */
            iovb = alloc_skb(NS_IOVBUFSIZE, GFP_ATOMIC);
            if (iovb == NULL) {
                printk("nicstar%d: out of iovec buffers.\n", card->index);
                atomic_inc(&vcc->stats->rx_drop);
                recycle_rx_buf(card, skb);
                return;
            }
        }
        else if (--card->iovpool.count < card->iovnr.min) {
            struct sk_buff  *new_iovb;

            if ((new_iovb = alloc_skb(NS_IOVBUFSIZE, GFP_ATOMIC)) != NULL) {
                skb_queue_tail(&card->iovpool.queue, new_iovb);
                card->iovpool.count++;
            }
        }
        vc->rx_iov = iovb;
        NS_SKB(iovb)->iovcnt = 0;
        iovb->len = 0;
        iovb->tail = iovb->data = iovb->head;
        NS_SKB(iovb)->vcc = vcc;
        /* IMPORTANT: a pointer to the sk_buff containing the small or large
                            buffer is stored as iovec base, NOT a pointer to the
        	            small or large buffer itself. */
    }
    else if (NS_SKB(iovb)->iovcnt >= NS_MAX_IOVECS) {
        printk("nicstar%d: received too big AAL5 SDU.\n", card->index);
        atomic_inc(&vcc->stats->rx_err);
        recycle_iovec_rx_bufs(card, (struct iovec *) iovb->data, NS_MAX_IOVECS);
        NS_SKB(iovb)->iovcnt = 0;
        iovb->len = 0;
        iovb->tail = iovb->data = iovb->head;
        NS_SKB(iovb)->vcc = vcc;
    }
    iov = &((struct iovec *) iovb->data)[NS_SKB(iovb)->iovcnt++];
    iov->iov_base = (void *) skb;
    iov->iov_len = ns_rsqe_cellcount(rsqe) *48;
    iovb->len += iov->iov_len;
    if (NS_SKB(iovb)->iovcnt == 1) {
        if (skb->list != &card->b0pool.queue) {
            printk("nicstar%d: Expected a small buffer, and this is not one.\n",
                   card->index);
            which_list(card, skb);
            atomic_inc(&vcc->stats->rx_err);
            recycle_rx_buf(card, skb);
            vc->rx_iov = NULL;
            recycle_iov_buf(card, iovb);
            return;
        }
    } else {
        /* NS_SKB(iovb)->iovcnt >= 2 */
        if (skb->list != &card->b1pool.queue) {
            printk("nicstar%d: Expected a large buffer, and this is not one.\n",
                    card->index);
            which_list(card, skb);
            atomic_inc(&vcc->stats->rx_err);
            recycle_iovec_rx_bufs(card, (struct iovec *) iovb->data,
                                    NS_SKB(iovb)->iovcnt);
            vc->rx_iov = NULL;
            recycle_iov_buf(card, iovb);
            return;
        }
    }
    if (ns_rsqe_eopdu(rsqe)) {
        /* This works correctly regardless of the endianness of the host */
        unsigned char   *L1L2 = (unsigned char *)((u32) skb->data +
                                                  iov->iov_len - 6);

        aal5_len = L1L2[0] << 8 | L1L2[1];
        len = (aal5_len == 0x0000) ? 0x10000 : aal5_len;
        if (ns_rsqe_crcerr(rsqe) ||
            len + 8 > iovb->len || len + (47 + 8) < iovb->len) {
            printk("nicstar%d: AAL5 CRC error", card->index);
            if (len + 8 > iovb->len || len + (47 + 8) < iovb->len)
                printk(" - PDU size mismatch.\n");
            else
                printk(".\n");

            atomic_inc(&vcc->stats->rx_err);
            recycle_iovec_rx_bufs(card, (struct iovec *) iovb->data,
                                  NS_SKB(iovb)->iovcnt);
            vc->rx_iov = NULL;
            recycle_iov_buf(card, iovb);
            return;
        }
        /* By this point we (hopefully) have a complete SDU without errors. */

        if (NS_SKB(iovb)->iovcnt == 1) {
            /* skb points to a small buffer */
            if (!atm_charge(vcc, skb->truesize)) {
                push_rxbufs(card, BUF_B0, (u32) skb, (u32) virt_to_bus(skb->data), 0, 0);
            } else {
                skb_put(skb, len);
                dequeue_sm_buf(card, skb);
#ifdef NS_USE_DESTRUCTORS
                skb->destructor = ns_sb_destructor;
#endif /* NS_USE_DESTRUCTORS */
                ATM_SKB(skb)->vcc = vcc;
                do_gettimeofday(&skb->stamp);
                vcc->push(vcc, skb);
                atomic_inc(&vcc->stats->rx);
            }
        }
        else if (NS_SKB(iovb)->iovcnt == 2) {
           /* One small plus one large buffer */

            struct sk_buff  *sb;

            sb = (struct sk_buff *)(iov - 1)->iov_base;

            /* skb points to a large buffer */
            if (len <= NS_B0BUFSIZE) {
                if (!atm_charge(vcc, sb->truesize)) {
                    push_rxbufs(card, BUF_B0, (u32) sb, (u32) virt_to_bus(sb->data), 0, 0);
                } else {
                    skb_put(sb, len);
                    dequeue_sm_buf(card, sb);
#ifdef NS_USE_DESTRUCTORS
                    sb->destructor = ns_sb_destructor;
#endif /* NS_USE_DESTRUCTORS */
                    ATM_SKB(sb)->vcc = vcc;
                    do_gettimeofday(&sb->stamp);
                    vcc->push(vcc, sb);
                    atomic_inc(&vcc->stats->rx);
                }
                push_rxbufs(card, BUF_B1, (u32) skb, (u32) virt_to_bus(skb->data), 0, 0);
            } else {
                /* len > NS_B0BUFSIZE, the usual case */
                if (!atm_charge(vcc, skb->truesize)) {
                    push_rxbufs(card, BUF_B1, (u32) skb, (u32) virt_to_bus(skb->data),
                                0, 0);
                } else {
                    dequeue_lg_buf(card, skb);
#ifdef NS_USE_DESTRUCTORS
                    skb->destructor = ns_lb_destructor;
#endif /* NS_USE_DESTRUCTORS */
                    skb_push(skb, NS_B0BUFSIZE);
                    memcpy(skb->data, sb->data, NS_B0BUFSIZE);
                    skb_put(skb, len - NS_B0BUFSIZE);
                    ATM_SKB(skb)->vcc = vcc;
                    do_gettimeofday(&skb->stamp);
                    vcc->push(vcc, skb);
                    atomic_inc(&vcc->stats->rx);
                }
                push_rxbufs(card, BUF_B0, (u32) sb, (u32) virt_to_bus(sb->data), 0, 0);
            }
        } else {
            /* Must push a huge buffer */
            struct sk_buff  *hb, *sb, *lb;
            int             remaining, tocopy;
            int             j;

            hb = skb_dequeue(& (card->hbpool.queue));
            if (hb == NULL) {
                /* No buffers in the queue */
                hb = alloc_skb(NS_HBUFSIZE, GFP_ATOMIC);
                if (hb == NULL) {
                    printk("nicstar%d: Out of huge buffers.\n", card->index);
                    atomic_inc(&vcc->stats->rx_drop);
                    recycle_iovec_rx_bufs(card, (struct iovec *) iovb->data,
                                            NS_SKB(iovb)->iovcnt);
                    vc->rx_iov = NULL;
                    recycle_iov_buf(card, iovb);
                    return;
                }
                else if (card->hbpool.count < card->hbnr.min) {
                    struct sk_buff  *new_hb;

                    if ((new_hb = alloc_skb(NS_HBUFSIZE, GFP_ATOMIC)) != NULL) {
                        skb_queue_tail(&card->hbpool.queue, new_hb);
                        card->hbpool.count++;
                    }
                }
            }
            else if (--card->hbpool.count < card->hbnr.min) {
                struct sk_buff  *new_hb;

                if ((new_hb = alloc_skb(NS_HBUFSIZE, GFP_ATOMIC)) != NULL) {
                    skb_queue_tail(&card->hbpool.queue, new_hb);
                    card->hbpool.count++;
                }
                if (card->hbpool.count < card->hbnr.min) {
                    if ((new_hb = alloc_skb(NS_HBUFSIZE, GFP_ATOMIC)) != NULL) {
                        skb_queue_tail(&card->hbpool.queue, new_hb);
                        card->hbpool.count++;
                    }
                }
            }
            iov = (struct iovec *) iovb->data;
            if (!atm_charge(vcc, hb->truesize)) {
                recycle_iovec_rx_bufs(card, iov, NS_SKB(iovb)->iovcnt);
                if (card->hbpool.count < card->hbnr.max) {
                    skb_queue_tail(&card->hbpool.queue, hb);
                    card->hbpool.count++;
                }
                else
                    dev_kfree_skb_any(hb);

            } else {
                /* Copy the small buffer to the huge buffer */
                sb = (struct sk_buff *) iov->iov_base;
                memcpy(hb->data, sb->data, iov->iov_len);
                skb_put(hb, iov->iov_len);
                remaining = len - iov->iov_len;
                iov++;
                /* Free the small buffer */
                push_rxbufs(card, BUF_B0, (u32) sb, (u32) virt_to_bus(sb->data), 0, 0);
                /* Copy all large buffers to the huge buffer and free them */
                for (j = 1; j < NS_SKB(iovb)->iovcnt; j++)
                {
                    lb = (struct sk_buff *) iov->iov_base;
                    tocopy = MIN(remaining, iov->iov_len);
                    memcpy(hb->tail, lb->data, tocopy);
                    skb_put(hb, tocopy);
                    iov++;
                    remaining -= tocopy;
                    push_rxbufs(card, BUF_B1, (u32) lb, (u32) virt_to_bus(lb->data), 0, 0);
                }
#ifdef EXTRA_DEBUG
                if (remaining != 0 || hb->len != len)
                    printk("nicstar%d: Huge buffer len mismatch.\n", card->index);

#endif /* EXTRA_DEBUG */
                ATM_SKB(hb)->vcc = vcc;
#ifdef NS_USE_DESTRUCTORS
                hb->destructor = ns_hb_destructor;
#endif /* NS_USE_DESTRUCTORS */
                do_gettimeofday(&hb->stamp);
                vcc->push(vcc, hb);
                atomic_inc(&vcc->stats->rx);
            }
        }
        vc->rx_iov = NULL;
        recycle_iov_buf(card, iovb);
    }
}

#ifdef NS_USE_DESTRUCTORS

static void ns_sb_destructor(struct sk_buff *sb)
{
    ns_dev          *card;
    u32             fbqp0, fbqp1;

    card = (ns_dev *) ATM_SKB(sb)->vcc->dev->dev_data;
    fbqp0 = readl(card->membase + FBQP0);
    fbqp1 = readl(card->membase + FBQP1);
    card->sbfqc = ns_fbqc_get(fbqp0);
    card->lbfqc = ns_fbqc_get(fbqp1);
    do
    {
        sb = alloc_skb(NS_B0SKBSIZE, GFP_KERNEL);
        if (sb == NULL)
            break;

        skb_queue_tail(&card->b0pool.queue, sb);
        skb_reserve(sb, NS_AAL0_HEADER);
        push_rxbufs(card, BUF_B0, (u32) sb, (u32) virt_to_bus(sb->data), 0, 0);
    }
    while (card->sbfqc < card->b0nr.min)
        ;

}

static void ns_lb_destructor(struct sk_buff *lb)
{
    ns_dev          *card;
    u32             fbqp0, fbqp1;

    card = (ns_dev *) ATM_SKB(lb)->vcc->dev->dev_data;
    fbqp0 = readl(card->membase + FBQP0);
    fbqp1 = readl(card->membase + FBQP1);
    card->sbfqc = ns_fbqc_get(fbqp0);
    card->lbfqc = ns_fbqc_get(fbqp1);
    do
    {
        lb = alloc_skb(NS_B1SKBSIZE, GFP_KERNEL);
        if (lb == NULL)
            break;

        skb_queue_tail(&card->b1pool.queue, lb);
        skb_reserve(lb, NS_B0BUFSIZE);
        push_rxbufs(card, BUF_B1, (u32) lb, (u32) virt_to_bus(lb->data), 0, 0);
    }
    while (card->lbfqc < card->b1nr.min)
        ;

}

static void ns_hb_destructor(struct sk_buff *hb)
{
    ns_dev          *card;

    card = (ns_dev *) ATM_SKB(hb)->vcc->dev->dev_data;
    while (card->hbpool.count < card->hbnr.init) {
        hb = alloc_skb(NS_HBUFSIZE, GFP_KERNEL);
        if (hb == NULL)
            break;

        skb_queue_tail(&card->hbpool.queue, hb);
        card->hbpool.count++;
    }
}

#endif /* NS_USE_DESTRUCTORS */

static void recycle_rx_buf(ns_dev *card, struct sk_buff *skb)
{
    if (skb->list == &card->b0pool.queue)
        push_rxbufs(card, BUF_B0, (u32) skb, (u32) virt_to_bus(skb->data), 0, 0);
    else if (skb->list == &card->b1pool.queue)
        push_rxbufs(card, BUF_B1, (u32) skb, (u32) virt_to_bus(skb->data), 0, 0);
    else {
        printk("nicstar%d: What kind of rx buffer is this?\n", card->index);
        dev_kfree_skb_any(skb);
    }
}

static void recycle_iovec_rx_bufs(ns_dev *card, struct iovec *iov, int count)
{
    struct sk_buff  *skb;

    for (; count > 0; count--) {
        skb = (struct sk_buff *)(iov++)->iov_base;
        if (skb->list == &card->b0pool.queue)
            push_rxbufs(card, BUF_B0, (u32) skb, (u32) virt_to_bus(skb->data), 0, 0);
        else if (skb->list == &card->b1pool.queue)
            push_rxbufs(card, BUF_B1, (u32) skb, (u32) virt_to_bus(skb->data), 0, 0);
        else {
            printk("nicstar%d: What kind of rx buffer is this?\n", card->index);
            dev_kfree_skb_any(skb);
        }
    }
}

static void recycle_iov_buf(ns_dev *card, struct sk_buff *iovb)
{
    if (card->iovpool.count < card->iovnr.max) {
        skb_queue_tail(&card->iovpool.queue, iovb);
        card->iovpool.count++;
    }
    else
        dev_kfree_skb_any(iovb);
}

static void dequeue_sm_buf(ns_dev *card, struct sk_buff *sb)
{
    skb_unlink(sb);
#ifdef NS_USE_DESTRUCTORS
    if (card->sbfqc < card->b0nr.min)
#else
    if (card->sbfqc < card->b0nr.init) {
        struct sk_buff  *new_sb;

        if ((new_sb = alloc_skb(NS_B0SKBSIZE, GFP_ATOMIC)) != NULL) {
            skb_queue_tail(&card->b0pool.queue, new_sb);
            skb_reserve(new_sb, NS_AAL0_HEADER);
            push_rxbufs(card, BUF_B0, (u32) new_sb, (u32) virt_to_bus(new_sb->data), 0, 0);
        }
    }
if (card->sbfqc < card->b0nr.init)
#endif /* NS_USE_DESTRUCTORS */
    {
        struct sk_buff  *new_sb;

        if ((new_sb = alloc_skb(NS_B0SKBSIZE, GFP_ATOMIC)) != NULL) {
            skb_queue_tail(&card->b0pool.queue, new_sb);
            skb_reserve(new_sb, NS_AAL0_HEADER);
            push_rxbufs(card, BUF_B0, (u32) new_sb, (u32) virt_to_bus(new_sb->data), 0, 0);
        }
    }
}

static void dequeue_lg_buf(ns_dev *card, struct sk_buff *lb)
{
    skb_unlink(lb);
#ifdef NS_USE_DESTRUCTORS
    if (card->lbfqc < card->b1nr.min)
#else
    if (card->lbfqc < card->b1nr.init) {
        struct sk_buff  *new_lb;

        if ((new_lb = alloc_skb(NS_B1SKBSIZE, GFP_ATOMIC)) != NULL) {
            skb_queue_tail(&card->b1pool.queue, new_lb);
            skb_reserve(new_lb, NS_B0BUFSIZE);
            push_rxbufs(card, BUF_B1, (u32) new_lb, (u32) virt_to_bus(new_lb->data), 0, 0);
        }
    }
if (card->lbfqc < card->b1nr.init)
#endif /* NS_USE_DESTRUCTORS */
    {
        struct sk_buff  *new_lb;

        if ((new_lb = alloc_skb(NS_B1SKBSIZE, GFP_ATOMIC)) != NULL) {
            skb_queue_tail(&card->b1pool.queue, new_lb);
            skb_reserve(new_lb, NS_B0BUFSIZE);
            push_rxbufs(card, BUF_B1, (u32) new_lb, (u32) virt_to_bus(new_lb->data), 0, 0);
        }
    }
}

static int ns_proc_read(struct atm_dev *dev, loff_t *pos, char *page)
{
    u32             fbqp0, fbqp1;
    ns_dev          *card;
    int             left;

    left = (int) *pos;
    card = (ns_dev *) dev->dev_data;
    fbqp0 = readl(card->membase + FBQP0);
    fbqp1 = readl(card->membase + FBQP1);
    if (!left--)
        return sprintf(page, "Pool   count    min   init    max \n");

    if (!left--)
        return sprintf(page, "Small  %5d  %5d  %5d  %5d \n",
                       ns_fbqc_get(fbqp0), card->b0nr.min, card->b0nr.init,
                       card->b0nr.max);

    if (!left--)
        return sprintf(page, "Large  %5d  %5d  %5d  %5d \n",
                       ns_fbqc_get(fbqp1), card->b1nr.min, card->b1nr.init,
                       card->b1nr.max);

    if (!left--)
        return sprintf(page, "Huge   %5d  %5d  %5d  %5d \n", card->hbpool.count,
                       card->hbnr.min, card->hbnr.init, card->hbnr.max);

    if (!left--)
        return sprintf(page, "Iovec  %5d  %5d  %5d  %5d \n", card->iovpool.count,
                       card->iovnr.min, card->iovnr.init, card->iovnr.max);

    if (!left--) {
        int             retval;

        retval = sprintf(page, "Interrupt counter: %u \n", card->intcnt);
        card->intcnt = 0;
        return retval;
    }
#if 0
    /* Dump 25.6 Mbps PHY registers */
    /* Now there's a 25.6 Mbps PHY driver this code isn't needed. I left it
          here just in case it's needed for debugging. */
    if (card->max_pcr == ATM_25_PCR && !left--) {
        u32             phy_regs[4];
        u32             i;

        for (i = 0; i < 4; i++)
        {
            waitfor_notbusy(card);
            writel(NS_CMD_READ_UTILITY | 0x00000100 | i, card->membase + CMD);
            waitfor_notbusy(card);
            phy_regs[i] = readl(card->membase + DR0)& 0x000000FF;
        }
        return sprintf(page, "PHY regs: 0x%02X 0x%02X 0x%02X 0x%02X \n",
                       phy_regs[0], phy_regs[1], phy_regs[2], phy_regs[3]);
    }
#endif /* 0 - Dump 25.6 Mbps PHY registers */
#if 0
    /* Dump TST */
    if (left-- < card->tst_num) {
        if (card->tste2vc[left + 1] == NULL)
            return sprintf(page, "%5d - VBR/UBR \n", left + 1);
        else
            return sprintf(page, "%5d - %d %d \n", left + 1,
                           card->tste2vc[left + 1]->tx_vcc->vpi,
                           card->tste2vc[left + 1]->tx_vcc->vci);

    }
#endif /* 0 */
    return 0;
}

static int ns_ioctl(struct atm_dev *dev, unsigned int cmd, void *arg)
{
    ns_dev          *card;
    pool_levels     pl;
    int             btype;
    unsigned long   flags;

    card = dev->dev_data;
    switch (cmd)
    {
      case NS_GETPSTAT:
        if (get_user(pl.buftype, & ((pool_levels *) arg)->buftype))
            return - EFAULT;

        switch (pl.buftype)
        {
          case NS_BUFTYPE_SMALL:
            pl.count = ns_fbqc_get(readl(card->membase + FBQP0));
            pl.level.min = card->b0nr.min;
            pl.level.init = card->b0nr.init;
            pl.level.max = card->b0nr.max;
            break;
          case NS_BUFTYPE_LARGE:
            pl.count = ns_fbqc_get(readl(card->membase + FBQP1));
            pl.level.min = card->b1nr.min;
            pl.level.init = card->b1nr.init;
            pl.level.max = card->b1nr.max;
            break;
          case NS_BUFTYPE_HUGE:
            pl.count = card->hbpool.count;
            pl.level.min = card->hbnr.min;
            pl.level.init = card->hbnr.init;
            pl.level.max = card->hbnr.max;
            break;
          case NS_BUFTYPE_IOVEC:
            pl.count = card->iovpool.count;
            pl.level.min = card->iovnr.min;
            pl.level.init = card->iovnr.init;
            pl.level.max = card->iovnr.max;
            break;
          default:
            return - ENOIOCTLCMD;
        }
        if (!copy_to_user((pool_levels *) arg, &pl, sizeof (pl)))
            return (sizeof (pl));
        else
            return - EFAULT;

      case NS_SETBUFLEV:
        if (!capable(CAP_NET_ADMIN))
            return - EPERM;

        if (copy_from_user(&pl, (pool_levels *) arg, sizeof (pl)))
            return - EFAULT;

        if (pl.level.min >= pl.level.init || pl.level.init >= pl.level.max)
            return - EINVAL;

        if (pl.level.min == 0)
            return - EINVAL;

        switch (pl.buftype)
        {
          case NS_BUFTYPE_SMALL:
            if (pl.level.max > TOP_B0)
                return - EINVAL;

            card->b0nr.min = pl.level.min;
            card->b0nr.init = pl.level.init;
            card->b0nr.max = pl.level.max;
            break;
          case NS_BUFTYPE_LARGE:
            if (pl.level.max > TOP_B1)
                return - EINVAL;

            card->b1nr.min = pl.level.min;
            card->b1nr.init = pl.level.init;
            card->b1nr.max = pl.level.max;
            break;
          case NS_BUFTYPE_HUGE:
            if (pl.level.max > TOP_HB)
                return - EINVAL;

            card->hbnr.min = pl.level.min;
            card->hbnr.init = pl.level.init;
            card->hbnr.max = pl.level.max;
            break;
          case NS_BUFTYPE_IOVEC:
            if (pl.level.max > TOP_IOVB)
                return - EINVAL;

            card->iovnr.min = pl.level.min;
            card->iovnr.init = pl.level.init;
            card->iovnr.max = pl.level.max;
            break;
          default:
            return - EINVAL;
        }
        return 0;
      case NS_ADJBUFLEV:
        if (!capable(CAP_NET_ADMIN))
            return - EPERM;

        btype = (int) arg;         /* an int is the same size as a pointer */
        switch (btype)
        {
          case NS_BUFTYPE_SMALL:
            while (card->sbfqc < card->b0nr.init) {
                struct sk_buff  *sb;

                sb = alloc_skb(NS_B0SKBSIZE, GFP_KERNEL);
                if (sb == NULL)
                    return - ENOMEM;

                skb_queue_tail(&card->b0pool.queue, sb);
                skb_reserve(sb, NS_AAL0_HEADER);
                push_rxbufs(card, BUF_B0, (u32) sb, (u32) virt_to_bus(sb->data), 0, 0);
            }
            break;
          case NS_BUFTYPE_LARGE:
            while (card->lbfqc < card->b1nr.init) {
                struct sk_buff  *lb;

                lb = alloc_skb(NS_B1SKBSIZE, GFP_KERNEL);
                if (lb == NULL)
                    return - ENOMEM;

                skb_queue_tail(&card->b1pool.queue, lb);
                skb_reserve(lb, NS_B0BUFSIZE);
                push_rxbufs(card, BUF_B1, (u32) lb, (u32) virt_to_bus(lb->data), 0, 0);
            }
            break;
          case NS_BUFTYPE_HUGE:
            while (card->hbpool.count > card->hbnr.init) {
                struct sk_buff  *hb;

                ns_grab_int_lock(card, flags);
                hb = skb_dequeue(&card->hbpool.queue);
                card->hbpool.count--;
                spin_unlock_irqrestore(&card->int_lock, flags);
                if (hb == NULL)
                    printk("nicstar%d: huge buffer count inconsistent.\n",
                           card->index);
                else
                    dev_kfree_skb_any(hb);

            }
            while (card->hbpool.count < card->hbnr.init) {
                struct sk_buff  *hb;

                hb = alloc_skb(NS_HBUFSIZE, GFP_KERNEL);
                if (hb == NULL)
                    return - ENOMEM;

                ns_grab_int_lock(card, flags);
                skb_queue_tail(&card->hbpool.queue, hb);
                card->hbpool.count++;
                spin_unlock_irqrestore(&card->int_lock, flags);
            }
            break;
          case NS_BUFTYPE_IOVEC:
            while (card->iovpool.count > card->iovnr.init) {
                struct sk_buff  *iovb;

                ns_grab_int_lock(card, flags);
                iovb = skb_dequeue(&card->iovpool.queue);
                card->iovpool.count--;
                spin_unlock_irqrestore(&card->int_lock, flags);
                if (iovb == NULL)
                    printk("nicstar%d: iovec buffer count inconsistent.\n",
                           card->index);
                else
                    dev_kfree_skb_any(iovb);

            }
            while (card->iovpool.count < card->iovnr.init) {
                struct sk_buff  *iovb;

                iovb = alloc_skb(NS_IOVBUFSIZE, GFP_KERNEL);
                if (iovb == NULL)
                    return - ENOMEM;

                ns_grab_int_lock(card, flags);
                skb_queue_tail(&card->iovpool.queue, iovb);
                card->iovpool.count++;
                spin_unlock_irqrestore(&card->int_lock, flags);
            }
            break;
          default:
            return - EINVAL;
        }
        return 0;
      default:
        if (dev->phy && dev->phy->ioctl) {
            return dev->phy->ioctl(dev, cmd, arg);
        }
        else {
            printk("nicstar%d: %s == NULL \n", card->index,
                   dev->phy
                       ? "dev->phy->ioctl"
                       : "dev->phy");
            return - ENOIOCTLCMD;
        }
    }
}

static void which_list(ns_dev *card, struct sk_buff *skb)
{
    printk("It's a %s buffer.\n", skb->list == &card->b0pool.queue
               ? "small" : skb->list == &card->b1pool.queue ? "large"
               :  skb->list == &card->hbpool.queue  ? "huge"
               :  skb->list == &card->iovpool.queue ? "iovec": "unknown");
}

#ifdef NS_POLLING
static void ns_poll(unsigned long arg)
{
    int             i;
    ns_dev          *card;
    unsigned long   flags;
    u32             stat_r;

    XPRINTK("nicstar: Entering ns_poll().\n");
    for (i = 0; i < num_cards; i++) {
        card = cards[i];
        if (spin_is_locked(&card->int_lock)) {
            /* Probably it isn't worth spinning */
            continue;
        }
        ns_grab_int_lock(card, flags);
        stat_r = readl(card->membase + STAT);
        if (stat_r & NS_STAT_TSIF) {
            writel(NS_STAT_TSIF, card->membase + STAT);
            printk("nicstar%d: TSI\n", card->index);
            process_tsq(card);
        }
        if (stat_r & NS_STAT_EOPDU) {
            writel(NS_STAT_EOPDU, card->membase + STAT);
            printk("nicstar%d: End Of PDU\n", card->index);
            process_rsq(card);
        }
        spin_unlock_irqrestore(&card->int_lock, flags);
    }
    mod_timer(&ns_timer, jiffies + NS_POLL_PERIOD);
    XPRINTK("nicstar: Leaving ns_poll().\n");
}

#endif

static int ns_parse_mac(char *mac, unsigned char *esi)
{
    int             i, j;
    short           byte1, byte0;

    if (mac == NULL || esi == NULL)
        return - 1;

    j = 0;
    for (i = 0; i < 6; i++) {
        if ((byte1 = ns_h2i(mac[j++])) < 0)
                     return - 1;

        if ((byte0 = ns_h2i(mac[j++])) < 0)
                     return - 1;

        esi[i] = (unsigned char)(byte1 *16 + byte0);
        if (i < 5) {
            if (mac[j++] != ':')
                return - 1;

        }
    }
    return 0;
}

static short ns_h2i(char c)
{
    if (c >= '0' && c <= '9')
        return (short)(c - '0');

    if (c >= 'A' && c <= 'F')
        return (short)(c - 'A' + 10);

    if (c >= 'a' && c <= 'f')
        return (short)(c - 'a' + 10);

    return - 1;
}

static void ns_phy_put(struct atm_dev *dev, unsigned char value,
                       unsigned long addr)
{
    ns_dev          *card;
    unsigned long   flags;

    card = dev->dev_data;
    ns_grab_res_lock(card, flags);
    waitfor_notbusy(card);
    writel((unsigned long) value, card->membase + DR0);
    writel(NS_CMD_WRITE_UTILITY | 0x00000100 | (addr & 0x000000FF),
           card->membase + CMD);
    spin_unlock_irqrestore(&card->res_lock, flags);
}

static unsigned char ns_phy_get(struct atm_dev *dev, unsigned long addr)
{
    ns_dev          *card;
    unsigned long   flags;
    unsigned long   data;

    card = dev->dev_data;
    ns_grab_res_lock(card, flags);
    waitfor_notbusy(card);
    writel(NS_CMD_READ_UTILITY | 0x00000100 | (addr & 0x000000FF),
           card->membase + CMD);
    waitfor_notbusy(card);
    data = readl(card->membase + DR0)& 0x000000FF;
    spin_unlock_irqrestore(&card->res_lock, flags);
    return (unsigned char) data;
}

static u8
ns_eeprom_byte_rd(ns_dev *card, u32 address)
{
    int             i, value = 0, command = 3;
    volatile        u32 gp = readl(card->membase + GP) & 0xfffffff8;

    udelay(5);                                         /* make sure idle */
    writel (gp | NS_GP_EECLK | NS_GP_EECS, card->membase + GP);         /* CS and Clock high */
    udelay(5);
    /* toggle in  READ CMD (00000011) */
    for (i=7; i >= 0; i--) {
        writel (gp | ((command >> i)& 1), card->membase + GP);           /* Clock low */
        udelay(5);
        writel (gp | NS_GP_EECLK | ((command >> i)& 1), card->membase + GP);    /* Clock high */
        udelay(5);
    }
    /* toggle in address */
    for (i = 7; i >= 0; i--) {
        writel (gp | ((address >> i)& 1), card->membase + GP);          /* Clock low */
        udelay(5);
        writel (gp | NS_GP_EECLK | ((address >> i)& 1), card->membase + GP);   /* Clock high */
        udelay(5);
    }
    /* read EEPROM data */
    for (i = 7; i >= 0; i--) {
        writel (gp, card->membase + GP);                                  /* Clock low */
        udelay(5);
        value |= (readl(card->membase + GP) & NS_GP_EEDI) >> (16 - i);
        writel (gp | NS_GP_EECLK, card->membase + GP);                     /* Clock high */
        udelay(5);
    }
    writel (gp, card->membase + GP);                                      /* CS and Clock low */
    return ((u8)value);
}

static void
ns_eeprom_rd (ns_dev *card, u32 ee_add, u8 *memptr, u32 len)
{
    while (len-- >0)
        *memptr++ = ns_eeprom_byte_rd(card, ee_add++);
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Christian Bucari <bucari@prosum.net>");
MODULE_DESCRIPTION("NICSTAR2 Driver for PROATM cards");

#if LINUX_VERSION_CODE >= KERNEL_VERSION (2,6,0)
module_init(nicstar_init);
module_exit(nicstar_cleanup);
#endif

