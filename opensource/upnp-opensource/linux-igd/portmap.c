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

#include "portmap.h"
#include <string.h>
#include <stdlib.h>

void PortMap_Construct(struct PortMap *pm)
{
}

void PortMap_Destruct(struct PortMap *pm)
{
        if (pm->m_RemoteHost)
                free(pm->m_RemoteHost);
        if (pm->m_ExternalIP)
                free(pm->m_ExternalIP);
        if (pm->m_InternalClient)
                free(pm->m_InternalClient);
        if (pm->m_PortMappingDescription)
                free(pm->m_PortMappingDescription);
	if (pm->m_PortMappingProtocol)
		free(pm->m_PortMappingProtocol);
}

void PortMap_Contruct1(struct PortMap *pm, char *RemoteHost, char *Proto,
		char *ExtIP, int ExtPort, char *IntIP,
		int IntPort, int Enabled, char *Desc, int LeaseDuration)
{
	if (RemoteHost)
        {
		pm->m_RemoteHost=strdup(RemoteHost);
//                m_RemoteHost = new char[strlen(RemoteHost)+1];
//                strcpy(m_RemoteHost, RemoteHost);
        }
        else pm->m_RemoteHost = NULL;

        if (Proto)
        {
		pm->m_PortMappingProtocol=strdup(Proto);
//                m_PortMappingProtocol = new char[strlen(Proto)+1];
//                strcpy(m_PortMappingProtocol, Proto);
        }
        else pm->m_PortMappingProtocol = NULL;

        if (ExtIP)
        {
		pm->m_ExternalIP=strdup(ExtIP);
//                m_ExternalIP = new char[strlen(ExtIP)+1];
//                strcpy(m_ExternalIP, ExtIP);
        }
        else pm->m_ExternalIP = NULL;

        pm->m_ExternalPort = ExtPort;

        if (IntIP)
        {
		pm->m_InternalClient=strdup(IntIP);
//                m_InternalClient = new char[strlen(IntIP)+1];
//                strcpy(m_InternalClient, IntIP);
        }
        else pm->m_InternalClient = NULL;

        pm->m_InternalPort = IntPort;

        if (Desc)
        {
		pm->m_PortMappingDescription=strdup(Desc);
//                m_PortMappingDescription = new char[strlen(Desc)+1];
//                strcpy(m_PortMappingDescription, Desc);
        }
        else pm->m_PortMappingDescription = NULL;

        pm->m_PortMappingLeaseDuration = LeaseDuration;
}
