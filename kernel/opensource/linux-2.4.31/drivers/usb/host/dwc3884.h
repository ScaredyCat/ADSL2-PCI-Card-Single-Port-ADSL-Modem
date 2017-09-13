/*
 * Copyright (C) 2006 EMSYS GmbH, Ilmenau, Germany
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __LINUX_DWC3884_HCD_H
#define __LINUX_DWC3884_HCD_H

#include <linux/types.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <linux/usb.h>
#include "../hcd.h"

/*--------------------------------------------------------------------------*/

enum dwc3884_xferdir { DIR_OUT, DIR_IN };
enum dwc3884_eptype { EPTYPE_CTRL, EPTYPE_ISO, EPTYPE_BULK, EPTYPE_INT };
enum dwc3884_pid { PID_UNDEFINED=-1, PID_DATA0=0, PID_DATA2, PID_DATA1, PID_SETUP };

/**
 * struct td - structure representing a transfer-descriptor
 * @is_input: direction of the transfer
 * @pid: packet-id to start the transfer with
 * @dma: dma-address of the payload
 * @len: the length in bytes of the payload
 * 
 * The transfer-descriptor (TD) describes one hardware-transfer on 
 * a specific channel. TDs are allocated for a specific URB after the channel 
 * has been initialized. The memory is released when the channel is released.
 * (This is the only dynamic memory allocation in the HCD.)
 *
 * One URB is divided into a series of TDs, which needs to be processed 
 * to get the URB done. For more information how this is done 
 * see urb_to_xfer_list(). 
 */
struct td {
  int is_input;         
  enum dwc3884_pid pid; 
  dma_addr_t dma;       
  int len;              

  struct list_head td_list;
};

/**
 * struct dwc3884_channel - structure representing one channel of the HC
 * @hcd: pointer the HCD the channel belong to
 * @index: index in the array of channels 
 * @regs: where the registers for the channel are mapped
 * @urb: the URB, this channel is allocated for
 * @td_list: list of TDs, needed to be processed to get the URB done
 * @current_td: pointer to the TD in the list currently beeing processed 
 * @error_counter: incremented after "transaction error"-event on the channel
 * @nack_counter: incremented after "transaction error"-event on the channel
 * @next_frame:    frame-number of the frame, where this channel should be
 * scheduled again (int) 
 *
 * This structure represents the status of one hardware-channel from the HCD's
 * point of view.  Channels are allocated for one URB and released after all 
 * transfers, needed to process the URB, has completed or and error occured 
 * (see request_channel()/free_channel()). 
 *
 * The channel-structure contains a list of transfer-descriptors (TD)
 * which needs to be processed by the hardware and the pointer to the TD, 
 * currently beeing processed by the hardware.
 * (This is inspired by the way how EHCI-controllers works.) 
 * 
 * The list is initialized for a given URB after a channel has been 
 * allocated (see urb_to_xfer_list()) and is beeing progressed when 
 * "transfer completed"-event on the channel occures 
 * (see channel_completed()). After the last TD has been reached or 
 * an error occurred while processing the current TD, the URB is added 
 * to the done-list (see donelist_add()) and the channel is released. 
 *
 * For BULK/CONTROL-URBs, channels are released after the URB has been 
 * added to the done-list. For INT/ISO-URBs, channels are allocated 
 * for the time the URB is in control of the HCD (until the URB is unlinked 
 * by the call to urb_dequeue()). 
 */
struct dwc3884_channel {
	struct dwc3884_hcd *hcd;
  int index;									

  struct dwc3884_channel_regs *regs;

  struct urb *urb;						
  struct list_head td_list;		
	struct td *current_td;			

  int error_counter;					
	int nack_counter;						
	unsigned long next_frame;		

	unsigned long enabled_at_frame;		
	unsigned long halted_at_frame;		
	u32 halted_with_status;
};

/**
 * struct urb_item - helper for implementing the done-list
 */
struct urb_item {
	struct urb *urb;
	struct list_head urb_list;
};

/**
 * struct dwc3884_hcd - structure holding the status of the driver
 * @lock: synchronizing the access the shared data of the HCD
 * @hcd: needed by the HCD-FW
 * @global: where the global-registers are mapped
 * @host: where the host-registers are mapped
 * @channel: array of channels
 * @n_chan: number of channels supported by the hardware 
 * (supposed to be always less than MAX_CHANNELS)
 * @done_tasklet: tasklet for the processing the done-list
 * @done_urbs: list of URBs beeing ready for completion
 * @prev_framenr: frame-number when last SOF-interrup occured
 * (used for detection of missed SOF-interrupts
 * @frame_counter: long-time frame counter used for scheduling
 *
 * This represent the current state of the HCD, which is basicaly 
 * described by the states of the channels, the content of 
 * the done-list and the value of the frame-counter.
 */
struct dwc3884_hcd {
	spinlock_t   lock;
	struct usb_hcd hcd;

	// core global registers: offset 0x0
	struct dwc3884_global_regs *global;

	// host mode registers: offset 0x400
	struct dwc3884_host_regs   *host;
	
	struct dwc3884_channel channel[MAX_CHANNELS];
	int n_chan;

	// bh of the interrupt handler processes completed(done) urbs
	struct tasklet_struct done_tasklet;
	struct list_head done_urbs;
	
	int prev_framenr;	
	unsigned long frame_counter;
};

#define hcd_to_dwc3884(hcd_ptr) container_of(hcd_ptr, struct dwc3884_hcd, hcd)

/*--------------------------------------------------------------------------*/

/*{{{ global registers */
struct dwc3884_global_regs
{
	u32 otgctl;
	u32 otgint;
	u32 ahbcfg;
#define GINTMSK (1<<0)
#define DMAENA  (1<<5)

	u32 usbcfg;
	u32 rstctl;
	u32 intsts;
	u32 intmsk;
#define DISCONNINT  (1<<29)
#define HCHINT  (1<<25)
#define HPRTINT (1<<24)
#define SOFINT  (1<<3)
#define OTGINT  (1<<2)
#define MODEMIS (1<<1)
#define CURMOD  (1<<0)

	u32 __unused1[6];

	u32 pvndctl;
	u32 gpio;
	u32 uid;
	u32 snpsid;
	u32 hwcfg1;
	u32 hwcfg2;
#define HOST_N_CHANNELS(a) ((((a)>>14)&0xf)+1)

	u32 hwcfg3;
	u32 hwcfg4;

} __attribute__ ((packed));
/*}}}*/

/*{{{ host registers */

struct dwc3884_host_regs
{
	u32 cfg;
	u32 fir;
	u32 frnum;
#define FRAMENUM(v)	(((v)>>0)&0xffff)
#define MAX_FRAMENUM	0x3fff
	
	u32 __reserved1;
	u32 ptxsts;
	u32 aint;
	u32 aintmsk;
	u8 __reserved2[0x440-0x41C];
	u32 portcs[4];
#define PORT_CONNECT (1<<0)
#define PORT_CSC     (1<<1)
#define PORT_PE      (1<<2)
#define PORT_PEC     (1<<3)
#define PORT_OC      (1<<4)
#define PORT_OCC     (1<<5)
#define PORT_RESUME  (1<<6)
#define PORT_SUSPEND (1<<7)
#define PORT_RESET   (1<<8)
#define PORT_POWER   (1<<12)

} __attribute__ ((packed));

enum dwc3884_portspeed { PORT_HS=0, PORT_FS, PORT_LS };

struct dwc3884_channel_regs
{
	u32 characteristics;
#define CHANNEL_MPS(v)   	(((v)>>0)&0x3ff)
#define CHANNEL_EPNUM(v)  (((v)>>11)&0xf)
#define CHANNEL_DIR(v)  	(((v)>>15)&0x1)	
#define CHANNEL_TYPE(v) 	(((v)>>18)&0x3)
#define CHANNEL_DEV(v) 		(((v)>>22)&0x7f)
#define CHAN_DISABLE_BIT 	30
#define CHAN_DISABLE  		(1<<CHAN_DISABLE_BIT)
#define CHAN_ENABLE_BIT 	31
#define CHAN_ENABLE   		(1<<CHAN_ENABLE_BIT)

	u32 splt;
	u32 interrupt;
	u32 intmsk;
#define CHAN_XFERCOMPL  (1<<0)
#define CHAN_HLTD     	(1<<1)
#define CHAN_AHBERR   	(1<<2)
#define CHAN_STALL    	(1<<3)
#define CHAN_NACK    		(1<<4)
#define CHAN_ACK    		(1<<5)
#define CHAN_NYET    		(1<<6)
#define CHAN_XACTERR  	(1<<7)
#define CHAN_BBLERR   	(1<<8)
#define CHAN_FRMOVRUN  	(1<<9)
#define CHAN_DATATGLERR (1<<10)

	u32 tsiz;
	u32 dma;
	u32 __reserved[2];

} __attribute__ ((packed));

/*}}}*/

/*--------------------------------------------------------------------------*/

#undef dbg
#undef err
#undef info
#undef warn

#ifdef DEBUG
#define dbg(format, arg...) printk(KERN_DEBUG __FILE__ ":%d: " format "\n" ,\
		__LINE__, ## arg)
#else
#define dbg(format, arg...)
#endif

#define err(format, arg...) printk(KERN_ERR __FILE__ ": " format "\n" , ## arg)
#define warn(format, arg...) printk(KERN_WARNING __FILE__ ": " format "\n" , ## arg)
#define info(format, arg...) printk(KERN_INFO __FILE__ ": " format "\n" , ## arg)

/*--------------------------------------------------------------------------*/

#endif // __LINUX_DWC3884_HCD_H

