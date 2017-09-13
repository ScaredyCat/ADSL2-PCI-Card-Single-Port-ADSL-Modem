/*
 * hardware independent gadget queue handling functions
 *
 * Copyright (C) 2006 EMSYS GmbH, Ilmenau, Germany, GmbH (http://www.emsys.de)
 * Copyright (C) 2006 Christian Schroeder (christian.schroeder@emsys.de)
 *
 */  

/*
 * count the entries in a td queue (hw independent)
 *
 */
static unsigned int count_queue(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  unsigned int                i  = 0;
  GADGET_NAME(__GADGET,_ep*)  ep = &dev->ep[epnum];
  GADGET_NAME(__GADGET,_td*)  td;
 
  list_for_each_entry(td, &ep->queue, queue) {
    i++;
  }
  return i;
}

/*
 * Starts the transfer queue for a specific endpoint (hw independent)
 *
 */
static void start_queue(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{

  GADGET_NAME(__GADGET,_td*) td;
  GADGET_NAME(__GADGET,_ep*) ep = &dev->ep[epnum];

  if(!ep_disabled(dev,epnum,EP_STALL)) {

    list_for_each_entry(td, &ep->queue, queue) {
      DEBUG(dev,"found td: (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
            td->req,td->buf,td->length,td->seqnum,td->flags);
    }
    
    td = list_entry(ep->queue.next,GADGET_NAME(__GADGET,_td),queue);
    if (!td) {
      WARN(dev,"tried to start an empty transfer queue.\n");
      return;
    }
    
    DEBUG(dev,"transfer queue started for ep %d.\n",epnum);
  
    if (dev->ep[epnum].dir == USB_DIR_OUT || TEST_FLAG(TD_IS_CTRL_OUT,&(td->flags))) {
      arm_fifo(dev,epnum,USB_DIR_OUT);
    }
    else {
      arm_fifo(dev,epnum,USB_DIR_IN);
    }
    CLEAR_FLAG(EP_QUEUE_STOPPED,&(ep->flags));
  }
  else {
    DEBUG(dev,"queue not started, ep%d stalled.\n",epnum);
  }
}

/*
 * interrupts the transfer queue and prevents further request processing (hw
 * independent)
 *
 */
static void stop_queue(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  GADGET_NAME(__GADGET,_ep*) ep = &dev->ep[epnum];
  if (!TEST_FLAG(EP_QUEUE_STOPPED,&(ep->flags))) {
    DEBUG(dev,"transfer queue stopped for ep %d.\n",epnum);
    SET_FLAG(EP_QUEUE_STOP_PENDING,&(ep->flags));
  }
}

/*
 * clears a queue deleting all pending transfers (hw independent)
 *
 */
static void clear_queue(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, int status)
{
  GADGET_NAME(__GADGET,_td*) td;
  GADGET_NAME(__GADGET,_ep*) ep = &dev->ep[epnum];

  /* stop the endpoint first, preventing further request queuing */
  ep_stop(dev,epnum);
  DEBUG(dev,"clear transfer queue for ep %d.\n",epnum);
  while (!list_empty (&ep->queue)) {
    td = list_entry(ep->queue.next,GADGET_NAME(__GADGET,_td),queue);
    if (TEST_FLAG(TD_IS_LAST,&(td->flags))) {
      /* last td which belongs to a request */
      if (td->req) { 
        CLEAR_FLAG(REQ_IS_QUEUED,&(td->req->flags));
        spin_unlock(&dev->lock);
        td->req->req.status = status;
        td->req->req.complete(&ep->ep,&td->req->req);
        spin_lock(&dev->lock);
      }
    }
    DEBUG(dev,"unlink td %p\n",td);
    list_del_init(&td->queue);
    kfree(td);
  }
  ep_start(dev,epnum);
}

/*
 * finalizes the current request and restarts the queue (hw independent)
 *
 */
static void complete_queue(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  GADGET_NAME(__GADGET,_td*)      td_iter;
  GADGET_NAME(__GADGET,_td*)      current_td  = 0;
  GADGET_NAME(__GADGET,_request*) current_req = 0;
  GADGET_NAME(__GADGET,_ep*)      ep = &dev->ep[epnum];

  DEBUG(dev,"complete transfer queue.\n");

  if (list_empty(&ep->queue)) {
    WARN(dev,"can't complete an empty transfer queue.\n");
    return;
  }

  /* the current td has to be at the top of the queue */
  current_td = list_entry(ep->queue.next,GADGET_NAME(__GADGET,_td),queue);

  if (current_td) {
    current_req = current_td->req;
    DEBUG(dev,"current td: r=%p,b=%p,l=%ld,s=%ld,f=%.2x.\n",
          current_td->req,current_td->buf,current_td->length,current_td->seqnum,current_td->flags);
  }
  DEBUG(dev,"%d entries in queue.\n",count_queue(dev,epnum));

  if (!TEST_FLAG(TD_IS_LAST,&(current_td->flags))) {
    if(TEST_FLAG(TD_IS_SHORT,&(current_td->flags))) {
      DEBUG(dev,"td is short for ep[%d]: (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
            epnum,
            current_td->req,current_td->buf,current_td->length,current_td->seqnum,current_td->flags);
      if (current_req) {
        CLEAR_FLAG(REQ_IS_QUEUED,&(current_req->flags));

#if GADGET_NEEDS_ALIGNED_BUFFERS
       /* Obviously the higher application layer submits a non aligned buffer, but we can use only
          buffers, which are already 32 bit aligned. This means that we have to copy the buffer
          to a aligned one temporarily. We have to restore the original buffer pointer here. */
 
        if(TEST_FLAG(REQ_IS_UNALIGNED,&(current_req->flags))) {
          /* FIXME: do we have to copy for ep0 too ??? */
          if(ep->dir == USB_DIR_OUT) {
            memcpy(current_req->unaligned_buf, current_req->req.buf, current_req->req.length);
          }
         
          kfree(current_req->req.buf);
          current_req->req.buf = current_req->unaligned_buf;
          CLEAR_FLAG(REQ_IS_UNALIGNED,&(current_req->flags));
        } 
#endif

         /* call complete handler */
         spin_unlock (&dev->lock);
  	 /* call complete callback */
         DEBUG(dev,"complete callback on %p,%p,%p,%p.\n",
               current_req,
               current_req->req.complete,
               &ep->ep,
               &current_req->req);
         /* TODO check this! */
         current_req->req.status = 0;
	 current_req->req.complete(&ep->ep,&current_req->req);
	 spin_lock (&dev->lock);
      }
    
      while (!list_empty (&ep->queue)) {
        td_iter = list_entry(ep->queue.next,GADGET_NAME(__GADGET,_td),queue);
        if (td_iter->req != current_req || TEST_FLAG(TD_IS_STATUS,&(td_iter->flags))) {
          break;
        }
        list_del_init(&td_iter->queue);
        kfree(td_iter);
      }
    }
    else {
      /* not last td of a request, unlink and destroy it */
      list_del_init(&current_td->queue);
      kfree(current_td);
    }
  }
  else {
    /* last td in a request */
    if (current_req) {
      CLEAR_FLAG(REQ_IS_QUEUED,&(current_req->flags));

#if GADGET_NEEDS_ALIGNED_BUFFERS
       if(TEST_FLAG(REQ_IS_UNALIGNED,&(current_req->flags))) {
         /* if its an out buffer copy the data back to the original buffer */
         /* FIXME: do we have to copy for ep0 too? */
         if(ep->dir == USB_DIR_OUT) {
           memcpy(current_req->unaligned_buf, current_req->req.buf, current_req->req.length);
         }
         
         kfree(current_req->req.buf);
         current_req->req.buf = current_req->unaligned_buf;
         CLEAR_FLAG(REQ_IS_UNALIGNED,&(current_req->flags));
       } 
#endif
      /* call complete handler */
	  spin_unlock (&dev->lock);
	  /* call complete callback */
          DEBUG(dev,"complete callback on %p,%p,%p,%p.\n",
          current_req,
          current_req->req.complete,
          &ep->ep,
          &current_req->req);
	  current_req->req.complete(&ep->ep,&current_req->req);
	  spin_lock (&dev->lock);
    }
    list_del_init(&current_td->queue);
    kfree(current_td);
  }

  /* stop queue if queue stop is pending */
  if (TEST_FLAG(EP_QUEUE_STOP_PENDING,&(ep->flags))) {
    SET_FLAG(EP_QUEUE_STOPPED,&(dev->ep[epnum].flags));
    CLEAR_FLAG(EP_QUEUE_STOP_PENDING,&(ep->flags));
    DEBUG(dev,"transfer queue at ep %d stopped.\n",epnum);
    return;
  }

  if (count_queue(dev,epnum)) {
    current_td = list_entry(ep->queue.next,GADGET_NAME(__GADGET,_td),queue);
    DEBUG(dev,"current td: r=%p,b=%p,l=%ld,s=%ld,f=%.2x.\n",
          current_td->req,current_td->buf,current_td->length,current_td->seqnum,current_td->flags);
  }
  else {
    current_td = 0;
  }

  DEBUG(dev,"%d entries in queue.\n",count_queue(dev,epnum));

  /* process next td if any and if the endpoint is still usable */
  if (current_td && ep_ok(dev,epnum)) {
    if (!current_td->seqnum) {
      init_xfer(dev,epnum,current_td->flags,current_td->req);
      start_queue(dev,epnum);
    }
    else {
      /* td belongs to the current request */
      start_queue(dev,epnum);
    }
  }
  else {
    SET_FLAG(EP_QUEUE_STOPPED,&(ep->flags));
    DEBUG(dev,"transfer queue at ep %d stopped.\n",epnum);
  }
}
