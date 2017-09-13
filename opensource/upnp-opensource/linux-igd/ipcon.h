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

#ifndef _IPCON_H_
#define _IPCON_H_

struct IPCon
{
	char *m_ifname;
};

void	IPCon_Construct(struct IPCon *ipcon, char *ifname);
void	IPCon_Destruct(struct IPCon *ipcon);
int	IPCon_SetIfName(struct IPCon *ipcon, char * ifname);
char	*IPCon_GetIfName(struct IPCon *ipcon);
char	*IPCon_GetIpAddrStr(struct IPCon *ipcon);
int	IPCon_IsIfUp(struct IPCon *ipcon);
char	*IPCon_GetIfStatStr(struct IPCon *ipcon);	

#endif
