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
 
/****************************************************************************
   Module      : $RCSfile: drv_api.h,v $
   Revision    : $Revision: 1.1 $
   Date        : $Date: 2005/08/30 10:42:55 $
   Description : This file contains the defines, the structures
                 declarations the tables declarations and the global
                 functions declarations.
   Remarks:

      Use compiler switch ENABLE_TRACE for trace output, for debugging
      purposes. Compiler switch for OS is needed. Use LINUX for linux
      and VXWORKS for VxWorks. WIN32 is just for test purposes for
      function tests.
*******************************************************************************/
#ifndef _DRV_API_H
#define _DRV_API_H

/* ============================= */
/* Global Defines                */
/* ============================= */

/* Actual Compiler Switches List
   NB: Please add the commented defines to
       your compiler before compiling
       e.g: -DLINUX -DPPC ...*/ 

/* Traces */
#if defined(DEBUG_DANUBE_OAK) && (defined(LINUX) || defined(VXWORKS))

   /*#define ENABLE_TRACE*/
   
   #ifndef LINUX
   /* on Linux output from interrupt not possible !!! */
      /*#define ENABLE_LOG*/
   #endif

#endif /* (defined(DEBUG_DANUBE_OAK) && defined(LINUX)) || defined(VXWORKS) */


/* ============================= */
/* includes                      */
/* ============================= */
#ifdef LINUX
   #include "common/src/sys_drv_linux.h"
   #include "common/src/sys_defs.h"
/*   #include "common/src/sys_fifo.h"    */
   #include "common/src/sys_debug.h"
#endif

#ifdef VXWORKS
   #include "common/src/sys_drv_vxworks.h"
   #include "common/src/sys_drv_defs.h"
   #include "common/src/sys_drv_fifo.h"
   #include "common/src/sys_drv_debug.h"
#endif

/* return values */
#ifndef OK
#define OK      0
#endif

#ifndef ERROR
#define ERROR  (-1)
#endif

#endif /* _DRV_API_H */

