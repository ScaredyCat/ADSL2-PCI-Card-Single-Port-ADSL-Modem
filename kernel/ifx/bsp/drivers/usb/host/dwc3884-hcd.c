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

/* This file contains the host-controller driver for DWC3884-0, 
 * the Synopsys USB 2.0 HC (version 2.20a). 
 *
 * HISTORY:
 * 2006-08-31:
 *  - major documentation update
 *  - extending the logging for performance analysis
 *  - added more logging for debugging the port-disable problem
 * 2006-08-19:
 *  - some small refactorings and documentation updates
 *  - changed the done-urbs handling, which solved some known bugs
 *    and also the performance problems.
 * 2006-08-17:
 *  - interrupt handling redone: bottom-half added
 *  - added reporting on missed SOF-interrupts.
 *  - removed most of the logging. it was the root cause of missing of
 *    many SOF-interrupts.
 * 2006-07-28:
 * 	- free_config() implemented 
 * 	- channel_irq(): extendesions for reporting/handling bus-errors
 * 	- corrected data-toggle/frame-overrun errors for INT-transactions
 * 
 * STATUS:
 * - the driver supports the bulk/control/interrupt transfers, 
 *   which were tested with a couple of different (bulk-only) mass-storage 
 *   devices, hubs and with one CDC-device (pegasus-driver compatible device).
 *
 * TODO: 
 * - support for the PING-protocol
 * - support for isochronous-transfers
 * - support fot split-transactions
 * - add code for statistics and benchmarking (with procfs-interface)  
 * - support for high-bandwidth endpoints
 * - more testing: MS, CDC, printer, hub... (LS: keyboard, mouse...) 
 *   (interrupt-transfers, bus-error handling...)
 * - power-management: suspend/resume
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>
#include <linux/delay.h>
#include <linux/slab.h>

#include <asm/bitops.h>
#include <asm/byteorder.h>
#include <asm/io.h>

#include <asm/danube/danube.h>
#include <asm/irq.h>
#include <asm/danube/irq.h>

/*--------------------------------------------------------------------------*/
/* configuration options: */

#undef DEBUG 1
#define MAX_XFER_SIZE 0x7ffff	//TODO: read this from the hardware
#define MAX_CHANNELS 16
#define HAVE_INTERRUPT_XFERS 1
#define USE_PHY_CONFIG 1

/*--------------------------------------------------------------------------*/

#include "dwc3884.h"

/*--------------------------------------------------------------------------*/
// TODO: add these to the danube/danube.h

#define DANUBE_DWC3884_BASE 0xbe101000

#define DANUBE_RCU_UBSCFG  ((volatile u32*)(DANUBE_RCU_BASE_ADDR + 0x18))
#define DANUBE_USBCFG_PHYSEL_BIT   12
#define DANUBE_USBCFG_HDSEL_BIT    11 // 0:host, 1:device
#define DANUBE_USBCFG_HOST_END_BIT 10 // 0:little_end, 1:big_end

/*--------------------------------------------------------------------------*/
/*{{{ debugging support: */
static const char* pipetype_to_string(int type)
{
	switch (type) {
		case PIPE_INTERRUPT:
			return "INTERRUPT";
		case PIPE_ISOCHRONOUS:
			return "ISOCHRONOUS";
		case PIPE_BULK:
			return "BULK";
		case PIPE_CONTROL:
			return "CONTROL";
		default:
			return "UNKNOWN";
	}
}

static void dump_urb(struct urb *urb)
{
	if(usb_pipecontrol(urb->pipe)) {
		struct usb_ctrlrequest *req = (struct usb_ctrlrequest*)urb->setup_packet;
		dbg("urb@%p: %s(%x), %s, %d/%d, %d:%d, %d",urb,
				pipetype_to_string(usb_pipetype(urb->pipe)), req->bRequest,
				usb_pipeout(urb->pipe) ? "OUT" : "IN",
				urb->actual_length, urb->transfer_buffer_length,
				usb_pipedevice(urb->pipe), usb_pipeendpoint(urb->pipe),
				urb->status); 
	} else if(usb_pipeint(urb->pipe)) {
		dbg("urb@%p: %s, %s, %d/%d, %d:%d, %d, start=%d, interval=%d", urb,
				pipetype_to_string(usb_pipetype(urb->pipe)),
				usb_pipeout(urb->pipe) ? "OUT" : "IN",
				urb->actual_length, urb->transfer_buffer_length,
				usb_pipedevice(urb->pipe), usb_pipeendpoint(urb->pipe),
				urb->status,
				urb->start_frame, urb->interval); 
	} else {
		dbg("urb@%p: %s, %s, %d/%d, %d:%d, %d",urb,
				pipetype_to_string(usb_pipetype(urb->pipe)),
				usb_pipeout(urb->pipe) ? "OUT" : "IN",
				urb->actual_length, urb->transfer_buffer_length,
				usb_pipedevice(urb->pipe), usb_pipeendpoint(urb->pipe),
				urb->status); 
	}
}

static const char* channeltype_to_string(int type)
{
	switch(type) {
		case EPTYPE_BULK:
			return "BULK"; 
		case EPTYPE_CTRL:
			return "CTRL"; 
		case EPTYPE_INT:
			return "INT"; 
		case EPTYPE_ISO:
			return "ISO"; 
		default:
			return "ILL";
	}
}

static const char* pid_to_string(enum dwc3884_pid pid)
{
	switch(pid) {
		case PID_DATA0:
			return "PID_DATA0";
		case PID_DATA2:
			return "PID_DATA2";
		case PID_DATA1:
			return "PID_DATA1";
		case PID_SETUP:
			return "PID_SETUP";
		default:
			return "PID_ILL";
	}
}

static void channel_dump(struct dwc3884_channel *chan)
{
	u32 tmp;
	
	dbg("channel%d:", chan->index );
	tmp = readl(&chan->regs->characteristics);
	dbg("  char    %08x (type=%s, mps=%d, ep=%x, dir=%s, dev=%x)",
			tmp,
			channeltype_to_string(CHANNEL_TYPE(tmp)),
			(tmp>>0)&0x7ff,
			(tmp>>11)&0xf,
			(tmp>>15)&0x1 ? "IN":"OUT",
			(tmp>>22)&0x7f
		 );
	dbg("  splt    %08x", readl(&chan->regs->splt));
	dbg("  int     %08x", readl(&chan->regs->interrupt));
	dbg("  intmsk  %08x", readl(&chan->regs->intmsk));
	tmp = readl(&chan->regs->tsiz);
	dbg("  tsiz    %08x (xfrsize=%d, pktcnt=%d, pid=%s)",
			tmp,
			(tmp>>0)&0x7ffff,
			(tmp>>19)&0x3ff,
			pid_to_string((tmp>>29)&0x3)
		 );
	dbg("  dma     %08x", readl(&chan->regs->dma));
	dbg(" urb=%p", chan->urb);
	dbg(" error_counter=%d", chan->error_counter);
	dbg(" nack_counter=%d", chan->nack_counter);
	dbg(" next_frame=%ld", chan->next_frame);
	dbg(" enabled_at_frame=%ld", chan->enabled_at_frame);		
	dbg(" halted_at_frame=%ld", chan->halted_at_frame);
	dbg(" halted_with_status=%08x", chan->halted_with_status);
}

/*}}}*/

/*--------------------------------------------------------------------------*/
/*{{{ channel event/status management: */

static struct td *td_alloc(int is_input, enum dwc3884_pid pid,
	dma_addr_t dma, int len, int mem_flags)
{
  struct td *td = (struct td*)kmalloc(sizeof(*td), mem_flags);
  if(td) {
    memset(td, 0, sizeof(*td));
    td->is_input = is_input;
    if(pid==PID_SETUP)
      td->is_input = 0;
    td->pid = pid;
    td->dma = dma;
    td->len = len;
    if(pid==PID_SETUP)
      td->len = 8;
    INIT_LIST_HEAD(&td->td_list);
  }
  return td;
}

static void td_free(struct td *td)
{
  kfree(td);
}

/*-------------------------------------------------------------------------*/

static int dwc3884_pipetype(int pipe)
{
  switch (usb_pipetype (pipe)) {
  case PIPE_INTERRUPT:
    return EPTYPE_INT;
  case PIPE_ISOCHRONOUS:
    return EPTYPE_ISO;
  case PIPE_CONTROL:
    return EPTYPE_CTRL;
  default:
    return EPTYPE_BULK;
  }
}

static int urb_to_xfer_list(struct urb*, struct list_head*, int);
static int channel_is_enabled(struct dwc3884_channel* chan); 

/**
 * channel_init - initializes a channel for processing a URB
 *
 * Initializes a just allocated channel for processing a 
 * URB. It's called from the urb_enqueue().
 */
static int channel_init(struct dwc3884_channel *chan,
                        struct urb *urb, int mem_flags)
{
  u32 tmp;
  int status;
  int is_input = usb_pipeout(urb->pipe) ? 0:1;
  int ep_num   = usb_pipeendpoint(urb->pipe);
  int ep_mps   = usb_maxpacket(urb->dev, urb->pipe, !is_input);
  int ep_type  = dwc3884_pipetype(urb->pipe);
  int dev_addr = usb_pipedevice(urb->pipe);
	int mc = 1;										 //1 transaction per (u)frame

  if(channel_is_enabled(chan))
    return -EBUSY;
	
  // init channel characteristics
  tmp  = ep_mps;                //endpoint mps
  tmp |= (ep_num & 0xf)<<11;     //endpoint number
  if(usb_pipeslow(urb->pipe))    //LS-device flag
    tmp |= 1<<17;
  tmp |= (ep_type & 0x3)<<18;    //endpoint type
	tmp |= (mc & 0x3)<<20;				 //multi-count
  tmp |= (dev_addr & 0x7f)<<22;  //device address
  writel(tmp, &chan->regs->characteristics);

  writel(0, &chan->regs->splt);
	writel(~0, &chan->regs->interrupt);
  writel(0, &chan->regs->intmsk);
	writel(0, &chan->regs->tsiz);
  writel(0, &chan->regs->dma);

  // init transfer list
  status = urb_to_xfer_list(urb, &chan->td_list, mem_flags);
  if(status) {
    err("urb_to_xfer_list() failed with %d", status);
    goto done;
  }

	// start with the first TD
	chan->current_td = list_entry(chan->td_list.next, struct td, td_list);

	chan->next_frame = 0;			//XXX: urb->start_frame; 
done:
  return status;
}

/*{{{*/
static int calc_packet_count(int byte_count, int mps);

/**
 * channel_xfer_init - initializes the channel-registers/fields 
 * for the specified TD
 *
 * This is called from channel_enable() to initialize the channel 
 * before the ChEna-bit is set to initiate the processing of the TD.
 */
static void channel_xfer_init(struct dwc3884_channel *chan, struct td *td)
{
	u32 tmp = readl(&chan->regs->characteristics);
	int ep_mps = tmp & 0x7ff;
	int pid = td->pid;
	
	// set direction
	if(td->is_input) 
		tmp |= 1<<15;
	else 
		tmp &= ~(1<<15);
	writel(tmp, &chan->regs->characteristics);

	// reset status & unmask events
	writel(~0, &chan->regs->interrupt);
	set_bit(1, &chan->regs->intmsk); // mask HLTD

	// set xfer_size, packet_count and PID
	tmp = td->len;
	tmp |= (calc_packet_count(td->len, ep_mps) & 0x3ff)<<19;
	if(pid == PID_UNDEFINED) {
		u32 tsiz = readl(&chan->regs->tsiz);
		pid = (tsiz>>29) & 0x3;
	}
	tmp |= pid<<29;
	writel(tmp, &chan->regs->tsiz);

	// set the dma_address
	writel(td->dma, &chan->regs->dma);

	chan->error_counter = 0;
	chan->nack_counter = 0;
}
/*}}}*/

/**
 * channel_enable - initiates the processing of the current transfer 
 * on the channel or does nothing...
 *
 * Calling this on a (disabled) channel initiates the processing 
 * of the current TD in the TD-list. After a succesful return from it, 
 * the software has to wait for any "channel halted"-event from the HC 
 * to get the status of the TD (see channel_irq()). 
 * 
 * This is called first from urb_enqueue() after channel_init() to start
 * the processing of the URB (first TD) and afterward for the other TD
 * in the sequence as they are in the list.
 */
static int channel_enable(struct dwc3884_channel *chan)
{
  if(channel_is_enabled(chan))
    return -EBUSY;

  if(chan->current_td) {
		channel_xfer_init(chan, chan->current_td);
    set_bit(CHAN_ENABLE_BIT, &chan->regs->characteristics);
		chan->enabled_at_frame = chan->hcd->frame_counter;
		dbg("%ld: ch%d enabled (%s/%d)", chan->enabled_at_frame, 
				chan->index, 
				(chan->current_td->is_input ? "IN":"OUT"), 
				chan->current_td->len);
    // from now we wait on some channel event...
  }

  return 0;
}

static int channel_is_enabled(struct dwc3884_channel* chan) 
{
  return test_bit(CHAN_ENABLE_BIT, &chan->regs->characteristics);
}

static int channel_disable(struct dwc3884_channel *chan)
{
  dbg("channel@%p: disabling...", chan->regs);
  set_bit(CHAN_DISABLE_BIT, &chan->regs->characteristics);
  return 0;
}

/*-------------------------------------------------------------------------*/
/*{{{ channel-event handling: */

static void channel_completed(struct dwc3884_channel*);
static void channel_rewind_buffer(struct dwc3884_channel*);

static void free_channel(struct dwc3884_hcd*, struct dwc3884_channel*);

/**
 * donelist_add - adds a URB to the done-list
 *
 * Adds the URB to the done-list and frees the channel allocated
 * for the URB (not for INT-URBs). The done-list contains all URBs 
 * that should be completed in the next call to complete_urbs().
 */
static int donelist_add(struct dwc3884_hcd *dwc3884, struct urb *urb) {
	struct urb_item *item; 
	struct dwc3884_channel *chan = (struct dwc3884_channel*)urb->hcpriv;
	if(!chan)
		return -EINVAL;

	item = kmalloc(sizeof(*item), GFP_ATOMIC);	// called always in interrupt
	if(item) {
		INIT_LIST_HEAD(&item->urb_list);
		item->urb = urb;
		list_add_tail(&item->urb_list, &dwc3884->done_urbs);
		if(!usb_pipeint(urb->pipe))
			free_channel(dwc3884, chan);		
		return 0;
	}
	return -ENOMEM;
}

/**
 * donelist_remove - removes a URB from the done-list
 *
 * This is called after the urb was given back or the completion-handler
 * was called.
 */
static void donelist_remove(struct dwc3884_hcd *dwc3884, struct urb_item *item) {
	list_del(&item->urb_list);
	kfree(item);
}

/**
 * channel_irq - handles all channel-interrupts (events)
 *
 * Contains the channel-event logics. 
 * It is called from the handle_int(), whenever a channel-interrupt 
 * on some channel occures.
 *
 * For more information see the Databook.
 */
static void channel_irq(struct dwc3884_channel *chan)
{
	u32 chan_status = readl(&chan->regs->interrupt);
	writel(chan_status, &chan->regs->interrupt);

	dbg("%ld: ch%d sts %08x", chan->hcd->frame_counter, chan->index, chan_status);
	
	if(chan_status & CHAN_HLTD) {
		chan->halted_at_frame = chan->hcd->frame_counter;
		chan->halted_with_status = chan_status;
	
		if(chan_status == CHAN_HLTD || !chan->urb) {
			// this happens when channel was disabled before...
			printk("ch[%d]: channel disabled...\n\r", chan->index);
			return;
		}
		
		if(chan_status & (CHAN_XFERCOMPL | CHAN_STALL)) {
			// xfer completed...
			if(chan_status & CHAN_STALL) {
				chan->urb->status = -EPIPE;
				donelist_add(chan->hcd, chan->urb);
			} else
				channel_completed(chan);
		
		} else if(chan_status & (CHAN_NACK | CHAN_NYET | CHAN_XACTERR)) {
			// maybe retry xfer...
			channel_rewind_buffer(chan);
			
			if(!usb_pipeint(chan->urb->pipe)) {
				
				if(chan_status & CHAN_XACTERR) {
					dbg("%ld: xact error #%d (status%d=%08x)",
							chan->hcd->frame_counter, chan->error_counter, chan->index, chan_status);
					channel_dump(chan);
					chan->error_counter++;
				} else if(chan_status & CHAN_NACK)
					chan->nack_counter++;
				
				if(chan->error_counter < 10)	{
					channel_enable(chan);
				} else {
					dbg("%s: cancelling urb@%p, too many errors...", 
							__FUNCTION__, chan->urb);
					chan->urb->status = -ENODEV;	// device not responding any more...
					donelist_add(chan->hcd, chan->urb);
				}
				
			}
			
		} else if(chan_status & (CHAN_DATATGLERR | CHAN_FRMOVRUN | CHAN_BBLERR)) {
			if(chan_status & CHAN_DATATGLERR) 	
				dbg("%s: data-toggle error", __FUNCTION__);
			else if(chan_status & CHAN_FRMOVRUN) 	
				dbg("%s: frame-overrun error", __FUNCTION__);
			else if(chan_status & CHAN_BBLERR)
				dbg("%s: babble error", __FUNCTION__);

			dbg("%s: status%d=%08x", __FUNCTION__, chan->index, chan_status);	
			channel_dump(chan);
			chan->urb->status = -EPROTO;
			donelist_add(chan->hcd, chan->urb);
		}
	}
}

/*{{{*/

/**
 * channel_rewind_buffer - rewinds the buffer for a NACK-ed transaction
 *
 * This is called from channel_irq() whenever "NAK resonse received"-
 * event occures on a channel. Upon this event for a pending OUT-transfer 
 * the payload buffer of the current TD needs to be "rewinded" before 
 * the channel can be enabled again. For IN-transfer nothing needs 
 * to be done.
 *
 * For more information see the Databook.
 */
static void channel_rewind_buffer(struct dwc3884_channel* chan)
{
  if(!chan->current_td->is_input) {
    u32 tmp = readl(&chan->regs->tsiz); 	  
		int packet_count = (tmp>>19)&0x3ff;
		int mps = readl(&chan->regs->characteristics)&0x7ff;
		int transferred_packets = 
			calc_packet_count(chan->current_td->len, mps) - packet_count; 
		int transferred_bytes = transferred_packets * mps;
    
	  // rewind buffer	
    chan->current_td->len -= transferred_bytes;
    chan->current_td->dma += transferred_bytes;
    chan->current_td->pid = PID_UNDEFINED;  
		
    chan->urb->actual_length += transferred_bytes;
  }
}

/**
 * channel_completed - handles the completion of the current transfer
 *
 * This is called from channel_irq() whenever "transfer completed"
 * event occures on a channel. The channel is re-enabled from here
 * with the next TD in the list or the URB, currently beeing handled 
 * by the channel, is added to the done-list when the chan->current_td 
 * points to the end of the list.
 */
static void channel_completed(struct dwc3884_channel *chan)
{
	u32 tmp;
	struct td *completed_td = chan->current_td;
	int transferred_bytes;
	
	if(!completed_td)
		BUG();
	
	transferred_bytes = completed_td->len;
	if(completed_td->is_input) {
		tmp = readl(&chan->regs->tsiz);
		int xfer_size = ((tmp>>0) & 0x7ffff);
		transferred_bytes = completed_td->len - xfer_size;
	} 

	if(completed_td->pid != PID_SETUP && completed_td->len > 0) {
		chan->urb->actual_length += transferred_bytes;
	}

	// decide, which TD will be the next one...
	chan->current_td = NULL;
	if(completed_td->is_input && transferred_bytes < completed_td->len) {
		// short packet received: cancel all following input TDs...
		struct list_head *pos = &completed_td->td_list;
		while(pos != &chan->td_list) {
			struct td *td = list_entry(pos, struct td, td_list);
			if(!td->is_input) {
			    //control packet??
				chan->current_td = td;
				break;
			}
			pos = pos->next;
		}
	} else {
		// otherways: go to the next td, if there...
		if(completed_td->td_list.next != &chan->td_list) {
				chan->current_td = list_entry(completed_td->td_list.next, 
						struct td, td_list);
		} 
	}

	if(!chan->current_td) {
		struct urb *urb = chan->urb;
		// save the datatoggle for the next xfer on the endpoint
		int pid = (readl(&chan->regs->tsiz)>>29) & 0x3;
		usb_settoggle(urb->dev, usb_pipeendpoint(urb->pipe),
				usb_pipeout(urb->pipe), (pid==PID_DATA0 ? 0:1));
		urb->status = 0; 
		donelist_add(chan->hcd, chan->urb);
	} else {
		int status = channel_enable(chan); // try the next TD...
		if(status)
			err("channel_enable() failed with %d", status);
	}
}
/*}}}*/

/*}}}*/

/*-------------------------------------------------------------------------*/
/*{{{*/

static int calc_packet_count(int byte_count, int mps) 
{
  int packet_count = 0;
  if(byte_count > 0) {
    packet_count = byte_count / mps;
    if((byte_count % mps) > 0)
      packet_count++;
  } else {
    packet_count = 1;
  }
  return packet_count;
}

/**
 * urb_to_xfer_list - scatters a URB to a list of transfer descriptors
 *
 * This is called from channel_init() to initalize the TD-list of the 
 * channel, which is then processed by the channel.
 *
 * This is based on code from ehci-qh.c.
 */
static int urb_to_xfer_list(struct urb *urb, struct list_head *td_list,
                            int mem_flags)
{
  int max_xfer_size = MAX_XFER_SIZE;  

  int is_output, ep_num, ep_mps;
  int rest;
  dma_addr_t dma;
  struct list_head *pos, *_tmp;
  struct td *td = NULL;
  enum dwc3884_pid pid = PID_DATA1;

  if(!urb || !td_list)
    return -EINVAL;

  is_output = usb_pipeout(urb->pipe);
  ep_num    = usb_pipeendpoint(urb->pipe);
  ep_mps    = usb_maxpacket(urb->dev, urb->pipe, is_output);

  if(ep_mps <= 0)
    return -EINVAL;

  /* {{{ maybe setup stage */
  if(usb_pipecontrol(urb->pipe))
  {
    /* setup stage */
    td = td_alloc(0/*OUT*/, PID_SETUP, urb->setup_dma, 8, mem_flags);
    if(!td)
      goto no_memory;
    list_add_tail(&td->td_list, td_list);

  } else if(usb_pipebulk(urb->pipe))
  {
    int toggle = usb_gettoggle(urb->dev, ep_num, is_output);
    if(!toggle)
      pid = PID_DATA0;
  }
  /*}}}*/

  /*{{{ data stage */
  //winder
  //max_xfer_size -= max_xfer_size % ep_mps;

  rest = urb->transfer_buffer_length;
  dma = urb->transfer_dma;
  while(rest > 0)
  {
    int len = min(rest, max_xfer_size);

    if(!is_output) {
      // input xfers has to be n*MPS long!
      if(len < ep_mps)
        len = ep_mps;
      len -= len % ep_mps;
    }

    td = td_alloc(!is_output, pid, dma, len, mem_flags);
    if(!td)
      goto no_memory;
    list_add_tail(&td->td_list, td_list);

    dma += len;
    rest -= len;

    // only the first TD has defined pid,
    // the following are initialized from the hardware
    pid = PID_UNDEFINED;
  }
  /*}}}*/

  /*{{{ maybe status stage or zero packet */
  if (usb_pipecontrol(urb->pipe))
  {
    // status stage
    if(is_output)
      td = td_alloc(1/*IN*/, PID_DATA1, 0, 0, mem_flags);
    else
      td = td_alloc(0/*OUT*/, PID_DATA1, 0, 0, mem_flags);

    if(!td)
      goto no_memory;
    list_add_tail(&td->td_list, td_list);

  } else if (usb_pipebulk(urb->pipe)
             && (urb->transfer_flags & URB_ZERO_PACKET)
             && !(urb->transfer_buffer_length % ep_mps) )
  {
    // terminating ZLP at the end of output bulk request
    td = td_alloc(!is_output, pid, 0, 0, mem_flags);
    if(!td)
      goto no_memory;
    list_add_tail(&td->td_list, td_list);
  }
  /*}}}*/

  return 0;

no_memory:
  // delete all TDs in the list
  list_for_each_safe(pos, _tmp, td_list)
  {
    struct td *td = list_entry(pos, struct td, td_list);
    list_del(pos);
    td_free(td);
  }
  return -ENOMEM;
}

/*}}}*/

/*}}}*/

/*--------------------------------------------------------------------------*/
/*{{{ root-hub code: */
#include "../hub.h"

/**
 * dwc3884_port_status_changed - checks the status of the specified
 * port (only one) for any changes
 *
 * Returns 0 if nothing changed, 1 otherways.
 */
static int dwc3884_port_status_changed(struct dwc3884_hcd* hcd, int port)
{
  u32 status = readl(&hcd->host->portcs[port]);
  u32 mask = PORT_CSC | PORT_PEC | PORT_OCC;
	int status_changed = status & mask;

	// for debugging the "port disabling" problem...
	if(status & PORT_PEC && !(status & PORT_PE)) {
		int i;
		
		info("%s: port %d disabled: portcs=%08x", 
				__FUNCTION__, port, status);	

		info("frame_counter=%ld", hcd->frame_counter);
		
		for(i = 0; i < hcd->n_chan; i++) {
			u32 allocated_channels = readl(&hcd->host->aintmsk);
			if(allocated_channels & (1<<i)) {
				struct dwc3884_channel *chan = &hcd->channel[i];
				channel_dump(chan);
			}
		}
	}
	
	return status_changed;
}

/**
 * hub_status_data - called by the HCD-FW to poll for
 * status changes on any port of the root-hub
 */
static int hub_status_data (struct usb_hcd *hcd, char *buf)
{
  struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
  unsigned long flags;
  int i, is_changed=0;

  spin_lock_irqsave(&dwc3884->lock, flags);

  buf[0] = 0;
  for(i = 1; i < 5; i++) {
    if(dwc3884_port_status_changed(dwc3884, i-1)) {
      buf[0] |= (1<<i);
      is_changed = 1;
    }
  }

  spin_unlock_irqrestore(&dwc3884->lock, flags);
  return is_changed ? 1 : 0;
}

/*{{{*/

/*
 * get_hub_descriptor - returns the hub-descriptor for the root-hub
 */
static void get_hub_descriptor(struct usb_hcd *hcd, char *buf, u16 wLength)
{
  struct usb_hub_descriptor *desc = (struct usb_hub_descriptor*)buf;
  memset(desc, 0, sizeof(struct usb_hub_descriptor));

  desc->bDescLength = 9;
  desc->bDescriptorType = 0x29;
  desc->bNbrPorts = 1;

  // 0-1: 1="per-port power control"
  u16 temp = 0x0001;
  // 2:  0="not part of a compound device"
  // 3-4: 1="per-port overcurrent reporting"
  temp  |= 0x0008;
  // 5-6: tt think time
  // 7:  port indicators supported
  //desc->wHubCharacteristics = (__force __u16)cpu_to_le16 (temp);
  desc->wHubCharacteristics = (__u16)cpu_to_le16(temp);

  desc->bPwrOn2PwrGood = 50;  // XXX: 100ms, check the manual
  desc->bHubContrCurrent = 0;  // max. power requirements of the hub

  // two bitmaps:  ports removable, and usb 1.0 legacy PortPwrCtrlMask
  desc->DeviceRemovable[0] = 0;   // all devices removable */
  desc->PortPwrCtrlMask[0] = 0xff;  // for USB 1.0 compatibility reasons
}

static int set_port_feature(struct usb_hcd *hcd, u16 wValue, u16 wIndex)
{
  struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
  u32 temp;
  int status = 0;

  dbg("set_port_feature(%p, 0x%x, %d)", hcd, wValue, wIndex);

  wIndex--;
  temp = readl(&dwc3884->host->portcs[wIndex]);

  // don't disable the port by writing 1 to this PrtEna-bit (COW)!!!
  temp &= ~PORT_PE;

	switch (wValue) {
		case USB_PORT_FEAT_POWER:
			dbg("USB_PORT_FEAT_POWER");
			writel(temp | PORT_POWER, &dwc3884->host->portcs[wIndex]);
			break;
		case USB_PORT_FEAT_RESET:
			dbg("USB_PORT_FEAT_RESET");
			// XXX: do asynchronous reset!!!
			writel(temp | PORT_RESET, &dwc3884->host->portcs[wIndex]);
			mdelay(100);
			writel(temp & ~PORT_RESET, &dwc3884->host->portcs[wIndex]);
			break;
		default:
			status = -EINVAL;
	}
  return status;
}

static int clear_port_feature(struct usb_hcd *hcd, u16 wValue, u16 wIndex)
{
  struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
  u32 temp;
  int status = 0;

  dbg("clear_port_feature(%p, 0x%x, %d)", hcd, wValue, wIndex);

  wIndex--;
  temp = readl(&dwc3884->host->portcs[wIndex]);

  // don't disable the port by writing 1 to this PrtEna-bit (COW)!!!
  temp &= ~PORT_PE;

	switch (wValue) {
		case USB_PORT_FEAT_ENABLE:
			dbg("USB_PORT_FEAT_ENABLE");
			writel(temp | PORT_PE, &dwc3884->host->portcs[wIndex]);
			break;
		case USB_PORT_FEAT_POWER:
			dbg("USB_PORT_FEAT_POWER");
			writel(temp & ~PORT_POWER, &dwc3884->host->portcs[wIndex]);
			break;
		case USB_PORT_FEAT_C_CONNECTION:
			dbg("USB_PORT_FEAT_C_CONNECTION");
			writel(temp | PORT_CSC, &dwc3884->host->portcs[wIndex]);	// clear portcs-bit
			break;
		case USB_PORT_FEAT_C_ENABLE:
			dbg("USB_PORT_FEAT_C_ENABLE");
			writel(temp | PORT_PEC, &dwc3884->host->portcs[wIndex]);
			break;
		case USB_PORT_FEAT_C_OVER_CURRENT:
			dbg("USB_PORT_FEAT_C_OVER_CURRENT");
			writel(temp | PORT_OCC, &dwc3884->host->portcs[wIndex]);
			break;
		case USB_PORT_FEAT_C_RESET:
			dbg("USB_PORT_FEAT_C_RESET");
			writel(temp | PORT_PEC, &dwc3884->host->portcs[wIndex]);
			break;
		default:
			status = -EINVAL;
	}

  return status;
}

static int get_port_status(struct usb_hcd *hcd,
                           u16 wIndex, char *buf, u16 wLength)
{
  struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);

  u32 status = 0;
  u32 portcs;

  wIndex--;
  portcs = readl(&dwc3884->host->portcs[wIndex]);

  // wPortChange bits
	// Note: the controller doesn't set the PORT_CSC-bit on disconnection!!!
  if (portcs & PORT_CSC || ((portcs & PORT_PEC) && !(portcs & PORT_CONNECT))) {
    status |= 1 << USB_PORT_FEAT_C_CONNECTION;	
	}
	
  if (portcs & PORT_PEC)
    status |= 1 << USB_PORT_FEAT_C_ENABLE;
  if (portcs & PORT_OCC)
    status |= 1 << USB_PORT_FEAT_C_OVER_CURRENT;

  if ((portcs & PORT_PEC) && (portcs & PORT_PE))
    status |= 1 << USB_PORT_FEAT_C_RESET;

  // wPortStatus bits
  if (portcs & PORT_CONNECT) {
    enum dwc3884_portspeed speed = (portcs>>17)&0x3;	  
    status |= 1 << USB_PORT_FEAT_CONNECTION;
    if(speed == PORT_HS) {
      status |= 1 << USB_PORT_FEAT_HIGHSPEED;
    } 
  }
  if (portcs & PORT_PE)
    status |= 1 << USB_PORT_FEAT_ENABLE;
  if (portcs & PORT_SUSPEND)
    status |= 1 << USB_PORT_FEAT_SUSPEND;
  if (portcs & PORT_OC)
    status |= 1 << USB_PORT_FEAT_OVER_CURRENT;
  if (portcs & PORT_RESET)
    status |= 1 << USB_PORT_FEAT_RESET;
  if (portcs & PORT_POWER)
    status |= 1 << USB_PORT_FEAT_POWER;

  *((u32*)buf) = cpu_to_le32 (status);
  return 0;
}
/*}}}*/


/**
 * hub_control - called by the HCD-FW to initiate a request 
 * to the root-hub
 *
 * This is called when an hub-request is directed to the root-hub.
 * Note: The root-hub is handled the same way as any other 
 * external hub. 
 */
static int hub_control (struct usb_hcd *hcd,
                        u16 typeReq, u16 wValue, u16 wIndex,
                        char *buf, u16 wLength)
{
  struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
  unsigned long flags;
  int status = 0;

  spin_lock_irqsave(&dwc3884->lock, flags);

	switch (typeReq) {
		case ClearHubFeature:
			dbg("ClearHubFeature");
			break;
		case ClearPortFeature:
			status = clear_port_feature(hcd, wValue, wIndex);
			break;
		case GetHubDescriptor:
			get_hub_descriptor(hcd, buf, wLength);
			break;
		case GetHubStatus:
			// dbg("GetHubStatus");
			memset (buf, 0, wLength);
			break;
		case GetPortStatus:
			status = get_port_status(hcd, wIndex, buf, wLength);
			break;
		case SetHubFeature:
			dbg("SetHubFeature");
			break;
		case SetPortFeature:
			status = set_port_feature(hcd, wValue, wIndex);
			break;
		default:
			status = -EINVAL;
	}

  spin_unlock_irqrestore(&dwc3884->lock, flags);
  return status;
}
/*}}}*/

/*--------------------------------------------------------------------------*/
/* {{{ memory lifecycle, channel-management: */

/**
 * request_channel - allocates a channel for specified URB
 *
 * Allocates a channel when possible for processing a specified URB.
 * Returns NULL if there are no free channel at the time.
 * It's called from urb_enqueue(). 
 */
static struct dwc3884_channel* request_channel(struct dwc3884_hcd *hcd,
		struct urb *urb) 
{
	int i;
	for(i = 0; i < hcd->n_chan; i++) {
		struct dwc3884_channel *chan = &hcd->channel[i];
		if(!channel_is_enabled(chan) && !chan->urb) {
			chan->urb = urb;
			urb->hcpriv = chan;
			// enable event-forwarding for the channel
			set_bit(chan->index, &hcd->host->aintmsk);
			return chan;
		}
	}
	return NULL;
}

/**
 * free_channel - de-allocates a channel
 *
 * De-allocates a channel after the processing of the URB has be completed 
 * (see complete_urb()) or cancelled by the application (see urb_dequeue()) or when the device 
 * hast been disconnected (see free_config()).
 */
static void free_channel(struct dwc3884_hcd *hcd, struct dwc3884_channel* chan)
{
	struct urb *urb = chan->urb;
	if(channel_is_enabled(chan)) 
	  channel_disable(chan);	

	if(!list_empty(&chan->td_list)) {	
		struct list_head *pos, *tmp;
		list_for_each_safe(pos, tmp, &chan->td_list) {
			struct td *td = list_entry(pos, struct td, td_list);
			list_del(pos);
			td_free(td);
		}
	} 

	urb->hcpriv = NULL;
	chan->urb = NULL;

	// disable event-forwarding for the channel
	clear_bit(chan->index, &hcd->host->aintmsk);
}

static struct dwc3884_hcd __this_hcd;

static struct usb_hcd *hcd_alloc (void)
{
  struct usb_hcd *hcd = &(__this_hcd.hcd);
  memset (&__this_hcd, 0, sizeof(struct dwc3884_hcd));
  return hcd;
}

static void hcd_free (struct usb_hcd *hcd)
{
}

/*}}}*/

/*--------------------------------------------------------------------------*/
/* {{{ initialization of the HC and the root hub: */

/*{{{*/
static void dwc3884_power_on(void)
{
  // set gpio's for controlling usb power and clock gating
#if 0
  clear_bit(8,  DANUBE_GPIO_P0_ALTSEL0);
  clear_bit(8,  DANUBE_GPIO_P0_ALTSEL1);
  set_bit(8,    DANUBE_GPIO_P0_OD);
  set_bit(8,    DANUBE_GPIO_P0_DIR);
  clear_bit(8,  DANUBE_GPIO_P0_PUDSEL);
  clear_bit(8,  DANUBE_GPIO_P0_PUDEN);
  set_bit(8,    DANUBE_GPIO_P0_OUT);
#endif

  // set clock gating
  set_bit(4,  DANUBE_CGU_IFCCR);
  set_bit(5,  DANUBE_CGU_IFCCR);

  // set power
  clear_bit(0,  DANUBE_PMU_PWDCR);
  clear_bit(6,  DANUBE_PMU_PWDCR);
  clear_bit(15, DANUBE_PMU_PWDCR);
}
/*}}}*/

static int dwc3884_reset(struct dwc3884_hcd *hcd)
{
  u32 tmp;
	// make the hardware be a host controller (default)
  clear_bit(DANUBE_USBCFG_HDSEL_BIT, DANUBE_RCU_UBSCFG);

  // set the HC's byte-order to big-endian
  // TODO: this should probably be set to the CPUs byte-order?!?
  set_bit(DANUBE_USBCFG_HOST_END_BIT, DANUBE_RCU_UBSCFG);

	set_bit(1, &hcd->global->rstctl); // gl reset
  mdelay(10);
	set_bit(0, &hcd->global->rstctl); // core reset
	mdelay(100);
	tmp = readl(&hcd->global->rstctl); 
  if(tmp & 0x3) 
		return -ETIMEDOUT;
  return 0;	
}

static void complete_urbs(unsigned long param);

/*
 * start - called by the HCD-FW to initialize the
 * driver/hardware at the start (after the call to the hcd_alloc())
 */
static int start (struct usb_hcd *hcd)
{
	struct dwc3884_hcd *dwc3884 = list_entry(hcd, struct dwc3884_hcd, hcd);
	struct usb_device *udev;
	struct usb_bus  *bus;
	u32 tmp;
	int status, i;

	dwc3884_power_on();

	spin_lock_init (&dwc_hcd->lock);
	
	tasklet_init(&dwc3884->done_tasklet, complete_urbs, (unsigned long)dwc3884);
	INIT_LIST_HEAD(&dwc3884->done_urbs);
	
	dwc3884->prev_framenr = -1;
	dwc3884->frame_counter = 0;

	dwc3884->global  = (struct dwc3884_global_regs*)hcd->regs;
	dwc3884->host    = (struct dwc3884_host_regs*)(hcd->regs + 0x400);
	struct dwc3884_channel_regs *channel_regs = 
		(struct dwc3884_channel_regs*)(hcd->regs + 0x500);

	tmp = readl(&dwc3884->global->hwcfg2);
	dwc3884->n_chan = HOST_N_CHANNELS(tmp);

	for(i = 0; i < dwc3884->n_chan; i++) {
		struct dwc3884_channel *chan = &dwc3884->channel[i];
		chan->hcd = dwc3884;
		chan->index = i;
		chan->regs = &channel_regs[i];
		chan->urb = NULL;
		INIT_LIST_HEAD(&chan->td_list);
		chan->current_td = NULL;
		chan->error_counter = 0;
	}

	status = dwc3884_reset(dwc3884);

	if(status) {
		err("dwc3884_reset() failed with %d", status);
		return status;
	}

	// check the current mode
	tmp = readl(&dwc3884->global->intsts);
	if(!(tmp & CURMOD)) {
		err("controler is in device mode!");
		return -ENODEV;
	}

	// set the HC mode: 1=FS/LS only
#if CONFIG_USB_DWC3884_FS
	set_bit(2, &dwc3884->host->cfg);
#else
	clear_bit(2, &dwc3884->host->cfg);	
#endif

#if USE_PHY_CONFIG
	writel(0x14014, 0xbe10103c);
#endif
	
	// unmask interrupts
	writel(HCHINT | MODEMIS | SOFINT, &dwc3884->global->intmsk);

	/* {{{ root-hub device init (adapted from ehci-hcd.c) */

	hcd->state = USB_STATE_READY; // after successful HW init ?!?

	/* wire up the root hub */
	bus = hcd_to_bus (hcd);
	bus->root_hub = udev = usb_alloc_dev (NULL, bus);
	if (!udev)
		return -ENOMEM;

	// hcd->state = USB_STATE_RUNNING;  // after the schedules are enabled ?!?

	/* From here on, khubd concurrently accesses the root
	 * hub; drivers will be talking to enumerated devices.
	 *
	 * Before this point the HC was idle/ready.  After, khubd
	 * and device drivers may start it running. */
	usb_connect (udev);

#ifdef CONFIG_USB_DWC3884_FS
	udev->speed = USB_SPEED_FULL;
#else
	udev->speed = USB_SPEED_HIGH;
#endif

	if (usb_new_device (hcd_to_bus(hcd)->root_hub) != 0)
	{
		bus->root_hub = 0;
		usb_free_dev (udev);
		return -ENODEV;
	}

	/*}}}*/

	tmp  = DMAENA | GINTMSK;  // DMAMode=ON, GlobInt=ON
	tmp |= 0x7<<1;            // BurstLen=INCR16
	writel(tmp, &dwc3884->global->ahbcfg);

	return 0;
}

/**
 * stop - called by the HCD-FW when the HCD should cleanly make the HC stop 
 * writing memory and doing I/O (on rmmod)
 */
static void stop (struct usb_hcd *hcd)
{
  struct dwc3884_hcd *dwc3884 = list_entry(hcd, struct dwc3884_hcd, hcd);

  dbg("%s(%p)", __FUNCTION__, hcd);

  clear_bit(0, &dwc3884->global->ahbcfg);  // GlobInt=OFF
  hcd->state = USB_STATE_HALT;
}
/*}}}*/

/*--------------------------------------------------------------------------*/

/** 
 * get_frame_number - called by the HCD-FW to get the current frame number 
 */
static int get_frame_number (struct usb_hcd *hcd)
{
	struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
	u32 tmp = readl(&dwc3884->host->frnum); 
  return (tmp & 0xffff);
}

/*--------------------------------------------------------------------------*/
/*{{{ interrupt handling: */

void mask_and_ack_danube_irq(int);

/**
 * complete_urbs - processes the done-list and completes all done urbs
 *
 * This is the bottom-half of the interrupt handler (see handle_int())
 * responsible for completing done URBs. 
 */
static void complete_urbs(unsigned long param)
{
	struct dwc3884_hcd *dwc3884 = (struct dwc3884_hcd*)param; 
	struct list_head *pos, *tmp;
	unsigned long flags;

	//winder
	spin_lock_irqsave(&dwc3884->lock, flags);

	list_for_each_safe(pos, tmp, &dwc3884->done_urbs) {
		struct urb_item *item = list_entry(pos, struct urb_item, urb_list); 
		if(usb_pipeint(item->urb->pipe)) {
			item->urb->complete(item->urb);
			item->urb->actual_length = 0;
			item->urb->status = -EINPROGRESS;	
		} else {
			dump_urb(item->urb);
			dbg("%ld: giveback urb@%p", dwc3884->frame_counter, item->urb);
			usb_hcd_giveback_urb(&dwc3884->hcd, item->urb, NULL);
		}
		donelist_remove(dwc3884, item);
	}
	spin_unlock_irqrestore(&dwc3884->lock, flags);
}

/**
 * scan_periodic - schedules the periodic-transfers for the next (u)frame
 *
 * This is called from handle_int() to check the next_frame-field of 
 * any channel allocated for a periodic-transfer (URB) if it needs to be 
 * scheduled (enabled) for the next (u)frame.
 */
static void scan_periodic(struct dwc3884_hcd *dwc3884) 
{
	int framenr = FRAMENUM(readl(&dwc3884->host->frnum));
	int i;
	for(i = 0; i < dwc3884->n_chan; i++) {
		struct dwc3884_channel *chan = &dwc3884->channel[i];
		if(!channel_is_enabled(chan) && chan->urb && usb_pipeint(chan->urb->pipe)) {
			struct urb *urb = chan->urb;
			if(chan->next_frame <= dwc3884->frame_counter) {
				chan->current_td = list_entry(chan->td_list.next, struct td, td_list); 
				// let the data-toggle be managed by the hardware...
				chan->current_td->pid = PID_UNDEFINED;	
				chan->next_frame = dwc3884->frame_counter + urb->interval;
				// schedule for the next frame (even/odd)
				if (framenr & 1) {
					clear_bit(29, &chan->regs->characteristics);
				} else {
					set_bit(29, &chan->regs->characteristics);
				}
				channel_enable(chan);
			} 
		}
	}
}

/**
 * handle_int - handles the interrupts from the hardware
 * 
 * The interrupt-handler called from the HCD-FW to handle
 * any event comming from the hardware. It calls the
 * channel_irq() on any event for some channel. Furthermore
 * it updates the frame_counter and initiates the periodic-scheduling
 * on every SOF-interrupt. It also initiates the processing 
 * of done-list (every URB gets done after a channel-interrupt).
 */
static void handle_int (struct usb_hcd *hcd, struct pt_regs *regs)
{
	struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
	u32 int_status = readl(&dwc3884->global->intsts);
	u32 tmp = int_status;
	u32 int_mask = readl(&dwc3884->global->intmsk);
	int i;

	int_status &= int_mask;
	if(int_status) {
		writel(int_status, &dwc3884->global->intsts);
		mask_and_ack_danube_irq(INT_NUM_IM1_IRL22);
		
		if(int_status & SOFINT) {
			/*{{{ check SOFINT sequence: */
			int framenr = FRAMENUM(readl(&dwc3884->host->frnum));
			if(dwc3884->prev_framenr > -1) {
				// check the difference of the succesive framenumbers
				int next_framenr = (dwc3884->prev_framenr + 1) % (MAX_FRAMENUM + 1);
				if(framenr != next_framenr) {
					int diff = framenr - dwc3884->prev_framenr;
					if(diff < 0)
						diff = MAX_FRAMENUM - abs(diff);
					dbg("%ld: missed %d sofints", dwc3884->frame_counter, diff);
				}
			}
			dwc3884->prev_framenr = framenr;
			/*}}}*/
			scan_periodic(dwc3884);
			dwc3884->frame_counter++;	// TODO: correct frame counter by the jitter of sof-interrupts
		}

		if(int_status & HCHINT) {
			// handle channel events
			u32 channel_mask = readl(&dwc3884->host->aint);
			for(i = 0; i < dwc3884->n_chan; i++) {
				if(channel_mask & (1<<i)) {
					channel_irq(&dwc3884->channel[i]);
				}
			}
			mask_and_ack_danube_irq(INT_NUM_IM1_IRL22);
			
			// TODO: schedule the tasklet only if some urb got completed
			tasklet_schedule(&dwc3884->done_tasklet);
		}
		
		if(int_status & MODEMIS) {
			err("mode mismatch detected!");
			// TODO: handle mode-mismatch error
		}
		
	} else {
		mask_and_ack_danube_irq(INT_NUM_IM1_IRL22); 	  
		warn("interrupt sharing: not for us");
		dbg("  intsts=%08x, intmsk=%08x", tmp, int_mask);
	}
	mask_and_ack_danube_irq(INT_NUM_IM1_IRL22); 	  
}
/*}}}*/

/*--------------------------------------------------------------------------*/
/* {{{  i/o requests management: */

/** 
 * urb_enqueue - called by the HCD-FW to register a new URB for processing
 * by this HCD
 *
 * Note: Non-error returns are a promise to giveback() the urb later.
 */
static int urb_enqueue (struct usb_hcd *hcd,
                        struct urb *urb, int mem_flags)
{
	struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
	struct dwc3884_channel *chan = NULL;
	int status = 0;
	unsigned long flags;

	dump_urb(urb);
	dbg("%ld: enqueue urb@%p", dwc3884->frame_counter, urb);

	switch (usb_pipetype (urb->pipe)) {
#if HAVE_INTERRUPT_XFERS==0
		case PIPE_INTERRUPT:
			return -ENOSYS;
#endif
		case PIPE_ISOCHRONOUS:
			return -ENOSYS; // not supported yet
	}

	spin_lock_irqsave(&dwc3884->lock, flags);

	chan = request_channel(dwc3884, urb);
	if(!chan) {
		err("request_channel() failed");
		status = -EBUSY;
		goto done;	
	}

	status = channel_init(chan, urb, mem_flags);
	if(status) {
		err("channel_init() failed with %d", status);
		goto done;
	}

	if(!usb_pipeint(urb->pipe)) {
		status = channel_enable(chan);
		if(status) {
			err("channel_enable() failed with %d", status);
			goto done;
		}
	}
	
done:
	if(status && chan)
		free_channel(dwc3884, chan);
	spin_unlock_irqrestore(&dwc3884->lock, flags);
	return status;
}

/** 
 * urb_dequeue - called by the HCD-FW to remove a URB from
 * this HCD
 *
 * This is called either on timeout or disconnect for non-periodic URBs 
 * to free the allocated resources. Furthermore is this called on 
 * periodic-URBs to deregister them from the schedule.
 * 
 * Note: Completions normally happens asynchronously (in interrupt).
 */
static int urb_dequeue (struct usb_hcd *hcd, struct urb *urb)
{
  struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
  struct dwc3884_channel *chan = NULL;
  unsigned long flags;

  dbg("%s(%p, %p)", __FUNCTION__, hcd, urb);

  chan = (struct dwc3884_channel*)urb->hcpriv;
  if(!chan) {
    // already dequeued or never enqueued...	  
    return 0;	
  }

	spin_lock_irqsave(&dwc3884->lock, flags);

  // FIXME: freeing channel should happen in the interrupt routine
  // - use AINTMSK bits for marking allocated/free channels
  // - the last event on allocated channel should be CHAN_HLTD after
  //   a channel_disable() or free_channel()

  free_channel(dwc3884, chan);

  spin_unlock_irqrestore(&dwc3884->lock, flags);

  dbg("killing urb@%p...", urb);
	urb->status = -ENOENT;
  usb_hcd_giveback_urb(hcd, urb, NULL);  // we release the URB now...

  return 0;
}
/*}}}*/

/*--------------------------------------------------------------------------*/

/**
 * free_config - called by the HCD-FW to free configuration resources 
 * allocated as needed during urb_enqueue(), and not freed by urb_dequeue()
 */
static void free_config (struct usb_hcd *hcd, struct usb_device *dev)
{
  struct dwc3884_hcd *dwc3884 = hcd_to_dwc3884(hcd);
  struct dwc3884_channel *chan = NULL;
  unsigned long flags;
	
	struct hcd_dev *hcdev = (struct hcd_dev*)dev->hcpriv;
	struct urb* urb;
	struct list_head *pos;

	info("%s(%p, %p)", __FUNCTION__, hcd, dev);

	spin_lock_irqsave(&dwc3884->lock, flags);
	
	list_for_each (pos, &hcdev->urb_list) {
		urb = list_entry (pos, struct urb, urb_list);
		chan = (struct dwc3884_channel*)urb->hcpriv;
		if(chan) 
			free_channel(dwc3884, chan);
	}

	spin_unlock_irqrestore(&dwc3884->lock, flags);
}

/*--------------------------------------------------------------------------*/
/* {{{ module/hcd-framework init/register */

static const struct hc_driver dwc3884_driver = {
    .description =  "dwc3884-hcd",
    .flags =  HCD_MEMORY | HCD_USB2 | HCD_NONPCI,
    .irq =   handle_int,
    .start =  start,
    .stop =   stop,
    .hcd_alloc =  hcd_alloc,
    .hcd_free =  hcd_free,
    .urb_enqueue =  urb_enqueue,
    .urb_dequeue =  urb_dequeue,
    .free_config =  free_config,
    .get_frame_number = get_frame_number,
    .hub_status_data = hub_status_data,
    .hub_control =  hub_control,
};

/*--------------------------------------------------------------------------*/
/* module parameter definitions */

int irq = INT_NUM_IM1_IRL22; // see danube/irq.h
MODULE_PARM(irq, "i");
MODULE_PARM_DESC(irq, "the interrupt number");

unsigned long iomem_base = DANUBE_DWC3884_BASE;
MODULE_PARM(iomem_base, "l");
MODULE_PARM_DESC(iomem_base, "the i/o memory address");

#define IOMEM_LEN 0x1000 //XXX: check the right value

/*--------------------------------------------------------------------------*/
/* module init/cleanup functions */

static int __init dwc3884_init (void)
{
  return usb_hcd_probe(&dwc3884_driver, irq, iomem_base, IOMEM_LEN);
}
module_init (dwc3884_init);

static void __exit dwc3884_cleanup (void)
{
  usb_hcd_remove(&__this_hcd.hcd);
}
module_exit (dwc3884_cleanup)

MODULE_LICENSE("GPL");

/*}}}*/

