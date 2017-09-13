/*
 * hardware independent gadget transfer handling functions
 *
 * Copyright (C) 2006 EMSYS GmbH, Ilmenau, Germany, GmbH (http://www.emsys.de)
 * Copyright (C) 2006 Christian Schroeder (christian.schroeder@emsys.de)
 *
 */  

/*
 * check the last transfer and updates the endpoints transfer status (hw
 * independent)
 *
 */
static void check_xfer(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum)
{
  volatile byte*                       buffer;
  unsigned long                        xf;
  GADGET_NAME(__GADGET,_td*)           td;
  GADGET_NAME(__GADGET,_request*)      req;
  GADGET_NAME(__GADGET,_ep*)           ep = &(dev->ep[epnum]);
  GADGET_NAME(__GADGET,_xfer_status*)  st = &(ep->xfstat);

  DEBUG(dev,"check xfer status.\n");

  td = list_entry(ep->queue.next,GADGET_NAME(__GADGET,_td),queue);

  if ((xf = transferred(dev,epnum))) {
    SET_FLAG(TD_IS_SHORT,(&(td->flags)));
    DEBUG(dev,"transfer counter not empty (tbytes=%ld).\n",xf);
  }

  if (ep->dir == USB_DIR_OUT || TEST_FLAG(TD_IS_CTRL_OUT,&(td->flags))) {
    buffer = ep_rx_buffer(dev,epnum);

#if !GADGET_SUPPORTS_DMA
    /* if we are using dma mode the buffer pointer could be a zero pointer */
    if (!buffer) {
      ERROR(dev,"endpoint %d rx buffer invalid.\n",epnum);
      set_interrupt(dev,FALSE);
      return;
    }
#endif

    /* copy data if valid receive buffer is given and data length is not zero */
    if (buffer && td->length && td->buf) {
       memcpy_fromio((void*)td->buf,(void*)buffer,td->length);
    }

    st->actual += (td->length)-xf;
  }
  else {
    st->actual += td->length;
  }

  if (TEST_FLAG(TD_IS_LAST,&(td->flags)) || TEST_FLAG(TD_IS_SHORT,&(td->flags))) {
    /* transfer complete, check status */
    req = td->req;

    /* check transmitted byte count */
    if (st->expected > st->actual) {
      if (req) {
        req->req.actual = st->actual;
        if (ep->dir == USB_DIR_OUT || TEST_FLAG(TD_IS_CTRL_OUT,&(td->flags))) {
          req->req.status = 0;
        }
        else {
          req->req.status = 0;
          DEBUG(dev,"not all bytes transmitted.\n");
        }
      }
    }
    else if (st->expected < st->actual) {
      if (req) {
        req->req.actual = st->actual;
        ERROR(dev,"transfer overflow.\n");
      }
    }
    else {
      if (req) {
        req->req.actual = st->actual;
        req->req.status = 0;
      }
    }

    if (TEST_FLAG(TD_IS_STATUS,&(td->flags))) {
      /* complete pending set address */
      if (TEST_FLAG(TD_SET_ADDRESS,&(td->flags))) {
#if !GADGET_HAS_ONE_STEP_SETADDR
        finalize_set_address(dev);
#endif
      }
    }
    if (TEST_FLAG(TD_IS_STATUS,&(td->flags))) {
      /* stall ep0 after status stage if pending */
      if (TEST_FLAG(TD_STALL_EP0,&(td->flags))) {
        //ep_disable(dev,0,EP_STALL);
        DEBUG(dev,"stalling ep0 by request is senseless, ignoring.\n");
      }
    }
  }
}

/*
 * init the endpoints transfer status (hw independent)
 *
 */
static void init_xfer(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, u16 flags,
                             GADGET_NAME(__GADGET,_request*) req)
{
  DEBUG(dev,"init xfer status.\n");

  GADGET_NAME(__GADGET,_ep*) ep = &(dev->ep[epnum]);
  GADGET_NAME(__GADGET,_xfer_status*) st = &(ep->xfstat);

  st->endpoint = ep;
  st->request = req;
  if (req) {
    st->expected = req->req.length;
  }
  else {
    /* low level ctrl request */
    st->expected = flags & TD_XFLEN;
  }
  st->actual = 0;
  st->error = XFER_OK;
}

/*
 * setup a new transfer, the flag field is used to support low level control 
 * requests (hw independent)
 *
 */
static ERROR setup_xfer(GADGET_NAME(__GADGET,_dev*) dev, u8 epnum, u16 flags,
                       GADGET_NAME(__GADGET,_request*) req)
{

  GADGET_NAME(__GADGET,_td*) td;
  byte shortp = 0;
  unsigned long xfsz;
  unsigned long i = 0;
  unsigned long len = 0; 
  GADGET_NAME(__GADGET,_ep*) ep = &(dev->ep[epnum]);

  DEBUG(dev,"setup xfer at ep%d.\n",epnum);

  xfsz = config_xfsz(dev,epnum);
  if (req) {
    len = req->req.length;
  }

  if (!req) {
    if (!(flags & TD_XFLEN)) {
      DEBUG(dev,"two-stage control transfer.\n");
      /* IN status stage */
      td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
      td->req = 0;
      td->seqnum = 0;
      td->flags = flags;
      td->buf = 0;
#if GADGET_SUPPORTS_DMA      
      td->dma = DMA_ADDR_INVALID;
#endif
      td->length = 0;
      SET_FLAG(TD_IS_STATUS,&(td->flags));
      SET_FLAG(TD_IS_LAST,&(td->flags));
      list_add_tail(&td->queue,&ep->queue);
      DEBUG(dev,"append td status in (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
            td->req,td->buf,td->length,td->seqnum,td->flags);
    }
    else if ((flags & TD_XFLEN) && !(flags & (1<<TD_IS_CTRL_OUT))) {
      DEBUG(dev,"three-stage low level control transfer.\n");
      /*  IN data stage, only one packet possible */
      td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
      td->req = 0;
      td->seqnum = 0;
      td->flags = flags;
      /* transfer length provided via flags */
      td->length = flags & TD_XFLEN;
#if GADGET_SUPPORTS_DMA      
      td->buf = ep_tx_buffer(dev,0);
      td->dma = __pa(td->buf);      
#else
      /* data are already in dpram, no copy needed */
      td->buf = 0;
#endif
      list_add_tail(&td->queue,&ep->queue);
      DEBUG(dev,"append td data in (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
          td->req,td->buf,td->length,td->seqnum,td->flags);
      /* OUT status stage */
      td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
      td->req = 0;
      td->seqnum = 1;
      td->flags = flags & ~0x7;     
      td->buf = 0;
#if GADGET_SUPPORTS_DMA      
      td->dma = DMA_ADDR_INVALID;      
#endif
      SET_FLAG(TD_IS_STATUS,&(td->flags));
      SET_FLAG(TD_IS_CTRL_OUT,&(td->flags));
      SET_FLAG(TD_IS_LAST,&(td->flags));
      td->length = 0;
      list_add_tail(&td->queue,&ep->queue);
      DEBUG(dev,"append td status out (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
            td->req,td->buf,td->length,td->seqnum,td->flags);
    }
    else {
      /* not yet used, but in case of use we have to queue a OUT data stage
         followed by a IN status stage */
      ERROR(dev,"unsupported request.\n");
      return FAIL; 
    }
  }
  else if (req && !epnum) {
    DEBUG(dev,"data phase of three-stage control transfer.\n");
    /* data stage, 0 until pre-ultimate packet */
    while (len > xfsz) {
      td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
      td->req = req;
      td->seqnum = i;
      td->flags = flags;
      td->length = xfsz;
      td->buf = (void*)(((byte*)(req->req.buf))+(i*xfsz));
#if GADGET_SUPPORTS_DMA      
      td->dma = __pa(td->buf);      
#endif
      list_add_tail(&td->queue,&ep->queue);
      DEBUG(dev,"append td data (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
            td->req,td->buf,td->length,td->seqnum,td->flags);
      len -= xfsz;
      i++;
    }
    /* data stage, last data packet */
    td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
    td->req = req;
    td->seqnum = i;
    td->flags = flags;
    td->length = len;
    td->buf = (void*)(((byte*)(req->req.buf))+(i*xfsz));
#if GADGET_SUPPORTS_DMA      
    td->dma = __pa(td->buf);      
#endif
    if ((len % (ep->ep.maxpacket)) || !len) {
      /* short packet, never append a zero length packet additionally */
      shortp = 1;
      if (req->req.zero) {
        DEBUG(dev,"ep %d zero data packet requested but not needed.\n",epnum);
      }
    }
    list_add_tail(&td->queue,&ep->queue);
    i++;
    DEBUG(dev,"append td short data (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
          td->req,td->buf,td->length,td->seqnum,td->flags);
#if 0 /* not yet tested */
    /* append zero packet if requested by gadget driver and if the 
       previous fragment was no short packet */
    if (req->req.zero && !shortp) {
      td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
      td->req = req;
      td->seqnum = i;
      td->flags = flags;
      td->length = 0;
      td->buf = 0;
#if GADGET_SUPPORTS_DMA      
      td->dma = DMA_ADDR_INVALID;      
#endif
      list_add_tail(&td->queue,&ep->queue);
      i++;
      DEBUG(dev,"append td zero data (r=%p,b=%p,l=%d,s=%d,f=%.2x).\n",
            td->req,td->buf,td->length,td->seqnum,td->flags);
    }
    else {
    /* FIXME: it could be, that the data stage is not yet finished, what could
              be indicated by the missing zero data packet at the end of the 
              request and if the last packet is exactly equal max packet size
              of the endpoint. In this case, we might have to skip the status 
              stage. All this is only needed, if the data stage of a control
              request is larger than the maximum packet size of the control
              endpoint. */
#endif
      /* IN status stage */
      if (flags & (1 << TD_IS_CTRL_OUT)) {     
        td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
        td->req = req;
        td->seqnum = i;
        td->flags = flags;
        SET_FLAG(TD_IS_STATUS,&(td->flags));
        SET_FLAG(TD_IS_LAST,&(td->flags));
        CLEAR_FLAG(TD_IS_CTRL_OUT,&(td->flags));
        /* transfer length provided via flags */
        td->length = flags & TD_XFLEN;
        td->buf = 0;
#if GADGET_SUPPORTS_DMA      
        td->dma = DMA_ADDR_INVALID;      
#endif
        list_add_tail(&td->queue,&ep->queue);
        DEBUG(dev,"append td status in (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
              td->req,td->buf,td->length,td->seqnum,td->flags);
      }
      else {
        /* OUT status stage */
        td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
        td->req = req;
        td->seqnum = i;
        td->flags = flags;
        SET_FLAG(TD_IS_STATUS,&(td->flags));
        SET_FLAG(TD_IS_LAST,&(td->flags));
        SET_FLAG(TD_IS_CTRL_OUT,&(td->flags));
        /* transfer length provided via flags */
        td->length = flags & TD_XFLEN;
        td->buf = 0;
#if GADGET_SUPPORTS_DMA      
        td->dma = DMA_ADDR_INVALID;      
#endif
        list_add_tail(&td->queue,&ep->queue);
        DEBUG(dev,"append td status out (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
              td->req,td->buf,td->length,td->seqnum,td->flags);
      }
#if 0
    }
#endif
  }
  else {
    DEBUG(dev,"data transfer at ep %d (len=%ld xfsz=%ld).\n",epnum,len,xfsz);
    /* 0 until pre-ultimate packet */
    while (len > xfsz) {
      td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
      td->req = req;
      td->seqnum = i;
      td->flags = flags;
      td->length = xfsz;
      td->buf = (void*)(((byte*)(req->req.buf))+(i*xfsz));
#if GADGET_SUPPORTS_DMA      
      td->dma = __pa(td->buf);      
#endif
      list_add_tail(&td->queue,&ep->queue);
      len -= xfsz;
      i++;
      DEBUG(dev,"append td data (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
            td->req,td->buf,td->length,td->seqnum,td->flags);
    }
    /* last packet */
    td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
    td->req = req;
    td->seqnum = i;
    td->flags = flags;
    td->length = len;
    td->buf = (void*)(((byte*)(req->req.buf))+(i*xfsz));
#if GADGET_SUPPORTS_DMA      
    td->dma = __pa(td->buf);      
#endif

    if ((len % (ep->ep.maxpacket)) || !len) {
      /* short packet, never append a zero length packet additionally */
      shortp = 1;
    }
    if (!req->req.zero || shortp) {
      SET_FLAG(TD_IS_LAST,&(td->flags));
      if (req->req.zero) {
        DEBUG(dev,"ep %d zero data packet requested but not needed.\n",epnum);
      }
    }
    list_add_tail(&td->queue,&ep->queue);
    i++;
    DEBUG(dev,"append td data (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
          td->req,td->buf,td->length,td->seqnum,td->flags);
    
    /* zero packet used as short packet */
    if (req->req.zero && !shortp) {
      td = (GADGET_NAME(__GADGET,_td*))kmalloc(sizeof(GADGET_NAME(__GADGET,_td)),GFP_ATOMIC);
      td->req = req;
      td->seqnum = i;
      td->flags = flags;
      td->length = 0;
      td->buf = 0;
#if GADGET_SUPPORTS_DMA      
      td->dma = DMA_ADDR_INVALID;      
#endif
      td->flags = flags;
      SET_FLAG(TD_IS_LAST,&(td->flags));
      list_add_tail(&td->queue,&ep->queue);
      i++;
      DEBUG(dev,"append td zero data (r=%p,b=%p,l=%ld,s=%ld,f=%.2x).\n",
            td->req,td->buf,td->length,td->seqnum,td->flags);
    }    
  }
  return OK;
}
