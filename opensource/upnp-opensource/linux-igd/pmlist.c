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

#include "pmlist.h"
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define PORTMAPLIST "/var/run/upnpd/portmap.list"

void PortMapList_Construct(struct PortMapList *pmlist)
{
}

void PortMapList_Destruct(struct PortMapList *pmlist)
{
	struct PortMap *itr;

	while(pmlist->m_pmap){
                PortMap_delPortForward(pmlist->m_pmap->m_PortMappingProtocol, pmlist->m_pmap->m_ExternalIP,
                                pmlist->m_pmap->m_ExternalPort, pmlist->m_pmap->m_InternalClient,
                                pmlist->m_pmap->m_InternalPort,pmlist->m_pmap->m_RemoteHost);
		itr=pmlist->m_pmap;
		NODE_DEL(pmlist->m_pmap, itr);
		PortMap_Destruct(itr);
		free(itr);
	}
}

int PortMapAdd(struct PortMapList *pmlist, char *RemoteHost, char *Proto, char *ExtIP, int ExtPort,
		char *IntIP, int IntPort, int Enabled, char *Desc, int LeaseDuration)
{

	int fd_socket, fd_proto;
	struct sockaddr_in addr;
	struct PortMap *temp;
	
	if ((strcmp(Proto,"TCP") == 0) || (strcmp(Proto,"tcp") == 0))
		fd_proto = SOCK_STREAM;
	else
		fd_proto = SOCK_DGRAM;
	
	if ((fd_socket = socket(AF_INET,fd_proto, 0)) == -1)
		printf("Socket Error\n");
//		syslog(LOG_DEBUG, "Socket Error");
	
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(ExtPort);
	if (bind(fd_socket, (struct sockaddr *) &addr, sizeof(addr)) == -1)
	{
		close(fd_socket);
//		syslog(LOG_DEBUG,"Error binding socket");
		return (718);
	}
	close (fd_socket);
	
//	temp = new PortMap;
	temp = (struct PortMap*)malloc(sizeof(struct PortMap));
	PortMap_Construct(temp);
	
	if (RemoteHost)
        {
		temp->m_RemoteHost=strdup(RemoteHost);
//                temp->m_RemoteHost = new char[strlen(RemoteHost)+1];
//                strcpy(temp->m_RemoteHost, RemoteHost);
        }
        else temp->m_RemoteHost = NULL;

        if (Proto)
	{
		temp->m_PortMappingProtocol=strdup(Proto);
//		temp->m_PortMappingProtocol = new char[strlen(Proto)+1];
//		strcpy(temp->m_PortMappingProtocol, Proto);
	}
	else temp->m_PortMappingProtocol = NULL;

        if (ExtIP)
        {
		temp->m_ExternalIP=strdup(ExtIP);
//                temp->m_ExternalIP = new char[strlen(ExtIP)+1];
//                strcpy(temp->m_ExternalIP, ExtIP);
        }
        else temp->m_ExternalIP = NULL;

        temp->m_ExternalPort = ExtPort;

        if (IntIP)
        {
		temp->m_InternalClient=strdup(IntIP);
//                temp->m_InternalClient = new char[strlen(IntIP)+1];
//                strcpy(temp->m_InternalClient, IntIP);
        }
        else temp->m_InternalClient = NULL;

        temp->m_InternalPort = IntPort;

        if (Desc)
        {
		temp->m_PortMappingDescription=strdup(Desc);
//                temp->m_PortMappingDescription = new char[strlen(Desc)+1];
//                strcpy(temp->m_PortMappingDescription, Desc);
        }
        else temp->m_PortMappingDescription = NULL;
        temp->m_PortMapping_gettime = time(0);
				if(LeaseDuration)
        		temp->m_PortMappingLeaseDuration = LeaseDuration+time(0);
        else
        		temp->m_PortMappingLeaseDuration = 0;		

//	m_pmap.push_back(temp);
	NODE_ENQUEUE(pmlist->m_pmap, temp);
	
	PortMap_addPortForward(Proto, ExtIP, ExtPort, IntIP, IntPort, Enabled, Desc, RemoteHost);
	
	return (1);
}


int PortMapDelete(struct PortMapList *pmlist, char *Proto, int ExtPort)
{
	struct PortMap *itr;
	
//	for (list<PortMap *>::iterator itr = m_pmap.begin(); itr != m_pmap.end(); itr++)
	/* Change this to use a double link list */
	for (itr = pmlist->m_pmap; itr; itr=itr->next)
	{
		if (( strcmp(itr->m_PortMappingProtocol,Proto) == 0 ) && (itr->m_ExternalPort == ExtPort))
		{
			PortMap_delPortForward(itr->m_PortMappingProtocol, itr->m_ExternalIP,
					itr->m_ExternalPort, itr->m_InternalClient,
					itr->m_InternalPort, itr->m_RemoteHost);
			
//			delete itr;
//			m_pmap.erase(itr);
			NODE_DEL(pmlist->m_pmap, itr);
			PortMap_Destruct(itr);
			free(itr);
			return (1);
		}
	}
	return 0;
}


int PortMap_addPortForward(char *Proto, char *ExtIP, int ExtPort, 
	char *IntIP,int IntPort, int Enabled, char *Desc, char *RemoteHost)
{
	char command[255];
	char *errstr;
	int retval;
	
	if(RemoteHost && (Check_IP(RemoteHost)==1))
			sprintf(command, IPTABLES " -t nat -A IFX_NAPT_DNAT_PM -p %s -s %s -d %s --dport %d -j DNAT --to %s:%d", Proto, RemoteHost, ExtIP, ExtPort, IntIP, IntPort);
	else	
			sprintf(command, IPTABLES " -t nat -A IFX_NAPT_DNAT_PM -p %s -d %s --dport %d -j DNAT --to %s:%d", Proto, ExtIP, ExtPort, IntIP, IntPort);
//syslog(LOG_ERR,"add port forward |%s|", command);
	retval = system(command);
	if (retval) {
		errstr = strerror(retval);
//		syslog(LOG_DEBUG, "Problem with cmd: %s: %s\n", command, errstr);
	}
	sprintf(command, IPTABLES " -I FORWARD -p %s -d %s --dport %d -j ACCEPT", Proto, IntIP, IntPort);
//syslog(LOG_ERR,"cont             |%s|", command);
//	system(command);
	retval = system(command);
	if (retval) {
		errstr = strerror(retval);
//		syslog(LOG_DEBUG, "Problem with cmd: %s: %s\n", command, errstr);
	}

	return (1);
}

int PortMap_delPortForward(char *Proto, char *ExtIP, int ExtPort, 
	char* IntIP, int IntPort, char *RemoteHost)
{
	char command[255];
	char *errstr;
	int retval;
	
	if(RemoteHost && (Check_IP(RemoteHost)==1))
			sprintf(command, IPTABLES  " -t nat -D IFX_NAPT_DNAT_PM -p %s -s %s -d %s --dport %d -j DNAT --to %s:%d", Proto, RemoteHost, ExtIP, ExtPort, IntIP, IntPort);
	else
			sprintf(command, IPTABLES  " -t nat -D IFX_NAPT_DNAT_PM -p %s -d %s --dport %d -j DNAT --to %s:%d", Proto, ExtIP, ExtPort, IntIP, IntPort);
//syslog(LOG_ERR,"del port forward |%s|", command);
	retval = system(command);	
	if (retval) {
		errstr = strerror(retval);
//		syslog(LOG_DEBUG, "Problem with cmd: %s: %s\n", command, errstr);
	}
	sprintf(command, IPTABLES  " -D FORWARD -p %s -d %s --dport %d -j ACCEPT", Proto, IntIP, IntPort);
//syslog(LOG_ERR,"cont             |%s|", command);
	retval = system(command);	
	if (retval) {
		errstr = strerror(retval);
//		syslog(LOG_DEBUG, "Problem with cmd: %s: %s\n", command, errstr);
	}
	return (1);
}

int PortMapList_Refresh(struct PortMapList *pmlist)
{
	struct PortMap *itr;
	FILE *pidfile;
	char *tmp_rh="";
	int lease_time;

	if ((pidfile = fopen(PORTMAPLIST, "w")) == NULL) {
			error("Failed to create pid file %s: %m", PORTMAPLIST);
			return 0;
  }
	for (itr = pmlist->m_pmap; itr; itr=itr->next)
	{
		//RemoteHost,ExternalPort,InternalClient,InternalPort,Proto,Duration,Description
		if(itr->m_InternalClient &&	itr->m_PortMappingProtocol && itr->m_PortMappingDescription)
		{
				if(itr->m_RemoteHost)
					tmp_rh = itr->m_RemoteHost;
				if(itr->m_PortMappingLeaseDuration)
					lease_time = 	itr->m_PortMappingLeaseDuration-itr->m_PortMapping_gettime;
				else
					lease_time = 0;	
				fprintf(pidfile, "RH=%s;EP=%d;IC=%s;IP=%d;PROT=%s;DUR=%d;DES=%s\n",
										tmp_rh,itr->m_ExternalPort,itr->m_InternalClient,
										itr->m_InternalPort,itr->m_PortMappingProtocol,lease_time
										,itr->m_PortMappingDescription);
		}								
	}	
	(void) fclose(pidfile);
	return 1;
}	

int PortMapList_Rebuild(struct PortMapList *pmlist)
{
	struct PortMap *itr;
	char *tmp_rh="";

	for (itr = pmlist->m_pmap; itr; itr=itr->next)
	{
		//RemoteHost,ExternalPort,InternalClient,InternalPort,Proto,Duration,Description
		if(itr->m_InternalClient &&	itr->m_PortMappingProtocol && itr->m_PortMappingDescription)
		{
					if(!(itr->m_PortMappingLeaseDuration)||(itr->m_PortMappingLeaseDuration > time(0)))
							PortMap_addPortForward(itr->m_PortMappingProtocol, itr->m_ExternalIP, itr->m_ExternalPort, 
																	itr->m_InternalClient, itr->m_InternalPort, 1, 0,itr->m_RemoteHost);
		}								
	}	
	return 1;
}

void PortMapList_ScanLease(struct PortMapList *pmlist)
{
	struct PortMap *itr;
	int has_match,size=0;
  
  for (itr = pmlist->m_pmap; itr; itr=itr->next)
	{
			size++;
	}	
	if(!size)
		return;
	while(size--)
	{	
			has_match = 0;
			for (itr = pmlist->m_pmap; itr; itr=itr->next)
			{
					if((time(0) > itr->m_PortMappingLeaseDuration)&& (itr->m_PortMappingLeaseDuration))
					{
        		PortMap_delPortForward(pmlist->m_pmap->m_PortMappingProtocol, pmlist->m_pmap->m_ExternalIP,
                         pmlist->m_pmap->m_ExternalPort, pmlist->m_pmap->m_InternalClient,
                         pmlist->m_pmap->m_InternalPort,pmlist->m_pmap->m_RemoteHost);
						NODE_DEL(pmlist->m_pmap, itr);
						PortMap_Destruct(itr);
						free(itr);
						has_match = 1;
						break;
					}	
			}
			if(!has_match)
				return;
	}		
}

int Check_IP(char* str)
{
	int   i;
	int   ip[4];
	char  tmp[16];

	if (!str)
		return -1;

	if (sscanf(str, "%d.%d.%d.%d%s", &ip[0], &ip[1], &ip[2], &ip[3], tmp) != 4)
		return -1;

	for (i = 0; i <= 3; i++)
	{
		if (ip[i] < 0 || ip[i] > 255)
			return -1;
	}
	return 1;
}

