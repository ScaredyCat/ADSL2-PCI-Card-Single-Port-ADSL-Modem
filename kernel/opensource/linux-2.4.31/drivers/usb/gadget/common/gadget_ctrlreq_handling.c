/*
 * hardware independent gadget low level control request handling functions
 *
 * Copyright (C) 2006 EMSYS GmbH, Ilmenau, Germany, GmbH (http://www.emsys.de)
 * Copyright (C) 2006 Christian Schroeder (christian.schroeder@emsys.de)
 *
 */  


/*
 * handles the set_address request 
 *
 */
static ERROR set_address(GADGET_NAME(__GADGET,_dev*) dev, unsigned short addr)
{
  ERROR ret = OK;
  dev->addr = addr;

  DEBUG(dev,"set address (first part, %d).\n",addr);
#if GADGET_HAS_ONE_STEP_SETADDR
  finalize_set_address(dev);
#endif

  if (list_empty(&dev->ep[0].queue)) {
    init_xfer(dev,0,0,0);
    ret = setup_xfer(dev,0,(1 << TD_SET_ADDRESS),0);
    start_queue(dev,0);
  }
  else {
    ERROR(dev,"ep0 queue not empty.\n");
    return FAIL;
  }
  return ret;
}

/*
 * handles the get_status request 
 * recipient: 0=device, 1=interface, 2=endpoint
 */
static ERROR get_status(GADGET_NAME(__GADGET,_dev*) dev, u8 r, u8 n)
{
  ERROR ret = OK;
  byte devstat = 0;
  byte epnum;
  volatile byte __iomem* txbuf = ep_tx_buffer(dev,0);

  epnum = (n & ~USB_DIR_MASK);
  DEBUG(dev,"get status (0x%.2x, 0x%.2x).\n",r,epnum);
  switch (r) {
    case USB_RECIP_ENDPOINT:
      if (epnum >= dev->epcnt) {
        WARN(dev,"ep %d doesn't exist.\n",epnum);
        ep_disable(dev,0,EP_STALL);
        return INVALID;
      }
      if (ep_disabled(dev,epnum,EP_STALL)) {
        *((volatile u16 __iomem*)(txbuf)) = cpu_to_le16(1);
      }
      else {
        *((volatile u16 __iomem*)(txbuf)) = cpu_to_le16(0);
      }
      break;
    case USB_RECIP_DEVICE:
      if (TEST_FLAG(DEV_IS_SELFPOWERED,&(dev->flags))) {
        devstat |= (1<<DEV_IS_SELFPOWERED);
      }
      if (TEST_FLAG(DEV_HAS_REM_WAKEUP,&(dev->flags))) {
        devstat |= (1<<DEV_HAS_REM_WAKEUP);
      }
      *((volatile u16 __iomem*)(txbuf)) = cpu_to_le16(devstat);
      break;
    case USB_RECIP_INTERFACE:
       *((volatile u16 __iomem*)(txbuf)) = 0;
      break;
    default:
      WARN(dev,"unknown recipient.\n");
      ep_disable(dev,0,EP_STALL);
      return INVALID;
      break;
  }
  if (list_empty(&dev->ep[0].queue)) {
    init_xfer(dev,0,2,0);
    ret = setup_xfer(dev,0,2,0);
    start_queue(dev,0);
  }
  else {
    ERROR(dev,"ep0 queue not empty.\n");
    return FAIL;
  }
  return ret;
}

/*
 * handles the set_feature request 
 * recipient: 0=device, 1=interface, 2=endpoint
 * feature: 0=endpoint stall, 1=remote_wakup
 *
 */
static ERROR set_feature(GADGET_NAME(__GADGET,_dev*) dev, u8 r, unsigned short f, u8 n)
{
  byte flags = 0;
  byte epnum;
  ERROR ret = OK;
  
  epnum = (n & ~USB_DIR_MASK);
  DEBUG(dev,"set feature (%d).\n",f);
  switch(r) {
    case USB_RECIP_ENDPOINT:
      if (epnum >= dev->epcnt) {
        WARN(dev,"ep %d doesn't exist.\n",epnum);
        ep_disable(dev,0,EP_STALL);
        return INVALID;
      }
      if (f == USB_FEATURE_STALL) {
        DEBUG(dev,"set stall (%d) feature.\n",epnum);
        if (!epnum) {
          SET_FLAG(TD_STALL_EP0,&flags);
        }
        else {
          /* don't stall an already stalled ep */
          if (!ep_disabled(dev,epnum,EP_STALL)) {
            if (count_queue(dev,epnum)) {
              WARN(dev,"stalling non empty queue.\n");
            }
            ep_disable(dev,epnum,EP_STALL);
          }
        }
      }
      else {
        WARN(dev,"unknown feature.\n");
        ep_disable(dev,0,EP_STALL);
        return INVALID;
      }
      break;
    case USB_RECIP_DEVICE:
      if (f == USB_FEATURE_WKUP) {
        DEBUG(dev,"set remote wakeup feature.\n");
        SET_FLAG(DEV_HAS_REM_WAKEUP,&dev->flags);
      }
      else {
        WARN(dev,"unknown feature.\n");
        ep_disable(dev,0,EP_STALL);
        return INVALID;
      }
      break;
    case USB_RECIP_INTERFACE:
      WARN(dev,"no interface features supported.\n");
      ep_disable(dev,0,EP_STALL);
      return INVALID;
      break;
    default:
       WARN(dev,"unknown recipient.\n");
       ep_disable(dev,0,EP_STALL);
       return INVALID;
       break;
  }
  if (list_empty(&dev->ep[0].queue)) {
    init_xfer(dev,0,0,0);
    ret = setup_xfer(dev,0,flags,0);
    start_queue(dev,0);
  }
  else {
    ERROR(dev,"ep0 queue not empty.\n");
    return FAIL;
  }
  return ret;
}

/*
 * handles the clear_feature request 
 * recipient: 0=device, 1=interface, 2=endpoint
 * feature: 0=endpoint stall, 1=remote_wakup
 *
 */
static ERROR clear_feature(GADGET_NAME(__GADGET,_dev*) dev, u8 r, unsigned short f, u8 n)
{
  byte epnum;
  byte qstart = 0;
  ERROR ret = OK;

  epnum = (n & ~USB_DIR_MASK);
  DEBUG(dev,"clear feature (%d).\n",f);
  switch(r) {
    case USB_RECIP_ENDPOINT:
      if (epnum >= dev->epcnt) {
        WARN(dev,"ep %d doesn't exist.\n",epnum);
        ep_disable(dev,0,EP_STALL);
        return INVALID;
      }
      if (f == USB_FEATURE_STALL) {
        DEBUG(dev,"clear stall feature.\n");
        if (ep_disabled(dev,epnum,EP_STALL)) {
          ep_enable(dev,epnum,EP_DESTALL);
          qstart = 1;
        }
      }
      else {
        WARN(dev,"unknown feature.\n");
        ep_disable(dev,0,EP_STALL);
        return INVALID;
      }
      break;
    case USB_RECIP_DEVICE:
      if (f == USB_FEATURE_WKUP) {
        DEBUG(dev,"clear remote wakeup feature.\n");
        CLEAR_FLAG(DEV_HAS_REM_WAKEUP,&dev->flags);

        if (list_empty(&dev->ep[0].queue)) {
          init_xfer(dev,0,0,0);
          ret = setup_xfer(dev,0,0,0);
          start_queue(dev,0);
        }
        else {
          ERROR(dev,"ep0 queue not empty.\n");
          return FAIL;
        }
      }
      else {
        WARN(dev,"unknown feature.\n");
        ep_disable(dev,0,EP_STALL);
        return INVALID;
      }
      break;
    case USB_RECIP_INTERFACE:
      WARN(dev,"no interface features supported.\n");
      ep_disable(dev,0,EP_STALL);
      return INVALID;
      break;
    default:
      WARN(dev,"unknown recipient.\n");
      ep_disable(dev,0,EP_STALL);
      return INVALID;
      break;
  }
  if (list_empty(&dev->ep[0].queue)) {
     init_xfer(dev,0,0,0);
     ret = setup_xfer(dev,0,0,0);
     start_queue(dev,0);
  }
  else {
     ERROR(dev,"ep0 queue not empty.\n");
     return FAIL;
  }
  /* restart data ep queue e.g. for csw transmitting. */
  if (count_queue(dev,epnum)) {
    start_queue(dev,epnum);
  }
  return ret;
}

/*
 * delegate a control request to the gadget driver 
 *
 */
static ERROR delegate_control_request(GADGET_NAME(__GADGET,_dev*) dev, 
                                      struct usb_ctrlrequest* req)
{
  int ret;

  DEBUG(dev,"delegate control request 0x%.2x 0x%.2x 0x%.4x 0x%.4x 0x%.4x \n",
        req->bRequestType,
        req->bRequest,
        req->wValue,
        req->wIndex,
        req->wLength);

  spin_unlock(&dev->lock);
  ret = dev->driver->setup(&dev->gadget,req);
  spin_lock(&dev->lock);

  if (ret < 0) {
     /* if something went wrong, stall ep0 */
     return FAIL;
  }
  return OK;
}

/*
 * decodes and executes the control request (hw independent)
 *
 */  
static int finalize_setup(GADGET_NAME(__GADGET,_dev*) dev, GADGET_NAME(__GADGET,_rq_data) req)
{
   int ret = OK;
   int delegate = 1;
   
  /* new setup_token received, clear ep0 td queue */
  ep_stop(dev,0);
  clear_queue(dev,0,-ECONNRESET);
  ep_start(dev,0);

  /* (re)enable setup stage */
  ep0_enable_setup_stage(dev);

  /* correct endianess, check correctness */
  //cpu_to_le32s(&req.raw[0]);
  //cpu_to_le32s(&req.raw[1]);
  le16_to_cpus(&req.ctrlreq.wValue);
  le16_to_cpus(&req.ctrlreq.wIndex);
  le16_to_cpus(&req.ctrlreq.wLength);

  DEBUG(dev,"setup 0x%.2x 0x%.2x 0x%.4x 0x%.4x 0x%.4x \n",
          req.ctrlreq.bRequestType,
          req.ctrlreq.bRequest,
          req.ctrlreq.wValue,
          req.ctrlreq.wIndex,
          req.ctrlreq.wLength);

  if(!(req.ctrlreq.bRequestType & (USB_TYPE_CLASS|USB_TYPE_VENDOR))) {
    delegate = 0;
    switch (req.ctrlreq.bRequest) {
      case USB_REQ_GET_STATUS:
        ret = get_status(dev,(req.ctrlreq.bRequestType & USB_RECIP_MASK),req.ctrlreq.wIndex);
        break;
      case USB_REQ_CLEAR_FEATURE:
        ret = clear_feature(dev,req.ctrlreq.bRequestType,req.ctrlreq.wValue,req.ctrlreq.wIndex);
        break;
      case USB_REQ_SET_FEATURE:
        ret = set_feature(dev,req.ctrlreq.bRequestType,req.ctrlreq.wValue,req.ctrlreq.wIndex);
        break;
      case USB_REQ_SET_ADDRESS:
        ret = set_address(dev,req.ctrlreq.wValue);
        break;
      case USB_REQ_GET_DESCRIPTOR:
        if (dev->speed != USB_SPEED_HIGH) {
          if ((((req.ctrlreq.wValue) >> 8) == USB_DT_DEVICE_QUALIFIER) ||
              (((req.ctrlreq.wValue) >> 8) == USB_DT_OTHER_SPEED_CONFIG))
          {
            ep_disable(dev,0,EP_STALL);
            return INVALID;
          }
        }
      default:
        delegate = 1;
        break;
    }
  }

  if(delegate) {
      /* delegate request to gadget driver. */
      if (req.ctrlreq.bRequestType & USB_DIR_MASK) {
        DEBUG(dev,"control read.\n");
        CLEAR_FLAG(DEV_CTRL_REQ_WRITE,&(dev->flags));
        CLEAR_FLAG(DEV_CTRL_REQ_DATALESS,&(dev->flags));
      }
      else {
        if (!req.ctrlreq.wLength) {
          DEBUG(dev,"control dataless.\n");
          SET_FLAG(DEV_CTRL_REQ_WRITE,&(dev->flags));
          SET_FLAG(DEV_CTRL_REQ_DATALESS,&(dev->flags));
        }
        else {
          DEBUG(dev,"control write.\n");
          SET_FLAG(DEV_CTRL_REQ_WRITE,&(dev->flags));
          CLEAR_FLAG(DEV_CTRL_REQ_DATALESS,&(dev->flags));
        }
      }
      ret = delegate_control_request(dev,&req.ctrlreq);
  }

  return ret;   
}
