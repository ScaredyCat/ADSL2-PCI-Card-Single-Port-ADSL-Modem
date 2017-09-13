/*
 * Driver for the Synopsys 3884-0 DWC USB 2.0 HS OTG Subsystem-AHB core.
 * Specs and errata are available from http://www.synopsys.com.
 *
 * Synopsys Inc. supported the development of this driver.
 *
 */

/*
 * Copyright (C) 2006 Christian Schroeder, EMSYS GmbH, Ilmenau, Germany
 * Copyright (C) 2006 EMSYS GmbH, ILmenau, Germany, GmbH (http://www.emsys.de) 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/version.h>
#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/smp_lock.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/usb_ch9.h>
#include <linux/usb_gadget.h>
#include <asm/bitops.h>
#include <asm/byteorder.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/danube/irq.h>
#include <asm/danube/danube.h>
#include <asm/system.h>
#include <asm/unaligned.h>

#include "dwc3884.h"

static int handler_counter = 0;

/* forward declarations */
static void ep_disable(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, BOOLEAN stall);
static void finalize_set_address(GADGET_NAME(__GADGET,_dev*) dev);
static void set_interrupt(GADGET_NAME(__GADGET,_dev*) dev, BOOLEAN onoff);
static void handle_connect(GADGET_NAME(__GADGET,_dev*) dev);
static void handle_disconnect(GADGET_NAME(__GADGET,_dev*) dev);

/*------- Part 1: ------------ module specific configuration -----------------*/

#define   DRIVER_DESC     "Synopsys DWC 3884 USB OTG Controller"
#define   DRIVER_VERSION  "2006 Jan 17"

static int csr_base = CSR_BASE;
static int irq_num  = INT_NUM_IM1_IRL22;
static int fsiz_perio_ep = FIFO_SIZE_PERIODIC;

static const char driver_name[] = GADGET_STRING(__GADGET);
static const char driver_desc[] = DRIVER_DESC;

/* a tasklet for connect/disconnect debounce */
static void connect_handler(unsigned long data);
static struct tasklet_struct tl_connect;

static GADGET_NAME(__GADGET,_dev*) the_controller;
static struct usb_ep_ops GADGET_NAME(__GADGET,_ep_ops);
static struct usb_gadget_ops GADGET_NAME(__GADGET,_gadget_ops);

/* up to 15 universal endpoints, depending on core configuration, we assume the
 * following default assignment:
 *
 * ep0 (0x00) : control endpoint (bidirectional)
 * ep1 (0x81) : bulk endpoint (in direction)
 * ep2 (0x02) : bulk endpoint (out direction)
 * ep3 (0x83) : interrupt endpoint (in direction)
 *
 * this is sufficient for most used device classes which don't need
 * isochronous transactions (hid, msc-bulkonly, msc-cbi, cdc-acm, cdc-ether,
 * cdc-rndis, printer)
 *
 * ep4 ... ep15: not yet assigned universal endpoints
 *
 */
static const char ep0name [] = "ep0";
static const char *ep_name [] = {
  ep0name,
  "ep1in-bulk", "ep2out-bulk", "ep3in-int",   
  "ep4"       , "ep5"        , "ep6"      ,   
  "ep7"       , "ep8"        , "ep9"      ,
  "ep10"      , "ep11"       , "ep12"     ,
  "ep13"      , "ep14"       , "ep15"
};

static struct ep_config ep_config_table[] = {
/* mps  endpoint type              direction      buffersize */
  {64 , USB_ENDPOINT_XFER_CONTROL, USB_DIR_BIDIR, 64},
  {512, USB_ENDPOINT_XFER_BULK   , USB_DIR_IN   , 4096},
  {512, USB_ENDPOINT_XFER_BULK   , USB_DIR_OUT  , 4096},
  {32 , USB_ENDPOINT_XFER_INT    , USB_DIR_IN   , 32},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0},
  {0  , USB_ENDPOINT_XFER_NONE   , USB_DIR_NONE , 0}
};

/*
 * calculates the maximum packet size dependent on configuration and device
 * speed grade.
 *
 */
static unsigned int config_mps(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, unsigned int pref)
{
  unsigned int sugg;

  if (!pref) {
    sugg = ep_config_table[epnum].mps;
  }
  else {
    sugg = pref;
  }

  if (epnum > EP_COUNT) {
    return 0;
  }

  switch (ep_config_table[epnum].type) {
    case USB_ENDPOINT_XFER_CONTROL:
       if (dev->gadget.speed == USB_SPEED_HIGH) {
         return 64;
       }
       else if (dev->gadget.speed == USB_SPEED_FULL) {
         if (sugg > 0 && sugg <= 8) {
           return MIN(8,ep_config_table[epnum].mps);
         }
         else if (sugg > 8 && sugg <= 16) {
           return MIN(16,ep_config_table[epnum].mps);
         }
         else if (sugg > 16 && sugg <= 32) {
           return MIN(32,ep_config_table[epnum].mps);
         }
         else if (sugg > 32) {
           return MIN(64,ep_config_table[epnum].mps);
         }
         else {
           return 8;
         }
       }
       else if (dev->gadget.speed == USB_SPEED_LOW) {
         return MIN(8,ep_config_table[epnum].mps);
       }
       else {
         return 64;
       }
      break;
    case USB_ENDPOINT_XFER_BULK:
        if (dev->gadget.speed == USB_SPEED_HIGH) {
          return MIN(512,ep_config_table[epnum].mps);
        }
        else if (dev->gadget.speed == USB_SPEED_FULL) {
          return MIN(64,ep_config_table[epnum].mps);
        }
        else {
          return 64;
        }
      break;
    case USB_ENDPOINT_XFER_INT:
        return MIN(sugg,ep_config_table[epnum].mps);
      break;
    case USB_ENDPOINT_XFER_ISOC:
        /* not yet supported */
        return 0;
      break;
    default:
      return 0;
      break;
  }
};

/*
 * calculates the maximum transfer size for an endpoint
 *
 */
static unsigned int config_xfsz(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  switch (ep_config_table[epnum].type) {
    case USB_ENDPOINT_XFER_CONTROL:
    case USB_ENDPOINT_XFER_BULK:
    case USB_ENDPOINT_XFER_INT:
      return MIN(dev->tflen,ep_config_table[epnum].buflen);
      break;
    case USB_ENDPOINT_XFER_ISOC:
      return 0;
      break;
    default:
      return 0;
      break;
  }
}

/*
 * returns the tx buffer of an endpoint
 *
 */
static volatile byte __iomem* ep_tx_buffer(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  /* not needed for dma mode, for non dma mode we have to allocate a piece
     of memory for each endpoint, could be done by allocating a bigger buffer
     for the device and dividing it into parts by specifying the appropriate
     offsets */
  if (!epnum) {
    return (((volatile byte __iomem*)dev->buf)+64);
  }
  else {
   return 0;
  }
}

/*
 * returns the rx buffer of an endpoint
 *
 */
static volatile byte __iomem* ep_rx_buffer(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  /* not needed for dma mode, for non dma mode we have to allocate a piece
     of memory for each endpoint, could be done by allocating a bigger buffer
     for the device and dividing it into parts by specifying the appropriate
     offsets */
  return 0;
}

/*
 * returns the setup buffer of the contol endpoint
 *
 */
static volatile byte* ep0_setup_buffer(GADGET_NAME(__GADGET,_dev*) dev)
{
  return ((volatile byte*)(dev->buf));
}

/*------- Part 2: ------------ hw specific fifo and transfer handling --------*/

/*
 * sets the device address when the status stage of the set_address request
 * is completed (hw sepcific)
 *   
 */
static void finalize_set_address(GADGET_NAME(__GADGET,_dev*) dev)
{
  DEBUG(dev,"set address (second part, %d).\n",dev->addr);
  write_slice(DCFG_DEV_ADDR_LBIT,DCFG_DEV_ADDR_HBIT,&(dev->usbd_regs->dcfg),dev->addr);
  dev->usb_state = USB_STATE_ADDRESS;
}

/*
 * initiates or cancels a global non periodic out NAK sequence
 *
 */
static void glbl_out_nak(GADGET_NAME(__GADGET,_dev*) dev, byte onoff)
{
  if (onoff) {
    set_rbit(DCTL_SGOUT_NAK_BIT,&(dev->usbd_regs->dctl));
  }
  else {
    set_rbit(DCTL_CGOUT_NAK_BIT,&(dev->usbd_regs->dctl));
  }
}

/*
 * initiates or cancels a global non periodic in NAK sequence
 *
 */
static void glbl_np_in_nak(GADGET_NAME(__GADGET,_dev*) dev, byte onoff)
{
  if (onoff) {
    set_rbit(DCTL_SGNPIN_NAK_BIT,&(dev->usbd_regs->dctl));
  }
  else {
    set_rbit(DCTL_CGNPIN_NAK_BIT,&(dev->usbd_regs->dctl));
  }
}

/*
 * helper function: reset the assignment between periodic in endpoints
 * and their related tx fifo numbers. Should be called only in basic
 * endpoint init during the drivers start sequence.
 *
 */
static void reset_tx_fifo_assignment(GADGET_NAME(__GADGET,_dev*) dev)
{
  int i;
  for (i=1; i<=dev->tx_fifo_num_max; i++) {
    dev->tx_fifo_num[i] = 0;
  }
}

/*
 * helper functions: assigns a tx fifo number to a periodic in endpoint,
 * used in ep_reset(). A periodic IN endpoint needs an associated tx fifo
 * number for operation.
 *
 */
static byte assign_tx_fifo(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  int i;

  for (i=1; i<=dev->tx_fifo_num_max; i++) {
    /* already assigned or still free */
    if (dev->tx_fifo_num[i] == epnum || !dev->tx_fifo_num[i]) {
      dev->tx_fifo_num[i] = epnum;
      write_slice(DIEPCTL_TX_FNUM_LBIT,DIEPCTL_TX_FNUM_HBIT,
                  &(dev->ep[epnum].in_regs->diepctl),i);
      return i;
    }
  }
  return 0;
}

/*
 * tries to wakeup the host connected to this controller (hw specific)
 *
 */
static void remote_wakeup(GADGET_NAME(__GADGET,_dev*) dev)
{
#if 0
  set_rbit(DCTL_RMT_WKUP_SIG_BIT,&(dev->usbd_regs->dctl));
#endif
}

/*
 *
 *
 */
static unsigned long frame_number(GADGET_NAME(__GADGET,_dev*) dev)
{
  return read_slice(DSTS_SOF_FN_LBIT,DSTS_SOF_FN_HBIT,&(dev->usbd_regs->dsts));
}

/*
 * count last number of transferred bytes for a specific endpoint (hw specific)
 *
 */
#define TRANSFER_COUNTER_MODE  USE_XFER_SIZE
static unsigned long transferred(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  GADGET_NAME(__GADGET,_td*) td = 0;
  GADGET_NAME(__GADGET,_ep*) ep = &dev->ep[epnum];
  unsigned long tf;
  
  /* FIXME: redundant information from xfer_size and packet_count, but
            while xfer_size is decremented during transfer between external
            memory and fifo memory, packet count ist decrementd during 
            transfer between fifo memory and the usb bus ??? Because both 
            fields are not decremented byte wise, but packet wise, after a 
            successful transfer both register slices should be zero. Read 
            manual carefully! */
  
  /* in and out ep regs are seperate */
  if (!epnum) {
    td = list_entry(ep->queue.next,GADGET_NAME(__GADGET,_td),queue);
    if (TEST_FLAG(TD_IS_CTRL_OUT,&(td->flags))) {
#if (TRANSFER_COUNTER_MODE == USE_XFER_SIZE)
      tf = read_slice(DOEPTSIZ_XFER_SIZE_LBIT, DOEPTSIZ_XFER_SIZE_HBIT_0,
                      &(ep->out_regs->doeptsiz));
#else
      tf = read_slice(DOEPTSIZ_PKT_CNT_LBIT, DOEPTSIZ_PKT_CNT_HBIT_0,
                      &(ep->out_regs->doeptsiz));
#endif
    }
    else {
#if (TRANSFER_COUNTER_MODE == USE_XFER_SIZE)
      tf = read_slice(DIEPTSIZ_XFER_SIZE_LBIT,DIEPTSIZ_XFER_SIZE_HBIT_0,
                      &(ep->in_regs->dieptsiz));
#else
      tf = read_slice(DIEPTSIZ_PKT_CNT_LBIT,DIEPTSIZ_PKT_CNT_HBIT_0,
                      &(ep->in_regs->dieptsiz));
#endif
    }
  }

  else {
#if (TRANSFER_COUNTER_MODE == USE_XFER_SIZE)
    if (ep->dir == USB_DIR_OUT) {
      tf = read_slice(DOEPTSIZ_XFER_SIZE_LBIT,DOEPTSIZ_XFER_SIZE_HBIT_N,
                      &(ep->out_regs->doeptsiz));
#else
      tf = read_slice(DOEPTSIZ_PKT_CNT_LBIT,DOEPTSIZ_PKT_CNT_HBIT_N,
                      &(ep->out_regs->doeptsiz));    
#endif
    }
    else {
#if (TRANSFER_COUNTER_MODE == USE_XFER_SIZE)
      tf = read_slice(DIEPTSIZ_XFER_SIZE_LBIT,DIEPTSIZ_XFER_SIZE_HBIT_N,
                      &(ep->in_regs->dieptsiz));
#else
      tf = read_slice(DIEPTSIZ_PKT_CNT_LBIT,DIEPTSIZ_PKT_CNT_HBIT_N,
                      &(ep->in_regs->dieptsiz));
#endif
    }
  }
  return tf;
}

/*
 * arms the rx/tx fifo and starts dma transfers immediately, in case of dma
 * and of course dma aware td buffers no memcopy operations and no intermediate 
 * ep buffers are needed 
 *
 */
static void arm_fifo(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, u8 dir)
{
  GADGET_NAME(__GADGET,_td*) td;

  td = list_entry(dev->ep[epnum].queue.next,GADGET_NAME(__GADGET,_td),queue);

  td->packets = td->length / dev->ep[epnum].ep.maxpacket;
  if(td->length % dev->ep[epnum].ep.maxpacket) td->packets++;

  if (dir == USB_DIR_IN) {
    /* arm_fifo for ep0 */
    if(epnum == 0) {
      /* initialize registers for dma in transfer */
      write_slice(DIEPTSIZ_XFER_SIZE_LBIT,DIEPTSIZ_XFER_SIZE_HBIT_0,
                  &(dev->ep[0].in_regs->dieptsiz),td->length);

      set_rbit(DIEPTSIZ_PKT_CNT_HBIT_0,&(dev->ep[0].in_regs->dieptsiz));

    }
    /* arm_fifo for ep other than ep0 */
    else {
      /* initialize registers for dma in transfer */
      write_slice(DIEPTSIZ_XFER_SIZE_LBIT,DIEPTSIZ_XFER_SIZE_HBIT_N,
                  &(dev->ep[epnum].in_regs->dieptsiz),td->length);

      write_slice(DIEPTSIZ_PKT_CNT_LBIT,DIEPTSIZ_PKT_CNT_HBIT_N,
                  &(dev->ep[epnum].in_regs->dieptsiz),td->packets);
      write_slice(DIEPTSIZ_MC_LBIT,DIEPTSIZ_MC_HBIT,&(dev->ep[epnum].in_regs->dieptsiz),1);
    }

    /* invalidate dma cache */
    if (td->req) {
      if (td->req->req.buf && td->req->req.length) {
        dma_cache_wback_inv((unsigned long) td->req->req.buf, td->req->req.length );
      }
    }
    else if (td->buf && td->length) {
        dma_cache_wback_inv((unsigned long) td->buf, td->length );
    }

    /* set the dma address */
    write_reg(&(dev->ep[epnum].in_regs->diepdma),td->dma);

    /* clear NAK bit and enable ep for transmit*/
    set_rbit(DIEPCTL_CNAK_BIT,&(dev->ep[epnum].in_regs->diepctl));
    set_rbit(DIEPCTL_EP_ENA_BIT,&(dev->ep[epnum].in_regs->diepctl));
  }
  else {
    /* initialize registers for dma out transfer */
    if( epnum == 0 ) {
      write_slice(DOEPTSIZ_XFER_SIZE_LBIT,DOEPTSIZ_XFER_SIZE_HBIT_0,
                  &(dev->ep[0].out_regs->doeptsiz),td->length);

      set_rbit(DOEPTSIZ_PKT_CNT_HBIT_0,&(dev->ep[0].out_regs->doeptsiz));
    }
    else {

      write_slice(DOEPTSIZ_XFER_SIZE_LBIT,DOEPTSIZ_XFER_SIZE_HBIT_N,
                  &(dev->ep[epnum].out_regs->doeptsiz),td->length);

      write_slice(DOEPTSIZ_PKT_CNT_LBIT,DOEPTSIZ_PKT_CNT_HBIT_N,
                  &(dev->ep[epnum].out_regs->doeptsiz),td->packets);
    }

    /* invalidate dma cache */
    if (td->req) {
      if (td->req->req.buf && td->req->req.length) {
        dma_cache_wback_inv((unsigned long) td->req->req.buf, td->req->req.length );
      }
    }
    else if (td->buf && td->length) {
        dma_cache_wback_inv((unsigned long) td->buf, td->length );
    }

    write_reg(&(dev->ep[epnum].out_regs->doepdma),td->dma);

    /* clear NAK bit and enable ep for tranmit*/
    set_rbit(DOEPCTL_CNAK_BIT,&(dev->ep[epnum].out_regs->doepctl));
    set_rbit(DOEPCTL_EP_ENA_BIT,&(dev->ep[epnum].out_regs->doepctl));

    DEBUG(dev,"arm out fifo: buf=%p, len=%ld.\n",td->buf, td->length);
  } 
}

/*------- Part 3: ------------ hw independent transfer and queue handling ----*/

#include "common/gadget_xfer_handling.c"

/*------- Part 4: ------------ endpoint handling -----------------------------*/

/*
 * determines if a specific ep is a non periodic in ep
 *
 */
static BOOLEAN ep_is_np_in(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  if (!epnum ||
      (dev->ep[epnum].dir == USB_DIR_IN && 
       dev->ep[epnum].type == USB_ENDPOINT_XFER_BULK)) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/*
 * returns TRUE if an ep is stopped (hw independent)
 *
 */
static BOOLEAN ep_stopped(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  if (TEST_FLAG(EP_IS_STOPPED,&(dev->ep[epnum].flags))) {
    return TRUE;
  }
  else {
    return FALSE;
  }  
}

/*
 * starts an endpoint (hw independent)
 *
 */
static void ep_start(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  if (TEST_FLAG(EP_IS_STOPPED,&(dev->ep[epnum].flags))) {
    DEBUG(dev,"ep %d started.\n",epnum);
    CLEAR_FLAG(EP_IS_STOPPED,&(dev->ep[epnum].flags));
  }
}

/*
 * stops an endpoint (hw independent)
 */
static void ep_stop(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  if (!TEST_FLAG(EP_IS_STOPPED,&(dev->ep[epnum].flags))) {
    DEBUG(dev,"ep %d stopped.\n",epnum);
    SET_FLAG(EP_IS_STOPPED,&(dev->ep[epnum].flags));
  }
}

/*
 * returns TRUE if an ep is disabled/stalled (hw specific)
 *
 */
static BOOLEAN ep_deactivated(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  volatile u32* regptr = 0;
  u8            regbit;

  if (!epnum) {
    return FALSE;
  }

  if (dev->ep[epnum].dir == USB_DIR_IN) {
    regptr = &(dev->ep[epnum].in_regs->diepctl);
    regbit = DIEPCTL_USB_ACT_EP_BIT;
  }
  else {
    regptr = &(dev->ep[epnum].out_regs->doepctl);
    regbit = DOEPCTL_USB_ACT_EP_BIT;    
  }

  /* check hardware */
  if (!test_rbit(regbit,regptr)) {
    SET_FLAG(EP_IS_DEACTIVATED,&(dev->ep[epnum].flags));
    return TRUE;
  }
  else {
    CLEAR_FLAG(EP_IS_DEACTIVATED,&(dev->ep[epnum].flags));
    return FALSE;
  }
}


/*
 * activates an endpoint, used in usb_ep_enable (not applicable for ep0)
 *
 */
static void ep_activate(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  volatile u32* regptr = 0;
  u8            regbit;

  DEBUG(dev,"ep %d activated.\n",epnum);

  if (!epnum) {
    return;
  }

  if (dev->ep[epnum].dir == USB_DIR_IN) {
    regptr = &(dev->ep[epnum].in_regs->diepctl);
    regbit = DIEPCTL_USB_ACT_EP_BIT;
  }
  else {
    regptr = &(dev->ep[epnum].out_regs->doepctl);
    regbit = DOEPCTL_USB_ACT_EP_BIT;    
  }

  set_rbit(regbit,regptr);
  CLEAR_FLAG(EP_IS_DEACTIVATED,&(dev->ep[epnum].flags));
}

/*
 * deactivates an endpoint, used in usb_ep_disable (not applicable for ep0)
 *
 */
static void ep_deactivate(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  volatile u32* regptr = 0;
  u8            regbit;

  DEBUG(dev,"ep %d deactivated.\n",epnum);

  if (!epnum) {
    return;
  }

  if (dev->ep[epnum].dir == USB_DIR_IN) {
    regptr = &(dev->ep[epnum].in_regs->diepctl);
    regbit = DIEPCTL_USB_ACT_EP_BIT;
  }
  else {
    regptr = &(dev->ep[epnum].out_regs->doepctl);
    regbit = DOEPCTL_USB_ACT_EP_BIT;    
  }

  clear_rbit(regbit,regptr);
  SET_FLAG(EP_IS_DEACTIVATED,&(dev->ep[epnum].flags));
}

/*
 * returns TRUE if an ep is disabled/stalled (hw specific) 
 *
 */
static BOOLEAN ep_disabled(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, BOOLEAN stall)
{
  volatile u32* regptr = 0;
  u8            regbit;
  u8            flag;
  int           ep0_out_stall = 0;  
  int           ep0_out_disbl = 0;

  if (stall) {
    flag = EP_IS_STALLED;
  }
  else {
    flag = EP_IS_DISABLED;
  }
  
  /* in most cases, it is suffcient to stall ep0 in IN direction */
  if (!epnum || dev->ep[epnum].dir == USB_DIR_IN) {
    if (stall) {
      regbit = DIEPCTL_STALL_BIT;
    }
    else {
      regbit = DIEPCTL_EP_DIS_BIT;
    }
    regptr = &(dev->ep[epnum].in_regs->diepctl);
  }
  else {
    if (stall) {
      regbit = DOEPCTL_STALL_BIT;
    }
    else {
      regbit = DOEPCTL_EP_DIS_BIT;
    }
    regptr = &(dev->ep[epnum].out_regs->doepctl);
  }

  /* FIXME: do we need to check if ep0 is stalled/disabled in out direction ? */
  if (!epnum && stall) {
    if (test_rbit(DOEPCTL_STALL_BIT,&(dev->ep[epnum].out_regs->doepctl))) {
      ep0_out_stall = 1;
    }
  }
  else if (!epnum && !stall) { 
    if (test_rbit(DOEPCTL_STALL_BIT,&(dev->ep[epnum].out_regs->doepctl))) {
      ep0_out_disbl = 1;
    }
  }

  if (test_rbit(regbit,regptr) || ep0_out_stall || ep0_out_disbl) {
    SET_FLAG(flag,&(dev->ep[epnum].flags));
    return TRUE;
  }
  else {
    CLEAR_FLAG(flag,&(dev->ep[epnum].flags));
    return FALSE;
  }
}

/*
 * enables an endpoint, if destalling, the STALL bit is cleared additionally .
 *
 */
static void ep_enable(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, BOOLEAN destall)
{
  if (!destall) {
    DEBUG(dev,"ep %d enabled.\n",epnum);
    /* no further action needed, the NAK is cleared during fifo arming */
    CLEAR_FLAG(EP_IS_DISABLED,&(dev->ep[epnum].flags));
  }
  else {
    DEBUG(dev,"ep %d destalled.\n",epnum);
    /* ep0 usually stalled in IN direction, but probably we can destall it
       in both directions */
    if (!epnum) {
      clear_rbit(DIEPCTL_STALL_BIT,&(dev->ep[0].in_regs->diepctl));
      clear_rbit(DOEPCTL_STALL_BIT,&(dev->ep[0].out_regs->doepctl));
    }
    /* out endpoint */
    else if (dev->ep[epnum].dir == USB_DIR_OUT) {
      clear_rbit(DOEPCTL_STALL_BIT,&(dev->ep[epnum].out_regs->doepctl));
      if (dev->ep[epnum].type == USB_ENDPOINT_XFER_BULK ||
          dev->ep[epnum].type == USB_ENDPOINT_XFER_INT) {
        set_rbit(DOEPCTL_SET_D0PID_BIT,&(dev->ep[epnum].out_regs->doepctl));
      }
    }
    else if (dev->ep[epnum].dir == USB_DIR_IN) {
      clear_rbit(DIEPCTL_STALL_BIT,&(dev->ep[epnum].in_regs->diepctl));
      if (dev->ep[epnum].type == USB_ENDPOINT_XFER_BULK ||
          dev->ep[epnum].type == USB_ENDPOINT_XFER_INT) {
        set_rbit(DIEPCTL_SET_D0PID_BIT,&(dev->ep[epnum].in_regs->diepctl));
      }
    }
    CLEAR_FLAG(EP_IS_STALLED,&(dev->ep[epnum].flags));
  }
  ep_start(dev,epnum);
}

/*
 * disables/stalls an endpoint, this routine doesn't use the buggy
 * nak_effective and ep_disabled interrupts but poll the appropriate
 * register bits instead.
 *    
 */
static void ep_disable(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, BOOLEAN stall)
{
  int i;
  
  if (!epnum && !stall) {
      /* disabling ep0 not possible */
      return;
  }

  if (!stall) {
    DEBUG(dev,"disabling ep%d.\n",epnum);
  }
  else {
    DEBUG(dev,"stalling ep%d.\n",epnum);
  }

  ep_stop(dev,epnum);

    /* other non periodic IN ep */
  if (ep_is_np_in(dev,epnum)) {
    /* 1. set global np in nak and wait for completion. */
    glbl_np_in_nak(dev,1);
    while(!test_rbit(DCTL_GNPIN_NAK_STS_BIT,&(dev->usbd_regs->dctl)));
    /* 2. disable all np in endpoints and wait for completion. */
    for(i=0;i<dev->epcnt;i++) {
      if (ep_is_np_in(dev,i)) {
        /* use the disable bit only if ena bit was set previously */
        if (test_rbit(DIEPCTL_EP_ENA_BIT,&(dev->ep[i].in_regs->diepctl))) {
          set_rbit(DIEPCTL_EP_DIS_BIT,&(dev->ep[i].in_regs->diepctl));          
          while(test_rbit(DIEPCTL_EP_ENA_BIT,&(dev->ep[i].in_regs->diepctl)) ||
                test_rbit(DIEPCTL_EP_DIS_BIT,&(dev->ep[i].in_regs->diepctl)));
        }
        set_rbit(DIEPCTL_SNAK_BIT,&(dev->ep[i].in_regs->diepctl));
        if ((epnum == i) && stall) {
          /* set stall bit additionally if needed */
          set_rbit(DIEPCTL_STALL_BIT,&(dev->ep[i].in_regs->diepctl));
        }
      }
    }
    /* 3. flush np in fifo and wait for completion. */
    write_slice(GRSTCTL_TX_FNUM_LBIT,GRSTCTL_TX_FNUM_HBIT,
                &(dev->core_regs->grstctl),0);
    set_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl));
    while(test_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl)));      
    /* 4. reenable other np in endpoints */
    for(i=0;i<dev->epcnt;i++) {
      if (ep_is_np_in(dev,i)) {
        if (i != epnum) {
          clear_rbit(DIEPCTL_SNAK_BIT,&(dev->ep[i].in_regs->diepctl));
        }
      }
    }      
    /* 5. clear global np in nak */         
    glbl_np_in_nak(dev,0);
  }
  /* periodic IN ep */
  else if (dev->ep[epnum].dir == USB_DIR_IN) {
    /* 1. set nak bit and wait for completion */
    set_rbit(DIEPCTL_SNAK_BIT,&(dev->ep[epnum].in_regs->diepctl));
    while(!test_bit(DIEPCTL_NAK_STS_BIT,&(dev->ep[epnum].in_regs->diepctl)));
    /* 2. set disable bit and wait for completion, use the dis bit only if
          the ena bit was set previously */
    if (test_rbit(DIEPCTL_EP_ENA_BIT,&(dev->ep[epnum].in_regs->diepctl))) {
      set_rbit(DIEPCTL_EP_DIS_BIT,&(dev->ep[epnum].in_regs->diepctl));          
      while(test_rbit(DIEPCTL_EP_ENA_BIT,&(dev->ep[epnum].in_regs->diepctl)) ||
            test_rbit(DIEPCTL_EP_DIS_BIT,&(dev->ep[epnum].in_regs->diepctl)));
    }
    if (stall) {
      /* set stall bit additionally if needed */
      set_rbit(DIEPCTL_STALL_BIT,&(dev->ep[epnum].in_regs->diepctl));
    }
    /* flush the single fifo */
    write_slice(GRSTCTL_TX_FNUM_LBIT,GRSTCTL_TX_FNUM_HBIT,
                &(dev->core_regs->grstctl),dev->ep[epnum].fifo_num);
    set_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl));
    while(test_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl)));
  }
  /* non periodic OUT ep */
  else if ((dev->ep[epnum].type == USB_ENDPOINT_XFER_BULK) &&
           (dev->ep[epnum].dir == USB_DIR_OUT)) {
    /* 1. set global np out nak and wait for completion */
    glbl_out_nak(dev,1);
    while(!test_rbit(DCTL_GOUT_NAK_STS_BIT,&(dev->usbd_regs->dctl)));

    /* 2. disable endpoint and wait for completion */
    if (test_rbit(DOEPCTL_EP_ENA_BIT,&(dev->ep[epnum].out_regs->doepctl))) {
      set_rbit(DOEPCTL_EP_DIS_BIT,&(dev->ep[epnum].out_regs->doepctl));          
    }
    while(test_rbit(DOEPCTL_EP_ENA_BIT,&(dev->ep[epnum].out_regs->doepctl)) ||
          test_rbit(DOEPCTL_EP_DIS_BIT,&(dev->ep[epnum].out_regs->doepctl)));
    set_rbit(DOEPCTL_SNAK_BIT,&(dev->ep[epnum].out_regs->doepctl));
    if (stall) {
      /* set stall bit additionally if needed */
      set_rbit(DOEPCTL_STALL_BIT,&(dev->ep[epnum].out_regs->doepctl));
    }    
    /* 3. clear global np out nak */
    glbl_out_nak(dev,0);
  }
  else {
    /* periodic out endpoints e.g. isochronous */
    ERROR(dev,"endpoint type not yet supported.\n");
    return;
  }

  if (!stall) {
    SET_FLAG(EP_IS_DISABLED,&(dev->ep[epnum].flags));
  }
  else {
    SET_FLAG(EP_IS_STALLED,&(dev->ep[epnum].flags));
  }
}

/*
 * test if an endpoint is usable for queuing (hw independent)
 *
 */
static BOOLEAN ep_ok(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  if (ep_stopped(dev,epnum)) {
    WARN(dev,"ep %d not ok (stopped).\n",epnum);
    return FALSE;
  }
  else if (ep_deactivated(dev,epnum)) {
    WARN(dev,"ep %d not ok (deactivated).\n",epnum);
    return FALSE;
  }
  else if (ep_disabled(dev,epnum,EP_NO_STALL)) {
    WARN(dev,"ep %d not ok (disabled).\n",epnum);
    return FALSE;
  }
  else if (ep_disabled(dev,epnum,EP_STALL)) {
    WARN(dev,"ep %d not ok (stalled).\n",epnum);
    return FALSE;
  }
  else {
    DEBUG(dev,"ep %d ok.\n",epnum);
    return TRUE;
  } 
}

/*
 * flushes an endpoint, used in usb_ep_fifo_flush routine, not yet
 * implemented. The dwc3884 core supports only the selective flushing
 * of a periodic tx fifo, the flushing of the common non periodic
 * transmit fifo or the flushing of the common rx fifo. This means,
 * the core doesn't always support the selective flushing of a single
 * non periodic endpoint.
 *
 */
static void ep_flush(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  DEBUG(dev,"ep %d flushed.\n",epnum);
#if 0
  if (dev->ep[epnum].dir == USB_DIR_IN) {
    write_slice(GRSTCTL_TX_FNUM_LBIT,GRSTCTL_TX_FNUM_HBIT,
                &(dev->core_regs->grstctl),dev->ep[epnum].fifo_num);
    set_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl));
    while(test_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl)));
  }
  else {
    /* FIXME: don't know how and why to flush OUT fifo */
  }
#endif
}

/*
 * count available space in an endpoint, used in usb_ep_fifo_status routine,
 * not yet implemented. See the note above in ep_flush().
 *
 */
static unsigned long ep_count(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  DEBUG(dev,"ep %d counted.\n",epnum);
  return 0;
  /* FIXME: don't know how to count the available bytes related to any
            endpoint, because there is no 1:1 endpoint_to_fifo mapping. */
}

/*
 * resets an endpoint (excluding ep0), should be used by usb_ep_enable
 * before endpoint is (re)started.
 *
 */
static ERROR ep_reset(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  ERROR retval = OK;
  
  DEBUG(dev,"ep %d reset.\n",epnum);

  if (!epnum) {
    return OK;
  }

  if ((dev->ep[epnum].desc)->bEndpointAddress & USB_DIR_MASK) {
    dev->ep[epnum].dir = USB_DIR_IN;
  }
  else {
    dev->ep[epnum].dir = USB_DIR_OUT;
  }

  dev->ep[epnum].type = dev->ep[epnum].desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK;
  dev->ep[epnum].fifo_size = 0;
  dev->ep[epnum].fifo_space = 0;
  dev->ep[epnum].ep.maxpacket = config_mps(dev,epnum,le16_to_cpu(dev->ep[epnum].desc->wMaxPacketSize));
  dev->ep[epnum].irqs = 0;

  if (dev->ep[epnum].dir == USB_DIR_IN) {

    /* assigns a tx fifo to an periodic in endpoint */
    if ((dev->ep[epnum].type == USB_ENDPOINT_XFER_ISOC) ||
        (dev->ep[epnum].type == USB_ENDPOINT_XFER_INT)) {
      dev->ep[epnum].fifo_num = assign_tx_fifo(dev,epnum);
      if (!dev->ep[epnum].fifo_num)
      {
        retval = 1;
      }
    }

    /* set registers: max packet size, start data toggle, ep type */
    write_slice(DIEPCTL_MPS_LBIT,DIEPCTL_MPS_HBIT_N,
                &(dev->ep[epnum].in_regs->diepctl),dev->ep[epnum].ep.maxpacket);
    set_rbit(DIEPCTL_SET_D0PID_BIT,&(dev->ep[epnum].in_regs->diepctl));
    write_slice(DIEPCTL_EP_TYPE_LBIT,DIEPCTL_EP_TYPE_HBIT,
                &(dev->ep[epnum].in_regs->diepctl),dev->ep[epnum].type);
  }
  else {
    dev->ep[epnum].dir = USB_DIR_OUT;
    /* set registers: max packet size, start data toggle, ep type */
    write_slice(DOEPCTL_MPS_LBIT,DOEPCTL_MPS_HBIT_N,
                &(dev->ep[epnum].out_regs->doepctl),dev->ep[epnum].ep.maxpacket);
    set_rbit(DOEPCTL_SET_D0PID_BIT,&(dev->ep[epnum].out_regs->doepctl));
    write_slice(DOEPCTL_EP_TYPE_LBIT,DOEPCTL_EP_TYPE_HBIT,
                &(dev->ep[epnum].out_regs->doepctl),dev->ep[epnum].type);
  }
  return retval;
}

/*
 * enable ep0 for receiving setup packets
 *
 */
static void ep0_enable_setup_stage(GADGET_NAME(__GADGET,_dev*) dev)
{
  DEBUG(dev,"(re)enabling ep0 for setup packets.\n");

  /* Sets the packet counter bit to one packet, the dwc3884 is not able
     to receive more than one data packet in one transaction. If more than
     64 bytes have to be transmitted over ep0, we will need more than one
     (dma) transactions */
  write_slice(DOEPTSIZ_PKT_CNT_LBIT,DOEPTSIZ_PKT_CNT_HBIT_0,
              &(dev->ep[0].out_regs->doeptsiz),1);

  /* Sets the transfer size for ep0, depends on the "enumerated" speed */
  write_slice(DOEPTSIZ_XFER_SIZE_LBIT,DOEPTSIZ_XFER_SIZE_HBIT_0,
              &(dev->ep[0].out_regs->doeptsiz),dev->ep[0].ep.maxpacket);

  /* Program the dma register with the setup buffer address */
  write_reg(&(dev->ep[0].out_regs->doepdma),dev->dma);

  /* Sets setup packet counter for ep0, this is the last step */
  write_slice(DOEPTSIZ_SUP_CNT_LBIT,DOEPTSIZ_SUP_CNT_HBIT,
              &(dev->ep[0].out_regs->doeptsiz),DOEPTSIZ_SUP_CNT_DEFAULT);

  /* enable the setup interrupt */
  set_rbit(DOEPMSK_SETUP_MSK,&(dev->usbd_regs->doepmsk));
  set_rbit(DIEPMSK_TIMEOUT_MSK,&(dev->usbd_regs->diepmsk));

  dma_cache_inv((unsigned long) dev->buf, EP0_BUFLEN);

  /* Enable ep0 for dma */
  set_rbit(DOEPCTL_EP_ENA_BIT,&(dev->ep[0].out_regs->doepctl));
}


/*
 * special case, stalling ep0 in out direction, this is needed,
 * if the host sends during dat stage of a control transfer more
 * data than specified in the setup packet. Not yet implemented,
 * because the core manual is not quite clear regading this, to be
 * done during integration.
 *
 */
#if 0
static void ep0_stall_out(GADGET_NAME(__GADGET,_dev*) dev)
{
  /* ToDo:
     - enable DIEPINT0.INTknTXFEmp and DOEPINT0.OUTTknEPDis Interrupt
     - wait on interrupt and set DOEPCTL0.STALL
     - clear Interrupt
   */
}
#endif

/*------- Part 5: ----------- queue routines ---------------------------------*/

#include "common/gadget_queue_handling.c"

/*------- Part 6: ----------- device init, reinit and reset ------------------*/

/*
 * forces connect by enabling the pullup resistor (hw specific)
 *
 */
static void set_pullup(GADGET_NAME(__GADGET,_dev*) dev, BOOLEAN onoff)
{
  if (onoff) {
    DEBUG(dev,"pullup on.\n");
    clear_rbit(DCTL_SFT_DISCON_BIT,&(dev->usbd_regs->dctl));
  }
  else {
    DEBUG(dev,"pullup off.\n");
    set_rbit(DCTL_SFT_DISCON_BIT,&(dev->usbd_regs->dctl));
  }
}

/*
 * clears pending interrupts (hw specific)
 *
 */
static void clear_irq_status(GADGET_NAME(__GADGET,_dev*) dev)
{
  int i,cnt;

  DEBUG(dev,"irq status cleared completely.\n");
  /* clear all interrupt status flags */
  write_reg(&(dev->core_regs->gotgint),0xffffffff);
  write_reg(&(dev->core_regs->gintsts),0xffffffff);
  cnt = read_slice(GHWCFG2_NUM_DEV_EPS_LBIT,GHWCFG2_NUM_DEV_EPS_HBIT,
                   &(dev->core_regs->ghwcfg2));
  for (i=0;i<cnt;i++) {
    write_reg(&(dev->ep[i].in_regs->diepint),0xffffffff);
    write_reg(&(dev->ep[i].out_regs->doepint),0xffffffff);
  }  
}

/*
 * enables/disables the global interrupt (hw specific)
 *
 */  
static void set_interrupt(GADGET_NAME(__GADGET,_dev*) dev, BOOLEAN onoff)
{
  if (onoff) {
    DEBUG(dev,"enable interrupt.\n");
    set_rbit(GAHBCFG_GLBL_INTR_MSK_BIT,&(dev->core_regs->gahbcfg)); /* global */
  }
  else {
    DEBUG(dev,"disable interrupt.\n");
    clear_rbit(GAHBCFG_GLBL_INTR_MSK_BIT,&(dev->core_regs->gahbcfg)); /* global */
  }
}

/*
 * basic endpoint setup, note that we can't determine the endpoints max packet
 * size here, because the device speed is only available after a chirp sequence  
 * 
 */
static void ep_init(GADGET_NAME(__GADGET,_dev*) dev)
{
  int i,cnt;
  GADGET_NAME(__GADGET,_ep*) ep;

  /* clear the gloabl_in_nak and the global_out_nak condition */
  glbl_out_nak(dev,0);
  glbl_np_in_nak(dev,0);

  INIT_LIST_HEAD(&dev->gadget.ep_list);

  cnt = read_slice(GHWCFG2_NUM_DEV_EPS_LBIT,GHWCFG2_NUM_DEV_EPS_HBIT,
                   &(dev->core_regs->ghwcfg2));
  dev->epcnt = cnt;
  /* reset tx fifo assignment for periodic IN endpoints */
  dev->tx_fifo_num_max = (unsigned short)read_slice(GHWCFG4_NUM_DEV_PERIO_EPS_LBIT,GHWCFG4_NUM_DEV_PERIO_EPS_HBIT,
                                            &(dev->core_regs->ghwcfg4));
  reset_tx_fifo_assignment(dev);
  /* basic endpoint init */
  for (i=0;i<cnt;i++) {
    ep = &dev->ep[i];
    ep->ep.name = ep_name[i];
    ep->ep.ops = &(GADGET_NAME(__GADGET,_ep_ops));
    ep->dev = dev;
    ep->irqs = 0;
    ep->desc = 0;
    ep->num = i;
    ep->fifo_size = 0;
    ep->fifo_space = 0;    
    ep->flags = 0;
    ep->xfstat.endpoint = ep;
    ep->dir =  ep_config_table[i].dir;
    ep->type = ep_config_table[i].type;
    ep->fifo_num = 0;
    SET_FLAG(EP_QUEUE_STOPPED,&(ep->flags));
    INIT_LIST_HEAD(&ep->queue);
    ep->ep.maxpacket = config_mps(dev,i,0);
    if (!i) {
      INIT_LIST_HEAD(&ep->ep.ep_list);
      CLEAR_FLAG(EP_IS_DEACTIVATED,&(dev->ep[0].flags));
      CLEAR_FLAG(EP_IS_DISABLED,&(dev->ep[0].flags));
      set_rbit(0,&(dev->usbd_regs->daint));
      set_rbit(16,&(dev->usbd_regs->daint));      
    }
    else {
      list_add_tail(&ep->ep.ep_list,&dev->gadget.ep_list);
      SET_FLAG(EP_IS_DISABLED,&ep->flags);
      SET_FLAG(EP_IS_DEACTIVATED,&ep->flags);

      /* DAINT is read-only! reset the diepint/doepint instad */
      if (ep->dir == USB_DIR_IN) {
        set_rbit(DIEPINT_XFER_COMPL_BIT,&(dev->ep[i].in_regs->diepint));
        set_rbit(DIEPINT_EP_DISBLD_BIT,&(dev->ep[i].in_regs->diepint));
        set_rbit(DIEPINT_AHB_ERR_BIT,&(dev->ep[i].in_regs->diepint));
        set_rbit(DIEPINT_TIMEOUT_BIT,&(dev->ep[i].in_regs->diepint));
        set_rbit(DIEPINT_IN_TKN_TXF_EMP_BIT,&(dev->ep[i].in_regs->diepint));
        set_rbit(DIEPINT_IN_TKN_EP_MIS_BIT,&(dev->ep[i].in_regs->diepint));
        set_rbit(DIEPCTL_CNAK_BIT,&(dev->ep[i].in_regs->diepctl));
      }
      else {
        set_rbit(DOEPINT_XFER_COMPL_BIT,&(dev->ep[i].out_regs->doepint));
        set_rbit(DOEPINT_EP_DISBLD_BIT,&(dev->ep[i].out_regs->doepint));
        set_rbit(DOEPINT_AHB_ERR_BIT,&(dev->ep[i].out_regs->doepint));
        set_rbit(DOEPINT_SETUP_BIT,&(dev->ep[i].out_regs->doepint));
        set_rbit(DOEPINT_OUTTKN_EP_DIS_BIT,&(dev->ep[i].out_regs->doepint));
      }
    }

    /* unmask the interrupt for every ep with a maxpacket size > 0 */
    if(ep_config_table[i].mps) {
       /* activate endpoint */
       if(ep->dir == USB_DIR_BIDIR) {
         set_rbit(i,&(dev->usbd_regs->daintmsk));
         set_rbit(i+16,&(dev->usbd_regs->daintmsk));
       }
       else if(ep->dir == USB_DIR_IN) {
         set_rbit(DIEPCTL_USB_ACT_EP_BIT,&(dev->ep[i].in_regs->diepctl));
         set_rbit(i,&(dev->usbd_regs->daintmsk));
       }
       else {
         set_rbit(DOEPCTL_USB_ACT_EP_BIT,&(dev->ep[i].out_regs->doepctl));
         set_rbit(i+16,&(dev->usbd_regs->daintmsk));
       }
    }

    /* FIXME: not sure if this is the right way. This assumes, that we have only ep0 and 
              one other non periodic (e.g. bulk) in endpoint */
    write_slice(DIEPCTL_NEXT_EP_LBIT,DIEPCTL_NEXT_EP_HBIT, &(dev->ep[0].in_regs->diepctl),1);
    write_slice(DIEPCTL_NEXT_EP_LBIT,DIEPCTL_NEXT_EP_HBIT, &(dev->ep[1].in_regs->diepctl),0);
  }

  /* provides ep0 to the gadget driver */
  dev->gadget.ep0 = &dev->ep[0].ep;
}

/*
 * basic ep0 init function, called from gadget_register_driver to prepare ep0
 * for receiving setup packets
 *
 */
static void ep0_start(GADGET_NAME(__GADGET,_dev*) dev)
{
  DEBUG(dev,"ep0 started.\n");

  ep_enable(dev,0,EP_DESTALL);

  /* enable interrupt */
  set_interrupt(dev,TRUE);

  /* FIXME: correct bit to determine port power, doesn't seem to work for the
            actual revision of the hardware. */
  if (test_rbit(GOTGCTL_B_SES_VLD_BIT,&(dev->core_regs->gotgctl))) {
    set_pullup(dev,TRUE);
  }

  /* reset status flags for ep0 */
  CLEAR_FLAG(EP_IS_STOPPED,&(dev->ep[0].flags));
  CLEAR_FLAG(EP_IS_DEACTIVATED,&(dev->ep[0].flags));
  CLEAR_FLAG(EP_IS_DISABLED,&(dev->ep[0].flags));
  CLEAR_FLAG(EP_IS_STALLED,&(dev->ep[0].flags));
}

/*
 * resets and disables the usb hardware (hw specific)
 *
 */
static void hw_reset(GADGET_NAME(__GADGET,_dev*) dev)
{
  DEBUG(dev,"hardware reset.\n");

  /* disable interrupt */
  set_interrupt(dev,FALSE);

  /* disable pullup */
  set_pullup(dev,FALSE);
  
  /* global interrupt sources */
  clear_bit(GINTMSK_MODE_MIS_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_OTG_INT_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_SOF_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_RX_FLV_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_NPTXF_EMPTY_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_GIN_NAK_EFF_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_GOUT_NAK_EFF_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_ULPICK_INT_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_I2C_INT_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_ERLY_SUSP_MSK_BIT,&(dev->core_regs->gintmsk));
  set_bit(GINTMSK_USB_SUSP_MSK_BIT,&(dev->core_regs->gintmsk));
  set_bit(GINTMSK_USB_RST_MSK_BIT,&(dev->core_regs->gintmsk));
  set_bit(GINTMSK_ENUM_DONE_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_ISO_OUT_DROP_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_EOPF_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_EP_MIS_MSK_BIT,&(dev->core_regs->gintmsk));
  set_bit(GINTMSK_INEP_INT_MSK_BIT,&(dev->core_regs->gintmsk));
  set_bit(GINTMSK_OEP_INT_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_INCOMP_IOS_IN_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_INCOMP_ISO_OUT_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_FET_SUSP_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_CONID_STS_CHNG_MSK_BIT,&(dev->core_regs->gintmsk));
  set_bit(GINTMSK_DISCONN_INT_MSK_BIT,&(dev->core_regs->gintmsk));
  clear_bit(GINTMSK_SESS_REQ_INT_MSK_BIT,&(dev->core_regs->gintmsk));
  set_bit(GINTMSK_WKUP_INT_MSK_BIT,&(dev->core_regs->gintmsk));

  /* endpoint related interrupt sources */
  /* IN */
  clear_rbit(DIEPMSK_INEP_NAK_EFF_MSK,&(dev->usbd_regs->diepmsk));
  clear_rbit(DIEPMSK_INTKN_EP_MIS_MSK,&(dev->usbd_regs->diepmsk));
  clear_rbit(DIEPMSK_INTKN_TXF_EMP_MSK,&(dev->usbd_regs->diepmsk));
  clear_rbit(DIEPMSK_TIMEOUT_MSK,&(dev->usbd_regs->diepmsk));
  clear_rbit(DIEPMSK_AHB_ERR_MSK,&(dev->usbd_regs->diepmsk));
  clear_rbit(DIEPMSK_EP_DISBLD_MSK,&(dev->usbd_regs->diepmsk));
  set_rbit(DIEPMSK_XFER_COMPL_MSK,&(dev->usbd_regs->diepmsk));
  /* OUT */
  clear_rbit(DOEPMSK_OUTTKN_EP_DIS_MSK,&(dev->usbd_regs->doepmsk));
  set_rbit(DOEPMSK_SETUP_MSK,&(dev->usbd_regs->doepmsk));
  clear_rbit(DOEPMSK_AHB_ERR_MSK,&(dev->usbd_regs->doepmsk));
  clear_rbit(DOEPMSK_EP_DISBLD_MSK,&(dev->usbd_regs->doepmsk));
  set_rbit(DOEPMSK_XFER_COMPL_MSK,&(dev->usbd_regs->doepmsk));
}

/*
 * basic hardware init, called by dwc3884_init() at driver start
 *
 */
static void hw_init(GADGET_NAME(__GADGET,_dev*) dev)
{
  int i;
  unsigned short sz_ptxfifo, sz_nptxfifo, sz_rxfifo, sz_fifo, num_perio_ep;

  DEBUG(dev,"hardware init.\n");
  set_rbit(0,&(dev->core_regs->grstctl));

  /* set gpio's for controlling usb power and clock gating */
  clear_rbit(8,&(dev->gpio_regs->p0_altsel0));
  clear_rbit(8,&(dev->gpio_regs->p0_altsel1));
  set_rbit(8,&(dev->gpio_regs->p0_od));
  set_rbit(8,&(dev->gpio_regs->p0_dir));
  clear_rbit(8,&(dev->gpio_regs->p0_puden));
  clear_rbit(8,&(dev->gpio_regs->p0_pudsel));
  set_rbit(8,&(dev->gpio_regs->p0_out));

  /* set clock gating */
  clear_rbit(4,&(dev->cgu_regs->ifccr));
  clear_rbit(5,&(dev->cgu_regs->ifccr));

  /* set power */
  clear_rbit(0,&(dev->pmu_regs->pwdcr));
  clear_rbit(6,&(dev->pmu_regs->pwdcr));
  clear_rbit(15,&(dev->pmu_regs->pwdcr));

  /* device mode, internal transceiver */
  set_rbit(USBCFG_HOST_DEVICE_BIT,&(dev->rcu_regs->usbcfg));
  set_rbit(USBCFG_HOST_END_BIT,&(dev->rcu_regs->usbcfg));

  /* check and set up dma mode, the driver doesn't support slave
     mode at the moment */
  if (read_slice(GHWCFG2_OTG_ARCH_LBIT,GHWCFG2_OTG_ARCH_HBIT,
                 &(dev->core_regs->ghwcfg2)) == GHWCFG2_OTG_ARCH_INT_DMA) {
    set_rbit(GAHBCFG_DMA_EN_BIT,&(dev->core_regs->gahbcfg));
    write_slice(GAHBCFG_HBST_LEN_LBIT,GAHBCFG_HBST_LEN_HBIT,
                &(dev->core_regs->gahbcfg),GADGET_AHB_BURSTLENGTH);
  }
  else {
    ERROR(dev, "Device not configured for DMA.\n");
  }

  /* check and set up operation and host/device mode */
  if ((read_slice(GHWCFG2_OTG_MODE_LBIT,GHWCFG2_OTG_MODE_HBIT,
                  &(dev->core_regs->ghwcfg2)) != GHWCFG2_OTG_MODE_HNP_SRP_BOTH)) {
      ERROR(dev, "Wrong OTG mode configuration.\n");
  }
  else if (test_rbit(GINTSTS_CUR_MOD_BIT,&(dev->core_regs->gintsts))) {
      ERROR(dev, "Core not in device mode.\n");
  }
  else {
    /* FIXME: If SRP or HNP is set, pullup switching will not work and so no USB
       reset will be generated. Normally, we could enable SRP or HNP otg
       capabilities, to get a connect event interrupt. */
    //set_rbit(GUSBCFG_HNP_CAP_BIT,&(dev->core_regs->gusbcfg));
    //set_rbit(GUSBCFG_SRP_CAP_BIT,&(dev->core_regs->gusbcfg));
  }

  /* setup phy interface */
  if (
       (read_slice(GHWCFG2_HS_PHY_TYPE_LBIT,GHWCFG2_HS_PHY_TYPE_HBIT,
                   &(dev->core_regs->ghwcfg2)) != GHWCFG2_HS_PHY_TYPE_UTMI) ||
       (read_slice(GHWCFG2_FS_PHY_TYPE_LBIT,GHWCFG2_FS_PHY_TYPE_HBIT,
                   &(dev->core_regs->ghwcfg2)) != GHWCFG2_FS_PHY_TYPE_NONE) ||
       (read_slice(GHWCFG4_PHY_DATA_WIDTH_LBIT,GHWCFG4_PHY_DATA_WIDTH_HBIT,
                   &(dev->core_regs->ghwcfg4)) != GHWCFG4_PHY_DATA_WIDTH_SEL)
     ) {
    ERROR(dev, "Wrong phy device configuration.\n");
  }
  else {
#if (GADGET_UTMI_PHY_BUSWIDTH == 16)
    /* 16 bit USB 2.0 UTMI+ PHY with recommended USB turnaround setting */
    clear_rbit(GUSBCFG_PHY_SEL_BIT,&(dev->core_regs->gusbcfg));
    clear_rbit(GUSBCFG_ULPI_UTMI_SEL_BIT,&(dev->core_regs->gusbcfg));
    set_rbit(GUSBCFG_PHY_IF_BIT,&(dev->core_regs->gusbcfg));
    write_slice(GUSBCFG_TRD_TIM_LBIT,GUSBCFG_TRD_TIM_HBIT,&(dev->core_regs->gusbcfg),5);
#elif (GADGET_UTMI_PHY_BUSWIDTH == 8)
    /* 8 bit USB 2.0 UTMI+ PHY with recommended USB turnaround setting */
    clear_rbit(GUSBCFG_PHY_SEL_BIT,&(dev->core_regs->gusbcfg));
    clear_rbit(GUSBCFG_ULPI_UTMI_SEL_BIT,&(dev->core_regs->gusbcfg));
    clear_rbit(GUSBCFG_PHY_IF_BIT,&(dev->core_regs->gusbcfg));
    write_slice(GUSBCFG_TRD_TIM_LBIT,GUSBCFG_TRD_TIM_HBIT,&(dev->core_regs->gusbcfg),9);
#else
#error wrong phy configuration.
#endif
  }

  /* FIFO ram allocation, set the GRXFSIZ, GNPTXFSIZ, DPTXFSIZn registers
   *
   * FIXME: this is a very rough calculation in the following way:
   *
   * - each periodic ep has a fifo size of 8 dwords (32 byte)
   * - 2/5 of the remaining fifo ram is assigned to the non periodic
   *   transmit fifo
   * - 3/5 of the remaining fifo ram is assigned to the rx fifo
   *
   */

  sz_fifo = (unsigned short)read_slice(GHWCFG3_DFIFO_DEPTH_LBIT,GHWCFG3_DFIFO_DEPTH_HBIT,
                                       &(dev->core_regs->ghwcfg3));

  num_perio_ep = (unsigned short)read_slice(GHWCFG4_NUM_DEV_PERIO_EPS_LBIT,GHWCFG4_NUM_DEV_PERIO_EPS_HBIT,
                                            &(dev->core_regs->ghwcfg4));

  dev->tx_fifo_num_max = num_perio_ep;

  fsiz_perio_ep = fsiz_perio_ep / 4;                /* in dwords */
  sz_ptxfifo  = num_perio_ep * fsiz_perio_ep;       /* in dwords */
  sz_nptxfifo = (sz_fifo - sz_ptxfifo) * 2 / 5;     /* in dwords */
  sz_nptxfifo = 4 * (sz_nptxfifo / 4);              /* rounding  */
  sz_rxfifo   = sz_fifo - sz_ptxfifo - sz_nptxfifo; /* in dwords */

  DEBUG(dev, "fszi_perio_ep=%x\n", fsiz_perio_ep );
  DEBUG(dev, "sz_ptxfifo=%x\n", sz_ptxfifo );
  DEBUG(dev, "sz_nptxfifo=%x\n", sz_nptxfifo);
  DEBUG(dev, "sz_rxfifo=%x\n", sz_rxfifo);
  
  write_slice(GRXFSIZ_RX_FDEP_LBIT,GRXFSIZ_RX_FDEP_HBIT,
              &(dev->core_regs->grxfsiz),sz_rxfifo);

  /* FIXME: do we have to use bytes or dwords, dwords assumed ? */
  write_slice(GNPTXFSIZ_NPTX_FST_ADDR_LBIT,GNPTXFSIZ_NPTX_FST_ADDR_HBIT,
              &(dev->core_regs->gnptxfsiz),sz_rxfifo);

  write_slice(GNPTXFSIZ_NPTX_FDEP_LBIT,GNPTXFSIZ_NPTX_FDEP_HBIT,
              &(dev->core_regs->gnptxfsiz),sz_nptxfifo);

  for(i=0; i<num_perio_ep; i++) {
    write_slice(DPTXFSIZ_DPTX_FSIZE_LBIT,DPTXFSIZ_DPTX_FSIZE_HBIT,
                &(dev->core_regs->dptxfsiz[i]),fsiz_perio_ep);

    write_slice(DPTXFSIZ_DPTX_FST_ADDR_LBIT,DPTXFSIZ_DPTX_FST_ADDR_HBIT,
                &(dev->core_regs->dptxfsiz[i]),sz_rxfifo+sz_nptxfifo+(i*fsiz_perio_ep));
  }

  /* flush all Tx fifos */
  write_slice(GRSTCTL_TX_FNUM_LBIT,GRSTCTL_TX_FNUM_HBIT,
              &(dev->core_regs->grstctl),GRSTCTL_TX_FLSH_ALL);
  set_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl));
  while(test_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl)));

  /* flush all Rx fifos */
  set_rbit(GRSTCTL_RX_FFLSH_BIT,&(dev->core_regs->grstctl));
  while(test_rbit(GRSTCTL_RX_FFLSH_BIT,&(dev->core_regs->grstctl)));

  /* setting non-zero status out handshake and frame interval,
     check these settings again */
  set_rbit(DCFG_NZ_STS_OUT_HSHK_BIT,&(dev->usbd_regs->dcfg));
  write_slice(DCFG_DEV_SPD_LBIT,DCFG_DEV_SPD_HBIT,
              &(dev->usbd_regs->dcfg),DCFG_DEV_SPD_HS);
  write_slice(DCFG_PER_FRINT_LBIT,DCFG_PER_FRINT_HBIT,
              &(dev->usbd_regs->dcfg),DCFG_PER_FRINT_80);
  /* set default address to 0 */
  write_slice(DCFG_DEV_ADDR_LBIT,DCFG_DEV_ADDR_HBIT,
              &(dev->usbd_regs->dcfg),0);

  /* read chip revision */
  dev->chiprev = read_reg(&(dev->core_regs->gsnpsid));


  /* determine maximum values for xfer size and packet count */
  dev->tflen = pow2(10 + read_slice(GHWCFG3_XFER_SIZE_WIDTH_LBIT,
                                    GHWCFG3_XFER_SIZE_WIDTH_HBIT,
                                    &(dev->core_regs->ghwcfg3)));
  dev->pkcnt = pow2(3 + read_slice(GHWCFG3_PKT_SIZE_WIDTH_LBIT,
                                   GHWCFG3_PKT_SIZE_WIDTH_HBIT,
                                   &(dev->core_regs->ghwcfg3)));

  hw_reset(dev);

  SET_FLAG(DEV_IS_INITIALIZED,&(dev->flags));
}

/*
 * stops usb hardware, e.g. after disconnect
 *
 */
static void hw_stop(GADGET_NAME(__GADGET,_dev*) dev,struct usb_gadget_driver* driver)
{
  int cnt, i;

  DEBUG(dev,"hardware stop.\n");
  hw_reset (dev);

  cnt = read_slice(GHWCFG2_NUM_DEV_EPS_LBIT,GHWCFG2_NUM_DEV_EPS_HBIT,
                   &(dev->core_regs->ghwcfg2));
  for (i = 0; i < cnt; i++) {
    clear_queue(dev,i,-ESHUTDOWN);
  }

  /* report disconnect */
  if (driver) {
    spin_unlock(&dev->lock);
    driver->disconnect (&dev->gadget);
    spin_lock(&dev->lock);
  }
  CLEAR_FLAG(DEV_IS_INITIALIZED,&(dev->flags));
}

/*
 * (re)initializes usb hardware (hw independent)
 *
 */
static void hw_reinit(GADGET_NAME(__GADGET,_dev*) dev)
{
  if (!TEST_FLAG(DEV_IS_INITIALIZED,&(dev->flags))) {
    DEBUG(dev,"hardware reinit.\n");

    /* reinitialize endpoints */
    ep_init(dev);
    SET_FLAG(DEV_IS_INITIALIZED,&(dev->flags));
  }
  set_interrupt(dev,TRUE);
  set_pullup(dev,TRUE);
}

/*------- Part 7: ------------ handling of low level control requests --------*/

/* These functions should be hardware independent.
 *  
 * - get_status  : writes directly into the tx memory, make sure that the
 *                 tx buffer an be accessed in an hw independent manner
 *
 * - set_address : delegate the hw specific part to a second stage, which
 *                 accesses the hardware
 *
 * - non standard control requests: not yet supported
 */

#include "common/gadget_ctrlreq_handling.c"

/*------- Part 8: ------------ interrupt handler and subfunctions ------------*/

/*
 * handles connect events 
 *
 */  
static void connect(GADGET_NAME(__GADGET,_dev*) dev)
{
  if (dev) {
    DEBUG(dev,"connect function called.\n");
    hw_reinit(dev);
  }
}

/*
 * handles disconnct events
 *
 */  
static void disconnect(GADGET_NAME(__GADGET,_dev*) dev)
{
  if (dev) {
    DEBUG(dev,"disconnect function called.\n");
    hw_stop(dev,dev->driver);

    /* this shouldn't be here, we do need it only if we don't get a connect interrupt to
       be sure, that the device is functional after reconnect. */ 
    //set_interrupt(dev,TRUE);
    //set_pullup(dev,TRUE);
  }
}


/*
 * called if a usb reset interrupt is detected, used for hardware init
 *
 */
static void handle_usb_reset_detected(GADGET_NAME(__GADGET,_dev*) dev)
{
  int i,cnt;

  /* Because of the fact that the danube board can't handle connect/disconnect 
     events wit interrupts as usual, and because it can't monitor the bus power 
     too, we have some limitations. The only way to reinitialize the hardware
     after connect/disconnect events is the usb reset interrupt. The pullup and
     the interrups is never switched off, if we don't unload the driver. If a
     usb reset occurs, we do the following:

     - clear all endpoint transfer queues 
     - report a possible disconnect to the gadget driver
     - reinitialize all endpoints
     - set nak handshake on all OUT endpoints as described in the manual

     Maybe further actions have to be done here.
   */

  WARN(dev,"handle bus reset.\n");
  dev->usb_state = USB_STATE_DEFAULT;

  /* this isn't documented in the datasheet, but without resetting the address
     register to 0, the chaper9 enumerate test will fail everytimes. */
  write_slice(DCFG_DEV_ADDR_LBIT,DCFG_DEV_ADDR_HBIT,&(dev->usbd_regs->dcfg),0);

  cnt = read_slice(GHWCFG2_NUM_DEV_EPS_LBIT,GHWCFG2_NUM_DEV_EPS_HBIT,
                   &(dev->core_regs->ghwcfg2));
  for (i = 0; i < cnt; i++) {
    clear_queue(dev,i,-ESHUTDOWN);
  }

  if(dev->driver && dev->driver->disconnect) {
    dev->driver->disconnect( &dev->gadget);
  }
  ep_init(dev);

  /* set the NAK handshake for all OUT endpoints, required by the core manual */
  set_rbit(DOEPCTL_SNAK_BIT,&(dev->ep[0].out_regs->doepctl));
  for (i=1; i<EP_COUNT; i++) {
    if (read_slice(2*i,2*i+1,&(dev->core_regs->ghwcfg1)) == GHWCFG1_EP_DIR_OUT) {
      set_rbit(DOEPCTL_SNAK_BIT,&(dev->ep[i].out_regs->doepctl));
    }
  }
}

/*
 * completes the hardware setup, called when the enumeration speed is known
 *
 */
static void handle_speed_enumeration_done(GADGET_NAME(__GADGET,_dev*) dev)
{
  unsigned short speed;
  int i;
  
  DEBUG(dev,"handle speed enumeration.\n");

  /* read enumeration speed from dsts */
  speed = (unsigned short)read_slice(DSTS_ENUM_SPD_LBIT,DSTS_ENUM_SPD_HBIT,
                                     &(dev->usbd_regs->dsts));

  if (!speed) {
    DEBUG(dev,"chirp sequence completed, highspeed detected.\n");
  }
  else {
    DEBUG(dev,"chirp sequence completed, fullspeed detected.\n");
  }

  /* store speed in dcfg */
  write_slice(DCFG_DEV_SPD_LBIT,DCFG_DEV_SPD_HBIT,
              &(dev->usbd_regs->dcfg),speed);

  /* store speed in gadget driver and controller */
  switch (speed) {
    case DCFG_DEV_SPD_LS:
      dev->speed = USB_SPEED_LOW;
      dev->gadget.speed = USB_SPEED_LOW;
      dev->gadget.is_dualspeed = FALSE;
      break;
    case DCFG_DEV_SPD_HS:
      dev->speed = USB_SPEED_HIGH;
      dev->gadget.speed = USB_SPEED_HIGH;
      dev->gadget.is_dualspeed = TRUE;
      break;
    case DCFG_DEV_SPD_FS11:
    case DCFG_DEV_SPD_FS20:
      dev->speed = USB_SPEED_FULL;
      dev->gadget.speed = USB_SPEED_FULL;
      dev->gadget.is_dualspeed = FALSE;
      break;
    default:
      dev->speed = USB_SPEED_UNKNOWN;
      dev->gadget.speed = USB_SPEED_UNKNOWN;
      break;
  }

  if (speed == DCFG_DEV_SPD_HS) {
    write_slice(GUSBCFG_TOUT_CAL_LBIT,GUSBCFG_TOUT_CAL_HBIT,
                &(dev->core_regs->gusbcfg),GADGET_TOUT_CALIBRATION);
  }
  else { 
    write_slice(GUSBCFG_TOUT_CAL_LBIT,GUSBCFG_TOUT_CAL_HBIT,
                &(dev->core_regs->gusbcfg),0);
  }

  /* set ep0's max packet size (IN and OUT mps is identical, only one
     setting is needed) */
    dev->ep[0].ep.maxpacket = config_mps(dev,0,0);
    write_slice(DIEPCTL_MPS_LBIT,DIEPCTL_MPS_HBIT_0,
                &(dev->ep[0].in_regs->diepctl),dev->ep[0].ep.maxpacket);

  /* set the mps for all data endpoints */
  for (i=1; i < dev->epcnt; i++) {
    dev->ep[i].ep.maxpacket = config_mps(dev,i,0);

    if (dev->ep[i].dir == USB_DIR_IN) {    
    write_slice(DIEPCTL_MPS_LBIT,DIEPCTL_MPS_HBIT_N,
                &(dev->ep[i].in_regs->diepctl),dev->ep[i].ep.maxpacket);
    }
    else {
    write_slice(DOEPCTL_MPS_LBIT,DOEPCTL_MPS_HBIT_N,
                &(dev->ep[i].out_regs->doepctl),dev->ep[i].ep.maxpacket);
    }
  }
 
  /* the remaining initialization make sense only when the gadget driver
     has registered, see ep0_start(), which is called from
     gadget_register_driver */

  /* enable ep0 out */
  ep0_enable_setup_stage(dev);
}

/*
 * Handles mode mismatch interrupt, mode mismatch interrupt is asserted if
 * in device mode host mode registers are accessed.
 *
 */
static void handle_mode_mismatch(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no mode_mismatch handler implemented.\n");
}

/*
 * OTG interrupt, in particular important for connect event detection
 *
 */
static void handle_otg_interrupt(GADGET_NAME(__GADGET,_dev*) dev)
{
  if (test_rbit(GOTGINT_SES_END_DET,&(dev->core_regs->gotgint))) {
    set_rbit(GOTGINT_SES_END_DET,&(dev->core_regs->gotgint));
    DEBUG(dev,"handle session_end_detected.\n");
  }

  if (test_rbit(GOTGINT_SES_REQ_SUS_STS_CHNG,&(dev->core_regs->gotgint))) {
    set_rbit(GOTGINT_SES_REQ_SUS_STS_CHNG,&(dev->core_regs->gotgint));
    DEBUG(dev,"handle session_request_suspend_status_change.\n");
  }

  if (test_rbit(GOTGINT_HST_NEG_SUC_STS_CHNG,&(dev->core_regs->gotgint))) {
    set_rbit(GOTGINT_HST_NEG_SUC_STS_CHNG,&(dev->core_regs->gotgint));
    DEBUG(dev,"handle host_negotiation_success_status_change.\n");
  }

  if (test_rbit(GOTGINT_HST_NEG_DET,&(dev->core_regs->gotgint))) {
    set_rbit(GOTGINT_HST_NEG_DET,&(dev->core_regs->gotgint));
    DEBUG(dev,"handle host_negotiation_detected.\n");
  }

  if (test_rbit(GOTGINT_ADEV_TOUT_CHG,&(dev->core_regs->gotgint))) {
    set_rbit(GOTGINT_ADEV_TOUT_CHG,&(dev->core_regs->gotgint));
    DEBUG(dev,"handle a_dev_tout_change.\n");
  }

  /* This is a (re)connect event */
  if (test_rbit(GOTGINT_DBNCE_DONE,&(dev->core_regs->gotgint))) {
    set_rbit(GOTGINT_DBNCE_DONE,&(dev->core_regs->gotgint));
    DEBUG(dev,"handle connect.\n");
    handle_connect(dev);
  }
}

/*
 * SOF interrupt, basicly not needed, not yet enabled
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_sof_received(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no sof handler implemented.\n");
}

/*
 * GLBL_NP_NAK_EFF interrupt, asserted if the global non periodic IN NAK
 * is effective.
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_global_np_in_nak_effective(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no global_non_periodic_in_nak_effective handler implemented.\n");
}

/*
 * IN_NAK_EFF interrupt, asserted if NAK on a periodic IN endpoint is effective
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_in_ep_nak_effective(GADGET_NAME(__GADGET,_dev*) dev, byte epnum)
{
  WARN(dev,"no periodic_in_nak_effective handler implemented.\n");
}

/*
 * Handles global out NAK effective interrupt
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_global_out_nak_effective(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no global_out_nak_effective handler implemented.\n");
}

/*
 * Early suspend interrupt, not yet used
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_early_suspend(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no early_suspend handler implemented.\n");
}

/*
 * Suspend interrupt
 *
 */
static void handle_suspend(GADGET_NAME(__GADGET,_dev*) dev)
{
  DEBUG(dev,"handle suspend.\n");
  if (dev->usb_state != USB_STATE_SUSPENDED &&
      test_rbit(DSTS_SUSP_STS_BIT,&(dev->usbd_regs->dsts))) {
    dev->prev_usb_state = dev->usb_state;
    dev->usb_state = USB_STATE_SUSPENDED;

    /* report suspend */
    if (dev->driver && dev->driver->suspend) {
      spin_unlock(&dev->lock);
      dev->driver->suspend (&dev->gadget);
      spin_lock(&dev->lock);
    }

    /* this makes sense only if we are able to monitor the bus power. If so, we can decide
       between suspend and disconnect, even if we don't get disconncet interrupts. But because 
       of the fact, that the danube board can't monitor the bus power, this would make the 
       chapter9 suspend/resume test fail.
     */
    //tasklet_schedule(&tl_connect);
  }
}

/*
 * End of periodic frame interrupt, not yet used
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_end_of_periodic_frame(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no end_of_periodic_frame handler implemented.\n");
}

/*
 * Endpoint mismatch interrupt, asserted if the core accesses the
 * non periodic endpoints in a non correct sequence, not yet used
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_endpoint_mismatch(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no endpoint_mismatch handler implemented.\n");
}

/*
 * Data fetch suspended interrupt, important only in dma mode, not yet used
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_data_fetch_suspended(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no data_fetch_suspended handler implemented.\n");
}

/*
 * Connector id status change interrupt, asserted if the otg connector
 * status is changed, not needed/used, because this is a device only driver
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_connector_id_status_change(GADGET_NAME(__GADGET,_dev*) dev)
{
  WARN(dev,"no connector_id_status_change handler implemented.\n");
}

/*
 * Disconnect interrupt
 *
 */
static void handle_disconnect(GADGET_NAME(__GADGET,_dev*) dev)
{
  DEBUG(dev,"handle disconnect.\n");
  tasklet_schedule(&tl_connect);
}

/*
 * Connect interrupt
 *
 */
static void handle_connect(GADGET_NAME(__GADGET,_dev*) dev)
{
  DEBUG(dev,"handle connect.\n");
  tasklet_schedule(&tl_connect);
}

/*
 * Wakeup interrupt
 *
 */
static void handle_wakeup(GADGET_NAME(__GADGET,_dev*) dev)
{
  DEBUG(dev,"handle resume.\n");
  handler_counter++;
  if (dev->usb_state == USB_STATE_SUSPENDED) {
    /* report wakeup */
    if (dev->driver && dev->driver->resume) {
      spin_unlock(&dev->lock);
      dev->driver->resume (&dev->gadget);
      spin_lock(&dev->lock);
    }

    /* restore the usb state to the previous value */
    dev->usb_state = dev->prev_usb_state;
    dev->prev_usb_state = USB_STATE_SUSPENDED;

    /* this makes sense only if we are able to monitor the bus power. If so, we can decide
       between resume and reconnect, even if we don't get connct interrupts. But because 
       of the fact, that the danube board can't monitor the bus power, this would make the 
       chapter9 suspend/resume test fail.
     */
    //tasklet_schedule(&tl_connect);
  }
}


/*
 * Handles transfer complete interrupt.
 *
 */
static void handle_transfer_complete(GADGET_NAME(__GADGET,_dev*) dev, byte num)
{
  DEBUG(dev,"handle transfer_complete for ep%d.\n", num);

  /* update transfer status */
  check_xfer(dev,num);
  /* restart queue */
  complete_queue(dev,num);

  /* reenable ep0 for setup stage */
  if(!num) {
    write_reg(&(dev->ep[0].out_regs->doepdma),dev->dma);

    write_slice(DOEPTSIZ_SUP_CNT_LBIT,DOEPTSIZ_SUP_CNT_HBIT,
                &(dev->ep[0].out_regs->doeptsiz),DOEPTSIZ_SUP_CNT_DEFAULT);

    /* Enable ep0 for dma */
    set_rbit(DOEPCTL_EP_ENA_BIT,&(dev->ep[0].out_regs->doepctl));
  }
}

/*
 * Handles ep disabled interrupt, asserted if the disable sequence for
 * any endpoint was completed.
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_ep_disabled(GADGET_NAME(__GADGET,_dev*) dev, byte num)
{
  WARN(dev,"no ep_disabled handler for ep%d implemented.\n",num);
}

/*
 * AHB error interrupt, not yet used
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_ahb_error(GADGET_NAME(__GADGET,_dev*) dev, byte num)
{
  WARN(dev,"no ahb_error handler for ep%d implemented.\n",num);
}

/*
 * Timeout interrupt, important for error handling especially during IN
 * transfers, has urgently to be implementd during target integration.
 *
 */
static void handle_timeout(GADGET_NAME(__GADGET,_dev*) dev, byte num)
{
  WARN(dev,"handle timeout for ep%d.\n",num);

#if 0
  /* This is the implementation of the actions, the application has to do
     in case of a timeout condition as described in the manual. But we 
     noticed, that if we set the EP_DIS bit (in case of previously set
     EP_ENA bit), both bits are never cleared if the endpoint is disabled.
     This would mean, that we are have a deadlock situation. 
   */
 
  int i;

  /* 1. the global non periodeic nak should be set by the core internally */
  if (!test_rbit(DCTL_GNPIN_NAK_STS_BIT,&(dev->usbd_regs->dctl))) {
    WARN(dev,"timeout condition without nak.\n");
    return;
  }
  /* 2. disable all np in endpoints and wait for completion. */
  for(i=0;i<dev->epcnt;i++) {
    if (ep_is_np_in(dev,i)) {
      /* use the disable bit only if ena bit was set previously */
      if (test_rbit(DIEPCTL_EP_ENA_BIT,&(dev->ep[i].in_regs->diepctl))) {

        /* FIXME: if EP_ENA is set and we are setting EP_DIS, EP_ENA or EP_DIS 
                  is never cleared. */
        WARN(dev,"timeout while ep%d is active.\n",i);
        set_rbit(DIEPCTL_EP_DIS_BIT,&(dev->ep[i].in_regs->diepctl));
        while(test_rbit(DIEPCTL_EP_ENA_BIT,&(dev->ep[i].in_regs->diepctl)) ||
              test_rbit(DIEPCTL_EP_DIS_BIT,&(dev->ep[i].in_regs->diepctl)));

        WARN(dev,"timeout processed.\n");
        if (test_rbit(DIEPCTL_EP_ENA_BIT,&(dev->ep[i].in_regs->diepctl))) {
          WARN(dev,"timeout EP_ENA still set.\n");
        }
        if (test_rbit(DIEPCTL_EP_DIS_BIT,&(dev->ep[i].in_regs->diepctl))) {
          WARN(dev,"timeout EP_DIS still set.\n");
        }
      }
      set_rbit(DIEPCTL_SNAK_BIT,&(dev->ep[i].in_regs->diepctl));
    }
  }
  /* 3. flush np in fifo and wait for completion. */
  write_slice(GRSTCTL_TX_FNUM_LBIT,GRSTCTL_TX_FNUM_HBIT,
              &(dev->core_regs->grstctl),0);
  set_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl));
  while(test_rbit(GRSTCTL_TX_FFLSH_BIT,&(dev->core_regs->grstctl)));
  /* 4. reenable other np in endpoints */
  for(i=0;i<dev->epcnt;i++) {
    if (ep_is_np_in(dev,i)) {
      clear_rbit(DIEPCTL_SNAK_BIT,&(dev->ep[i].in_regs->diepctl));
    }
  }
  /* 5. clear global np in nak */
  glbl_np_in_nak(dev,0);
  /* 6. restart the transfer queue */
  if (count_queue(dev,num)) {
    start_queue(dev,num);
  }
#endif
}

/*
 * InTkn_while_TXFifo_empty interrupt
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_in_tkn_when_txf_empty(GADGET_NAME(__GADGET,_dev*) dev, byte num)
{
   WARN(dev,"no in_tkn_when_txf_empty handler for ep%d implemented.\n", num);
}

/*
 * InTkn_with_EP_Mismatch interrupt
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_in_tkn_with_ep_mismatch(GADGET_NAME(__GADGET,_dev*) dev, byte num)
{
   WARN(dev,"no in token_with_ep_mismatch handler for ep%d implemented.\n", num);
}

/*
 * OutTkn_While_EP_Disabled interrupt
 * FIXME: probably not needed, but at least to check !
 *
 */
static void handle_out_tkn_while_ep_disabled(GADGET_NAME(__GADGET,_dev*) dev, byte num)
{
  WARN(dev,"no out_token_while_ep_disabled handler for ep%d implemented.\n",num);
}

/*
 * Handles setup transactions, asserted after a setup packet was received,
 * now we have to start the decoding of the request.
 *
 */
static void handle_setup(GADGET_NAME(__GADGET,_dev*) dev)
{
  int ret = 0;
  GADGET_NAME(__GADGET,_rq_data) req;
  byte sup_cnt;

  DEBUG(dev,"handle setup.\n");

  /* store setup counter */
  sup_cnt = (byte)read_slice(DOEPTSIZ_SUP_CNT_LBIT,DOEPTSIZ_SUP_CNT_HBIT,
                             &(dev->ep[0].out_regs->doeptsiz));

  dma_cache_inv((unsigned long) dev->buf, EP0_BUFLEN);

  /* destall ep0 after received control request */
  ep_enable(dev,0,EP_DESTALL);

  volatile byte* sbf = ep0_setup_buffer(dev);

  /* FIXME: setup interrupt asserted but setup counter not decremented, 
            hardware bug ? */
  if(sup_cnt == DOEPTSIZ_SUP_CNT_DEFAULT) {
    WARN(dev, "setup counter not decremented.\n");
    sup_cnt--;
  }

  if(sup_cnt < DOEPTSIZ_SUP_CNT_DEFAULT) {
    /* read request from dma memory */
    req.raw [0] = *((u32*) (sbf+((2-sup_cnt)*4)));
    req.raw [1] = *((u32*) (sbf+((3-sup_cnt)*4)));
  
    /* Sets setup packet counter for ep0, this is the last step */
    write_slice(DOEPTSIZ_SUP_CNT_LBIT,DOEPTSIZ_SUP_CNT_HBIT,
                &(dev->ep[0].out_regs->doeptsiz),DOEPTSIZ_SUP_CNT_DEFAULT);

    ret = finalize_setup(dev, req);

    /* stall ep0 in case of error */
    if (ret < 0) {
      ep_disable(dev,0,EP_STALL);
    }
  } else {
    WARN(dev, "setup counter not decremented.\n");
  }
}

/*
 * tasklet function which calls the connect and disconnect sequences,
 * because of the tasklet queuing properies this can be used as debounce
 * mechanism too.  
 * 
 */
static void connect_handler(unsigned long data)
{
  GADGET_NAME(__GADGET,_dev*) dev  = the_controller;
  DEBUG(dev,"connect handler called.\n");

  /* FIXME: correct bit to determine port power, doesn't seem to work
            for the actual revision of the hardware. */
  if (test_rbit(GOTGCTL_B_SES_VLD_BIT,&(dev->core_regs->gotgctl))) {
    if (test_rbit(DCTL_SFT_DISCON_BIT,&(dev->usbd_regs->dctl))) {
      connect(dev);
    }
  }
  else {
    if (!test_rbit(DCTL_SFT_DISCON_BIT,&(dev->usbd_regs->dctl))) {
      disconnect(dev);
    }
  }
}

/*
 * main interrupt handler: checks the irq status registers, clears the
 * interrupt status and calls the event specific handler functions
 *
 */
static irqreturn_t GADGET_NAME(__GADGET,_irq)(int irq, void* _dev, struct pt_regs* r)
{
  GADGET_NAME(__GADGET,_dev*) dev = _dev;
  byte pos;
  u32 gintsts,doepint,diepint;

  spin_lock(&dev->lock);

  gintsts = read_reg(&(dev->core_regs->gintsts)); // & read_reg(&(dev->core_regs->gintmsk));
  gintsts &= read_reg(&(dev->core_regs->gintmsk));

  if( gintsts != 0 ) {
    /* usb reset detected */
    if (TEST_FLAG(GINTSTS_USB_RST_BIT,&gintsts)) {
      set_rbit(GINTSTS_USB_RST_BIT,&(dev->core_regs->gintsts));
      handle_usb_reset_detected(dev);
    }

    /* OUT endpoint interrupt*/
    if (TEST_FLAG(GINTSTS_OEP_INT_BIT,&gintsts)) {
      /* FIXME: can we be sure that only one bit is set ?? */
      pos = find_bit(&(dev->usbd_regs->daint), 16);
      /* FIXME: we can't read doepmsk, because this returns the lowest four bits of diepmsk, hardware bug. */
      doepint = read_reg(&(dev->ep[pos].out_regs->doepint)) /* & read_reg(&(dev->usbd_regs->doepmsk)) */; 
      
      if (TEST_FLAG(DOEPINT_XFER_COMPL_BIT,&doepint)) {
        /* transfer complete */
        set_rbit(DOEPINT_XFER_COMPL_BIT,&(dev->ep[pos].out_regs->doepint));
        handle_transfer_complete(dev,pos);
      }
  
      if (TEST_FLAG(DOEPINT_EP_DISBLD_BIT,&doepint)) {
        /* ep disabled */
        set_rbit(DOEPINT_EP_DISBLD_BIT,&(dev->ep[pos].out_regs->doepint));
        handle_ep_disabled(dev,pos);
      }
  
      if (TEST_FLAG(DOEPINT_AHB_ERR_BIT,&doepint)) {
        /* ahb error */
        set_rbit(DOEPINT_AHB_ERR_BIT,&(dev->ep[pos].out_regs->doepint));
        handle_ahb_error(dev,pos);
      }
  
      if(TEST_FLAG(DOEPINT_OUTTKN_EP_DIS_BIT,&doepint)) {
        /* out token received when endpoint disabled, ep0 only */
        set_rbit(DOEPINT_OUTTKN_EP_DIS_BIT,&(dev->ep[pos].out_regs->doepint));
        handle_out_tkn_while_ep_disabled(dev,0);
      }
  
      if(TEST_FLAG(DOEPINT_SETUP_BIT,&doepint)) {
        /* setup packet received, ep0 only */
        set_rbit(DOEPINT_SETUP_BIT,&(dev->ep[pos].out_regs->doepint));
        handle_setup(dev);
      }
    } /* OUT endpoint interrupt */
  
    /* IN endpoint interrupt */
    if (TEST_FLAG(GINTSTS_IEP_INT_BIT,&gintsts)) {
      /* FIXME: can we be sure that only one bit is set ?? */
      pos = find_bit(&(dev->usbd_regs->daint), 0);
      diepint = read_reg(&(dev->ep[pos].in_regs->diepint)) & read_reg(&(dev->usbd_regs->diepmsk));
  
      if (TEST_FLAG(DIEPINT_XFER_COMPL_BIT,&diepint)) {
        /* transfer complete */
        set_rbit(DIEPINT_XFER_COMPL_BIT,&(dev->ep[pos].in_regs->diepint));
        handle_transfer_complete(dev,pos);
      }
  
      if (TEST_FLAG(DIEPINT_EP_DISBLD_BIT,&diepint)) {
        /* ep disabled */
        set_rbit(DIEPINT_EP_DISBLD_BIT,&(dev->ep[pos].in_regs->diepint));
        handle_ep_disabled(dev,pos);
      }
  
      if (TEST_FLAG(DIEPINT_AHB_ERR_BIT,&diepint)) {
        /* ahb error */
        set_rbit(DIEPINT_AHB_ERR_BIT,&(dev->ep[pos].in_regs->diepint));
        handle_ahb_error(dev,pos);
      }
  
      if (TEST_FLAG(DIEPINT_TIMEOUT_BIT,&diepint)) {
        /* timeout */
        set_rbit(DIEPINT_TIMEOUT_BIT,&(dev->ep[pos].in_regs->diepint));
        handle_timeout(dev,pos);
      }
  
      if (TEST_FLAG(DIEPINT_IN_TKN_TXF_EMP_BIT,&diepint)) {
        /* in token received when tx fifo empty */
        set_rbit(DIEPINT_IN_TKN_TXF_EMP_BIT,&(dev->ep[pos].in_regs->diepint));
        handle_in_tkn_when_txf_empty(dev,pos);
      }
  
      if (TEST_FLAG(DIEPINT_IN_TKN_EP_MIS_BIT,&diepint)) {
        /* in token received with ep mismatch */
        set_rbit(DIEPINT_IN_TKN_EP_MIS_BIT,&(dev->ep[pos].in_regs->diepint));
        handle_in_tkn_with_ep_mismatch(dev,pos);
      }
  
      if (TEST_FLAG(DIEPINT_IN_EP_NAK_EFF_BIT,&diepint)) {
        /* in ep NAK effective, periodic ep only */
        set_rbit(DIEPINT_IN_EP_NAK_EFF_BIT,&(dev->ep[pos].in_regs->diepint));
        handle_in_ep_nak_effective(dev,pos);
      }
    } /* IN endpoint interrupt */
  
    /* mode mismatch */
    if (TEST_FLAG(GINTSTS_MODE_MIS_BIT,&gintsts)) {
      set_rbit(GINTSTS_MODE_MIS_BIT,&(dev->core_regs->gintsts));
      handle_mode_mismatch(dev);
    }
  
    /* otg interrupt */
    if (TEST_FLAG(GINTSTS_OTG_INT_BIT,&gintsts)) {
      handle_otg_interrupt(dev);
    }
  
    /* sof token received */
    if (TEST_FLAG(GINTSTS_SOF_BIT,&gintsts)) {
      set_rbit(GINTSTS_SOF_BIT,&(dev->core_regs->gintsts));
      handle_sof_received(dev);
    }
  
    /* global non periodic in NAK effective */
    if (TEST_FLAG(GINTSTS_GIN_NAK_EFF_BIT,&gintsts)) {
      handle_global_np_in_nak_effective(dev);
    }
  
    /* global out NAK effective */
    if (TEST_FLAG(GINTSTS_GOUT_NAK_EFF_BIT,&gintsts)) {
      handle_global_out_nak_effective(dev);
    }
  
    /* early suspend */
    if (TEST_FLAG(GINTSTS_ERLY_SUSP_BIT,&gintsts)) {
      set_rbit(GINTSTS_ERLY_SUSP_BIT,&(dev->core_regs->gintsts));
      handle_early_suspend(dev);
    }
  
    /* usb suspend */
    if (TEST_FLAG(GINTSTS_USB_SUSP_BIT,&gintsts)) {
      set_rbit(GINTSTS_USB_SUSP_BIT,&(dev->core_regs->gintsts));
      handle_suspend(dev);
    }
  
    /* enumeration speed determined */
    if (TEST_FLAG(GINTSTS_ENUM_DONE_BIT,&gintsts)) {
      handle_speed_enumeration_done(dev);
      set_rbit(GINTSTS_ENUM_DONE_BIT,&(dev->core_regs->gintsts));
    }
  
    /* end of periodic frame */
    if (TEST_FLAG(GINTSTS_EOPF_BIT,&gintsts)) {
      set_rbit(GINTSTS_EOPF_BIT,&(dev->core_regs->gintsts));
      handle_end_of_periodic_frame(dev);
    }
  
    /* endpoint mismatch */
    if (TEST_FLAG(GINTSTS_EP_MIS_BIT,&gintsts)) {
      set_rbit(GINTSTS_EP_MIS_BIT,&(dev->core_regs->gintsts));
      handle_endpoint_mismatch(dev);
    }
  
    /* data fetch suspended */
    if (TEST_FLAG(GINTSTS_FET_SUSP_BIT,&gintsts)) {
      set_rbit(GINTSTS_FET_SUSP_BIT,&(dev->core_regs->gintsts));
      handle_data_fetch_suspended(dev);
    }
  
    /* connector id status change, otg only */
    if (TEST_FLAG(GINTSTS_CONID_STS_CHNG_BIT,&gintsts)) {
      set_rbit(GINTSTS_CONID_STS_CHNG_BIT,&(dev->core_regs->gintsts));
      handle_connector_id_status_change(dev);
    }
  
    /* disconnect */
    if (TEST_FLAG(GINTSTS_DISCONN_INT_BIT,&gintsts)) {
      set_rbit(GINTSTS_DISCONN_INT_BIT,&(dev->core_regs->gintsts));
      handle_disconnect(dev);
    }
  
    /* wakeup */
    if (TEST_FLAG(GINTSTS_WKUP_INT_BIT,&gintsts)) {
      set_rbit(GINTSTS_WKUP_INT_BIT,&(dev->core_regs->gintsts));
      handle_wakeup(dev);
    }
  }

  spin_unlock(&dev->lock);
  return IRQ_HANDLED;
}

/*------- Part 9: ------------ the gadget api --------------------------------*/

#include "common/gadget_api.c"

/*------- Part 10: ---------- module load/unlod ------------------------------*/

/* these functions are based heavily on the underlaying board/soc hardware
   and should be implemented for each controller driver uniquely */

/*
 * unloading driver
 *
 */
static void GADGET_NAME(__GADGET,_remove)(void)
{
  GADGET_NAME(__GADGET,_dev*) dev = the_controller;

  DEBUG(dev,"unloading.\n");

  /* stop hardware and unregister driver */
  if (dev->driver) {
    WARN (dev,"driver still in use.\n");
    usb_gadget_unregister_driver(dev->driver);
  }

  /* common gadget cleanaup */

  if (TEST_FLAG(DEV_IS_ENABLED,&(dev->flags))) {
    free_irq(irq_num,dev);
    CLEAR_FLAG(DEV_GOT_IRQ,&(dev->flags));
  }

  if (dev) {
    /* deallocate ep0's setup buffer */
    if (dev->buf) {
      kfree((void*)dev->buf);
    }
    kfree(dev);
  }

  DEBUG(dev,"finalizing cleanup.\n");
  the_controller = 0;
}

/*
 * loading driver
 *
 */
static int GADGET_NAME(__GADGET,_init)(void)
{
  GADGET_NAME(__GADGET,_dev*) dev;
  int retval,i;
  void* buf;
  byte __iomem* maddr;

  DEBUG(dev,"loading.\n");

  if (the_controller) {
    ERROR(dev,"contoller busy\n");
    return -EBUSY;
  }

  /* request controller device */
  dev = kmalloc(sizeof *dev, GFP_KERNEL);
  if (dev == NULL){
    retval = -ENOMEM;
    ERROR(dev,"no kernel memory available.\n");
    goto failed;
  }
  else {
    memset (dev, 0, sizeof *dev);
    spin_lock_init(&dev->lock);
  }
  /* set default state */
  dev->usb_state = USB_STATE_DEFAULT;

  /* Allocates buffer for ep0, used for setup packets and low level control
     request processing */
  buf = kmalloc(EP0_BUFLEN, GFP_ATOMIC | GFP_DMA);
  if (!buf){
    dev->buf = 0;
    dev->dma = DMA_ADDR_INVALID;
    ERROR(dev, "Can't allocate dma buffer.\n");
  }
  else {
    memset (buf, 0, EP0_BUFLEN);
    /* the dma address has to be a physical address */
    dev->dma = __pa(buf);
    dev->buf = buf;
  }

  /* common gadget setup */
  DEBUG(dev,"basic setup.\n");
  dev->gadget.name = (char*)driver_name;
  dev->gadget.ops = &(GADGET_NAME(__GADGET,_gadget_ops));
  dev->gadget.dev.bus_id = "gadget";
  dev->gadget.dev.driver_data = 0;
  dev->gadget.is_dualspeed = 1;
  dev->gadget.speed = USB_SPEED_FULL;
  /* should be USB_SPEED_UNKNOWN!!! */
  dev->speed = USB_SPEED_FULL;

  /* request interrupt */
  if (request_irq(irq_num, GADGET_NAME(__GADGET,_irq), SA_SHIRQ, driver_name, dev) != 0) {
    retval = -EBUSY;
    ERROR(dev,"no irq acquired.\n");
    goto failed;
  }
  SET_FLAG(DEV_GOT_IRQ,&(dev->flags));

  maddr = (byte __iomem*)csr_base;

  DEBUG(dev,"storing register adresses.\n");
  dev->core_regs  = (struct GADGET_NAME(__GADGET,_core_regs*))(maddr) ;
  dev->pwclk_regs = (struct GADGET_NAME(__GADGET,_pwclk_regs*))(maddr+PWCLK_REGS_OFFSET);
  dev->usbd_regs  = (struct GADGET_NAME(__GADGET,_usbd_regs*))(maddr+USBD_REGS_OFFSET);

  for (i=0; i < EP_COUNT; i++) {
    dev->ep[i].in_regs  = (struct GADGET_NAME(__GADGET,_ep_in_regs*))
      (maddr + EP_IN_REGS_OFFSET + (i*EP_REG_SIZE));
    dev->ep[i].out_regs = (struct GADGET_NAME(__GADGET,_ep_out_regs*))
      (maddr + EP_OUT_REGS_OFFSET + (i*EP_REG_SIZE));
  }

  dev->rcu_regs   = (struct GADGET_NAME(__GADGET,_rcu_regs*))(RCU_REGS_BASE);
  dev->pmu_regs   = (struct GADGET_NAME(__GADGET,_pmu_regs*))(PMU_REGS_BASE);
  dev->cgu_regs   = (struct GADGET_NAME(__GADGET,_cgu_regs*))(CGU_REGS_BASE);
  dev->gpio_regs  = (struct GADGET_NAME(__GADGET,_gpio_regs*))(GPIO_REGS_BASE);

  /* initialize usb hardware */
  hw_init(dev);
  DEBUG(dev,"Device at %p.\n",dev);
  ep_init(dev);

  DEBUG(dev,"finalizing setup.\n");
  the_controller = dev;
  SET_FLAG(DEV_IS_ENABLED,&(dev->flags));

  return 0;

failed:
  WARN(dev,"something went wrong, going away.\n");
	GADGET_NAME(__GADGET,_remove)();
	return retval;
}

/* This define is eeded only for external compile tests, we can remove it
   inside kernel tree */
#ifndef TEST_COMPILE
EXPORT_SYMBOL(usb_gadget_register_driver);
EXPORT_SYMBOL(usb_gadget_unregister_driver);
#endif

/*
 * module parameters, at least we have to specify:
 *
 * 1) csr base address
 * 2) irq number
 * 3) fifo size for periodic interrupt IN endpoints
 *
 */
MODULE_PARM (csr_base, "i");
MODULE_PARM_DESC (csr_base, "csr base address");
MODULE_PARM (irq_num, "i");
MODULE_PARM_DESC (irq_num, "irq number");
MODULE_PARM (fsiz_perio_ep, "i");
MODULE_PARM_DESC (fsiz_perio_ep, "fifo depth for periodic endpoints");

/* common module stuff */
MODULE_DESCRIPTION (DRIVER_DESC);
MODULE_AUTHOR ("Christian Schroeder");
MODULE_LICENSE ("GPL");

/* module init and destroy */
static int __init init(void)
{
  DEBUG(dev,"loading module.\n");
  tasklet_init(&tl_connect,connect_handler,0L);
  return GADGET_NAME(__GADGET,_init)();
}
module_init(init);

static void __exit cleanup(void)
{
  DEBUG(dev,"unloading module.\n");
  tasklet_kill(&tl_connect);
  GADGET_NAME(__GADGET,_remove)();
}
module_exit(cleanup);
