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
   Module      : $RCSfile: drv_DANUBE_VCPU_Api.h,v $
   Revision    : $Revision: 1.1 $
   Date        : $Date: 2005/08/30 10:42:55 $
   Description :

******************************************************************************/
#ifndef _drv_DANUBEVCPUAPI_h
#define _drv_DANUBEVCPUAPI_h

#ifndef DRV_DANUBE_VCPU_NAME
   #ifdef LINUX
      #define DRV_DANUBE_VCPU_NAME          "vcpu"
   #else
      #define DRV_DANUBE_VCPU_NAME          "/dev/VCPU/0"
   #endif
#else
   #error module name already specified
#endif

/* driver version */
#define DRV_SCC_HDLC_VER_MAJOR       0
#define DRV_SCC_HDLC_VER_MINOR       3
#define DRV_SCC_HDLC_VER_STEP        0
#define DRV_SCC_HDLC_VER_TYPE        2

#define DRV_DANUBE_VCPU_VER_STR        "0.1.0"

#define DRV_DANUBE_VCPU_WHAT_STR \
   "@(#)DANUBE MIPS24KEc VCPU mailbox driver, Version "DRV_DANUBE_VCPU_VER_STR

/* how many channels to handle (max) */
/* #define DRV_DANUBE_OAK_MAX_CHANNELS   3  */

#endif /* _drv_danube_oak_api_h */
