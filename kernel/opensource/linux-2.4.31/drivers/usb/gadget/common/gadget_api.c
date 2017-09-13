/*
 * hardware independent gadget api functions
 *
 * Copyright (C) 2006 EMSYS GmbH, Ilmenau, Germany, GmbH (http://www.emsys.de)
 * Copyright (C) 2006 Christian Schroeder (christian.schroeder@emsys.de)
 *
 */  

/*------- Part 9.1: ---------- the endpoint operations table -----------------*/

/*
 * enables a non-crtl endpoint for operation
 *
 */
static int GADGET_NAME(__GADGET,_enable)(struct usb_ep* _ep,
                                         const struct usb_endpoint_descriptor *desc)
{
  GADGET_NAME(__GADGET,_dev*)    dev;
  GADGET_NAME(__GADGET,_ep*)     ep;
  unsigned long                  flags;

  ep = container_of(_ep,GADGET_NAME(__GADGET,_ep),ep);
  DEBUG(dev,"usb_ep_enable ep %d (mps=%d).\n",ep->num,ep->ep.maxpacket);

  /* no iso support at the moment */
  if ((desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_ISOC) {
    return -EOPNOTSUPP;
  }

  if (!_ep || !desc || _ep->name == ep0name || desc->bDescriptorType != USB_DT_ENDPOINT) {
    return -EINVAL;
  }


  dev = ep->dev;

  if (!dev->driver || dev->gadget.speed == USB_SPEED_UNKNOWN) {
    return -ECONNRESET;
  }

  spin_lock_irqsave(&dev->lock, flags);
  ep_stop(ep->dev,ep->num);
  ep->desc = desc;

  /* enable means reset followed by start */
  if (ep_reset(ep->dev,ep->num)) {
    spin_unlock_irqrestore(&dev->lock, flags);
    return -EINVAL;
  }
  ep_activate(ep->dev,ep->num);
  ep_enable(ep->dev,ep->num,EP_NO_DESTALL);
  ep_start(ep->dev,ep->num);
  spin_unlock_irqrestore(&dev->lock, flags);

  return OK;
}

/*
 * disables a non-crtl endpoint for operation and clears its request queue
 *
 */
static int GADGET_NAME(__GADGET,_disable)(struct usb_ep* _ep)
{
  GADGET_NAME(__GADGET,_ep*) ep;
  unsigned long              flags;

  ep = container_of(_ep,GADGET_NAME(__GADGET,_ep),ep);
  DEBUG(dev,"usb_ep_disable ep %d.\n",ep->num);

  if (!_ep || !ep->desc || _ep->name == ep0name || (ep->dir != USB_DIR_IN &&
                                                    ep->dir != USB_DIR_OUT)) {
    return -EINVAL;
  }

  spin_lock_irqsave(&ep->dev->lock, flags);

  /* disable means stop, queue clearing, deactivate */
  ep_stop(ep->dev,ep->num);
  ep->desc = 0; 
  stop_queue(ep->dev,ep->num);
  clear_queue(ep->dev,ep->num,-ESHUTDOWN);
  //ep_disable(ep->dev,ep->num,EP_NO_STALL);
  ep_deactivate(ep->dev,ep->num);

  spin_unlock_irqrestore(&ep->dev->lock, flags);
  return OK;
}

/*
 * allocate request to use with an endpoint, we are using kmalloc() because
 * we don't know, if the PCI subsystem is available on the target.
 *
 */
static struct usb_request* GADGET_NAME(__GADGET,_alloc_request)(struct usb_ep* _ep,
                                                                int gfp_flags)
{
  GADGET_NAME(__GADGET,_ep*)      ep;
  GADGET_NAME(__GADGET,_request*) req;

  ep = container_of(_ep,GADGET_NAME(__GADGET,_ep),ep);
  DEBUG(dev,"usb_ep_alloc_request at ep %d\n",ep->num);

  req = kmalloc(sizeof *req, gfp_flags);

  if (!req) {
    ERROR(dev,"request allocation failed.\n");
    return 0;
  }

  memset(req, 0, sizeof *req);
  INIT_LIST_HEAD(&req->queue);

  return &req->req;
}

/*
 * frees a request
 *
 */
static void GADGET_NAME(__GADGET,_free_request)(struct usb_ep* _ep,
                                                struct usb_request* _req)
{
  GADGET_NAME(__GADGET,_ep*)      ep;
  GADGET_NAME(__GADGET,_request*) req;

  ep = container_of(_ep,GADGET_NAME(__GADGET,_ep),ep);
  req = container_of(_req,GADGET_NAME(__GADGET,_request),req);
  DEBUG(dev,"usb_ep_free_request at ep%d.\n",ep->num);

  kfree(req);
}

/*
 * allocates request buffer.
 *
 */

static void* GADGET_NAME(__GADGET,_alloc_buffer)(struct usb_ep* _ep,
                                                 u32 len,
                                                 dma_addr_t* dma,
                                                 int gfp_flags)
{
  GADGET_NAME(__GADGET,_ep*) ep;
  void*                      retval = 0;

  ep = container_of(_ep,GADGET_NAME(__GADGET,_ep),ep);
  DEBUG(dev,"usb_ep_alloc_buffer at ep%d (size = %d).\n",ep->num,len);

  /* FIXME: kmalloc cannot provide larger buffers than 128 kBytes */
  if (len > 0x20000) {
    ERROR(dev,"kmalloc can't allocate more than 128 kBytes.\n");
    BUG();
    return 0;
  }

  /* FIXME: we expect a buffer here, which is multiple of max packet size
            if we expect to receive data or if this is ep0 */
  if ((ep->dir == USB_DIR_OUT || !ep->num) && (len % _ep->maxpacket)) {
    ERROR(dev,"expecting a buffer with a size on a multiple of max packet size.\n");
    return 0;
  }

  if (!_ep) {
    return 0;
  }

#if !GADGET_SUPPORTS_DMA
  *dma = DMA_ADDR_INVALID;
  retval = kmalloc(len, gfp_flags);
#else
  retval = kmalloc(len, gfp_flags | GFP_DMA);
  if (retval) {
    *dma = __pa(retval);
    DEBUG(dev,"Got DMA Buffer.\n");
  }
  else {
    *dma = DMA_ADDR_INVALID;
  }
#endif

  if (!retval) {
    ERROR(dev,"buffer allocation failed.\n");
  }
  memset(retval,0x00,len);

  return retval;
}

/*
 * frees a previously allocated request buffer
 *
 */
static void GADGET_NAME(__GADGET,_free_buffer)(struct usb_ep *_ep,
                                               void *buf, dma_addr_t dma,
                                               u32 len)
{
  GADGET_NAME(__GADGET,_ep*) ep;

  ep = container_of(_ep,GADGET_NAME(__GADGET,_ep),ep);
  DEBUG(dev,"usb_ep_free_buffer at ep%d (size = %d).\n",ep->num,len);

  kfree (buf);
#if GADGET_SUPPORTS_DMA
  /* something to do here ? */
#endif
}

/*
 * Push a request in the endpoint's request queue and process it immediately
 * if the queue is empty, the complete function has to be called by the
 * transfer_complete interrupt handler, which also dequeues the request after
 * completion.
 *
 */
static int GADGET_NAME(__GADGET,_queue)(struct usb_ep *_ep,
                                        struct usb_request* _req,
                                        int gfp_flags)
{
  GADGET_NAME(__GADGET,_request*) req;
  GADGET_NAME(__GADGET,_ep*)      ep;
  GADGET_NAME(__GADGET,_dev*)     dev;
  unsigned long                   flags;
  byte                            td_flags = 0;

  req = container_of(_req,GADGET_NAME(__GADGET,_request),req);
  ep = container_of (_ep,GADGET_NAME(__GADGET,_ep),ep);

  DEBUG(dev,"usb_ep_queue at ep%d.\n",ep->num);

  if (!_req || !_req->complete || !_req->buf) {
    return -EINVAL;
  }

  if (!_ep || (!ep->desc && ep->num != 0)) {
    return -EINVAL;
  }

  dev = ep->dev;

  if (ep->num && !_req->length && _req->zero) {
    DEBUG(dev,"ignore double zero packet at ep %d.\n",ep->num);
    return OK;
  }
  
  DEBUG(dev,"usb_ep_queue at ep %d (buf=%p, len=%d, zero=%d).\n",
        ep->num,
        req->req.buf,
        req->req.length,
        req->req.zero);

  if (!dev->driver ||
      dev->gadget.speed == USB_SPEED_UNKNOWN) {
    return -ECONNRESET;
  }

#if GADGET_NEEDS_ALIGNED_BUFFERS
  /* Obviously the higher application layer submits a non aligned buffer, but we can use only
     buffers, which are already 32 bit aligned. This means that we have to copy the buffer 
     to a aligned one temporarily. We have to save the original buffer pointer additionally, to
     be able later to restore the original request. */

  if (req->req.length && ((u32) req->req.buf) % 4) {
    SET_FLAG(REQ_IS_UNALIGNED,&(req->flags));
    req->unaligned_buf = req->req.buf;

    req->req.buf = kmalloc(req->req.length, GFP_ATOMIC | GFP_DMA);
    if(ep->dir == USB_DIR_IN) {
      /* FIXME: do we have to copy for ep0 too ??? */
      memcpy(req->req.buf, req->unaligned_buf, req->req.length);
    }
  }
#endif

  spin_lock_irqsave(&dev->lock, flags);

  _req->status = -EINPROGRESS;
  _req->actual = 0;

  if (!ep->num && TEST_FLAG(DEV_CTRL_REQ_WRITE,&(dev->flags))) {
    /* control request with data out stage is pending */
    if(!TEST_FLAG(DEV_CTRL_REQ_DATALESS,&(dev->flags))) { 
      DEBUG(dev,"control data out stage.\n");
    }
    SET_FLAG(TD_IS_CTRL_OUT,&td_flags);
  }
 
  if (req && list_empty(&ep->queue)) {
    if (!ep->num && !req->req.length && TEST_FLAG(DEV_CTRL_REQ_DATALESS,&(dev->flags))) {
      /* FIXME: dataless control request is pending, it seems that the gadget 
                driver tries to indicate this by a single zero data packet, we 
                should ignore this an have to queue a status in stage */
      DEBUG(dev,"pseudo zero data stage.\n");
      init_xfer(dev,0,0,0);
      setup_xfer(dev,0,0,0);
      start_queue(dev,0);
    }
    else {
      init_xfer(dev,ep->num,td_flags,req);
      setup_xfer(dev,ep->num,td_flags,req);
      start_queue(dev,ep->num);
    }
  }
  else if (req) {
    setup_xfer(dev,ep->num,td_flags,req);
  }
  else {
    BUG();
  }

  SET_FLAG(REQ_IS_QUEUED,&(req->flags));

  spin_unlock_irqrestore(&dev->lock, flags);
  return OK;
}

/*
 * Removes a request from the endpoint's queue. Note, that this
 * is not the normal dequeuing mechanism, which is done by the
 * transfer_complete interrupt_handler.
 *
 */
static int GADGET_NAME(__GADGET,_dequeue)(struct usb_ep *_ep,
                                          struct usb_request* _req)
{
  GADGET_NAME(__GADGET,_td*)      td;
  GADGET_NAME(__GADGET,_ep*)      ep;
  GADGET_NAME(__GADGET,_request*) req;
  unsigned long                   flags;
  struct list_head*               iter;

  ep = container_of(_ep,GADGET_NAME(__GADGET,_ep),ep);
  DEBUG(dev,"usb_ep_dequeue at ep %d.\n",ep->num);

  if (!_ep || (!ep->desc && ep->num != 0) || !_req) {
    return -EINVAL;
  }

  req = container_of(_req,GADGET_NAME(__GADGET,_request),req);

  if (!_req || !_req->complete || !_req->buf || !list_empty(&req->queue)) {
    return -EINVAL;
  }

  spin_lock_irqsave(&ep->dev->lock, flags);

  /* stop endpoint and request queue */
  ep_stop(ep->dev,ep->num);
  stop_queue(ep->dev,ep->num);

  iter = ep->queue.next;

  /* unlink request from queue */
  while (!list_empty(&ep->queue)) {
    td = list_entry(iter,GADGET_NAME(__GADGET,_td),queue);
    iter=td->queue.next;
    if (td->req == req) {
      if (TEST_FLAG(TD_IS_LAST,&(td->flags))) {
        /* last td which belongs to a request */
        CLEAR_FLAG(REQ_IS_QUEUED,&(req->flags));
        spin_unlock(&ep->dev->lock);
        req->req.status = -ECONNRESET;
        req->req.complete(&ep->ep,&req->req);
        spin_lock(&ep->dev->lock);
      }
      DEBUG(dev,"removing td: l=%lx,sq=%ld,queue(%p,%p).\n",td->length,td->seqnum,td->queue.prev,td->queue.next);
      list_del_init(&td->queue);
      kfree(td);
    }
  }

  /* restart endpoint and request queue with the first request in it*/
  ep_start(ep->dev,ep->num);
  start_queue(ep->dev,ep->num);

  spin_unlock_irqrestore(&ep->dev->lock, flags);
  return OK;
}

/*
 * Stalls or destalls an endpoint, probably not applicable for ep0.
 *
 */
static int GADGET_NAME(__GADGET,_set_halt)(struct usb_ep* _ep,
                                           int value)
{
  GADGET_NAME(__GADGET,_ep*) ep;
  unsigned long              flags;
  int			                   retval = OK;

  ep = container_of (_ep,GADGET_NAME(__GADGET,_ep),ep);
  DEBUG(dev,"usb_ep_set_halt at ep%d.\n",ep->num);

  if (!_ep /*|| (!ep->desc && ep->num != 0)*/ ) {
    set_interrupt(ep->dev,FALSE);
    return -EINVAL;
  }

  if (!ep->dev->driver || ep->dev->gadget.speed == USB_SPEED_UNKNOWN) {
    return -ECONNRESET;
  }

  spin_lock_irqsave(&ep->dev->lock, flags);

  if (!list_empty (&ep->queue)) {
    retval = -EAGAIN;
	}
	else {
		if (value) {
      ep_disable(ep->dev,ep->num,EP_STALL);
		} else
      ep_enable(ep->dev,ep->num,EP_DESTALL);
	}

  spin_unlock_irqrestore(&ep->dev->lock, flags);
	return retval;
}

/*
 * Reports the status of a specific endpoint (fifo), counting the
 * available/remaining bytes in it.
 *
 */
static int GADGET_NAME(__GADGET,_fifo_status)(struct usb_ep* _ep)
{
  GADGET_NAME(__GADGET,_ep*) ep;
  int                        avail;

  ep = container_of (_ep,GADGET_NAME(__GADGET,_ep),ep);

  DEBUG(dev,"usb_ep_fifo_status at ep %d.\n",ep->num);

  if (!_ep || (!ep->desc && ep->num != 0)) {
    return -ENODEV;
  }

  if (!ep->dev->driver || ep->dev->gadget.speed == USB_SPEED_UNKNOWN) {
    return -ECONNRESET;
  }

  avail = ep_count(ep->dev,ep->num);

  if (avail > ep->fifo_size)
    return -EOVERFLOW;
  if (ep->dir == USB_DIR_IN) {
    avail = ep->fifo_size - avail;
  }

  return avail;
}

/*
 * Flushes a specific endpoint (fifo)
 *
 */
static void GADGET_NAME(__GADGET,_fifo_flush)(struct usb_ep* _ep)
{
  GADGET_NAME(__GADGET,_ep*) ep;

  ep = container_of(_ep,GADGET_NAME(__GADGET,_ep),ep);

  DEBUG(dev,"usb_ep_fifo_flush at ep %d.\n",ep->num);

  if (!_ep || (!ep->desc && ep->num != 0)) {
    return;
  }

  if (!ep->dev->driver || ep->dev->gadget.speed == USB_SPEED_UNKNOWN) {
    return;
  }

  ep_flush(ep->dev,ep->num);
}

/* The endpoint operations table */
static struct usb_ep_ops GADGET_NAME(__GADGET,_ep_ops) = {
  .enable            = GADGET_NAME(__GADGET,_enable),
  .disable           = GADGET_NAME(__GADGET,_disable),
  .alloc_request     = GADGET_NAME(__GADGET,_alloc_request),
  .free_request      = GADGET_NAME(__GADGET,_free_request),
  .alloc_buffer      = GADGET_NAME(__GADGET,_alloc_buffer),
  .free_buffer       = GADGET_NAME(__GADGET,_free_buffer),
  .queue             = GADGET_NAME(__GADGET,_queue),
  .dequeue           = GADGET_NAME(__GADGET,_dequeue),
  .set_halt          = GADGET_NAME(__GADGET,_set_halt),
  .fifo_status       = GADGET_NAME(__GADGET,_fifo_status),
  .fifo_flush        = GADGET_NAME(__GADGET,_fifo_flush),
};

/*------- Part 9.2: ---------- the gadget operations table -------------------*/

/*
 * Gets the currect frame number, this is a simple register read.
 *
 */
static int GADGET_NAME(__GADGET,_get_frame)(struct usb_gadget* _gadget)
{
  GADGET_NAME(__GADGET,_dev*) dev;
  unsigned long               flags;
  int                         retval;

  if (!_gadget) {
    return -ENODEV;
  }

  dev = container_of (_gadget,GADGET_NAME(__GADGET,_dev),gadget);

  spin_lock_irqsave(&dev->lock, flags);

  retval = frame_number(dev);

  spin_unlock_irqrestore(&dev->lock, flags);
  return retval;
}

/*
 * Tries to wake up the host connected to this device, this causes a simple
 * bit set operation in the core.
 *
 */
static int GADGET_NAME(__GADGET,_wakeup)(struct usb_gadget* _gadget)
{
  GADGET_NAME(__GADGET,_dev*) dev;
  unsigned long               flags;

  if (!_gadget) {
    return 0;
  }

  dev = container_of(_gadget,GADGET_NAME(__GADGET,_dev),gadget);

	spin_lock_irqsave(&dev->lock, flags);
  remote_wakeup(dev);
	spin_unlock_irqrestore(&dev->lock, flags);
	return OK;
}

/*
 * Sets or clears the "selfpowered" feature for the device, actually this
 * information is only stored in a status flag.
 *
 */
static int GADGET_NAME(__GADGET,_set_selfpowered)(struct usb_gadget* _gadget, int value)
{
  GADGET_NAME(__GADGET,_dev*) dev;
  unsigned long               flags;

  if (!_gadget) {
    return 0;
  }

  dev = container_of(_gadget,GADGET_NAME(__GADGET,_dev),gadget);

  spin_lock_irqsave(&dev->lock, flags);
  if (value) {
    SET_FLAG(DEV_IS_SELFPOWERED,&dev->flags);
  }
  else {
    CLEAR_FLAG(DEV_IS_SELFPOWERED,&dev->flags);
  }
  spin_unlock_irqrestore(&dev->lock, flags);
  return OK;
}

/*
 * checks if the bus power is present
 *
 */
static int GADGET_NAME(__GADGET,_vbus_session)(struct usb_gadget* _gadget, int is_active)
{
  return OK;
}

/*
 * constrains controller's bus power usage
 *
 */
static int GADGET_NAME(__GADGET,_vbus_draw)(struct usb_gadget* _gadget, unsigned int mA)
{
  return OK;
}

/*
 * checks the status of the pullup resistor
 *
 */
static int GADGET_NAME(__GADGET,_pullup)(struct usb_gadget* _gadget, int is_on)
{
  return OK;
}


/*
 * The swiss army knife function, probably for implementing all issues, which
 * were forgotten before.
 *
 */
static int GADGET_NAME(__GADGET,_ioctl)(struct usb_gadget* _gadget, unsigned int code, unsigned long param)
{
  return OK;
}

/* The gadget operations table */
static struct usb_gadget_ops GADGET_NAME(__GADGET,_gadget_ops) = {
  .get_frame         = GADGET_NAME(__GADGET,_get_frame),
  .wakeup            = GADGET_NAME(__GADGET,_wakeup),
  .set_selfpowered   = GADGET_NAME(__GADGET,_set_selfpowered),
  .vbus_draw         = GADGET_NAME(__GADGET,_vbus_draw),
  .vbus_session      = GADGET_NAME(__GADGET,_vbus_session),
  .pullup            = GADGET_NAME(__GADGET,_pullup),
  .ioctl             = GADGET_NAME(__GADGET,_ioctl)
};

/*------- Part 9.3: --------- gadget driver connect/disconnct ----------------*/

/*
 * called by the gadget to bind itself to the controller driver
 *
 */
int usb_gadget_register_driver (struct usb_gadget_driver* driver)
{
  struct GADGET_NAME(__GADGET,_dev*) dev = the_controller;
  int			retval = OK;

  DEBUG(dev,"register_driver.\n");

  if (!driver
      || !driver->bind
      || !driver->unbind
      || !driver->setup) {
    return -EINVAL;
  }

  if (!dev) {
		return -ENODEV;
  }

  if (dev->driver) {
    return -EBUSY;
  }

  dev->driver = driver;

  DEBUG(dev,"binding gadget at %p.\n",&dev->gadget);
  retval = driver->bind(&dev->gadget);

  if (retval) {
    dev->driver = 0;
    return retval;
  }

  hw_reinit(dev);
  clear_irq_status(dev);
  ep0_start(dev);
  
  return 0;
}

/*
 * called by the gadget to unbind itself from the controller driver
 *
 */
int usb_gadget_unregister_driver (struct usb_gadget_driver* driver)
{
  struct GADGET_NAME(__GADGET,_dev*) dev = the_controller;

  DEBUG(dev,"disconnecting gadget driver from gadget.\n");
  if (!dev) {
    return -ENODEV;
  }

  if (!driver || driver != dev->driver) {
    return -EINVAL;
  }

  DEBUG(dev,"stopping hardware and informing gadget driver.\n");
  hw_stop(dev, driver);
  GADGET_NAME(__GADGET,_pullup)(&dev->gadget, 0);
  driver->unbind(&dev->gadget);
  dev->driver = 0;
  return 0;
}
