/************************************************************************
 *
 * Copyright (c) 2004
 * Infineon Technologies AG
 * St. Martin Strasse 53; 81669 Muenchen; Germany
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 ************************************************************************/
 
/******************************************************************************
   File        : $RCSfile: drv_DANUBE_VCPU_Interface.h,v $
   Revision    : $Revision: 1.1 $
   Date        : $Date: 2005/08/30 10:42:55 $
   Description :

******************************************************************************/
#ifndef _drv_DANUBEVCPUINTERFACE_H
#define _drv_DANUBEVCPUINTERFACE_H

/** magic number */
#define DANUBE_MPS_VCPU_MAGIC 'O'

/* Register/unregister event notification */
#define FIO_DANUBE_VCPU_EVENT_REG       _IO(DANUBE_MPS_VCPU_MAGIC, 1)
#define FIO_DANUBE_VCPU_EVENT_UNREG     _IO(DANUBE_MPS_VCPU_MAGIC, 2)
/* Abort/reset all mailbox buffers */
#define FIO_DANUBE_MPS_MB_READ         _IO(DANUBE_MPS_VCPU_MAGIC, 3)
#define FIO_DANUBE_MPS_MB_WRITE        _IO(DANUBE_MPS_VCPU_MAGIC, 4)
/* #define FIO_DANUBE_VCPU_MB_WR_ACCESS    _IO(DANUBE_MPS_VCPU_MAGIC, 5)  */
/* OAK hard/soft reset */
#define FIO_DANUBE_VCPU_RESET           _IO(DANUBE_MPS_VCPU_MAGIC, 6)
/* OAK reboot */
#define FIO_DANUBE_VCPU_RESTART         _IO(DANUBE_MPS_VCPU_MAGIC, 7)

/* Version string */
#define FIO_DANUBE_VCPU_GETVERSION      _IO(DANUBE_MPS_VCPU_MAGIC, 8)
/* reset the queue */
#define FIO_DANUBE_MPS_MB_RST_QUEUE     _IO(DANUBE_MPS_VCPU_MAGIC, 9)
/* oak firmware download */
#define  FIO_DANUBE_VCPU_DWNLOAD        _IO(DANUBE_MPS_VCPU_MAGIC, 17)
/* register access for Wineasy */
#define  FIO_DANUBE_TXFIFO_SET         _IO(DANUBE_MPS_VCPU_MAGIC, 18)
#define  FIO_DANUBE_TXFIFO_GET         _IO(DANUBE_MPS_VCPU_MAGIC, 19)
#endif /* _drv_DANUBEVCPUINTERFACE_H */

