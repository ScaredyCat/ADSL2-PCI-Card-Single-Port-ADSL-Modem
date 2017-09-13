/******************************************************************************
*  Copyright (c) 2002 Linux UPnP Internet Gateway Device Project              *    
*  All rights reserved.                                                       *
*                                                                             *   
*  This file is part of The Linux UPnP Internet Gateway Device (IGD).         *
*                                                                             *
*  The Linux UPnP IGD is free software; you can redistribute it and/or modify *
*  it under the terms of the GNU General Public License as published by       *
*  the Free Software Foundation; either version 2 of the License, or          *
*  (at your option) any later version.                                        *
*                                                                             *    
*  The Linux UPnP IGD is distributed in the hope that it will be useful,      *
*  but WITHOUT ANY WARRANTY; without even the implied warranty of             *
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
*  GNU General Public License for more details.                               *
*                                                                             *   
*  You should have received a copy of the GNU General Public License          * 
*  along with Foobar; if not, write to the Free Software                      *
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA  *
*                                                                             *  
*                                                                             *  
******************************************************************************/

#ifndef _GATEWAY_H_
#define _GATEWAY_H_ 

extern pthread_mutex_t DevMutex;

#define INIT_PORT          52869
#define INIT_DESC_DOC      "gatedesc"
#define INIT_CONF_DIR      "/ramdisk/etc/linux-igd/"

#define GATE_SERVICE_SERVCOUNT  3
//#define GATE_SERVICE_OSINFO             0
#define GATE_SERVICE_LAYER3             0
#define GATE_SERVICE_CONFIG             1
#define GATE_SERVICE_CONNECT    2


#define GATE_OSINFO_VARCOUNT    4
#define GATE_OSINFO_MAJORVER    0
#define GATE_OSINFO_MINORVER    1
#define GATE_OSINFO_BUILDNUM    2
#define GATE_OSINFO_PCNAME              3

#define CONFIG_VARCOUNT    4
#define CONFIG_COLOR               0

#define MAX_VAL_LEN 32

/* This should be the maximum VARCOUNT from above */
#define MAXVARS OSINFO_VARCOUNT

#endif
