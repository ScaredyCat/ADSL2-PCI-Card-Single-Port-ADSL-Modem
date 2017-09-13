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
   Module      : $RCSfile: drv_DANUBE_VCPU_Linux.h,v $
   Revision    : $Revision: 1.1 $
   Date        : $Date: 2005/08/30 10:42:55 $
   Description :

******************************************************************************/
#ifndef _drv_DANUBEVCPULINUX_H
#define _drv_DANUBEVCPULINUX_H

s32 DANUBE_MPS_DSP_Open(struct inode *inode, struct file *file_p);
s32 DANUBE_MPS_DSP_Close(struct inode *inode, struct file *filp);
s32 DANUBE_MPS_DSP_Ioctl(struct inode *inode, struct file *file_p, unsigned s32 nCmd, unsigned long arg);
s32 DANUBE_MPS_DSP_RegisterDataCallback(DEVTYPE type, UINT dir, void (*callback)(DEVTYPE type));
s32 DANUBE_MPS_DSP_UnregisterDataCallback(DEVTYPE type, UINT dir);
s32 DANUBE_MPS_DSP_ReadMailbox(DEVTYPE type, DSP_READWRITE *rw);
s32 DANUBE_MPS_DSP_WriteMailbox(DEVTYPE type, DSP_READWRITE *rw);

#endif /* #ifndef _drv_DANUBEVCPULINUX_H */
