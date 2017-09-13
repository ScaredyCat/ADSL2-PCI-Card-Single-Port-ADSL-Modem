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

#include "ipcon.h"
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>


int get_sockfd()
{
	static int sockfd = -1;
	
	if (sockfd == -1)
	{
		if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) == -1)
		{
			perror("user: socket creating failed");
			return (-1);
		}
	}
	return sockfd;
}

void IPCon_Construct(struct IPCon *ipcon, char *ifname)
{
	ipcon->m_ifname = strdup(ifname);
}

void IPCon_Destruct(struct IPCon *ipcon)
{
	if (ipcon->m_ifname)
	{
		free(ipcon->m_ifname);
		ipcon->m_ifname = NULL;
	}
}

int IPCon_SetIfName(struct IPCon *ipcon, char * ifname)
{
	if(ipcon->m_ifname) free(ipcon->m_ifname);
	ipcon->m_ifname = strdup(ifname);
	if(ipcon->m_ifname)
		return 1;
	else
		return 0;
}

char * IPCon_GetIfName(struct IPCon *ipcon)
{
	return ipcon->m_ifname;
}

char * IPCon_GetIpAddrStr(struct IPCon *ipcon)
{
	struct ifreq ifr;
	struct sockaddr_in *saddr;
	int fd;
	char *address;

	address = (char*)malloc(20);
	fd = get_sockfd();
	if (fd >= 0 )
	{
		strcpy(ifr.ifr_name, ipcon->m_ifname);
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
		{
			saddr = (struct sockaddr_in *)&ifr.ifr_addr;
			strcpy(address,inet_ntoa(saddr->sin_addr));
			return address;
		}
		else
		{
			close(fd);
			return NULL;
		}
	}
	return NULL;
}

int GetIpAddressStr(char *address, char *ifname)
{
   struct ifreq ifr;
   struct sockaddr_in *saddr;
   int fd;
   int succeeded = 0;

   fd = get_sockfd();
   if (fd >= 0 )
   {
      strcpy(ifr.ifr_name, ifname);
      ifr.ifr_addr.sa_family = AF_INET;
      if (ioctl(fd, SIOCGIFADDR, &ifr) == 0)
      {
         saddr = (struct sockaddr_in *)&ifr.ifr_addr;
         strcpy(address,inet_ntoa(saddr->sin_addr));
         succeeded = 1;
      }
      else
      {   
      	 close(fd);   
         succeeded = 0;
      }
   }
   return succeeded;
}

int IPCon_IsIfUp(struct IPCon *ipcon)
{
	struct ifreq ifr;
	int fd;

	fd = get_sockfd();
	if (fd >=0)
	{
		if (strlen(ipcon->m_ifname) < sizeof(ifr.ifr_name))
			strcpy (ifr.ifr_name, ipcon->m_ifname);
		ifr.ifr_addr.sa_family = AF_INET;
		if (ioctl(fd, SIOCGIFFLAGS, &ifr) < 0)
		{
			close(fd);
			return (-1);
		}
		if (ifr.ifr_flags & IFF_UP)
		{
			close(fd);
			return (1);
		}
		else
		{
			close(fd);
			return(0);
		}
	}
	return (-1);
}

char * IPCon_GetIfStatStr(struct IPCon *ipcon)
{
	if (IPCon_IsIfUp(ipcon))
		return ("UP");
	else
		return NULL;
}

